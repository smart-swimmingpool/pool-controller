/*
 * Author: Lübbe Onken (http://github.com/luebbe)
 */

#include "TimeClientHelper.hpp"

// NTP Client
const char *TC_SERVER = "europe.pool.ntp.org";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, TC_SERVER);

// For starters use hardwired Central European Time (Berlin, Paris, ...)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120}; // Central European Summer Time
TimeChangeRule CET = {"CET ", Last, Sun, Oct, 3, 60};   // Central European Standard Time
Timezone Europe(CEST, CET);

// Japanese Time Zone (Tokyo)
TimeChangeRule JPST = {"JST", First, Sun, Mar, 0, 9 * 60}; // UTC + 9 hours
Timezone Japan(JPST, JPST);

TimeZoneInfo _timezones[2] = {
  { "Berlin", &Europe },
  {"Tokyo", &Japan}
};

void timeClientSetup()
{
  // initialize NTP Client
  timeClient.begin();

  // Set callback for time library and leave the sync to the NTP client
  setSyncProvider(getUtcTime);
  setSyncInterval(0);
}

int getTzCount()
{
  return (sizeof(_timezones) / sizeof(_timezones[0]));
}

time_t getUtcTime()
{
  if (timeClient.update())
  {
    return timeClient.getEpochTime();
  }
  else
  {
    return 0;
  }
}

time_t getTimeFor(int index, TimeChangeRule **tcr)
{
  if (index < getTzCount())
  {
    // Zeturn the time for the selected time zone
    return _timezones[index].timezone->toLocal(getUtcTime(), tcr);
  }
  else
  {
    return getUtcTime();
  }
}

String getTimeInfoFor(int index)
{
  if (index < getTzCount())
  {
    // Return the time for the selected time zone
    return _timezones[index].description;
  }
  else
  {
    return "UTC";
  }
}

String getFormattedTime(time_t rawTime)
{
  unsigned long hours = (rawTime % 86400L) / 3600;
  String hoursStr = hours < 10 ? "0" + String(hours) : String(hours);

  unsigned long minutes = (rawTime % 3600) / 60;
  String minuteStr = minutes < 10 ? "0" + String(minutes) : String(minutes);

  unsigned long seconds = rawTime % 60;
  String secondStr = seconds < 10 ? "0" + String(seconds) : String(seconds);

  return hoursStr + ":" + minuteStr + ":" + secondStr;
}

/**
 *
 */
tm getCurrentDateTime() {

  TimeChangeRule *tcr = NULL;
  time_t     t        = getTimeFor(0, &tcr);
  struct tm timeinfo =  *localtime(&t);

  return timeinfo;
}
