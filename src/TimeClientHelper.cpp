/*
 * Author: Lübbe Onken (http://github.com/luebbe)
 */

#include "TimeClientHelper.hpp"

// NTP Client
const char *TC_SERVER = "europe.pool.ntp.org";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, TC_SERVER);

// Central European Time (Berlin, Paris, ...)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120}; // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};   // Central European Standard Time
Timezone Europe(CEST, CET);

// Eastern European Time (Helsinki, Athens, ...)
TimeChangeRule EEST = {"EEST", Last, Sun, Mar, 3, 180}; // Eastern European Summer Time
TimeChangeRule EET = {"EET ", Last, Sun, Oct, 4, 120};   // Eastern European Standard Time
Timezone EasternEurope(EEST, EET);

// Western European Time (London, Lisbon, ...)
TimeChangeRule BST = {"BST", Last, Sun, Mar, 1, 60};  // British Summer Time
TimeChangeRule GMT = {"GMT", Last, Sun, Oct, 2, 0};    // Greenwich Mean Time
Timezone WesternEurope(BST, GMT);

// US Eastern Time (New York, Washington, ...)
TimeChangeRule EDT = {"EDT", Second, Sun, Mar, 2, -240}; // Eastern Daylight Time (UTC-4)
TimeChangeRule EST = {"EST", First, Sun, Nov, 2, -300};  // Eastern Standard Time (UTC-5)
Timezone USEastern(EDT, EST);

// US Central Time (Chicago, Houston, ...)
TimeChangeRule CDT = {"CDT", Second, Sun, Mar, 2, -300}; // Central Daylight Time (UTC-5)
TimeChangeRule CST = {"CST", First, Sun, Nov, 2, -360};  // Central Standard Time (UTC-6)
Timezone USCentral(CDT, CST);

// US Mountain Time (Denver, ...)
// Note: Most of Arizona does not observe DST
TimeChangeRule MDT = {"MDT", Second, Sun, Mar, 2, -360}; // Mountain Daylight Time (UTC-6)
TimeChangeRule MST = {"MST", First, Sun, Nov, 2, -420};  // Mountain Standard Time (UTC-7)
Timezone USMountain(MDT, MST);

// US Pacific Time (Los Angeles, San Francisco, ...)
TimeChangeRule PDT = {"PDT", Second, Sun, Mar, 2, -420}; // Pacific Daylight Time (UTC-7)
TimeChangeRule PST = {"PST", First, Sun, Nov, 2, -480};  // Pacific Standard Time (UTC-8)
Timezone USPacific(PDT, PST);

// Australian Eastern Time (Sydney, Melbourne, ...)
TimeChangeRule AEDT = {"AEDT", First, Sun, Oct, 2, 660}; // Australian Eastern Daylight Time (UTC+11)
TimeChangeRule AEST = {"AEST", First, Sun, Apr, 3, 600}; // Australian Eastern Standard Time (UTC+10)
Timezone AustralianEastern(AEDT, AEST);

// Japan Time Zone (Tokyo) - No DST
TimeChangeRule JST = {"JST", First, Sun, Mar, 0, 9 * 60}; // UTC + 9 hours
Timezone Japan(JST, JST);

// China Time Zone (Beijing) - No DST
TimeChangeRule CST_CHINA = {"CST", First, Sun, Mar, 0, 8 * 60}; // UTC + 8 hours
Timezone China(CST_CHINA, CST_CHINA);

TimeZoneInfo _timezones[10] = {
  { "Central European", &Europe },
  { "Eastern European", &EasternEurope },
  { "Western European", &WesternEurope },
  { "US Eastern", &USEastern },
  { "US Central", &USCentral },
  { "US Mountain", &USMountain },
  { "US Pacific", &USPacific },
  { "Australian Eastern", &AustralianEastern },
  { "Japan", &Japan },
  { "China", &China }
};

int _selectedTimezoneIndex = 0; // Default to Central European Time

void timeClientSetup() {
  // initialize NTP Client
  timeClient.begin();

  // Set callback for time library and leave the sync to the NTP client
  setSyncProvider(getUtcTime);
  setSyncInterval(0);
}

int getTzCount() {
  return (sizeof(_timezones) / sizeof(_timezones[0]));
}

time_t getUtcTime() {
  if (timeClient.update()) {
    return timeClient.getEpochTime();
  }   else {
    return 0;
  }
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
