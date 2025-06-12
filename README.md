# AlarmScheduler Library

The **AlarmScheduler** library is an Arduino-compatible library for scheduling and managing alarms on an ESP32 using a **DS1302 Real-Time Clock (RTC)**. It supports **weekly day-based**, **one-time date-based**, and **yearly date-based** alarms, with a **JSON-based interface** for configuration and control. Designed for **non-blocking operation**, it is ideal for applications like **home automation**, **IoT devices**, and **timed control systems**.

---

## ✨ Features

### Three Alarm Types:
- **Weekly Day-Based**: Alarms trigger on specific days (e.g., every Monday at 08:30).
- **One-Time Date-Based**: Alarms trigger once on a specific date and time (e.g., 2025-06-15 at 08:30).
- **Yearly Date-Based**: Alarms trigger annually on a specific month and day (e.g., every June 15 at 09:00).

### Additional Features:
- **Multiple Callbacks**: Supports up to 4 callbacks (IDs 1–4), each with up to 10 alarms.
- **JSON Interface**: Add, delete, configure, and list alarms via Serial or other interfaces.
- **Non-Blocking Operation**: Checks alarms every 500ms, debounced to minute-level.
- **Robust Error Handling**: Validates JSON, dates, and times with clear messages.
- **Time Synchronization**: Syncs with DS1302 every 5 minutes.
- **Flexible Output**: Callback functions receive `id` and `isOn` (ON/OFF) flags.
- **Priority Handling**: Date-based alarms override day-based alarms at same time.
- **RTC Integration**: Easy pin configuration for DS1302 (RST, DAT, CLK).

---

## 💡 Use Cases

### Home Automation
- Turn lights on/off on specific days/times.
- Trigger one-time events (e.g., reminder notifications).

### IoT Devices
- Schedule sensor readings or actuator actions.
- Log events with timestamped Serial outputs.

### Timed Control Systems
- HVAC maintenance reminders.
- Lab equipment automation.

### Educational Projects
- Demonstrate Arduino, RTC, and JSON parsing concepts.

---

## 📦 Dependencies

Install these via the Arduino Library Manager:

- **RtcDS1302** by Makuna (v2.x)
- **Time** by PaulStoffregen (v1.x)
- **ArduinoJson** by Benoit Blanchon (v6.x)
