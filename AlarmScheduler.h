#ifndef ALARM_SCHEDULER_H
#define ALARM_SCHEDULER_H

#include <Arduino.h>
#include <RtcDS1302.h>
#include <TimeLib.h>
#include <ArduinoJson.h>

class CallbackAlarms {
private:
  uint8_t callbackId; // 1–4
  void (*Callback)(int id, bool isOn); // User callback
  struct Alarm {
    bool isActive;       // true if enabled
    bool isDateBased;    // true=date, false=day
    bool isOneTime;      // true=one-time, false=yearly (for date-based)
    bool days[7];        // sun=0, ..., sat=6
    uint16_t year;       // e.g., 2025
    uint8_t month;       // 1–12
    uint8_t date;        // 1–31
    uint8_t hour;        // 0–23
    uint8_t minute;      // 0–59
    bool action;         // true=ON, false=OFF
  };
  Alarm alarms[10];      // 10 alarms per callback
  uint8_t alarmCount;    // Active alarms (0–10)
  unsigned long lastTriggerMinute; // Debouncing (Unix minutes)

public:
  CallbackAlarms(uint8_t id);
  bool addAlarm(JsonDocument& doc);
  bool deleteAlarm(uint8_t id);
  void checkAlarms();
  void listAlarms(JsonArray& arr);
  void setCallback(void (*callback)(int id, bool isOn)) { Callback = callback; }
};

class AlarmScheduler {
private:
  CallbackAlarms callbacks[4]; // IDs 1–4
  RtcDS1302<ThreeWire>* rtc;   // Pointer to RTC
  ThreeWire* wire;             // Pointer to ThreeWire
  time_t getRtcTime();
  unsigned long lastSyncMillis; // For manual sync

public:
  AlarmScheduler();
  ~AlarmScheduler();
  void begin(uint8_t rstPin, uint8_t datPin, uint8_t clkPin);
  void registerCallback(uint8_t id, void (*callback)(int id, bool isOn));
  void processJson(String& json);
  void checkAlarms();
  String printTime();
  bool isTimeSet();
};

#endif