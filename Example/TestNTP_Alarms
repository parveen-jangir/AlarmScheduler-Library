#include <Arduino.h>
#include <WiFi.h>
#include "AlarmScheduler.h"

// DS1302 RTC Pin Definitions
#define RST_PIN 16 // CE
#define DAT_PIN 13 // IO
#define CLK_PIN 14 // SCLK

// Wi-Fi Credentials - Replace with your credentials
const char* ssid = "Engineer";
const char* password = "12345678";

// Create AlarmScheduler instance
AlarmScheduler scheduler;

// Callback functions for different alarm IDs
void callback1(int id, bool isOn) {
  Serial.println("=== CALLBACK 1 TRIGGERED ===");
  Serial.println("ID: " + String(id) + ", Action: " + (isOn ? "ON" : "OFF"));
  // Add your device control logic here
  // Example: digitalWrite(LED_PIN, isOn ? HIGH : LOW);
}

void callback2(int id, bool isOn) {
  Serial.println("=== CALLBACK 2 TRIGGERED ===");
  Serial.println("ID: " + String(id) + ", Action: " + (isOn ? "ON" : "OFF"));
  // Add your device control logic here
}

void callback3(int id, bool isOn) {
  Serial.println("=== CALLBACK 3 TRIGGERED ===");
  Serial.println("ID: " + String(id) + ", Action: " + (isOn ? "ON" : "OFF"));
  // Add your device control logic here
}

void callback4(int id, bool isOn) {
  Serial.println("=== CALLBACK 4 TRIGGERED ===");
  Serial.println("ID: " + String(id) + ", Action: " + (isOn ? "ON" : "OFF"));
  // Add your device control logic here
}

void connectToWiFi() {
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.println("Connected to Wi-Fi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("Failed to connect to Wi-Fi");
  }
}

void disconnectWiFi() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.println("Wi-Fi disconnected to save power");
}

void printMenu() {
  Serial.println("\n=================== ALARM SCHEDULER MENU ===================");
  Serial.println("Available Commands (send as JSON):");
  Serial.println();
  Serial.println("1. Set Time:");
  Serial.println("   {\"command\":\"set\",\"time\":\"2025-06-15 14:30:00\"}");
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
  Serial.println("- wifi_on    : Connect to WiFi");
  Serial.println("- wifi_off   : Disconnect WiFi");
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
  
  Serial.println("===== Enhanced AlarmScheduler Test Program =====");
  
  // Initialize AlarmScheduler
  scheduler.begin(RST_PIN, DAT_PIN, CLK_PIN);
  
  // Register callback functions
  scheduler.registerCallback(1, callback1);
  scheduler.registerCallback(2, callback2);
  scheduler.registerCallback(3, callback3);
  scheduler.registerCallback(4, callback4);
  
  Serial.println("AlarmScheduler initialized successfully!");
  
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
    else if (input == "wifi_on") {
      connectToWiFi();
    }
    else if (input == "wifi_off") {
      disconnectWiFi();
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
  
  // Print time every 10 seconds (optional - remove if too verbose)
  static unsigned long lastTimePrint = 0;
  if (millis() - lastTimePrint >= 10000) {
    Serial.println("Current Time: " + scheduler.printTime());
    lastTimePrint = millis();
  }
  
  delay(100); // Small delay to prevent excessive CPU usage
}
