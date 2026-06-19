// Copyright (c) 2018-2026 Smart Swimming Pool, Stephan Strittmatter

/**
 * @file TimeClientHelper.cpp
 * @brief NTP client, timezone database, daylight-saving switching, and time estimation.
 */

#include "TimeClientHelper.hpp"
#include "NetworkManager.hpp"

// NTP Client
WiFiUDP ntpUDP;
std::unique_ptr<NTPClient> timeClient;
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};  // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};    // Central European Standard Time
Timezone Europe(CEST, CET);

// Eastern European Time (Helsinki, Athens, ...)
TimeChangeRule EEST = {"EEST", Last, Sun, Mar, 3, 180};  // Eastern European Summer Time
TimeChangeRule EET = {"EET ", Last, Sun, Oct, 4, 120};   // Eastern European Standard Time
Timezone EasternEurope(EEST, EET);

// Western European Time (London, Lisbon, ...)
TimeChangeRule BST = {"BST", Last, Sun, Mar, 1, 60};  // British Summer Time
TimeChangeRule GMT = {"GMT", Last, Sun, Oct, 2, 0};   // Greenwich Mean Time
Timezone WesternEurope(BST, GMT);

// US Eastern Time (New York, Washington, ...)
TimeChangeRule EDT = {"EDT", Second, Sun, Mar, 2, -240};  // Eastern Daylight Time (UTC-4)
TimeChangeRule EST = {"EST", First, Sun, Nov, 2, -300};   // Eastern Standard Time (UTC-5)
Timezone USEastern(EDT, EST);

// US Central Time (Chicago, Houston, ...)
TimeChangeRule CDT = {"CDT", Second, Sun, Mar, 2, -300};  // Central Daylight Time (UTC-5)
TimeChangeRule CST = {"CST", First, Sun, Nov, 2, -360};   // Central Standard Time (UTC-6)
Timezone USCentral(CDT, CST);

// US Mountain Time (Denver, ...)
// Note: Most of Arizona does not observe DST
TimeChangeRule MDT = {"MDT", Second, Sun, Mar, 2, -360};  // Mountain Daylight Time (UTC-6)
TimeChangeRule MST = {"MST", First, Sun, Nov, 2, -420};   // Mountain Standard Time (UTC-7)
Timezone USMountain(MDT, MST);

// US Pacific Time (Los Angeles, San Francisco, ...)
TimeChangeRule PDT = {"PDT", Second, Sun, Mar, 2, -420};  // Pacific Daylight Time (UTC-7)
TimeChangeRule PST = {"PST", First, Sun, Nov, 2, -480};   // Pacific Standard Time (UTC-8)
Timezone USPacific(PDT, PST);

// Australian Eastern Time (Sydney, Melbourne, ...)
TimeChangeRule AEDT = {"AEDT", First, Sun, Oct, 2, 660};  // Australian Eastern Daylight Time (UTC+11)
TimeChangeRule AEST = {"AEST", First, Sun, Apr, 3, 600};  // Australian Eastern Standard Time (UTC+10)
Timezone AustralianEastern(AEDT, AEST);

// Japan Time Zone (Tokyo) - No DST
TimeChangeRule JST = {"JST", First, Sun, Mar, 0, 9 * 60};  // UTC + 9 hours
Timezone Japan(JST, JST);

// China Time Zone (Beijing) - No DST
TimeChangeRule CST_CHINA = {"CST", First, Sun, Mar, 0, 8 * 60};  // UTC + 8 hours
Timezone China(CST_CHINA, CST_CHINA);

TimeZoneInfo _timezones[10] = {{"Central Europe (CET/CEST)", &Europe}, {"Eastern Europe (EET/EEST)", &EasternEurope},
  {"Western Europe (WET/WEST)", &WesternEurope}, {"US Eastern Time", &USEastern}, {"US Central Time", &USCentral},
  {"US Mountain Time", &USMountain}, {"US Pacific Time", &USPacific}, {"Australian Eastern", &AustralianEastern},
  {"Japan (JST)", &Japan}, {"China (CST)", &China}};

// Pointer array for MQTT Select / web UI labels — mirrors _timezones descriptions
static const char *kTzLabels[] = {"Central Europe (CET/CEST)", "Eastern Europe (EET/EEST)", "Western Europe (WET/WEST)",
  "US Eastern Time", "US Central Time", "US Mountain Time", "US Pacific Time", "Australian Eastern", "Japan (JST)",
  "China (CST)"};
static constexpr int kTzLabelCount = sizeof(kTzLabels) / sizeof(kTzLabels[0]);

int _selectedTimezoneIndex = 0;  // Default to Central European Time

// Time sync tracking
static time_t _lastValidTime = 0;          // Last known good time from NTP
static uint32_t _lastValidTimeMillis = 0;  // millis() when last valid time was captured
static bool _timeSyncValid = false;        // Whether time sync is currently valid

// Configurable thresholds (P9)
static uint8_t _greenMaxHours = 1;   // GREEN→YELLOW after this many hours
static uint8_t _redAfterHours = 24;  // YELLOW→RED after this many hours

void timeClientSetup(const char *ntpServer) {
  // Create NTP client with configured server using unique_ptr for automatic memory management
  // Note: Using new directly since make_unique may not be available in all C++ versions
  timeClient.reset(new NTPClient(ntpUDP, ntpServer));

  // initialize NTP Client
  timeClient->begin();

  // Set callback for time library and leave the sync to the NTP client
  setSyncProvider(getUtcTime);
  setSyncInterval(3600);  // Sync every hour (3600 seconds)
}

int getTzCount() {
  return (sizeof(_timezones) / sizeof(_timezones[0]));
}

time_t getUtcTime() {
  if (timeClient && PoolController::NetworkManager::isWiFiConnected() && timeClient->update()) {
    time_t ntpTime = timeClient->getEpochTime();

    // Validate time is reasonable (after 2020-01-01)
    if (ntpTime >= MIN_VALID_TIME) {
      _lastValidTime = ntpTime;
      _lastValidTimeMillis = millis();
      _timeSyncValid = true;
      return ntpTime;
    }
  }

  // NTP update failed or returned invalid time
  // Use cached time + elapsed millis as fallback
  if (_lastValidTime > 0) {
    // Capture millis() once to avoid multiple calls and ensure consistency
    uint32_t nowMillis = millis();
    // Unsigned arithmetic handles wraparound correctly (every ~49 days)
    uint32_t elapsed = nowMillis - _lastValidTimeMillis;
    time_t estimatedTime = _lastValidTime + (elapsed / 1000);

    // Mark sync as invalid if we've been running on cached time too long
    uint32_t redMs = static_cast<uint32_t>(_redAfterHours) * 3600000UL;
    if (elapsed > redMs) {
      _timeSyncValid = false;
    }

    return estimatedTime;
  }

  // No valid time available at all
  _timeSyncValid = false;
  return 0;
}

bool isTimeSyncValid() {
  return _timeSyncValid;
}

TimeDegradation getTimeDegradation() {
  if (_timeSyncValid && _lastValidTime > 0) {
    // Sync was valid within the last RED_AFTER_HOURS — check how recent
    uint32_t elapsed = millis() - _lastValidTimeMillis;
    uint32_t greenMs = static_cast<uint32_t>(_greenMaxHours) * 3600000UL;
    // Unsigned arithmetic handles millis() wraparound (~49 days)
    if (elapsed < greenMs) {  // < greenMaxHours
      return TimeDegradation::GREEN;
    } else if (elapsed < static_cast<uint32_t>(_redAfterHours) * 3600000UL) {
      return TimeDegradation::YELLOW;  // between green and red thresholds
    }
    // Falls through to RED below
  }
  // > redAfterHours since last sync, or never synced
  return TimeDegradation::RED;
}

time_t getLastValidSyncTime() {
  return _lastValidTime;
}

bool forceNtpUpdate() {
  // getUtcTime() calls timeClient->update() internally, which sends an NTP
  // request and returns the validated epoch time on success.
  time_t t = getUtcTime();
  return (t >= MIN_VALID_TIME);
}

void setTimeDegradationGreenHours(uint8_t hours) {
  _greenMaxHours = (hours > 0) ? hours : 1;
}

uint8_t getTimeDegradationGreenHours() {
  return _greenMaxHours;
}

void setTimeDegradationRedHours(uint8_t hours) {
  // Red must be strictly greater than green, otherwise there is no YELLOW range.
  uint8_t minRed = _greenMaxHours + 1;
  _redAfterHours = (hours >= minRed && hours <= 72) ? hours : (minRed < 72 ? minRed : 72);
}

uint8_t getTimeDegradationRedHours() {
  return _redAfterHours;
}

time_t getTimeFor(int index, TimeChangeRule **tcr) {
  if (index >= 0 && index < getTzCount()) {
    // Return the time for the selected time zone
    return _timezones[index].timezone->toLocal(getUtcTime(), tcr);
  } else {
    return getUtcTime();
  }
}

String getTimeInfoFor(int index) {
  if (index >= 0 && index < getTzCount()) {
    // Return the time for the selected time zone
    return _timezones[index].description;
  } else {
    return "UTC";
  }
}

const char *const *getTimezoneLabelList() {
  return kTzLabels;
}

int getTimezoneLabelCount() {
  return kTzLabelCount;
}

int getTimezoneIndexFromLabel(const String &label) {
  for (int i = 0; i < kTzLabelCount; i++) {
    if (label == kTzLabels[i])
      return i;
  }
  return -1;
}

String getFormattedTime(time_t rawTime) {
  unsigned long hours = (rawTime % 86400L) / 3600;
  String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

  unsigned long minutes = (rawTime % 3600) / 60;
  String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

  unsigned long seconds = rawTime % 60;
  String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

  return hoursStr + ":" + minuteStr + ":" + secondStr;
}

void setTimezoneIndex(int index) {
  if (index >= 0 && index < getTzCount()) {
    _selectedTimezoneIndex = index;
  }
}

int getTimezoneIndex() {
  return _selectedTimezoneIndex;
}
