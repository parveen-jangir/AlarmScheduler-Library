#include "AlarmScheduler.h"

// Helper: Validate date
bool isValidDate(int year, int month, int date) {
  if (year < 2000 || year > 2099 || month < 1 || month > 12 || date < 1 || date > 31) return false;
  int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) daysInMonth[1] = 29;
  return date <= daysInMonth[month - 1];
}

// CallbackAlarms Implementation
CallbackAlarms::CallbackAlarms(uint8_t id) : callbackId(id), Callback(nullptr), alarmCount(0), lastTriggerMinute(0) {
  for (int i = 0; i < 10; i++) {
    alarms[i].isActive = false;
    alarms[i].isDateBased = false;
    alarms[i].isOneTime = false;
  }
}

bool CallbackAlarms::addAlarm(JsonDocument& doc) {
  if (alarmCount >= 10) {
    doc.clear();
    doc["status"] = "error";
    doc["message"] = "Callback full";
    return false;
  }
  // Find inactive slot
  int slot = -1;
  for (int i = 0; i < 10; i++) {
    if (!alarms[i].isActive) {
      slot = i;
      break;
    }
  }
  if (slot == -1) return false;

  // Validate type
  String type = doc["type"].as<String>();
  if (type != "day" && type != "date") {
    doc.clear();
    doc["status"] = "error";
    doc["message"] = "Invalid type";
    return false;
  }
  bool isDateBased = (type == "date");

  // Validate oneTime
  if (isDateBased) {
    if (!doc.containsKey("oneTime")) {
      doc.clear();
      doc["status"] = "error";
      doc["message"] = "oneTime required for date-based";
      return false;
    }
  } else if (doc.containsKey("oneTime") && doc["oneTime"].as<bool>()) {
    doc.clear();
    doc["status"] = "error";
    doc["message"] = "oneTime only for date-based";
    return false;
  }

  // Validate time
  String timeStr = doc["time"].as<String>();
  int hour, minute;
  if (sscanf(timeStr.c_str(), "%d:%d", &hour, &minute) != 2 ||
      hour < 0 || hour > 23 || minute < 0 || minute > 59) {
    doc.clear();
    doc["status"] = "error";
    doc["message"] = "Invalid time format";
    return false;
  }

  // Validate action
  String action = doc["action"].as<String>();
  if (action != "ON" && action != "OFF") {
    doc.clear();
    doc["status"] = "error";
    doc["message"] = "Invalid action";
    return false;
  }

  // Validate zone_data
  if (!doc.containsKey("zone_data")) {
    doc.clear();
    doc["status"] = "error";
    doc["message"] = "zone_data required";
    return false;
  }
  
  JsonObject zoneDataObj = doc["zone_data"].as<JsonObject>();
  if (zoneDataObj.isNull()) {
    doc.clear();
    doc["status"] = "error";
    doc["message"] = "zone_data must be JSON object";
    return false;
  }
  
  // Check zone_data size (approximate)
  String zoneDataStr;
  serializeJson(zoneDataObj, zoneDataStr);
  if (zoneDataStr.length() > 1000) {
    doc.clear();
    doc["status"] = "error";
    doc["message"] = "zone_data too large (max 1000 bytes)";
    return false;
  }

  // Initialize alarm
  Alarm& alarm = alarms[slot];
  alarm.isActive = true;
  alarm.isDateBased = isDateBased;
  alarm.isOneTime = isDateBased ? doc["oneTime"].as<bool>() : false;
  alarm.hour = hour;
  alarm.minute = minute;
  alarm.action = (action == "ON");

  // Handle day-based
  if (!isDateBased) {
    for (int i = 0; i < 7; i++) alarm.days[i] = false;
    JsonArray days = doc["days"].as<JsonArray>();
    if (days.isNull() || days.size() == 0) {
      doc.clear();
      doc["status"] = "error";
      doc["message"] = "Days required";
      return false;
    }
    for (JsonVariant dayVar : days) {
      String day = dayVar.as<String>();
      day.toLowerCase();
      if (day == "sun") alarm.days[0] = true;
      else if (day == "mon") alarm.days[1] = true;
      else if (day == "tue") alarm.days[2] = true;
      else if (day == "wed") alarm.days[3] = true;
      else if (day == "thu") alarm.days[4] = true;
      else if (day == "fri") alarm.days[5] = true;
      else if (day == "sat") alarm.days[6] = true;
      else {
        doc.clear();
        doc["status"] = "error";
        doc["message"] = "Invalid day";
        return false;
      }
    }
  } else {
    // Handle date-based
    String dateStr = doc["date"].as<String>();
    int year, month, date;
    if (sscanf(dateStr.c_str(), "%d-%d-%d", &year, &month, &date) != 3 ||
        !isValidDate(year, month, date)) {
      doc.clear();
      doc["status"] = "error";
      doc["message"] = "Invalid date format";
      return false;
    }
    alarm.year = year;
    alarm.month = month;
    alarm.date = date;
  }

  // Disable same-time alarms
  for (int i = 0; i < 10; i++) {
    if (i != slot && alarms[i].isActive &&
        alarms[i].hour == alarm.hour && alarms[i].minute == alarm.minute) {
      alarms[i].isActive = false;
      alarmCount--;
    }
  }

  alarmCount++;

  // Store zone_data in separate file
  DynamicJsonDocument zoneDoc(8192);
  File file = SPIFFS.open("/zone_data.json", FILE_READ);
  JsonArray zoneDataArray;
  if (file) {
    DeserializationError error = deserializeJson(zoneDoc, file);
    if (!error && zoneDoc.containsKey("zone_data")) {
      zoneDataArray = zoneDoc["zone_data"].as<JsonArray>();
    }
    file.close();
  }
  if (zoneDataArray.isNull()) {
    zoneDataArray = zoneDoc.createNestedArray("zone_data");
  }
  
  // Add/update zone data entry
  bool found = false;
  for (JsonObject obj : zoneDataArray) {
    if (obj["callback"].as<int>() == callbackId && obj["id"].as<int>() == slot) {
      obj["zone_data"] = zoneDataObj;
      found = true;
      break;
    }
  }
  if (!found) {
    JsonObject newZoneEntry = zoneDataArray.createNestedObject();
    newZoneEntry["callback"] = callbackId;
    newZoneEntry["id"] = slot;
    newZoneEntry["zone_data"] = zoneDataObj;
  }
  
  // Save zone data
  file = SPIFFS.open("/zone_data.json", FILE_WRITE);
  if (file) {
    serializeJson(zoneDoc, file);
    file.close();
  }

  doc.clear();
  doc["status"] = "success";
  doc["id"] = slot;
  return true;
}

bool CallbackAlarms::deleteAlarm(uint8_t id) {
  if (id >= 10 || !alarms[id].isActive) return false;
  alarms[id].isActive = false;
  alarmCount--;
  
  // Delete corresponding zone data
  File file = SPIFFS.open("/zone_data.json", FILE_READ);
  if (file) {
    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (!error && doc.containsKey("zone_data")) {
      JsonArray zoneArray = doc["zone_data"].as<JsonArray>();
      JsonArray newArray = doc.createNestedArray("new_zone_data");
      
      for (JsonObject obj : zoneArray) {
        if (!(obj["callback"].as<int>() == callbackId && obj["id"].as<int>() == id)) {
          JsonObject newObj = newArray.createNestedObject();
          for (JsonPair pair : obj) {
            newObj[pair.key()] = pair.value();
          }
        }
      }
      
      doc.remove("zone_data");
      doc["zone_data"] = newArray;
      doc.remove("new_zone_data");
      
      file = SPIFFS.open("/zone_data.json", FILE_WRITE);
      if (file) {
        serializeJson(doc, file);
        file.close();
      }
    }
  }
  
  return true;
}

bool CallbackAlarms::checkAlarms() {
  if (timeStatus() != timeSet || !Callback) return false;
  unsigned long currentMinute = now() / 60;
  if (currentMinute == lastTriggerMinute) return false;

  bool stateChanged = false;
  bool dateTriggered = false;
  // Check date-based alarms first
  for (int i = 0; i < 10; i++) {
    if (!alarms[i].isActive || !alarms[i].isDateBased) continue;
    bool isMatch;
    if (alarms[i].isOneTime) {
      isMatch = (alarms[i].year == year() && alarms[i].month == month() &&
                 alarms[i].date == day() && alarms[i].hour == hour() &&
                 alarms[i].minute == minute());
    } else {
      isMatch = (alarms[i].month == month() && alarms[i].date == day() &&
                 alarms[i].hour == hour() && alarms[i].minute == minute());
    }
    if (isMatch) {
      // Load zone data for this alarm
      DynamicJsonDocument zoneDoc(1200);
      JsonObject zoneData = zoneDoc.to<JsonObject>();
      
      File file = SPIFFS.open("/zone_data.json", FILE_READ);
      if (file) {
        DynamicJsonDocument fileDoc(8192);
        DeserializationError error = deserializeJson(fileDoc, file);
        if (!error && fileDoc.containsKey("zone_data")) {
          JsonArray zoneArray = fileDoc["zone_data"].as<JsonArray>();
          for (JsonObject zoneObj : zoneArray) {
            if (zoneObj["callback"].as<int>() == callbackId && zoneObj["id"].as<int>() == i) {
              JsonObject srcZoneData = zoneObj["zone_data"].as<JsonObject>();
              for (JsonPair pair : srcZoneData) {
                zoneData[pair.key()] = pair.value();
              }
              break;
            }
          }
        }
        file.close();
      }
      
      Callback(callbackId, alarms[i].action, zoneData);
      if (alarms[i].isOneTime) {
        alarms[i].isActive = false;
        alarmCount--;
        stateChanged = true;
      }
      char timeStr[20];
      snprintf(timeStr, sizeof(timeStr), "%04d/%02d/%02d %02d:%02d:%02d",
               year(), month(), day(), hour(), minute(), second());
      Serial.println("Callback " + String(callbackId) + " triggered at " + String(timeStr));
      dateTriggered = true;
    }
  }

  // Check day-based alarms if no date-based triggered
  if (!dateTriggered) {
    for (int i = 0; i < 10; i++) {
      if (!alarms[i].isActive || alarms[i].isDateBased) continue;
      if (alarms[i].days[weekday() - 1] && alarms[i].hour == hour() &&
          alarms[i].minute == minute()) {
        // Load zone data for this alarm
        DynamicJsonDocument zoneDoc(1200);
        JsonObject zoneData = zoneDoc.to<JsonObject>();
        
        File file = SPIFFS.open("/zone_data.json", FILE_READ);
        if (file) {
          DynamicJsonDocument fileDoc(8192);
          DeserializationError error = deserializeJson(fileDoc, file);
          if (!error && fileDoc.containsKey("zone_data")) {
            JsonArray zoneArray = fileDoc["zone_data"].as<JsonArray>();
            for (JsonObject zoneObj : zoneArray) {
              if (zoneObj["callback"].as<int>() == callbackId && zoneObj["id"].as<int>() == i) {
                JsonObject srcZoneData = zoneObj["zone_data"].as<JsonObject>();
                for (JsonPair pair : srcZoneData) {
                  zoneData[pair.key()] = pair.value();
                }
                break;
              }
            }
          }
          file.close();
        }
        
        Callback(callbackId, alarms[i].action, zoneData);
        char timeStr[20];
        snprintf(timeStr, sizeof(timeStr), "%04d/%02d/%02d %02d:%02d:%02d",
                 year(), month(), day(), hour(), minute(), second());
        Serial.println("Callback " + String(callbackId) + " triggered at " + String(timeStr));
      }
    }
  }

  lastTriggerMinute = currentMinute;
  return stateChanged;
}

void CallbackAlarms::listAlarms(JsonArray& arr) {
  for (int i = 0; i < 10; i++) {
    if (!alarms[i].isActive) continue;
    JsonObject obj = arr.createNestedObject();
    obj["callback"] = callbackId;
    obj["id"] = i;
    obj["type"] = alarms[i].isDateBased ? "date" : "day";
    char timeStr[6];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d", alarms[i].hour, alarms[i].minute);
    obj["time"] = timeStr;
    obj["action"] = alarms[i].action ? "ON" : "OFF";
    
    // Add zone_data to the output
    DynamicJsonDocument zoneDoc(1200);
    JsonObject zoneData = zoneDoc.to<JsonObject>();
    File file = SPIFFS.open("/zone_data.json", FILE_READ);
    if (file) {
      DynamicJsonDocument fileDoc(8192);
      DeserializationError error = deserializeJson(fileDoc, file);
      if (!error && fileDoc.containsKey("zone_data")) {
        JsonArray zoneArray = fileDoc["zone_data"].as<JsonArray>();
        for (JsonObject zoneObj : zoneArray) {
          if (zoneObj["callback"].as<int>() == callbackId && zoneObj["id"].as<int>() == i) {
            JsonObject srcZoneData = zoneObj["zone_data"].as<JsonObject>();
            for (JsonPair pair : srcZoneData) {
              zoneData[pair.key()] = pair.value();
            }
            break;
          }
        }
      }
      file.close();
    }
    obj["zone_data"] = zoneData;
    
    if (alarms[i].isDateBased) {
      char dateStr[11];
      snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d", alarms[i].year, alarms[i].month, alarms[i].date);
      obj["date"] = dateStr;
      obj["oneTime"] = alarms[i].isOneTime;
    } else {
      JsonArray days = obj.createNestedArray("days");
      if (alarms[i].days[0]) days.add("sun");
      if (alarms[i].days[1]) days.add("mon");
      if (alarms[i].days[2]) days.add("tue");
      if (alarms[i].days[3]) days.add("wed");
      if (alarms[i].days[4]) days.add("thu");
      if (alarms[i].days[5]) days.add("fri");
      if (alarms[i].days[6]) days.add("sat");
    }
  }
}

void CallbackAlarms::clearAlarms() {
  for (int i = 0; i < 10; i++) {
    alarms[i].isActive = false;
    alarms[i].isDateBased = false;
    alarms[i].isOneTime = false;
    for (int j = 0; j < 7; j++) alarms[i].days[j] = false;
    alarms[i].year = 0;
    alarms[i].month = 0;
    alarms[i].date = 0;
    alarms[i].hour = 0;
    alarms[i].minute = 0;
    alarms[i].action = false;
  }
  alarmCount = 0;
}

// AlarmScheduler Implementation
AlarmScheduler::AlarmScheduler(unsigned long timeOffset) : callbacks{CallbackAlarms(1), CallbackAlarms(2), CallbackAlarms(3), CallbackAlarms(4)},
                                  rtc(nullptr), wire(nullptr), ntpUDP(nullptr), timeClient(nullptr), lastSyncMillis(0), offset(timeOffset), spiffsInitialized(false) {}

AlarmScheduler::~AlarmScheduler() {
  delete timeClient;
  delete ntpUDP;
  delete rtc;
  delete wire;
}

void AlarmScheduler::updateOffsetValue(unsigned long _offset){
  offset = _offset;
  timeClient->setTimeOffset(_offset);
  setRTCFromNTP();
}

void AlarmScheduler::begin(uint8_t rstPin, uint8_t datPin, uint8_t clkPin) {
  wire = new ThreeWire(datPin, clkPin, rstPin);
  rtc = new RtcDS1302<ThreeWire>(*wire);
  rtc->Begin();
  
  // Initialize NTP client components
  ntpUDP = new WiFiUDP();
  timeClient = new NTPClient(*ntpUDP, "pool.ntp.org", offset, 86400000);
  
  // Initialize SPIFFS
  spiffsInitialized = SPIFFS.begin(true); // Format on failure
  if (!spiffsInitialized) {
    Serial.println("Failed to initialize SPIFFS");
  } else {
    Serial.println("SPIFFS initialized successfully");
  }
  
  if (rtc->GetIsRunning()) {
    Serial.println("DS1302 initialized and running.");
  } else {
    Serial.println("DS1302 detected but not running. Set time using 'set' command.");
    rtc->SetIsRunning(true);
  }
  
  // Initial sync
  time_t rtcTime = getRtcTime();
  if (rtcTime > 0) setTime(rtcTime);
  
  // Load alarms from SPIFFS
  if (spiffsInitialized) {
    if (!loadAlarmsFromSpiffs()) {
      Serial.println("Failed to load alarms from SPIFFS");
    } else {
      Serial.println("Alarms loaded from SPIFFS successfully");
    }
  }
}

void AlarmScheduler::registerCallback(uint8_t id, void (*callback)(int id, bool isOn, JsonObject& zoneData)) {
  if (id < 1 || id > 4) return;
  callbacks[id - 1].setCallback(callback);
}

time_t AlarmScheduler::getRtcTime() {
  if (!rtc) return 0;
  RtcDateTime now = rtc->GetDateTime();
  if (now.IsValid()) {
    return now.TotalSeconds() + 946684800L; // Unix time (2000 to 1970 offset)
  }
  return 0;
}

bool AlarmScheduler::setRTCFromNTP() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }
  
  if (!timeClient) return false;
  
  if (!timeClient->update()) {
    return false;
  }
  
  unsigned long epochTime = timeClient->getEpochTime();
  
  // Validate epoch time (must be after 2025 and before 2100)
  if (epochTime < 1735689600 || epochTime > 4102444800) {
    Serial.println("Invalid NTP time received. Skipping RTC update.");
    return false;
  }
  
  // Convert epoch time to RtcDateTime
  time_t t = epochTime;
  struct tm *timeInfo = gmtime(&t);
  RtcDateTime ntpTime = RtcDateTime(
    timeInfo->tm_year + 1900,
    timeInfo->tm_mon + 1,
    timeInfo->tm_mday,
    timeInfo->tm_hour,
    timeInfo->tm_min,
    timeInfo->tm_sec
  );
  
  // Set RTC
  rtc->SetDateTime(ntpTime);
  
  // Update TimeLib as well
  setTime(epochTime);
  
  if (rtc->IsDateTimeValid()) {
    Serial.println("RTC time set from NTP successfully!");
    return true;
  } else {
    Serial.println("Failed to set RTC time from NTP.");
    return false;
  }
}

bool AlarmScheduler::syncWithNTP() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("No Wi-Fi connection. Cannot sync with NTP.");
    return false;
  }
  
  if (!timeClient) return false;
  
  // Begin NTP client if not already started
  timeClient->begin();
  
  bool success = setRTCFromNTP();
  
  // End NTP client to free resources
  timeClient->end();
  
  return success;
}

bool AlarmScheduler::saveZoneDataToSpiffs() {
  if (!spiffsInitialized) {
    return false;
  }

  DynamicJsonDocument doc(8192); // Larger buffer for zone data
  JsonArray zoneDataArray = doc.createNestedArray("zone_data");
  
  // Read existing zone data file if it exists
  File file = SPIFFS.open("/zone_data.json", FILE_READ);
  if (file) {
    DynamicJsonDocument existingDoc(8192);
    DeserializationError error = deserializeJson(existingDoc, file);
    if (!error && existingDoc.containsKey("zone_data")) {
      JsonArray existing = existingDoc["zone_data"].as<JsonArray>();
      for (JsonObject obj : existing) {
        JsonObject newObj = zoneDataArray.createNestedObject();
        for (JsonPair pair : obj) {
          newObj[pair.key()] = pair.value();
        }
      }
    }
    file.close();
  }

  // Write back to file
  file = SPIFFS.open("/zone_data.json", FILE_WRITE);
  if (!file) {
    return false;
  }

  if (serializeJson(doc, file) == 0) {
    file.close();
    return false;
  }

  file.close();
  return true;
}

bool AlarmScheduler::loadZoneDataForAlarm(uint8_t callbackId, uint8_t alarmId, JsonObject& zoneData) {
  if (!spiffsInitialized) {
    return false;
  }

  File file = SPIFFS.open("/zone_data.json", FILE_READ);
  if (!file) {
    return false;
  }

  DynamicJsonDocument doc(8192);
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    return false;
  }

  JsonArray zoneDataArray = doc["zone_data"].as<JsonArray>();
  for (JsonObject obj : zoneDataArray) {
    if (obj["callback"].as<int>() == callbackId && obj["id"].as<int>() == alarmId) {
      JsonObject srcZoneData = obj["zone_data"].as<JsonObject>();
      for (JsonPair pair : srcZoneData) {
        zoneData[pair.key()] = pair.value();
      }
      return true;
    }
  }
  return false;
}

bool AlarmScheduler::deleteZoneDataFromSpiffs(uint8_t callbackId, uint8_t alarmId) {
  if (!spiffsInitialized) {
    return false;
  }

  File file = SPIFFS.open("/zone_data.json", FILE_READ);
  if (!file) {
    return true; // No file means nothing to delete
  }

  DynamicJsonDocument doc(8192);
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    return false;
  }

  JsonArray zoneDataArray = doc["zone_data"].as<JsonArray>();
  JsonArray newArray = doc.createNestedArray("new_zone_data");
  
  // Copy all entries except the one to delete
  for (JsonObject obj : zoneDataArray) {
    if (!(obj["callback"].as<int>() == callbackId && obj["id"].as<int>() == alarmId)) {
      JsonObject newObj = newArray.createNestedObject();
      for (JsonPair pair : obj) {
        newObj[pair.key()] = pair.value();
      }
    }
  }

  // Replace the array
  doc.remove("zone_data");
  doc["zone_data"] = newArray;
  doc.remove("new_zone_data");

  // Write back to file
  file = SPIFFS.open("/zone_data.json", FILE_WRITE);
  if (!file) {
    return false;
  }

  if (serializeJson(doc, file) == 0) {
    file.close();
    return false;
  }

  file.close();
  return true;
}

bool AlarmScheduler::saveAlarmsToSpiffs() {
  if (!spiffsInitialized) {
    StaticJsonDocument<128> response;
    response["status"] = "error";
    response["message"] = "SPIFFS not initialized";
    serializeJson(response, Serial);
    Serial.println();
    return false;
  }

  DynamicJsonDocument doc(4096); // Large enough for 40 alarms
  JsonArray alarms = doc.createNestedArray("alarms");
  for (int i = 0; i < 4; i++) {
    callbacks[i].listAlarms(alarms);
  }

  File file = SPIFFS.open("/alarms.json", FILE_WRITE);
  if (!file) {
    StaticJsonDocument<128> response;
    response["status"] = "error";
    response["message"] = "Failed to open alarms.json for writing";
    serializeJson(response, Serial);
    Serial.println();
    return false;
  }

  if (serializeJson(doc, file) == 0) {
    file.close();
    StaticJsonDocument<128> response;
    response["status"] = "error";
    response["message"] = "Failed to write to alarms.json";
    serializeJson(response, Serial);
    Serial.println();
    return false;
  }

  file.close();
  return true;
}

bool AlarmScheduler::loadAlarmsFromSpiffs() {
  if (!spiffsInitialized) {
    StaticJsonDocument<128> response;
    response["status"] = "error";
    response["message"] = "SPIFFS not initialized";
    serializeJson(response, Serial);
    Serial.println();
    return false;
  }

  File file = SPIFFS.open("/alarms.json", FILE_READ);
  if (!file) {
    StaticJsonDocument<128> response;
    response["status"] = "error";
    response["message"] = "No alarms.json file found";
    serializeJson(response, Serial);
    Serial.println();
    return false;
  }

  DynamicJsonDocument doc(4096); // Large enough for 40 alarms
  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    StaticJsonDocument<128> response;
    response["status"] = "error";
    response["message"] = "Invalid JSON in alarms.json";
    serializeJson(response, Serial);
    Serial.println();
    return false;
  }

  // Clear existing alarms
  for (int i = 0; i < 4; i++) {
    callbacks[i].clearAlarms();
  }

  JsonArray alarms = doc["alarms"].as<JsonArray>();
  bool success = true;
  for (JsonObject alarm : alarms) {
    DynamicJsonDocument alarmDoc(512);
    for (JsonPair pair : alarm) {
      alarmDoc[pair.key()] = pair.value();
    }
    alarmDoc["command"] = "add";
    int callbackId = alarm["callback"].as<int>();
    if (callbackId < 1 || callbackId > 4) {
      StaticJsonDocument<128> response;
      response["status"] = "error";
      response["message"] = "Invalid callback ID in alarms.json: " + String(callbackId);
      serializeJson(response, Serial);
      Serial.println();
      success = false;
      continue;
    }
    if (!callbacks[callbackId - 1].addAlarm(alarmDoc)) {
      success = false;
    }
  }

  return success;
}

void AlarmScheduler::processJson(String& json) {
  StaticJsonDocument<768> doc;
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    StaticJsonDocument<128> response;
    response["status"] = "error";
    response["message"] = "Invalid JSON";
    serializeJson(response, Serial);
    Serial.println();
    return;
  }

  String command = doc["command"].as<String>();
  bool saveToSpiffs = false;

  if (command == "set") {
    String timeStr = doc["time"].as<String>();
    int year, month, date, hour, minute, second = 0;
    int fields = sscanf(timeStr.c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &date, &hour, &minute, &second);
    if (fields >= 5 && isValidDate(year, month, date) &&
        hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 && second >= 0 && second <= 59) {
      if (rtc) {
        RtcDateTime rtcTime(year, month, date, hour, minute, second);
        rtc->SetDateTime(rtcTime);
        setTime(rtcTime.TotalSeconds() + 946684800L);
      }
      doc.clear();
      doc["status"] = "success";
    } else {
      doc.clear();
      doc["status"] = "error";
      doc["message"] = "Invalid time format. Use YYYY-MM-DD HH:MM[:SS]";
    }
    serializeJson(doc, Serial);
    Serial.println();
  } else if (command == "ntp") {
    bool success = syncWithNTP();
    doc.clear();
    doc["command"] = "ntp";
    doc["status"] = success ? "success" : "error";
    if (!success) {
      if (WiFi.status() != WL_CONNECTED) {
        doc["message"] = "No Wi-Fi connection";
      } else {
        doc["message"] = "NTP sync failed";
      }
    } else {
      doc["message"] = "RTC synced with NTP (IST)";
      doc["time"] = printTime();
    }
    serializeJson(doc, Serial);
    Serial.println();
  } else if (command == "add") {
    int callbackId = doc["callback"].as<int>();
    if (callbackId < 1 || callbackId > 4) {
      doc.clear();
      doc["status"] = "error";
      doc["message"] = "Invalid callback ID. Use 1–4";
      serializeJson(doc, Serial);
      Serial.println();
      return;
    }
    bool success = callbacks[callbackId - 1].addAlarm(doc);
    if (!success && !doc.containsKey("status")) {
      doc.clear();
      doc["status"] = "error";
      doc["message"] = "Failed to add alarm";
    }
    serializeJson(doc, Serial);
    Serial.println();
    if (success) saveToSpiffs = true;
  } else if (command == "delete") {
    int callbackId = doc["callback"].as<int>();
    int id = doc["id"].as<int>();
    if (callbackId < 1 || callbackId > 4) {
      doc.clear();
      doc["status"] = "error";
      doc["message"] = "Invalid callback ID";
      serializeJson(doc, Serial);
      Serial.println();
      return;
    }
    bool success = callbacks[callbackId - 1].deleteAlarm(id);
    doc.clear();
    doc["status"] = success ? "success" : "error";
    if (!success) doc["message"] = "Invalid ID";
    serializeJson(doc, Serial);
    Serial.println();
    if (success) saveToSpiffs = true;
  } else if (command == "list") {
    doc.clear();
    doc["command"] = "list";
    JsonArray alarms = doc.createNestedArray("alarms");
    for (int i = 0; i < 4; i++) {
      callbacks[i].listAlarms(alarms);
    }
    serializeJsonPretty(doc, Serial);
    Serial.println();
  } else if (command == "time") {
    doc.clear();
    doc["command"] = "time";
    doc["time"] = printTime();
    serializeJsonPretty(doc, Serial);
    Serial.println();
  } else {
    StaticJsonDocument<128> response;
    response["status"] = "error";
    response["message"] = "Unknown command";
    serializeJson(response, Serial);
    Serial.println();
  }

  if (saveToSpiffs) {
    if (!saveAlarmsToSpiffs()) {
      Serial.println("Failed to save alarms to SPIFFS after command");
    } else {
      Serial.println("Alarms saved to SPIFFS successfully");
    }
  }
}

void AlarmScheduler::checkAlarms() {
  // Sync time every 24 hours
  if (millis() - lastSyncMillis >= 86400000UL) {
    time_t rtcTime = getRtcTime();
    if (rtcTime > 0) setTime(rtcTime);
    lastSyncMillis = millis();
  }
  bool stateChanged = false;
  for (int i = 0; i < 4; i++) {
    if (callbacks[i].checkAlarms()) {
      stateChanged = true;
    }
  }
  if (stateChanged) {
    if (!saveAlarmsToSpiffs()) {
      Serial.println("Failed to save alarms to SPIFFS after state change");
    } else {
      Serial.println("Alarms saved to SPIFFS successfully after state change");
    }
  }
}

String AlarmScheduler::printTime() {
  if (!rtc) return "RTC not initialized!";
  RtcDateTime now = rtc->GetDateTime();
  if (now.IsValid()) {
    // Validate year to catch garbage values
    if (now.Year() < 2025 || now.Year() > 2099) {
      return "Invalid RTC year: " + String(now.Year()) + ". RTC may need reset.";
    }
    char dateTimeString[20];
    snprintf(dateTimeString, sizeof(dateTimeString), "%04u/%02u/%02u %02u:%02u:%02u",
             now.Year(), now.Month(), now.Day(), now.Hour(), now.Minute(), now.Second());
    return String(dateTimeString);
  }
  return "RTC time invalid!";
}

bool AlarmScheduler::isTimeSet() {
  return timeStatus() == timeSet;
}
