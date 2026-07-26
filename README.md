# cow_health-monitoring_system
A device to help cattle rearers to track health of cow in their cattle and take better care for them

An offline, battery-powered livestock health monitoring system built on the ESP32. It tracks body temperature, movement/activity, and battery level, then serves a live mobile-friendly dashboard over its own Wi-Fi hotspot — no internet or router required.
FEATURES:
Body temperature via DS18B20 (Celsius & Fahrenheit)
Orientation-independent activity detection via MPU6050 accelerometer
Battery voltage monitoring via a resistor voltage divider
Standalone Wi-Fi Access Point — connect your phone directly to the ESP32
Live web dashboard (/) with auto-refreshing cards, an activity graph, color-coded health status, blinking alert background, and audible alarm
JSON API (/data) for integrating with other apps
Automatic health classification: HEALTHY, INACTIVE, or SICK / HYPER
HARDWARE:
Component	ESP32 Pin
DS18B20 data	GPIO 5 (with 4.7kΩ pull-up to 3.3V)
MPU6050 SDA	GPIO 19
MPU6050 SCL	GPIO 18
Battery divider output	GPIO 34 (ADC, input-only)
LIBRARIES REQURIED:
Install these via Arduino IDE Library Manager:

OneWire
DallasTemperature
(WiFi.h, WebServer.h, Wire.h ship with the ESP32 board package.)

SETUP:
Wire up the DS18B20, MPU6050, and battery voltage divider as per the pin table above.
Open Cow_Health_Monitor.ino in the Arduino IDE.
Select your ESP32 board and port.
Upload the sketch.
On your phone, connect to the Wi-Fi network Cow_Health_Monitor (password: 12345678).
Open http://192.168.4.1 in a browser to view the dashboard.
NOTES:
Adjust the batteryVoltage multiplier in the code to match your actual voltage divider ratio.
Tune the activity noise threshold and the SICK / HYPER thresholds (temperature ≥ 40°C, activity > 250) to fit your specific animal and sensor mounting.

