// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter
//
// SPDX-License-Identifier: MIT

/**
 * @file ConfigManager.cpp
 * @brief Persistent configuration — NVS (Preferences) read/write and factory reset.
 */

#include "ConfigManager.hpp"

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <Preferences.h>
#include <mbedtls/md.h>

#include "LogCapture.hpp"
#include "Version.h"

namespace PoolController {

WiFiConfig ConfigManager::wifi_;
MqttConfig ConfigManager::mqtt_;
NtpConfig ConfigManager::ntp_;
ControllerSettings ConfigManager::settings_;
String ConfigManager::adminPasswordHash_ = "";
bool ConfigManager::configured_ = false;
bool ConfigManager::recordInvalid_ = false;

// Default password is "admin" (SHA-256 hash, not a real secret)
// This hash is intentionally hardcoded for first-time setup
// gitleaks:allow - known default password hash
static constexpr const char *kDefaultPasswordHash =  // NOLINT(whitespace/line_length)
  "8c6976e5b5410415bde908bd4dee15dfb167a9c873fc4bb8a81f6f2ab448a918";

static void hashSha256(const String &input, char (&output)[65]) {
  uint8_t hash[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, (const uint8_t *)input.c_str(), input.length());
  mbedtls_md_finish(&ctx, hash);
  mbedtls_md_free(&ctx);

  for (int i = 0; i < 32; i++) {
    snprintf(output + (i * 2), sizeof(output) - (i * 2), "%02x", hash[i]);
  }
  output[64] = '\0';
}

// ── NVS Key Names ──

static constexpr const char *kNvsNamespace = "config";
static constexpr const char *kWifiSsid = "wifi_ssid";
static constexpr const char *kWifiPass = "wifi_pass";
static constexpr const char *kMqttHost = "mqtt_host";
static constexpr const char *kMqttPort = "mqtt_port";
static constexpr const char *kMqttUser = "mqtt_user";
static constexpr const char *kMqttPass = "mqtt_pass";
static constexpr const char *kNtpServer = "ntp_server";
static constexpr const char *kNtpTz = "ntp_tz";
static constexpr const char *kSetInterval = "set_interval";
static constexpr const char *kSetMaxPool = "set_maxpool";
static constexpr const char *kSetMinSolar = "set_minsolar";
static constexpr const char *kSetHyst = "set_hyst";
static constexpr const char *kSetOpMode = "set_opmode";
static constexpr const char *kSetTzIdx = "set_tzidx";
static constexpr const char *kSetGreen = "set_green";
static constexpr const char *kSetRed = "set_red";
static constexpr const char *kSetCircThresh = "set_circth";
static constexpr const char *kSetCircFactor = "set_circfa";
static constexpr const char *kSetCircMax = "set_circmx";
static constexpr const char *kSetBtn1Min = "set_btn1min";
static constexpr const char *kSetBtn1Max = "set_btn1max";
static constexpr const char *kSetBtn2Min = "set_btn2min";
static constexpr const char *kSetBtn2Max = "set_btn2max";
static constexpr const char *kSetBtn3Min = "set_btn3min";
static constexpr const char *kSetBtn3Max = "set_btn3max";
static constexpr const char *kSetBtnNoPress = "set_btnnop";
static constexpr const char *kAdmPass = "adm_pass";
static constexpr const char *kCfgConfigured = "cfg_configured";

// ── Configuration Record ──
//
// The whole configuration is stored as one NVS blob. NVS writes a blob
// completely before it drops the previous one, so a power loss during save()
// leaves either the old or the new configuration, never a mix. The per-key
// layout above is still read once to migrate devices from older firmware.
//
// Layout (little-endian): magic u32, version u16, payload length u16,
// payload, CRC-32 u32 over everything before it.

static constexpr const char *kConfigRecord = "cfg_record";
static constexpr std::uint32_t kRecordMagic = 0x47464350;  // "PCFG"
static constexpr std::uint16_t kRecordVersion = 1;
static constexpr size_t kRecordHeaderSize = 8;
static constexpr size_t kRecordMaxSize = 2048;

namespace {

std::uint32_t crc32(const std::uint8_t *data, size_t length) {
  std::uint32_t crc = 0xFFFFFFFFu;
  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
  }
  return ~crc;
}

class RecordWriter final {
public:
  void u8(std::uint8_t value) { bytes_.push_back(value); }
  void u16(std::uint16_t value) {
    u8(static_cast<std::uint8_t>(value));
    u8(static_cast<std::uint8_t>(value >> 8));
  }
  void u32(std::uint32_t value) {
    u16(static_cast<std::uint16_t>(value));
    u16(static_cast<std::uint16_t>(value >> 16));
  }
  void i32(long value) { u32(static_cast<std::uint32_t>(static_cast<std::int32_t>(value))); }
  void f64(double value) {
    std::uint64_t raw = 0;
    memcpy(&raw, &value, sizeof(raw));
    u32(static_cast<std::uint32_t>(raw));
    u32(static_cast<std::uint32_t>(raw >> 32));
  }
  void str(const String &value) {
    const size_t length = value.length();
    u16(static_cast<std::uint16_t>(length > 0xFFFF ? 0xFFFF : length));
    bytes_.insert(bytes_.end(), value.c_str(), value.c_str() + (length > 0xFFFF ? 0xFFFF : length));
  }
  std::vector<std::uint8_t> &bytes() { return bytes_; }

private:
  std::vector<std::uint8_t> bytes_;
};

class RecordReader final {
public:
  RecordReader(const std::uint8_t *data, size_t length) : data_(data), length_(length) {}
  bool ok() const { return ok_; }
  bool atEnd() const { return pos_ == length_; }

  std::uint8_t u8() {
    if (!ok_ || pos_ + 1 > length_) {
      ok_ = false;
      return 0;
    }
    return data_[pos_++];
  }
  std::uint16_t u16() {
    const std::uint16_t low = u8();
    return static_cast<std::uint16_t>(low | (static_cast<std::uint16_t>(u8()) << 8));
  }
  std::uint32_t u32() {
    const std::uint32_t low = u16();
    return low | (static_cast<std::uint32_t>(u16()) << 16);
  }
  long i32() { return static_cast<long>(static_cast<std::int32_t>(u32())); }
  double f64() {
    const std::uint64_t low = u32();
    const std::uint64_t raw = low | (static_cast<std::uint64_t>(u32()) << 32);
    double value = 0;
    memcpy(&value, &raw, sizeof(value));
    return value;
  }
  String str() {
    const std::uint16_t length = u16();
    if (!ok_ || pos_ + length > length_) {
      ok_ = false;
      return String("");
    }
    std::string text(reinterpret_cast<const char *>(data_ + pos_), length);
    pos_ += length;
    return String(text.c_str());
  }

private:
  const std::uint8_t *data_;
  size_t length_;
  size_t pos_{0};
  bool ok_{true};
};

/** Everything save() persists, so decoding can fail without touching the live state. */
struct ConfigImage final {
  WiFiConfig wifi;
  MqttConfig mqtt;
  NtpConfig ntp;
  ControllerSettings settings;
  String adminPasswordHash;
  bool configured{false};
};

std::vector<std::uint8_t> encodeRecord(const ConfigImage &image) {
  RecordWriter payload;
  payload.str(image.wifi.ssid);
  payload.str(image.wifi.password);
  payload.str(image.mqtt.host);
  payload.u16(image.mqtt.port);
  payload.str(image.mqtt.username);
  payload.str(image.mqtt.password);
  payload.str(image.ntp.server);
  payload.i32(image.ntp.timezone);
  const ControllerSettings &s = image.settings;
  payload.i32(s.loopInterval);
  payload.f64(s.tempMaxPool);
  payload.f64(s.tempMinSolar);
  payload.f64(s.tempHysteresis);
  payload.f64(s.tempCircThreshold);
  payload.u16(s.tempCircFactor);
  payload.u16(s.tempCircMaxRuntime);
  payload.str(s.opMode);
  payload.i32(s.timeLossGreenHours);
  payload.i32(s.timeLossRedHours);
  payload.i32(s.timezoneIndex);
  payload.u16(s.btn1Min);
  payload.u16(s.btn1Max);
  payload.u16(s.btn2Min);
  payload.u16(s.btn2Max);
  payload.u16(s.btn3Min);
  payload.u16(s.btn3Max);
  payload.u16(s.btnNoPress);
  payload.str(image.adminPasswordHash);
  payload.u8(image.configured ? 1 : 0);

  RecordWriter record;
  record.u32(kRecordMagic);
  record.u16(kRecordVersion);
  record.u16(static_cast<std::uint16_t>(payload.bytes().size()));
  std::vector<std::uint8_t> &bytes = record.bytes();
  bytes.insert(bytes.end(), payload.bytes().begin(), payload.bytes().end());
  record.u32(crc32(bytes.data(), bytes.size()));
  return bytes;
}

bool decodeRecord(const std::uint8_t *data, size_t length, ConfigImage &image) {
  if (length < kRecordHeaderSize + 4) {
    return false;
  }
  RecordReader header(data, kRecordHeaderSize);
  const std::uint32_t magic = header.u32();
  const std::uint16_t version = header.u16();
  const std::uint16_t payloadLength = header.u16();
  if (magic != kRecordMagic || version != kRecordVersion || kRecordHeaderSize + payloadLength + 4 != length) {
    return false;
  }
  RecordReader trailer(data + length - 4, 4);
  if (trailer.u32() != crc32(data, length - 4)) {
    return false;
  }

  RecordReader payload(data + kRecordHeaderSize, payloadLength);
  image.wifi.ssid = payload.str();
  image.wifi.password = payload.str();
  image.mqtt.host = payload.str();
  image.mqtt.port = payload.u16();
  image.mqtt.username = payload.str();
  image.mqtt.password = payload.str();
  image.ntp.server = payload.str();
  image.ntp.timezone = payload.i32();
  ControllerSettings &s = image.settings;
  s.loopInterval = payload.i32();
  s.tempMaxPool = payload.f64();
  s.tempMinSolar = payload.f64();
  s.tempHysteresis = payload.f64();
  s.tempCircThreshold = payload.f64();
  s.tempCircFactor = payload.u16();
  s.tempCircMaxRuntime = payload.u16();
  s.opMode = payload.str();
  s.timeLossGreenHours = payload.i32();
  s.timeLossRedHours = payload.i32();
  s.timezoneIndex = static_cast<int>(payload.i32());
  s.btn1Min = payload.u16();
  s.btn1Max = payload.u16();
  s.btn2Min = payload.u16();
  s.btn2Max = payload.u16();
  s.btn3Min = payload.u16();
  s.btn3Max = payload.u16();
  s.btnNoPress = payload.u16();
  image.adminPasswordHash = payload.str();
  image.configured = payload.u8() != 0;
  return payload.ok() && payload.atEnd();
}

}  // namespace

// ── Lifecycle ──

bool ConfigManager::begin() {
  LOG_INFO("✓ NVS config namespace opened\n");
  return load();
}

bool ConfigManager::load() {
  Preferences prefs;
  if (!prefs.begin(kNvsNamespace, true)) {  // read-only mode
    LOG_ERROR("✖ Failed to open NVS config namespace\n");
    reset();
    return false;
  }

  const size_t recordLength = prefs.getBytesLength(kConfigRecord);
  if (recordLength > 0) {
    ConfigImage image;
    bool valid = false;
    // Check the stored length before allocating: a bogus length must not
    // exhaust the heap at boot.
    if (recordLength <= kRecordMaxSize) {
      std::vector<std::uint8_t> record(recordLength);
      valid = prefs.getBytes(kConfigRecord, record.data(), recordLength) == recordLength &&
        decodeRecord(record.data(), recordLength, image);
    }
    prefs.end();
    if (!valid) {
      // A record exists but cannot be trusted (corrupted, or written by newer
      // firmware). The legacy keys may hold older credentials, such as an admin
      // password changed since, so they are not used. The device starts like a
      // factory-new one instead: defaults in RAM, unconfigured, and the stored
      // record stays untouched until the next save replaces it.
      LOG_ERROR(
        "✖ Configuration record invalid — starting unconfigured with defaults; setup AP requires the default admin login\n");
      image = ConfigImage{};
      image.adminPasswordHash = kDefaultPasswordHash;
    }
    wifi_ = image.wifi;
    mqtt_ = image.mqtt;
    ntp_ = image.ntp;
    settings_ = image.settings;
    adminPasswordHash_ = image.adminPasswordHash;
    configured_ = image.configured;
    recordInvalid_ = !valid;
    if (valid) {
      LOG_INFO("✓ Configuration loaded from NVS\n");
    }
    return valid;
  }

  // No record yet: a device updated from older firmware still has its
  // configuration in the per-key layout. The next save() migrates it.
  wifi_.ssid = prefs.getString(kWifiSsid, "");
  wifi_.password = prefs.getString(kWifiPass, "");

  mqtt_.host = prefs.getString(kMqttHost, "");
  mqtt_.port = prefs.getUInt(kMqttPort, 1883);
  mqtt_.username = prefs.getString(kMqttUser, "");
  mqtt_.password = prefs.getString(kMqttPass, "");

  ntp_.server = prefs.getString(kNtpServer, "pool.ntp.org");
  ntp_.timezone = prefs.getLong(kNtpTz, 0);

  settings_.loopInterval = prefs.getLong(kSetInterval, 10);
  settings_.tempMaxPool = prefs.getDouble(kSetMaxPool, 28.5);
  settings_.tempMinSolar = prefs.getDouble(kSetMinSolar, 55.0);
  settings_.tempHysteresis = prefs.getDouble(kSetHyst, 1.0);
  settings_.opMode = prefs.getString(kSetOpMode, "auto");
  settings_.timezoneIndex = prefs.getInt(kSetTzIdx, 0);
  settings_.timeLossGreenHours = prefs.getUChar(kSetGreen, 1);
  settings_.timeLossRedHours = prefs.getUChar(kSetRed, 24);
  settings_.tempCircThreshold = prefs.getDouble(kSetCircThresh, 24.0);
  settings_.tempCircFactor = prefs.getUShort(kSetCircFactor, 30);
  settings_.tempCircMaxRuntime = prefs.getUShort(kSetCircMax, 720);
  settings_.btn1Min = prefs.getUShort(kSetBtn1Min, 3100);
  settings_.btn1Max = prefs.getUShort(kSetBtn1Max, 3520);
  settings_.btn2Min = prefs.getUShort(kSetBtn2Min, 3520);
  settings_.btn2Max = prefs.getUShort(kSetBtn2Max, 3880);
  settings_.btn3Min = prefs.getUShort(kSetBtn3Min, 3880);
  settings_.btn3Max = prefs.getUShort(kSetBtn3Max, 4095);
  settings_.btnNoPress = prefs.getUShort(kSetBtnNoPress, 4096);

  adminPasswordHash_ = prefs.getString(kAdmPass, kDefaultPasswordHash);
  configured_ = prefs.getBool(kCfgConfigured, false);

  prefs.end();

  LOG_INFO("✓ Configuration loaded from NVS\n");
  return true;
}

bool ConfigManager::save() {
  ConfigImage image;
  image.wifi = wifi_;
  image.mqtt = mqtt_;
  image.ntp = ntp_;
  image.settings = settings_;
  image.adminPasswordHash = adminPasswordHash_;
  image.configured = configured_;
  const std::vector<std::uint8_t> record = encodeRecord(image);
  if (record.size() > kRecordMaxSize) {
    LOG_ERROR("✖ Configuration record too large (%u bytes)\n", static_cast<unsigned>(record.size()));
    return false;
  }

  Preferences prefs;
  if (!prefs.begin(kNvsNamespace, false)) {  // read-write mode
    LOG_ERROR("✖ Failed to open NVS config namespace for writing\n");
    return false;
  }
  // One putBytes() replaces the whole configuration at once; the legacy keys
  // are left untouched and are no longer read once the record exists.
  const bool written = prefs.putBytes(kConfigRecord, record.data(), record.size()) == record.size();
  prefs.end();

  if (!written) {
    LOG_ERROR("✖ Failed to write configuration to NVS\n");
    return false;
  }

  recordInvalid_ = false;
  LOG_INFO("✓ Configuration saved to NVS\n");
  return true;
}

void ConfigManager::reset() {
  // Clear all keys in the config namespace
  Preferences prefs;
  if (prefs.begin(kNvsNamespace, false)) {
    prefs.clear();
    prefs.end();
  }

  wifi_ = WiFiConfig{};
  mqtt_ = MqttConfig{};
  ntp_ = NtpConfig{};
  settings_ = ControllerSettings{};
  adminPasswordHash_ = kDefaultPasswordHash;  // Reset to default "admin" password
  configured_ = false;
  recordInvalid_ = false;

  LOG_INFO("✓ Configuration reset to factory defaults\n");
}

bool ConfigManager::resetToDefaults() {
  const WiFiConfig previousWifi = wifi_;
  const MqttConfig previousMqtt = mqtt_;
  const NtpConfig previousNtp = ntp_;
  const ControllerSettings previousSettings = settings_;
  const String previousAdminHash = adminPasswordHash_;
  const bool previousConfigured = configured_;

  // save() rewrites every key, so the namespace holds the defaults afterwards
  // without the window an up-front clear() would open.
  wifi_ = WiFiConfig{};
  mqtt_ = MqttConfig{};
  ntp_ = NtpConfig{};
  settings_ = ControllerSettings{};
  adminPasswordHash_ = kDefaultPasswordHash;
  configured_ = false;
  if (!save()) {
    wifi_ = previousWifi;
    mqtt_ = previousMqtt;
    ntp_ = previousNtp;
    settings_ = previousSettings;
    adminPasswordHash_ = previousAdminHash;
    configured_ = previousConfigured;
    // A single failed put may have left some keys at their defaults.
    if (!save()) {
      LOG_ERROR("✖ Could not restore the previous configuration in NVS\n");
    }
    return false;
  }
  LOG_INFO("✓ Configuration reset to factory defaults\n");
  return true;
}

// ── Boot Version Tracking ──

void ConfigManager::logOtaTransition() {
  Preferences prefs;
  prefs.begin("ota-version", false);  // namespace "ota-version" in NVS

  String previousVersion = prefs.getString("fw_version", "");
  String runningVersion = FW_VERSION;

  if (previousVersion.isEmpty()) {
    // First boot ever — nothing to compare
    LOG_INFO("ℹ First boot — firmware version %s\n", runningVersion.c_str());
  } else if (previousVersion != runningVersion) {
    // Version changed — OTA update just happened
    LOG_INFO("◉ OTA UPDATE DETECTED: %s → %s\n", previousVersion.c_str(), runningVersion.c_str());

    // Update stored version to match running version
    prefs.putString("fw_version", runningVersion);
  } else {
    // Normal boot — same version
    LOG_INFO("ℹ Normal boot — firmware %s (no OTA change)\n", runningVersion.c_str());
  }

  prefs.end();
}

void ConfigManager::setAdminPassword(const String &newPassword) {
  char hash[65];
  hashSha256(newPassword, hash);
  adminPasswordHash_ = hash;
}

bool ConfigManager::verifyAdminPassword(const String &password) {
  char hash[65];
  hashSha256(password, hash);
  return adminPasswordHash_ == hash;
}

namespace {
// Process-wide sensor-mapping cache so hot loops avoid NVS traffic.
bool s_mappingCacheValid = false;
std::uint8_t s_cachedSolarAddress[8] = {0};
std::uint8_t s_cachedPoolAddress[8] = {0};

// Both roles in one value: solar ROM followed by pool ROM.
constexpr const char *kSensorMappingKey = "mapping";
constexpr size_t kSensorMappingRecordSize = 16;

struct SensorMappingReadResult final {
  bool readSucceeded{false};
  bool hasMapping{false};
};

SensorMappingReadResult readSensorMappingFromNvs(uint8_t solarAddr[8], uint8_t poolAddr[8]) {
  memset(solarAddr, 0, 8);
  memset(poolAddr, 0, 8);

  Preferences prefs;
  if (!prefs.begin("ds18b20", true)) {
    return {};
  }

  // The combined record is written in one NVS operation, so it always holds a
  // consistent pair. The legacy per-role keys are read only when it is absent.
  std::uint8_t record[kSensorMappingRecordSize] = {0};
  if (prefs.getBytes(kSensorMappingKey, record, sizeof(record)) == sizeof(record)) {
    prefs.end();
    memcpy(solarAddr, record, 8);
    memcpy(poolAddr, record + 8, 8);
    SensorMappingReadResult result;
    result.readSucceeded = true;
    result.hasMapping = true;
    return result;
  }

  const size_t solarLength = prefs.getBytes("solar_adr", solarAddr, 8);
  const size_t poolLength = prefs.getBytes("pool_adr", poolAddr, 8);
  prefs.end();

  if (solarLength != 8) {
    memset(solarAddr, 0, 8);
  }
  if (poolLength != 8) {
    memset(poolAddr, 0, 8);
  }

  SensorMappingReadResult result;
  result.readSucceeded = true;
  result.hasMapping = solarLength == 8 || poolLength == 8;
  return result;
}
}  // namespace

bool ConfigManager::saveSensorMapping(const uint8_t solarAddr[8], const uint8_t poolAddr[8]) {
  s_mappingCacheValid = false;
  Preferences prefs;
  if (!prefs.begin("ds18b20", false)) {  // read-write
    LOG_ERROR("✖ Failed to open NVS sensor-mapping namespace for writing\n");
    return false;
  }
  // A single putBytes() is all-or-nothing in NVS, so a failed write leaves the
  // previous pair in place instead of a new solar and an old pool ROM.
  std::uint8_t record[kSensorMappingRecordSize];
  memcpy(record, solarAddr, 8);
  memcpy(record + 8, poolAddr, 8);
  const bool written = prefs.putBytes(kSensorMappingKey, record, sizeof(record)) == sizeof(record);
  if (written) {
    // The record takes precedence; dropping the legacy keys only tidies up.
    prefs.remove("solar_adr");
    prefs.remove("pool_adr");
  }
  prefs.end();
  if (!written) {
    LOG_ERROR("✖ Failed to write sensor mapping to NVS\n");
    return false;
  }

  char buf[17];
  snprintf(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X%02X%02X", solarAddr[0], solarAddr[1], solarAddr[2], solarAddr[3],
    solarAddr[4], solarAddr[5], solarAddr[6], solarAddr[7]);
  LOG_INFO("✓ Sensor mapping saved: Solar [%s]", buf);
  snprintf(buf, sizeof(buf), "%02X%02X%02X%02X%02X%02X%02X%02X", poolAddr[0], poolAddr[1], poolAddr[2], poolAddr[3], poolAddr[4],
    poolAddr[5], poolAddr[6], poolAddr[7]);
  LOG_INFO(", Pool [%s]\n", buf);
  return true;
}

bool ConfigManager::loadSensorMapping(uint8_t solarAddr[8], uint8_t poolAddr[8]) {
  // Preserve the legacy return contract: true means at least one role exists.
  return readSensorMappingFromNvs(solarAddr, poolAddr).hasMapping;
}

bool ConfigManager::loadSensorMappingCached(uint8_t solarAddr[8], uint8_t poolAddr[8]) {
  if (!s_mappingCacheValid) {
    std::uint8_t solarCandidate[8] = {0};
    std::uint8_t poolCandidate[8] = {0};
    const SensorMappingReadResult readResult = readSensorMappingFromNvs(solarCandidate, poolCandidate);
    if (!readResult.readSucceeded) {
      // Do not poison the process-wide cache on a transient NVS failure.
      memset(solarAddr, 0, 8);
      memset(poolAddr, 0, 8);
      return false;
    }
    memcpy(s_cachedSolarAddress, solarCandidate, 8);
    memcpy(s_cachedPoolAddress, poolCandidate, 8);
    s_mappingCacheValid = true;
  }
  memcpy(solarAddr, s_cachedSolarAddress, 8);
  memcpy(poolAddr, s_cachedPoolAddress, 8);
  return true;
}

/** @brief Check if an 8-byte address is all zeros. */
static bool isSensorAddressZero(const uint8_t addr[8]) {
  for (uint8_t i = 0; i < 8; i++) {
    if (addr[i] != 0)
      return false;
  }
  return true;
}

}  // namespace PoolController
