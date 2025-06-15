#include <AlarmScheduler.h>

AlarmScheduler scheduler;

void togglePin5(int id, bool isOn) {
  digitalWrite(5, isOn ? HIGH : LOW);
  Serial.println("LED on GPIO 5 set to " + String(isOn ? "ON" : "OFF"));
}

void setup() {
  Serial.begin(115200);
  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);
  scheduler.begin(16, 13, 14); // DS1302: RST=16, DAT=13, CLK=14
  scheduler.registerCallback(1, togglePin5);
  Serial.println("LED Toggle Example Started");
}

void loop() {
  if (Serial.available()) {
    String json = Serial.readStringUntil('\n');
    scheduler.processJson(json);
  }
  scheduler.checkAlarms();
}