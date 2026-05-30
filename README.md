# Predictive Maintenance System 🔧

> IoT-based real-time machine health monitoring using vibration and temperature sensors on Arduino Uno

![Arduino](https://img.shields.io/badge/Arduino-Uno-teal)
![Sensors](https://img.shields.io/badge/Sensors-MPU6050%20%7C%20DS18B20-blue)
![License](https://img.shields.io/badge/License-MIT-green)

---

## 📌 Problem Statement

Unexpected industrial machine failures cause costly downtime and safety hazards. Traditional maintenance is either reactive (fix after failure) or scheduled (fix on a timetable). This system enables **predictive maintenance** — detecting abnormal conditions *before* failure occurs, using real-time vibration and temperature data.

---

## 🏗️ System Architecture

```
[MPU6050]──────┐
               ├──► Arduino Uno ──► LCD Display (16x2 I2C)
[DS18B20]──────┘         │
                         ├──► Buzzer (DANGER alert)
                         └──► Serial Monitor (data logging)
```

---

## 🧠 Core Algorithm

### Auto-Calibration (5 seconds on startup)
```
Baseline = average vibration over 5 seconds (normal machine state)
Warning Threshold = Baseline + 0.25
Danger  Threshold = Baseline + 0.50
```

### Machine Health Formula
```
Health% = 100 - VibrationPenalty(0–60) - TemperaturePenalty(0–40)
```

| Health % | Status |
|---|---|
| 90 – 100 | Excellent |
| 70 – 89  | Good |
| 40 – 69  | Maintenance Required |
| Below 40 | Critical |

### State Classification

| State   | Condition |
|---|---|
| 🟢 NORMAL  | Vibration < Warning threshold AND Temp < 60°C |
| 🟡 WARNING | Vibration approaching danger OR Temp 60–80°C |
| 🔴 DANGER  | Vibration > Danger threshold OR Temp > 80°C |

---

## 🔌 Hardware Required

| Component | Quantity |
|---|---|
| Arduino Uno | 1 |
| MPU6050 Accelerometer/Gyro | 1 |
| DS18B20 Digital Temperature Sensor | 1 |
| 16×2 I2C LCD Display | 1 |
| Active Buzzer | 1 |
| 4.7kΩ Resistor (DS18B20 pull-up) | 1 |
| Breadboard + Jumper Wires | — |

---

## 🔧 Circuit Connections

| Component | Arduino Pin |
|---|---|
| MPU6050 SDA | A4 |
| MPU6050 SCL | A5 |
| MPU6050 VCC / GND | 3.3V / GND |
| DS18B20 Data | D4 (with 4.7kΩ to VCC) |
| DS18B20 VCC / GND | 5V / GND |
| LCD SDA / SCL | A4 / A5 |
| Buzzer (+) | D8 |

---

## 📁 Project Structure

```
PredictiveMaintenance/
├── PredictiveMaintenance.ino   # Complete Arduino sketch
└── README.md
```

---

## 🚀 Getting Started

### 1. Install Arduino Libraries
Open Arduino IDE → Tools → Manage Libraries → Install:
- `MPU6050` by Electronic Cats
- `DallasTemperature` by Miles Burton
- `OneWire` by Jim Studt
- `LiquidCrystal I2C` by Frank de Brabander

### 2. Upload
1. Open `PredictiveMaintenance.ino` in Arduino IDE
2. Select **Board**: Arduino Uno
3. Select correct **Port**
4. Upload

### 3. Startup Sequence
```
LCD: "Calibrating..."
LCD: "Keep Machine ON"
[5 seconds of baseline measurement]
LCD: "Cal. Complete!"
LCD: "System Ready"
```

---

## 📊 Serial Monitor Output

```
================================================
  Predictive Maintenance System — Ready
================================================
  Baseline Vibration : 0.182
  Warning Threshold  : 0.432
  Danger Threshold   : 0.682
------------------------------------------------
  Time(s)  Vib    Temp   State    Health
------------------------------------------------
  6s       0.21   32.5C  NORMAL   97%
  7s       0.23   32.6C  NORMAL   96%
  12s      0.48   35.1C  WARNING  71%
  18s      0.74   38.2C  DANGER   42%
```

---

## 📺 LCD Display Logic

**NORMAL / WARNING state:**
```
VIB:0.23
STATE:NORMAL
```

**DANGER state:**
```
TEMP:38.5°C
HEALTH:42% MNT
```

---

## 🎯 Applications

- Industrial motors and pumps
- Rotating machinery and compressors
- Manufacturing equipment
- Condition-based maintenance systems
- Academic IoT demonstrations

---

## 👨‍💻 Author

**Pranav** — AI & Data Science, VIT Pune  
[GitHub](https://github.com/PranavNJain)

---

## 📄 License

MIT License
