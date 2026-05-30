/*
  ============================================================
  Predictive Maintenance System
  Using Vibration (MPU6050) & Temperature (DS18B20) Monitoring
  ------------------------------------------------------------
  Hardware : Arduino Uno
  Sensors  : MPU6050 (accelerometer/gyro), DS18B20 (temperature)
  Output   : 16x2 I2C LCD, Buzzer, Serial Monitor

  Machine States : NORMAL → WARNING → DANGER
  Health Formula : Health% = 100 - vibPenalty - tempPenalty

  Health Ranges:
    90–100%  → Excellent
    70–89%   → Good
    40–69%   → Maintenance Required
    <40%     → Critical
  ============================================================
*/

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <MPU6050.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// ── Pin Definitions ────────────────────────────────────────
#define ONE_WIRE_BUS  4     // DS18B20 data pin
#define BUZZER_PIN    8     // Active buzzer

// ── Thresholds ────────────────────────────────────────────
#define TEMP_WARNING     60.0   // °C — warning threshold
#define TEMP_DANGER      80.0   // °C — danger threshold
#define CALIBRATION_SECS 5      // Calibration duration

// Dynamic vibration thresholds (set after calibration)
float vibWarningThreshold = 0.40;
float vibDangerThreshold  = 0.70;

// ── Objects ────────────────────────────────────────────────
LiquidCrystal_I2C lcd(0x27, 16, 2);
MPU6050 mpu;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensor(&oneWire);

// ── Data Log (last 10 readings) ───────────────────────────
struct LogEntry {
  unsigned long timestamp;
  float vibration;
  float temperature;
  String state;
  int health;
};
const int LOG_SIZE = 10;
LogEntry dataLog[LOG_SIZE];
int logIndex = 0;

// ── Calibration ────────────────────────────────────────────
float baselineVibration = 0.0;
bool  calibrationDone   = false;

// ── Setup ──────────────────────────────────────────────────
void setup() {
  Serial.begin(9600);
  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);

  // LCD init
  lcd.init();
  lcd.backlight();

  // MPU6050 init
  Wire.begin();
  mpu.initialize();
  if (!mpu.testConnection()) {
    lcd.setCursor(0, 0); lcd.print("MPU6050 ERROR!");
    Serial.println("[ERROR] MPU6050 not found.");
    while (true);
  }

  // DS18B20 init
  tempSensor.begin();

  // Calibration
  calibrate();

  Serial.println("================================================");
  Serial.println("  Predictive Maintenance System — Ready");
  Serial.println("================================================");
  Serial.print("  Baseline Vibration : "); Serial.println(baselineVibration, 3);
  Serial.print("  Warning Threshold  : "); Serial.println(vibWarningThreshold, 3);
  Serial.print("  Danger Threshold   : "); Serial.println(vibDangerThreshold, 3);
  Serial.println("------------------------------------------------");
  Serial.println("  Time(s)  Vib    Temp   State    Health");
  Serial.println("------------------------------------------------");
}

// ── Main Loop ──────────────────────────────────────────────
void loop() {
  // 1. Read sensors
  float vibration   = readVibration();
  float temperature = readTemperature();

  // 2. Classify state
  String state = classifyState(vibration, temperature);

  // 3. Calculate health
  int health = calculateHealth(vibration, temperature);

  // 4. Update LCD
  updateLCD(vibration, temperature, state, health);

  // 5. Buzzer alert
  if (state == "DANGER") {
    tone(BUZZER_PIN, 1000, 300);
  } else {
    noTone(BUZZER_PIN);
  }

  // 6. Log data
  logData(vibration, temperature, state, health);

  // 7. Serial output
  Serial.print("  ");
  Serial.print(millis() / 1000);        Serial.print("s\t");
  Serial.print(vibration, 2);           Serial.print("\t");
  Serial.print(temperature, 1);         Serial.print("C\t");
  Serial.print(state);                  Serial.print("\t");
  Serial.print(health);                 Serial.println("%");

  delay(1000);
}

// ── Calibration ────────────────────────────────────────────
void calibrate() {
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Calibrating...");
  lcd.setCursor(0, 1); lcd.print("Keep Machine ON ");
  Serial.println("[CAL] Starting 5-second calibration...");

  float sum = 0.0;
  int samples = 0;
  unsigned long start = millis();

  while (millis() - start < CALIBRATION_SECS * 1000UL) {
    sum += readVibration();
    samples++;

    // Progress dots on LCD
    lcd.setCursor(samples % 16, 1);

    delay(200);
  }

  baselineVibration  = sum / samples;
  vibWarningThreshold = baselineVibration + 0.25;
  vibDangerThreshold  = baselineVibration + 0.50;
  calibrationDone     = true;

  Serial.print("[CAL] Done. Baseline = ");
  Serial.println(baselineVibration, 3);

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Cal. Complete!");
  lcd.setCursor(0, 1); lcd.print("System Ready    ");
  delay(1500);
  lcd.clear();
}

// ── Sensor Readers ─────────────────────────────────────────
float readVibration() {
  int16_t ax, ay, az, gx, gy, gz;
  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  // Convert raw to g-units
  float axG = ax / 16384.0;
  float ayG = ay / 16384.0;
  float azG = az / 16384.0;

  // Remove gravity (subtract 1g from dominant axis)
  azG -= 1.0;

  // Vibration magnitude
  return sqrt(axG * axG + ayG * ayG + azG * azG);
}

float readTemperature() {
  tempSensor.requestTemperatures();
  float t = tempSensor.getTempCByIndex(0);
  if (t == DEVICE_DISCONNECTED_C) {
    Serial.println("[WARN] DS18B20 disconnected — using 25.0C");
    return 25.0;
  }
  return t;
}

// ── State Classification ────────────────────────────────────
String classifyState(float vib, float temp) {
  if (vib >= vibDangerThreshold || temp >= TEMP_DANGER) return "DANGER";
  if (vib >= vibWarningThreshold || temp >= TEMP_WARNING) return "WARNING";
  return "NORMAL";
}

// ── Health Calculation ──────────────────────────────────────
int calculateHealth(float vib, float temp) {
  // Vibration penalty (0–60 points)
  float vibRange   = vibDangerThreshold - baselineVibration;
  float vibExcess  = max(0.0f, vib - baselineVibration);
  float vibPenalty = min(60.0f, (vibExcess / vibRange) * 60.0f);

  // Temperature penalty (0–40 points)
  float tempPenalty = 0.0;
  if (temp > TEMP_DANGER)        tempPenalty = 40.0;
  else if (temp > TEMP_WARNING)  tempPenalty = ((temp - TEMP_WARNING) / (TEMP_DANGER - TEMP_WARNING)) * 40.0;

  int health = (int)(100.0 - vibPenalty - tempPenalty);
  return constrain(health, 0, 100);
}

// ── LCD Update ─────────────────────────────────────────────
void updateLCD(float vib, float temp, String state, int health) {
  lcd.clear();

  if (state == "DANGER") {
    // DANGER mode: show temperature + health
    lcd.setCursor(0, 0);
    lcd.print("TEMP:");
    lcd.print(temp, 1);
    lcd.print((char)223); // degree symbol
    lcd.print("C  ");

    lcd.setCursor(0, 1);
    lcd.print("HEALTH:");
    lcd.print(health);
    lcd.print("% ");
    lcd.print(healthLabel(health));

  } else {
    // NORMAL / WARNING: show vibration + state
    lcd.setCursor(0, 0);
    lcd.print("VIB:");
    lcd.print(vib, 2);
    lcd.print("        ");

    lcd.setCursor(0, 1);
    lcd.print("STATE:");
    lcd.print(state);
    lcd.print("    ");
  }
}

// ── Health Label ───────────────────────────────────────────
String healthLabel(int h) {
  if (h >= 90) return "EXC";
  if (h >= 70) return "GD";
  if (h >= 40) return "MNT";
  return "CRIT";
}

// ── Data Logging ───────────────────────────────────────────
void logData(float vib, float temp, String state, int health) {
  dataLog[logIndex % LOG_SIZE] = {
    millis() / 1000,
    vib,
    temp,
    state,
    health
  };
  logIndex++;
}

// ── Print Log (call via Serial command) ────────────────────
void printLog() {
  Serial.println("\n===== DATA LOG (last 10 readings) =====");
  Serial.println("Time(s)  Vib    Temp   State    Health");
  Serial.println("---------------------------------------");
  int count = min(logIndex, LOG_SIZE);
  for (int i = 0; i < count; i++) {
    LogEntry& e = dataLog[i];
    Serial.print(e.timestamp);   Serial.print("s\t");
    Serial.print(e.vibration, 2); Serial.print("\t");
    Serial.print(e.temperature, 1); Serial.print("C\t");
    Serial.print(e.state);       Serial.print("\t");
    Serial.print(e.health);      Serial.println("%");
  }
  Serial.println("=======================================\n");
}
