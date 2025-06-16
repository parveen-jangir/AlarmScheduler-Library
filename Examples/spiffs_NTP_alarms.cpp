#include <Arduino.h>
#include <WiFi.h>
#include "AlarmScheduler.h"

// Create AlarmScheduler instance
AlarmScheduler scheduler(19800); //19800 for india


// Callback functions
void callback1(int id, bool isOn) {
  Serial.println("=== CALLBACK 1 TRIGGERED ===");
  Serial.println("ID: " + String(id) + ", Action: " + (isOn ? "ON" : "OFF"));
}

void callback2(int id, bool isOn) {
  Serial.println("=== CALLBACK 2 TRIGGERED ===");
  Serial.println("ID: " + String(id) + ", Action: " + (isOn ? "ON" : "OFF"));
}

void callback3(int id, bool isOn) {
  Serial.println("=== CALLBACK 3 TRIGGERED ===");
  Serial.println("ID: " + String(id) + ", Action: " + (isOn ? "ON" : "OFF"));
}

void callback4(int id, bool isOn) {
  Serial.println("=== CALLBACK 4 TRIGGERED ===");
  Serial.println("ID: " + String(id) + ", Action: " + (isOn ? "ON" : "OFF"));
}

void printMenu() {
  Serial.println("\n=================== ALARM SCHEDULER MENU ===================");
  Serial.println("Available Commands (send as JSON):");
  Serial.println();
  Serial.println("1. Set Time:");
  Serial.println("   {\"command\":\"set\",\"time\":\"2025-06-16 14:30:00\"}");
  Serial.println();
  Serial.println("2. Sync with NTP (requires WiFi):");
  Serial.println("   {\"command\":\"ntp\"}");
  Serial.println();
  Serial.println("3. Add Day-based Alarm:");
  Serial.println("   {\"command\":\"add\",\"callback\":1,\"type\":\"day\",\"days\":[\"mon\",\"wed\",\"fri\"],\"time\":\"08:30\",\"action\":\"ON\"}");
  Serial.println();
  Serial.println("4. Add Date-based Alarm (yearly):");
  Serial.println("   {\"command\":\"add\",\"callback\":2,\"type\":\"date\",\"date\":\"2025-12-25\",\"time\":\"09:00\",\"action\":\"ON\",\"oneTime\":false}");
  Serial.println();
  Serial.println("5. Add One-time Date Alarm:");
  Serial.println("   {\"command\":\"add\",\"callback\":3,\"type\":\"date\",\"date\":\"2025-06-20\",\"time\":\"15:00\",\"action\":\"OFF\",\"oneTime\":true}");
  Serial.println();
  Serial.println("6. Delete Alarm:");
  Serial.println("   {\"command\":\"delete\",\"callback\":1,\"id\":0}");
  Serial.println();
  Serial.println("7. List All Alarms:");
  Serial.println("   {\"command\":\"list\"}");
  Serial.println();
  Serial.println("8. Get Current Time:");
  Serial.println("   {\"command\":\"time\"}");
  Serial.println();
  Serial.println("Simple Commands:");
  Serial.println("- menu       : Show this menu");
  Serial.println("- status     : Show current status");
  Serial.println("===========================================================");
}

void printStatus() {
  Serial.println("\n=================== SYSTEM STATUS ===================");
  Serial.println("WiFi Status: " + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected"));
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("IP Address: " + WiFi.localIP().toString());
  }
  Serial.println("RTC Status: " + String(scheduler.isTimeSet() ? "Time Set" : "Time Not Set"));
  Serial.println("Current Time: " + scheduler.printTime());
  Serial.println("====================================================");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("===== AlarmScheduler with SPIFFS Test Program =====");
  
  // Initialize AlarmScheduler
  scheduler.begin(16, 13, 14);
  
  // Register callback functions
  scheduler.registerCallback(1, callback1);
  scheduler.registerCallback(2, callback2);
  scheduler.registerCallback(3, callback3);
  scheduler.registerCallback(4, callback4);
  
  Serial.println("AlarmScheduler and LED Strips initialized successfully!");
  
  // Show initial status
  printStatus();
  printMenu();
}

void loop() {
  // Check for alarms
  scheduler.checkAlarms();
  
  // Handle serial commands
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    // Handle simple commands
    if (input == "menu") {
      printMenu();
    }
    else if (input == "status") {
      printStatus();
    }
    else if (input.startsWith("{")) {
      // Handle JSON commands
      Serial.println("Processing command: " + input);
      scheduler.processJson(input);
    }
    else if (input.length() > 0) {
      Serial.println("Unknown command: " + input);
      Serial.println("Type 'menu' for available commands");
    }
  }
  
  
  // Print time every 10 seconds
  static unsigned long lastTimePrint = 0;
  if (millis() - lastTimePrint >= 10000) {
    Serial.println("Current Time: " + scheduler.printTime());
    lastTimePrint = millis();
  }
  
  delay(100);
}
