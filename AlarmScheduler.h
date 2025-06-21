#ifndef ALARM_SCHEDULER_H
#define ALARM_SCHEDULER_H

#include <Arduino.h>
#include <RtcDS1302.h>
#include <TimeLib.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <SPIFFS.h>

class ZoneAlarms
{
private:
  uint8_t zoneId;                                        // 1–4
  void (*Zone)(int id, String _action, JsonObject &zoneData); // Updated zone signature
  struct Alarm
  {
    bool isActive;    // true if enabled
    bool isDateBased; // true=date, false=day
    bool isOneTime;   // true=one-time, false=yearly (for date-based)
    bool days[7];     // sun=0, ..., sat=6
    uint16_t year;    // e.g., 2025
    uint8_t month;    // 1–12
    uint8_t date;     // 1–31
    uint8_t hour;     // 0–23
    uint8_t minute;   // 0–59
    String action;      // true=ON, false=OFF
  };
  Alarm alarms[10];                // 10 alarms per zone
  uint8_t alarmCount;              // Active alarms (0–10)
  unsigned long lastTriggerMinute; // Debouncing (Unix minutes)

public:
  ZoneAlarms(uint8_t id);
  bool addAlarm(JsonDocument &doc);
  bool deleteAlarm(uint8_t id);
  bool checkAlarms(); // Modified to accept scheduler reference
  void listAlarms(JsonArray &arr);
  void setZone(void (*zone)(int id, String _action, JsonObject &zoneData)) { Zone = zone; }
  void clearAlarms();
  bool hasZone() const { return Zone != nullptr; }
};

class AlarmScheduler
{
private:
  ZoneAlarms zones[4]; // IDs 1–4
  RtcDS1302<ThreeWire> *rtc;   // Pointer to RTC
  ThreeWire *wire;             // Pointer to ThreeWire
  WiFiUDP *ntpUDP;             // Pointer to NTP UDP
  NTPClient *timeClient;       // Pointer to NTP client
  time_t getRtcTime();
  unsigned long lastSyncMillis; // For manual sync
  bool setRTCFromNTP();         // NTP sync function
  unsigned long offset;
  bool spiffsInitialized; // Track SPIFFS initialization

public:
  AlarmScheduler(unsigned long timeOffset = 19800);
  ~AlarmScheduler();
  void begin(uint8_t rstPin, uint8_t datPin, uint8_t clkPin);
  void registerZone(uint8_t id, void (*zone)(int id, String _action, JsonObject &zoneData));
  void processJson(String &json);
  void checkAlarms();
  String printTime();
  bool isTimeSet();
  bool syncWithNTP();
  void updateOffsetValue(unsigned long offset);
  bool saveAlarmsToSpiffs();
  bool loadAlarmsFromSpiffs();

  bool saveZoneDataToSpiffs();
  bool loadZoneDataForAlarm(uint8_t zoneId, uint8_t alarmId, JsonObject &zoneData);
  bool deleteZoneDataFromSpiffs(uint8_t zoneId, uint8_t alarmId);
};

#endif
