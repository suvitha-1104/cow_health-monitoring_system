/*
  ============================================================
  Cow Health Monitor - ESP32 Based
  ------------------------------------------------------------
  Features:
   - DS18B20 body temperature sensing
   - MPU6050 accelerometer based activity/motion detection
   - Battery voltage monitoring (via voltage divider)
   - ESP32 Wi-Fi Access Point + Web Dashboard (no internet needed)
   - Live JSON data API at /data
   - Health classification: HEALTHY / INACTIVE / SICK-HYPER

  Hardware:
   - ESP32 Dev Board
   - DS18B20 temperature sensor -> GPIO 5 (with 4.7k pull-up to 3.3V)
   - MPU6050 accelerometer -> I2C (SDA: GPIO 19, SCL: GPIO 18)
   - Battery voltage divider output -> GPIO 34 (ADC, input only)

  Libraries required (install via Arduino Library Manager):
   - OneWire
   - DallasTemperature
   - (WiFi.h, WebServer.h, Wire.h are built-in with ESP32 core)
  ============================================================
*/

#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ---------------- GPIO Pin Definitions ----------------
#define MPU_SDA       19
#define MPU_SCL       18
#define ONE_WIRE_BUS  5
#define BATTERY_PIN   34   // ADC input-only pin

// ---------------- Object Creation ----------------
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
WebServer server(80);

// ---------------- Wi-Fi Access Point Credentials ----------------
const char* ssid     = "Cow_Health_Monitor";
const char* password  = "12345678";   // min 8 characters required by ESP32 AP

// ---------------- Sensor / State Variables ----------------
float tempC = 0, tempF = 0;
float batteryVoltage = 0;

int16_t ax, ay, az;
int16_t prevAx = 0, prevAy = 0, prevAz = 0;

int activity = 0;
String health = "HEALTHY";

// ---------------- Function Declarations ----------------
String webpage();
void readMPU();
void handleRoot();
void handleData();

//                          SETUP
void setup() {
  Serial.begin(115200);
  delay(200);

  // Start Wi-Fi Access Point
  WiFi.softAP(ssid, password);
  Serial.println("Access Point Started");
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());   // usually 192.168.4.1

  // I2C init for MPU6050
  Wire.begin(MPU_SDA, MPU_SCL);

  // Wake up MPU6050 (it starts in sleep mode)
  Wire.beginTransmission(0x68);
  Wire.write(0x6B);   // PWR_MGMT_1 register
  Wire.write(0);       // wake up
  Wire.endTransmission(true);

  // DS18B20 init
  sensors.begin();

  // Baseline accelerometer reading
  readMPU();
  prevAx = ax;
  prevAy = ay;
  prevAz = az;

  // Web server routes
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();
  Serial.println("Web server started");
}

//                          LOOP
void loop() {
  // ---- Temperature ----
  sensors.requestTemperatures();
  tempC = sensors.getTempCByIndex(0);
  tempF = (tempC * 9.0 / 5.0) + 32.0;

  // ---- Accelerometer ----
  readMPU();

  // Orientation-independent activity calculation
  int delta = abs(ax - prevAx) + abs(ay - prevAy) + abs(az - prevAz);
  activity = delta / 50;

  // Noise filtering
  if (activity < 10) activity = 0;

  // Save current as previous for next loop
  prevAx = ax;
  prevAy = ay;
  prevAz = az;

  // ---- Battery Voltage ----
  int adc = analogRead(BATTERY_PIN);
  float v = (adc / 4095.0) * 3.3;
  batteryVoltage = v * 2.0;  // adjust multiplier to match your divider ratio

  // ---- Health Classification ----
  if (tempC >= 40.0 || activity > 250) {
    health = "SICK / HYPER";
  } else if (activity == 0) {
    health = "INACTIVE";
  } else {
    health = "HEALTHY";
  }

  // ---- Handle web clients ----
  server.handleClient();

  delay(1000);
}

//                 MPU6050 Accelerometer Read
void readMPU() {
  Wire.beginTransmission(0x68);
  Wire.write(0x3B);  // starting register for accelerometer data (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(0x68, 6, true);

  ax = (Wire.read() << 8 | Wire.read());
  ay = (Wire.read() << 8 | Wire.read());
  az = (Wire.read() << 8 | Wire.read());
}
//                 JSON Data API (/data)
void handleData() {
  String json = "{";
  json += "\"tempC\":" + String(tempC, 2) + ",";
  json += "\"tempF\":" + String(tempF, 2) + ",";
  json += "\"activity\":" + String(activity) + ",";
  json += "\"battery\":" + String(batteryVoltage, 2) + ",";
  json += "\"health\":\"" + health + "\"";
  json += "}";

  server.send(200, "application/json", json);
}

//                 Root Page Handler (/)
void handleRoot() {
  server.send(200, "text/html", webpage());
}


//                 HTML + CSS + JS Dashboard
String webpage() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Cow Health Monitor</title>";
  html += "<style>";
  html += "body{font-family:Arial,Helvetica,sans-serif;background:#f2f2f2;margin:0;padding:0;transition:background 0.3s;}";
  html += ".container{max-width:480px;margin:0 auto;padding:16px;}";
  html += "h1{text-align:center;color:#2c3e50;font-size:22px;}";
  html += ".card{background:#fff;border-radius:12px;box-shadow:0 2px 8px rgba(0,0,0,0.15);padding:16px;margin-bottom:14px;}";
  html += ".label{color:#7f8c8d;font-size:13px;text-transform:uppercase;}";
  html += ".value{font-size:28px;font-weight:bold;color:#2c3e50;}";
  html += ".status{font-size:20px;font-weight:bold;padding:10px;border-radius:8px;text-align:center;color:#fff;}";
  html += ".HEALTHY{background:#27ae60;}";
  html += ".INACTIVE{background:#f39c12;}";
  html += ".SICK{background:#e74c3c;}";
  html += "#datetime{text-align:center;color:#555;font-size:13px;margin-bottom:10px;}";
  html += "canvas{width:100%;background:#fafafa;border-radius:8px;}";
  html += "@keyframes blink{0%{background:#ffffff;}50%{background:#ffcccc;}100%{background:#ffffff;}}";
  html += ".alert{animation:blink 1s infinite;}";
  html += "</style></head><body id='body'>";

  html += "<div class='container'>";
  html += "<h1>🐄 Cow Health Monitor</h1>";
  html += "<div id='datetime'></div>";

  html += "<div class='card'><div class='label'>Body Temperature</div>";
  html += "<div class='value' id='temp'>-- &deg;C / -- &deg;F</div></div>";

  html += "<div class='card'><div class='label'>Activity Level</div>";
  html += "<div class='value' id='activity'>--</div>";
  html += "<canvas id='graph' height='80'></canvas></div>";

  html += "<div class='card'><div class='label'>Battery Voltage</div>";
  html += "<div class='value' id='battery'>-- V</div></div>";

  html += "<div class='card'><div class='label'>Health Status</div>";
  html += "<div class='status' id='status'>--</div></div>";

  html += "</div>"; // container

  // ---------------- JavaScript ----------------
  html += "<script>";
  html += "let history=[];";
  html += "function updateDateTime(){";
  html += "document.getElementById('datetime').innerText=new Date().toLocaleString();}";
  html += "setInterval(updateDateTime,1000);updateDateTime();";

  html += "function drawGraph(){";
  html += "const c=document.getElementById('graph');const ctx=c.getContext('2d');";
  html += "c.width=c.clientWidth;";
  html += "ctx.clearRect(0,0,c.width,c.height);";
  html += "ctx.beginPath();ctx.strokeStyle='#2980b9';ctx.lineWidth=2;";
  html += "let step=c.width/Math.max(history.length-1,1);";
  html += "history.forEach((v,i)=>{let x=i*step;let y=c.height-Math.min(v,c.height);";
  html += "if(i===0){ctx.moveTo(x,y);}else{ctx.lineTo(x,y);}});";
  html += "ctx.stroke();}";

  html += "function beep(){";
  html += "try{const ctx=new (window.AudioContext||window.webkitAudioContext)();";
  html += "const o=ctx.createOscillator();const g=ctx.createGain();";
  html += "o.connect(g);g.connect(ctx.destination);o.frequency.value=880;";
  html += "o.start();setTimeout(()=>o.stop(),300);}catch(e){}}";

  html += "function fetchData(){";
  html += "fetch('/data').then(r=>r.json()).then(d=>{";
  html += "document.getElementById('temp').innerHTML=d.tempC.toFixed(1)+' &deg;C / '+d.tempF.toFixed(1)+' &deg;F';";
  html += "document.getElementById('activity').innerText=d.activity;";
  html += "document.getElementById('battery').innerText=d.battery.toFixed(2)+' V';";
  html += "let statusEl=document.getElementById('status');";
  html += "let bodyEl=document.getElementById('body');";
  html += "statusEl.innerText=d.health;";
  html += "statusEl.className='status';bodyEl.classList.remove('alert');";
  html += "if(d.health.indexOf('SICK')>=0){statusEl.classList.add('SICK');bodyEl.classList.add('alert');beep();}";
  html += "else if(d.health==='INACTIVE'){statusEl.classList.add('INACTIVE');}";
  html += "else{statusEl.classList.add('HEALTHY');}";
  html += "history.push(d.activity);if(history.length>30)history.shift();";
  html += "drawGraph();";
  html += "});}";

  html += "setInterval(fetchData,1000);fetchData();";
  html += "</script>";

  html += "</body></html>";
  return html;
}
