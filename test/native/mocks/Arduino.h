#pragma once

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstdarg>
#include <cmath>
#include <algorithm>
#include <string>
#include <map>
#include <vector>
#include <chrono>

typedef int __FlashStringHelper;

#ifndef NAN
#define NAN __builtin_nanf("")
#endif

// ---- Basic types ----
typedef uint8_t byte;
typedef uint16_t word;

// ---- String class (Arduino-compatible) ----
class String {
public:
  String() : data_("") {}
  String(const char *s) : data_(s ? s : "") {}                          // NOLINT
  String(int val) : data_(std::to_string(val)) {}                       // NOLINT
  String(unsigned int val) : data_(std::to_string(val)) {}              // NOLINT
  String(long val) : data_(std::to_string(val)) {}                      // NOLINT
  String(unsigned long val) : data_(std::to_string(val)) {}             // NOLINT
  String(float val, int decimals = 2) : data_(std::to_string(val)) {}   // NOLINT
  String(double val, int decimals = 2) : data_(std::to_string(val)) {}  // NOLINT
  String(const String &other) : data_(other.data_) {}
  String(const char *s, size_t len) : data_(s, len) {}
  String(char c) : data_(1, c) {}  // NOLINT

  const char *c_str() const { return data_.c_str(); }
  operator std::string() const { return data_; }
  bool concat(const char *s) {
    data_ += s;
    return true;
  }
  bool concat(const String &s) {
    data_ += s.data_;
    return true;
  }
  size_t length() const { return data_.length(); }
  bool isEmpty() const { return data_.empty(); }
  void reserve(size_t n) { data_.reserve(n); }
  String &operator+=(const String &rhs) {
    data_ += rhs.data_;
    return *this;
  }
  int toInt() const { return atoi(data_.c_str()); }
  float toFloat() const { return static_cast<float>(atof(data_.c_str())); }
  String substring(int start, int end = -1) const {
    if (end < 0)
      end = data_.length();
    return data_.substr(start, end - start).c_str();
  }
  int indexOf(const String &s) const { return data_.find(s.data_); }
  int indexOf(char c) const { return data_.find(c); }
  void toUpperCase() { std::transform(data_.begin(), data_.end(), data_.begin(), ::toupper); }
  void toLowerCase() { std::transform(data_.begin(), data_.end(), data_.begin(), ::tolower); }
  String &operator=(const String &rhs) {
    data_ = rhs.data_;
    return *this;
  }
  String &operator=(const char *rhs) {
    data_ = rhs ? rhs : "";
    return *this;
  }
  String operator+(const String &rhs) const { return (data_ + rhs.data_).c_str(); }
  bool operator==(const String &rhs) const { return data_ == rhs.data_; }
  bool operator!=(const String &rhs) const { return data_ != rhs.data_; }
  bool operator<(const String &rhs) const { return data_ < rhs.data_; }
  bool operator>(const String &rhs) const { return data_ > rhs.data_; }
  bool operator>=(const String &rhs) const { return data_ >= rhs.data_; }
  bool operator<=(const String &rhs) const { return data_ <= rhs.data_; }
  char operator[](int index) const { return data_[index]; }
  char &operator[](int index) { return data_[index]; }
  bool operator==(const char *rhs) const { return data_ == rhs; }
  friend String operator+(const char *lhs, const String &rhs) { return String(lhs) + rhs; }
  void clear() { data_.clear(); }
  bool startsWith(const String &prefix) const { return data_.find(prefix.data_) == 0; }
  bool endsWith(const String &suffix) const {
    if (suffix.length() > length())
      return false;
    return data_.substr(length() - suffix.length()) == suffix.data_;
  }
  bool equalsIgnoreCase(const String &other) const {
    if (length() != other.length())
      return false;
    std::string lower1 = data_, lower2 = other.data_;
    std::transform(lower1.begin(), lower1.end(), lower1.begin(), ::tolower);
    std::transform(lower2.begin(), lower2.end(), lower2.begin(), ::tolower);
    return lower1 == lower2;
  }
  bool equalsIgnoreCase(const char *other) const { return equalsIgnoreCase(String(other)); }
  bool equals(const String &other) const { return data_ == other.data_; }
  bool equals(const char *other) const { return data_ == other; }
  char charAt(int index) const { return data_[index]; }
  void trim() {
    const char *ws = " \t\n\r";
    size_t start = data_.find_first_not_of(ws);
    size_t end = data_.find_last_not_of(ws);
    if (start == std::string::npos)
      data_.clear();
    else
      data_ = data_.substr(start, end - start + 1);
  }

private:
  std::string data_;
};

// ---- Serial mock ----
class SerialClass {
public:
  void begin(int) {}
  void print(const char *s) {
    fprintf(stdout, "%s", s);
    fflush(stdout);
  }
  void print(int v) {
    fprintf(stdout, "%d", v);
    fflush(stdout);
  }
  void print(unsigned int v) {
    fprintf(stdout, "%u", v);
    fflush(stdout);
  }
  void print(float v) {
    fprintf(stdout, "%f", v);
    fflush(stdout);
  }
  void print(double v) {
    fprintf(stdout, "%f", v);
    fflush(stdout);
  }
  void println(const char *s) {
    fprintf(stdout, "%s\n", s);
    fflush(stdout);
  }
  void println(int v) {
    fprintf(stdout, "%d\n", v);
    fflush(stdout);
  }
  void println(unsigned int v) {
    fprintf(stdout, "%u\n", v);
    fflush(stdout);
  }
  void println(float v) {
    fprintf(stdout, "%f\n", v);
    fflush(stdout);
  }
  void println(double v) {
    fprintf(stdout, "%f\n", v);
    fflush(stdout);
  }
  void println() {
    fprintf(stdout, "\n");
    fflush(stdout);
  }
  void printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    fflush(stdout);
    va_end(args);
  }
  int available() { return 0; }
  int read() { return -1; }
  size_t readBytes(char *, size_t) { return 0; }
  void flush() {}
};
static SerialClass Serial;

// ---- Time functions ----
inline uint32_t millis() {
  static auto start = std::chrono::steady_clock::now();
  auto now = std::chrono::steady_clock::now();
  return std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
}

inline void delay(uint32_t) {}

// ---- Min/max ----
#undef min
#undef max
template <typename T> inline T min(T a, T b) {
  return a < b ? a : b;
}
template <typename T> inline T max(T a, T b) {
  return a > b ? a : b;
}

// ---- IPAddress ----
class IPAddress {
public:
  IPAddress() : addr_{0, 0, 0, 0} {}
  IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) : addr_{a, b, c, d} {}
  explicit IPAddress(uint32_t ip) {
    addr_[0] = (ip >> 0) & 0xFF;
    addr_[1] = (ip >> 8) & 0xFF;
    addr_[2] = (ip >> 16) & 0xFF;
    addr_[3] = (ip >> 24) & 0xFF;
  }
  String toString() const {
    char buf[20];
    snprintf(buf, sizeof(buf), "%d.%d.%d.%d", addr_[0], addr_[1], addr_[2], addr_[3]);
    return String(buf);
  }
  bool operator==(const IPAddress &o) const {
    return addr_[0] == o.addr_[0] && addr_[1] == o.addr_[1] && addr_[2] == o.addr_[2] && addr_[3] == o.addr_[3];
  }
  operator uint32_t() const { return (uint32_t)addr_[3] << 24 | (uint32_t)addr_[2] << 16 | (uint32_t)addr_[1] << 8 | addr_[0]; }
  uint8_t operator[](int i) const { return addr_[i]; }
  uint8_t &operator[](int i) { return addr_[i]; }

private:
  uint8_t addr_[4];
};

// ---- WiFi class mock ----
enum WiFiMode_t { WIFI_OFF = 0, WIFI_STA = 1, WIFI_AP = 2, WIFI_APSTA = 3 };
enum wl_status_t {
  WL_IDLE_STATUS = 0,
  WL_NO_SSID_AVAIL = 1,
  WL_SCAN_COMPLETED = 2,
  WL_CONNECTED = 3,
  WL_CONNECT_FAILED = 4,
  WL_CONNECTION_LOST = 5,
  WL_DISCONNECTED = 6
};
typedef int WiFiEvent_t;

class WiFiClass {
public:
  static wl_status_t status() { return WL_CONNECTED; }
  static int RSSI() { return -65; }
  static IPAddress localIP() { return IPAddress(192, 168, 1, 100); }
  static IPAddress softAPIP() { return IPAddress(192, 168, 4, 1); }
  static uint8_t *macAddress(uint8_t *mac) {
    static uint8_t m[6] = {0x84, 0xcc, 0xa8, 0x5a, 0x2d, 0x80};
    memcpy(mac, m, 6);
    return mac;
  }
  static String macAddress() { return "84:cc:a8:5a:2d:80"; }
  static String SSID() { return "TestSSID"; }
  static String SSID(int networkItem) { return String("TestAP"); }
  static int RSSI(int networkItem) { return -65; }
  static int encryptionType(int networkItem) { return 0; }
  static int scanNetworks() { return 2; }
  static String SOFTAPSSID() { return "PoolController-AP"; }
  static bool mode(WiFiMode_t) { return true; }
  static bool begin(const char *, const char *) { return true; }
  static void onEvent(void (*)(WiFiEvent_t)) {}
};
static WiFiClass WiFi;
#define WIFI_AUTH_OPEN 0

// ---- ESP class mock ----
class ESPClass {
public:
  static uint32_t getFreeHeap() { return 180000; }
  static uint32_t getMaxAllocHeap() { return 45000; }
  static uint32_t getEfuseMac() { return 0xDEADBEEF; }
  static void restart() {}
  static uint32_t getFlashChipSize() { return 4 * 1024 * 1024; }
  static uint32_t getSketchSize() { return 1200000; }
  static uint32_t getFreeSketchSpace() { return 800000; }
  static uint32_t getChipId() { return 0x5A2D80; }
};
static ESPClass ESP;

// ---- Preferences mock ----
class Preferences {
public:
  Preferences() : started_(false) {}
  bool begin(const char *ns, bool readOnly) {
    started_ = true;
    namespace_ = ns;
    return true;
  }
  void end() { started_ = false; }
  void clear() { s_data.clear(); }
  bool remove(const char *key) {
    s_data.erase(key);
    return true;
  }

  size_t putBytes(const char *key, const void *value, size_t len) {
    if (!started_)
      return 0;
    std::string &v = s_data[key];
    v.assign((const char *)value, len);
    return len;
  }

  size_t getBytes(const char *key, void *buf, size_t maxLen) {
    if (!started_ || s_data.find(key) == s_data.end())
      return 0;
    const std::string &v = s_data[key];
    size_t cpy = std::min(maxLen, v.size());
    memcpy(buf, v.data(), cpy);
    return cpy;
  }

  size_t putString(const char *key, const char *val) {
    if (!started_)
      return 0;
    s_data[key] = val;
    return strlen(val);
  }

  String getString(const char *key, const String &defaultVal = "") {
    if (!started_ || s_data.find(key) == s_data.end())
      return String(defaultVal);
    return String(s_data[key].c_str());
  }

  size_t putInt(const char *key, int val) {
    if (!started_)
      return 0;
    s_data[key] = std::to_string(val);
    return sizeof(int);
  }

  int getInt(const char *key, int defaultVal = 0) {
    if (!started_ || s_data.find(key) == s_data.end())
      return defaultVal;
    return atoi(s_data[key].c_str());
  }

  size_t putUInt(const char *key, unsigned int val) { return putInt(key, (int)val); }

  unsigned int getUInt(const char *key, unsigned int defaultVal = 0) { return (unsigned int)getInt(key, (int)defaultVal); }

  size_t putFloat(const char *key, float val) {
    if (!started_)
      return 0;
    s_data[key] = std::to_string(val);
    return sizeof(float);
  }

  float getFloat(const char *key, float defaultVal = 0.0f) {
    if (!started_ || s_data.find(key) == s_data.end())
      return defaultVal;
    return atof(s_data[key].c_str());
  }

  size_t putLong(const char *key, long val) {
    if (!started_)
      return 0;
    s_data[key] = std::to_string(val);
    return sizeof(long);
  }

  long getLong(const char *key, long defaultVal = 0) {
    if (!started_ || s_data.find(key) == s_data.end())
      return defaultVal;
    return atol(s_data[key].c_str());
  }

  size_t putDouble(const char *key, double val) {
    if (!started_)
      return 0;
    s_data[key] = std::to_string(val);
    return sizeof(double);
  }

  double getDouble(const char *key, double defaultVal = 0.0) {
    if (!started_ || s_data.find(key) == s_data.end())
      return defaultVal;
    return atof(s_data[key].c_str());
  }

  size_t putBool(const char *key, bool val) {
    if (!started_)
      return 0;
    s_data[key] = val ? "1" : "0";
    return 1;
  }

  bool getBool(const char *key, bool defaultVal = false) {
    if (!started_ || s_data.find(key) == s_data.end())
      return defaultVal;
    return s_data[key] == "1";
  }

  size_t putUChar(const char *key, uint8_t val) { return putInt(key, (int)val); }

  uint8_t getUChar(const char *key, uint8_t defaultVal = 0) { return (uint8_t)getInt(key, (int)defaultVal); }

  size_t putUShort(const char *key, uint16_t val) { return putInt(key, (int)val); }

  uint16_t getUShort(const char *key, uint16_t defaultVal = 0) { return (uint16_t)getInt(key, (int)defaultVal); }

  size_t putString(const char *key, const String &val) { return putString(key, val.c_str()); }

  bool isKey(const char *key) { return s_data.find(key) != s_data.end(); }

private:
  bool started_;
  std::string namespace_;
  static std::map<std::string, std::string> s_data;
};

// ---- LittleFS / FS mock ----
class File {
public:
  File() : valid_(false) {}
  bool operator!() const { return !valid_; }
  explicit operator bool() const { return valid_; }
  size_t size() { return data_.size(); }
  String readString() { return String(data_.c_str()); }
  size_t write(const uint8_t *, size_t) { return 0; }
  void close() {}
  bool seek(size_t) { return false; }
  int read() { return -1; }
  size_t readBytes(char *, size_t) { return 0; }
  bool available() { return false; }

  void _setData(const std::string &d) {
    data_ = d;
    valid_ = true;
  }
  void _setValid(bool v) { valid_ = v; }

private:
  bool valid_ = false;
  std::string data_;
};

class LittleFSClass {
public:
  bool begin(bool) { return true; }
  bool exists(const char *) { return false; }
  File open(const char *, const char * = "r") { return File(); }
  bool remove(const char *) { return true; }
  bool rename(const char *, const char *) { return true; }
  void end() {}
};
static LittleFSClass LittleFS;

// ---- PROGMEM macros ----
#define PROGMEM
#define PSTR(x) (x)
#define FPSTR(x) (x)
#define F(x) (x)
#define snprintf_P snprintf
#define strlcpy_P strlcpy
#define strcmp_P strcmp
#define vsnprintf_P vsnprintf
#define pgm_read_byte(x) (*(const uint8_t *)(x))
#define pgm_read_word(x) (*(const uint16_t *)(x))
#define pgm_read_dword(x) (*(const uint32_t *)(x))
#define pgm_read_float(x) (*(const float *)(x))
#define strlen_P strlen
#define strcpy_P strcpy
#define memcpy_P memcpy
#define sprintf_P sprintf

// ---- bit macros ----
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) ((bitvalue) ? bitSet(value, bit) : bitClear(value, bit))
#define lowByte(w) ((uint8_t)((w) & 0xff))
#define highByte(w) ((uint8_t)((w) >> 8))
#define bit(b) (1UL << (b))

// ---- GPIO constants ----
#define HIGH 0x1
#define LOW 0x0
#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2
#define INPUT_PULLDOWN 0x3

inline void pinMode(uint8_t, uint8_t) {}

// ---- digitalWrite/digitalRead capture ----
// Records the last value written to each pin so tests can assert on actual
// GPIO polarity behavior (e.g. relay active-HIGH vs active-LOW regression
// tests) instead of only observing the no-op default.
inline std::map<uint8_t, uint8_t> &_digitalPinState() {
  static std::map<uint8_t, uint8_t> state;
  return state;
}
inline void digitalWrite(uint8_t pin, uint8_t value) {
  _digitalPinState()[pin] = value;
}
inline int digitalRead(uint8_t pin) {
  auto &state = _digitalPinState();
  auto it = state.find(pin);
  return it != state.end() ? it->second : 0;
}
inline void _resetDigitalPinState() {
  _digitalPinState().clear();
}
inline int analogRead(uint8_t) {
  return 0;
}
inline void analogWrite(uint8_t, int) {}
inline void analogWriteResolution(int) {}
inline void analogReadResolution(int) {}
inline void randomSeed(uint32_t) {}
inline long random(long max) {
  return rand() % max;
}
inline long random(long min, long max) {
  return min + rand() % (max - min);
}
inline void attachInterrupt(uint8_t, void (*)(), int) {}
inline void detachInterrupt(uint8_t) {}
inline void yield() {}
inline void optimistic_yield(uint32_t) {}
inline int map(int x, int in_min, int in_max, int out_min, int out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
inline uint32_t constrain(uint32_t x, uint32_t a, uint32_t b) {
  return x < a ? a : (x > b ? b : x);
}
inline float constrain(float x, float a, float b) {
  return x < a ? a : (x > b ? b : x);
}
inline double constrain(double x, double a, double b) {
  return x < a ? a : (x > b ? b : x);
}

// ---- stdlib for Arduino ----
#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

// ---- Printable ----
class Print;
class Printable {
public:
  virtual ~Printable() {}
  virtual size_t printTo(Print &p) const = 0;
};

// ---- Print ----
class Print {
public:
  virtual size_t write(uint8_t) = 0;
  size_t print(const char *s) { return write((const uint8_t *)s, strlen(s)); }
  size_t print(int v) {
    char b[16];
    snprintf(b, sizeof(b), "%d", v);
    return print(b);
  }
  size_t print(unsigned int v) {
    char b[16];
    snprintf(b, sizeof(b), "%u", v);
    return print(b);
  }
  size_t print(long v) {
    char b[32];
    snprintf(b, sizeof(b), "%ld", v);
    return print(b);
  }
  size_t print(unsigned long v) {
    char b[32];
    snprintf(b, sizeof(b), "%lu", v);
    return print(b);
  }
  size_t print(float v, int = 2) {
    char b[32];
    snprintf(b, sizeof(b), "%f", v);
    return print(b);
  }
  size_t print(double v, int = 2) {
    char b[32];
    snprintf(b, sizeof(b), "%f", v);
    return print(b);
  }
  size_t println() { return write('\n'); }
  size_t println(const char *s) {
    size_t n = print(s);
    n += println();
    return n;
  }
  size_t println(int v) {
    size_t n = print(v);
    n += println();
    return n;
  }
  size_t println(unsigned int v) {
    size_t n = print(v);
    n += println();
    return n;
  }
  size_t println(long v) {
    size_t n = print(v);
    n += println();
    return n;
  }
  size_t println(unsigned long v) {
    size_t n = print(v);
    n += println();
    return n;
  }
  size_t println(float v, int d = 2) {
    size_t n = print(v, d);
    n += println();
    return n;
  }
  size_t println(double v, int d = 2) {
    size_t n = print(v, d);
    n += println();
    return n;
  }
  virtual size_t write(const uint8_t *buffer, size_t size) {
    size_t n = 0;
    for (size_t i = 0; i < size; i++)
      n += write(buffer[i]);
    return n;
  }
  int availableForWrite() { return 0; }
};

// ---- Stream ----
class Stream : public Print {
public:
  virtual int available() = 0;
  virtual int read() = 0;
  virtual int peek() = 0;
  void setTimeout(uint32_t) {}
  bool find(const char *) { return false; }
  String readString() { return String(); }
  String readStringUntil(char) { return String(); }
  size_t readBytes(char *b, size_t l) {
    for (size_t i = 0; i < l; i++)
      b[i] = read();
    return l;
  }
  int parseInt() { return 0; }
  float parseFloat() { return 0.0f; }
};

// ---- HardwareSerial ----
class HardwareSerial : public Stream {
public:
  void begin(unsigned long) {}
  void end() {}
  void setDebugOutput(bool) {}
  int available() override { return 0; }
  int read() override { return -1; }
  int peek() override { return -1; }
  size_t write(uint8_t) override { return 1; }
  using Print::write;
  operator bool() { return true; }
};
static HardwareSerial Serial0;

// ---- WiFiClient ----
class WiFiClient {
public:
  int connect(const char *, uint16_t) { return 1; }
  int connected() { return 1; }
  size_t write(const uint8_t *, size_t) { return 0; }
  int read() { return -1; }
  int available() { return 0; }
  size_t readBytes(uint8_t *buf, size_t len) {
    memset(buf, 0, len);
    return len;
  }
  void stop() {}
  void setTimeout(uint32_t) {}
  void setNoDelay(bool) {}
};

// ---- WiFiUDP ----
class WiFiUDP {
public:
  uint8_t begin(uint16_t) { return 1; }
  void stop() {}
  int beginPacket(const char *, uint16_t) { return 1; }
  int beginPacket(IPAddress, uint16_t) { return 1; }
  size_t write(const uint8_t *, size_t) { return 0; }
  int endPacket() { return 1; }
  int parsePacket() { return 0; }
  int read(unsigned char *, size_t) { return 0; }
};

// ---- Time library types ----
// time_t is provided by system <sys/types.h> (included transitively)
#ifndef TIMECHANGERULE_DEFINED
#define TIMECHANGERULE_DEFINED
struct TimeChangeRule {
  char abbrev[6] = "UTC";
  uint8_t week = 0;
  uint8_t dow = 0;
  uint8_t month = 0;
  uint8_t hour = 0;
  int offset = 0;
};
#endif

inline time_t time(time_t *t) {
  time_t now = millis() / 1000;
  if (t)
    *t = now;
  return now;
}

// ---- misc ----
// NORVI_AE01_R is defined via -D in CMakeLists.txt
#ifndef NORVI_AE01_R
#define NORVI_AE01_R
#endif
#define LED_BUILTIN 2
#define INPUT 0x0
#define OUTPUT 0x1
#define INPUT_PULLUP 0x2

// ---- std::function for callbacks ----
#include <functional>

// ---- Empty WString compat ----
// Already provided by String class above

// ---- Version compat ----
#define ARDUINO 106

// ---- Math ----
#define PI 3.1415926535897932384626433832795
inline float radians(float d) {
  return d * PI / 180.0f;
}
inline float degrees(float r) {
  return r * 180.0f / PI;
}
inline float sq(float x) {
  return x * x;
}

inline bool isnan(float x) {
  return x != x;
}  // NaN check
