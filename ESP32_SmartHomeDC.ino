#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <Wire.h>
#include <DHT.h>

// ===========================================
// KONFIGURASI WIFI & DATABASE SERVER
// ===========================================
const char* ssid = "UNIB";
const char* password = "";

// Ganti dengan IP komputer server
const String serverIP = "khaki-turtle-323390.hostingersite.com";
const String projectFolder = "SmartHomeDC";

// URL endpoints
const String getDataURL = "https://" + serverIP + "/getdata.php";
const String updateDataURL = "https://" + serverIP + "/updatedata.php";

//========================================
// KONFIGURASI PIN ESP32
//========================================
#define DHT1_PIN 18     // DHT1 Kamar (GPIO18)
#define DHT2_PIN 19     // DHT2 Ruang Tamu (GPIO19)
#define LDR_PIN 32      // Sensor Cahaya Teras (GPIO32)
#define SDA_PIN 21      // I2C SDA (GPIO21)
#define SCL_PIN 22      // I2C SCL (GPIO22)
#define ON_Board_LED 2  // LED onboard ESP32

// DHT sensor typ
#define DHT_TYPE DHT11

//========================================
// KONFIGURASI I2C ADDRESSES
//========================================
#define TCA9548A_ADDRESS 0x70
#define PCF8574_RELAY_ADDR 0x24
#define PCF8574_SENSOR_ADDR 0x20
#define INA219_BATTERY_ADDR 0x41

// INA226 & INA219 Registers
#define INA226_CONFIG_REG 0x00
#define INA226_SHUNT_VOLTAGE 0x01
#define INA226_BUS_VOLTAGE 0x02
#define INA226_POWER_REG 0x03
#define INA226_CURRENT_REG 0x04
#define INA226_CALIBRATION_REG 0x05

#define INA219_CONFIG_REG 0x00
#define INA219_SHUNT_VOLTAGE 0x01
#define INA219_BUS_VOLTAGE 0x02
#define INA219_POWER_REG 0x03
#define INA219_CURRENT_REG 0x04
#define INA219_CALIBRATION_REG 0x05

//========================================
// MAPPING PERANGKAT
//========================================
// Relay mapping (PCF8574 0x24)
#define RELAY_L1 0  // Lampu 1 (Teras) - 10W
#define RELAY_L2 1  // Lampu 2 (Ruang Tamu) - 30W
#define RELAY_K2 2  // Kipas 2 (Ruang Tamu) - 12W
#define RELAY_L3 3  // Lampu 3 (Kamar) - 20W
#define RELAY_K1 4  // Kipas 1 (Kamar) - 12W
#define RELAY_L4 5  // Lampu 4 (WC) - 10W
#define RELAY_L5 6  // Lampu 5 (Dapur) - 30W

// PIR mapping (PCF8574 0x20)
#define PIR_TERAS 0   // PIR1 (Teras)
#define PIR_R_TAMU 1  // PIR2 (Ruang Tamu)
#define PIR_KAMAR 2   // PIR3 (Kamar)
#define PIR_WC 3      // PIR4 (WC)
#define PIR_DAPUR 4   // PIR5 (Dapur)

// INA Sensor channel mapping (TCA9548A)
#define INA1_CHANNEL 1  // Lampu 1 (INA226)
#define INA2_CHANNEL 2  // Lampu 2 (INA226)
#define INA3_CHANNEL 3  // Kipas 2 (INA226)
#define INA4_CHANNEL 4  // Lampu 3 (INA226)
#define INA5_CHANNEL 5  // Kipas 1 (INA226)
#define INA6_CHANNEL 6  // Lampu 4 (INA226)
#define INA7_CHANNEL 7  // Lampu 5 (INA226)
#define INA8_CHANNEL 0  // USB (INA219)

//========================================
// GLOBAL VARIABLES
//========================================
DHT dht1(DHT1_PIN, DHT_TYPE);  // DHT1 Kamar
DHT dht2(DHT2_PIN, DHT_TYPE);  // DHT2 Ruang Tamu

// PCF8574 state variables
byte pcfRelayState = 0xFF;   // State untuk PCF relay
byte pcfSensorState = 0xFF;  // State untuk PCF sensor

//========================================
// HTTP COMMUNICATION VARIABLES
//========================================
String postData = "";
String payload = "";

//========================================
// INA219 BATTERY SENSOR VARIABLES
//========================================
struct INA219BatteryData {
  float voltage = 12.86;
  bool isConnected = false;
  unsigned long lastRead = 0;
};
INA219BatteryData ina219Battery;

//========================================
// VOLTAGE COMPENSATION
//========================================
struct BatteryCompensation {
  float internalResistance = 0.06;  // 60 milli-ohm
  float maxCompensation = 0.4;      // Max kompensasi 400mV
  float minVoltage = 10.8;          // Voltage minimum valid
  float maxVoltage = 12.86;         // Voltage maximum valid
};

BatteryCompensation batteryComp;

//========================================
// STRUKTUR DATA
//========================================
struct INASensor {
  int channel;
  byte address;
  char name[20];
  String type;
  float voltage;
  float current;
  float power;
  float shuntVoltage;
  bool isConnected;
  unsigned long lastRead;
};

struct DeviceState {
  bool L1 = false;
  bool L2 = false;
  bool L3 = false;
  bool L4 = false;
  bool L5 = false;
  bool K1 = false;
  bool K2 = false;
  bool USB = true;
};

struct SensorData {
  float temperatureKamar = 0;
  float temperatureRTamu = 0;
  float humidityKamar = 0;
  float humidityRTamu = 0;
  int lightLevel = 0;
  bool pirTeras = false;
  bool pirRuangTamu = false;
  bool pirKamar = false;
  bool pirWC = false;
  bool pirDapur = false;
};

struct PowerData {
  float voltage = 0;
  float current = 0;
  float power = 0;
};

struct BatteryData {
  float voltage = 12.86;
  float soc = 100.0;
  float estimatedRuntime = 0;
  bool isCharging = false;
};

struct SystemState {
  String mode = "MANUAL";  // MULAI DENGAN MANUAL
  String deviceId = "esp32_smart_home_01";
  DeviceState devices;
  SensorData sensors;
  PowerData powerL1, powerL2, powerL3, powerL4, powerL5, powerK1, powerK2, powerUSB;
  BatteryData battery;
  String statusReadSensors = "SUCCEED";
  unsigned long lastSensorRead = 0;
  unsigned long deviceTimers[8] = { 0 };
  float totalPower = 0;
  float totalCurrent = 0;
};

struct BatteryMonitoring {
  float voltageHistory[5] = { 12.6, 12.6, 12.6, 12.6, 12.6 };
  int historyIndex = 0;
  bool isCharging = false;
  bool emergencyModeActive = false;
  float recoverySOC = 90.0;
  float emergencySOC = 35.0;  // Emergency pada 35%
  unsigned long lastChargingCheck = 0;
  unsigned long emergencyStartTime = 0;
  float preEmergencyVoltage = 12.6;
  bool lastChargingState = false;
  int stableCount = 0;
};

// Global instance
BatteryMonitoring batteryMonitor;
SystemState systemState;

// INA Sensors array
INASensor inaSensors[8] = {
  { INA1_CHANNEL, 0x40, "INA1(L1)", "INA226", 0, 0, 0, 0, false, 0 },
  { INA2_CHANNEL, 0x40, "INA2(L2)", "INA226", 0, 0, 0, 0, false, 0 },
  { INA3_CHANNEL, 0x40, "INA3(K2)", "INA226", 0, 0, 0, 0, false, 0 },
  { INA4_CHANNEL, 0x40, "INA4(L3)", "INA226", 0, 0, 0, 0, false, 0 },
  { INA5_CHANNEL, 0x44, "INA5(K1)", "INA226", 0, 0, 0, 0, false, 0 },
  { INA6_CHANNEL, 0x40, "INA6(L4)", "INA226", 0, 0, 0, 0, false, 0 },
  { INA7_CHANNEL, 0x40, "INA7(L5)", "INA226", 0, 0, 0, 0, false, 0 },
  { INA8_CHANNEL, 0x40, "INA8(USB)", "INA219", 0, 0, 0, 0, false, 0 }
};

//========================================
// KONSTANTA SISTEM
//========================================
const int NUM_INA_SENSORS = 8;
const int SENSOR_READ_INTERVAL = 1000;
const int AUTO_OFF_DURATION = 10000;        // 10 detik
const float TEMP_THRESHOLD = 32.0;          // 32°C
const int LIGHT_THRESHOLD = 150;            // 150 lux
const float LOW_BATTERY_THRESHOLD = 35.0;   // 35%
const int DATABASE_UPDATE_INTERVAL = 5000;  // 5 detik
unsigned long lastDatabaseUpdate = 0;

// Tabel konversi tegangan ke SOC
const float VOLTAGE_SOC_TABLE[][2] = {
  { 12.860, 100.0 }, { 12.654, 90.0 }, { 12.448, 80.0 }, { 12.242, 70.0 }, { 12.036, 60.0 }, { 11.830, 50.0 }, { 11.624, 40.0 }, { 11.512, 35.0 }, { 11.418, 30.0 }, { 11.212, 20.0 }, { 11.006, 10.0 }, { 10.800, 0.0 }
};

//========================================
// INA219 BATTERY SENSOR FUNCTIONS
//========================================
void initializeINA219Battery() {
  Serial.println("=== INITIALIZING INA219 BATTERY SENSOR ===");
  Serial.printf("Address: 0x%02X (default, langsung ke bus I2C)\n", INA219_BATTERY_ADDR);
  Serial.printf("Purpose: Battery voltage monitoring only\n");

  // Test koneksi ke INA219 Battery
  Wire.beginTransmission(INA219_BATTERY_ADDR);
  byte error = Wire.endTransmission();

  if (error == 0) {
    ina219Battery.isConnected = true;
    Serial.println("✅ INA219 Battery sensor DETECTED!");

    // Konfigurasi INA219 untuk voltage monitoring saja
    configureINA219Battery();

    // Initial reading
    readINA219Battery();

    Serial.printf("✅ Initial voltage reading: %.2fV\n", ina219Battery.voltage);
  } else {
    ina219Battery.isConnected = false;
    Serial.println("❌ INA219 Battery sensor NOT FOUND!");
    Serial.printf("I2C Error Code: %d\n", error);
    Serial.println("⚠️ This is normal - battery reading via direct I2C");
  }

  Serial.println("==========================================");
}

void configureINA219Battery() {
  Serial.println("Configuring INA219 Battery sensor (VOLTAGE ONLY)...");

  // Konfigurasi pembacaan voltage baterai saja
  uint16_t config = 0;
  config |= (1 << 13);  // BRNG: 32V (untuk baterai 12V)
  config |= (3 << 11);  // PGA: ±320mV (maksimum range)
  config |= (3 << 7);   // BADC: 12-bit 532μs (akurasi tinggi)
  config |= (3 << 3);   // SADC: 12-bit 532μs (akurasi tinggi)
  config |= 0x7;        // Mode: shunt + bus, continuous


  writeINARegister(INA219_BATTERY_ADDR, INA219_CONFIG_REG, config);
  Serial.printf("INA219 Battery Config (VOLTAGE ONLY):\n");
  Serial.printf("- Voltage Range: 32V (for 12V battery)\n");
  Serial.printf("- Resolution: 4mV per bit\n");
  Serial.printf("- Mode: Continuous voltage monitoring\n");

  delay(50);  // Stabilisasi
}

void readINA219Battery() {
  if (!ina219Battery.isConnected) return;

  // Baca hanya bus voltage (tegangan baterai)
  uint16_t busVoltageRaw = readINARegister(INA219_BATTERY_ADDR, INA219_BUS_VOLTAGE);

  // Konversi ke nilai fisik (hanya voltage)
  ina219Battery.voltage = (busVoltageRaw >> 3) * 0.004;  // LSB = 4mV, shift untuk menghilangkan bit status

  // Update timestamp
  ina219Battery.lastRead = millis();
}

float getBatteryVoltageFromINA219() {
  readINA219Battery();
  return ina219Battery.voltage;
}

void debugINA219Battery() {
  if (!ina219Battery.isConnected) {
    Serial.println("❌ INA219 Battery not connected - cannot debug");
    return;
  }

  Serial.println("=== INA219 BATTERY SENSOR DEBUG (VOLTAGE ONLY) ===");

  // Baca raw values untuk debugging
  uint16_t busRaw = readINARegister(INA219_BATTERY_ADDR, INA219_BUS_VOLTAGE);
  uint16_t configReg = readINARegister(INA219_BATTERY_ADDR, INA219_CONFIG_REG);

  Serial.printf("Raw Values:\n");
  Serial.printf("- Bus Voltage Raw: 0x%04X (%d)\n", busRaw, busRaw);
  Serial.printf("- Config Register: 0x%04X\n", configReg);

  Serial.printf("Calculated Values:\n");
  Serial.printf("- Battery Voltage: %.3fV\n", ina219Battery.voltage);

  Serial.printf("Status:\n");
  Serial.printf("- Connected: %s\n", ina219Battery.isConnected ? "YES" : "NO");
  Serial.printf("- Last Read: %lu ms ago\n", millis() - ina219Battery.lastRead);
  Serial.printf("- I2C Address: 0x%02X\n", INA219_BATTERY_ADDR);

  Serial.println("==================================================");
}

//========================================
// BATTERY VOLTAGE COMPENSATION FUNCTIONS
//========================================
float getCompensatedBatteryVoltage() {
  // Raw voltage dari INA219
  float rawVoltage = getBatteryVoltageFromINA219();

  // Total current dari semua beban
  float totalCurrent = systemState.totalCurrent;

  // Hitung kompensasi: V_real = V_measured + (I × R_internal)
  float compensation = totalCurrent * batteryComp.internalResistance;

  // Batasi kompensasi maksimal (safety)
  compensation = constrain(compensation, 0, batteryComp.maxCompensation);

  // Apply kompensasi
  float compensatedVoltage = rawVoltage + compensation;

  // Validasi hasil
  compensatedVoltage = constrain(compensatedVoltage,
                                 batteryComp.minVoltage,
                                 batteryComp.maxVoltage);

  return compensatedVoltage;
}

void debugCompensation() {
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug < 3000) return;  // Debug setiap 3 detik

  float rawVoltage = getBatteryVoltageFromINA219();
  float compensatedVoltage = getCompensatedBatteryVoltage();
  float totalCurrent = systemState.totalCurrent;
  float compensation = compensatedVoltage - rawVoltage;

  Serial.println("\n=== VOLTAGE COMPENSATION DEBUG ===");
  Serial.printf("Raw Voltage: %.3fV\n", rawVoltage);
  Serial.printf("Total Current: %.3fA\n", totalCurrent);
  Serial.printf("Compensation: +%.3fV\n", compensation);
  Serial.printf("Final Voltage: %.3fV\n", compensatedVoltage);
  Serial.printf("SOC: %.1f%%\n", systemState.battery.soc);
  Serial.println("==================================\n");

  lastDebug = millis();
}

void setBatteryCompensationParameters() {
  Serial.println("=== COMPENSATION PARAMETERS ===");
  Serial.printf("Internal R: %.3f ohm\n", batteryComp.internalResistance);
  Serial.printf("Max Compensation: %.1fmV\n", batteryComp.maxCompensation * 1000);
  Serial.println("===============================");
}

void calibrateBatteryCompensation() {
  Serial.println("\n=== BATTERY COMPENSATION CALIBRATION ===");
  Serial.println("Ikuti langkah ini untuk kalibrasi:");
  Serial.println("1. Matikan semua beban (manual mode)");
  Serial.println("2. Catat voltage tanpa beban");
  Serial.println("3. Nyalakan beban tertentu (misal L1+L2)");
  Serial.println("4. Catat voltage dengan beban");
  Serial.println("5. Hitung: R = (V_no_load - V_load) / I_load");
  Serial.println("");
  Serial.println("Contoh:");
  Serial.println("- No load: 12.8V");
  Serial.println("- With 2A load: 12.6V");
  Serial.println("- R = (12.8 - 12.6) / 2 = 0.1 ohm");
  Serial.println("");
  Serial.printf("Current setting: %.3f ohm\n", batteryComp.internalResistance);
  Serial.println("Adjust batteryComp.internalResistance jika perlu");
  Serial.println("========================================\n");
}

//=======================
// PCF8574 FUNCTIONS
//=======================
bool testPCF(int addr) {
  Wire.beginTransmission(addr);
  return (Wire.endTransmission() == 0);
}

byte readPCF8574(int addr) {
  Wire.requestFrom(addr, 1);
  if (Wire.available()) {
    return Wire.read();
  }
  return 0xFF;
}

void writePCF8574(int addr, byte data) {
  Wire.beginTransmission(addr);
  Wire.write(data);
  Wire.endTransmission();
}

void initializePCF8574(int addr) {
  Serial.print("Initializing PCF8574 addr 0x");
  Serial.print(addr, HEX);
  Serial.print(": ");

  if (testPCF(addr)) {
    Serial.println("OK");
    byte initialState = 0xFF;
    writePCF8574(addr, initialState);
    delay(100);

    if (addr == PCF8574_RELAY_ADDR) {
      pcfRelayState = initialState;
    } else if (addr == PCF8574_SENSOR_ADDR) {
      pcfSensorState = initialState;
    }
  } else {
    Serial.println("ERROR");
  }
}

bool readPin(int addr, int pin) {
  byte data = readPCF8574(addr);
  return (data & (1 << pin)) != 0;
}

void setOutputPin(int addr, int pin, bool state) {
  byte* currentState;

  if (addr == PCF8574_RELAY_ADDR) {
    currentState = &pcfRelayState;
  } else if (addr == PCF8574_SENSOR_ADDR) {
    currentState = &pcfSensorState;
  } else {
    return;
  }

  if (state) {
    *currentState |= (1 << pin);
  } else {
    *currentState &= ~(1 << pin);
  }

  writePCF8574(addr, *currentState);
}

int readPIR(int addr, int pin) {
  return readPin(addr, pin) ? 1 : 0;
}

int readLDR(int pin) {
  int ldrValue = analogRead(pin);
  float voltage = (3.3 / 4095.0) * ldrValue;
  int lux = max(0, (int)(250.0 / voltage) - 50);
  return lux;
}

//==========================
// TCA9548A & INA FUNCTIONS
//==========================
bool testTCA9548A() {
  Wire.beginTransmission(TCA9548A_ADDRESS);
  return (Wire.endTransmission() == 0);
}

void selectTCA9548AChannel(uint8_t channel) {
  if (channel > 7) return;
  Wire.beginTransmission(TCA9548A_ADDRESS);
  Wire.write(1 << channel);
  Wire.endTransmission();
  delay(5);
}

void disableTCA9548AChannels() {
  Wire.beginTransmission(TCA9548A_ADDRESS);
  Wire.write(0x00);
  Wire.endTransmission();
}

uint16_t readINARegister(byte address, byte reg) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.endTransmission();

  Wire.requestFrom(address, 2);
  if (Wire.available() == 2) {
    uint16_t value = Wire.read() << 8;
    value |= Wire.read();
    return value;
  }
  return 0;
}

void writeINARegister(byte address, byte reg, uint16_t value) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write((value >> 8) & 0xFF);
  Wire.write(value & 0xFF);
  Wire.endTransmission();
}

void configureINA226(INASensor* sensor) {
  selectTCA9548AChannel(sensor->channel);
  uint16_t config = 0x4527;
  writeINARegister(sensor->address, INA226_CONFIG_REG, config);
  uint16_t calibration = 512;
  writeINARegister(sensor->address, INA226_CALIBRATION_REG, calibration);
  delay(10);
}

void configureINA219(INASensor* sensor) {
  selectTCA9548AChannel(sensor->channel);

  // Konfigurasi khusus untuk USB (3A max)
  float maxExpectedCurrent = 3.0;  // 3A maksimum
  float shuntResistor = 0.1;       // Nilai shunt resistor (ohm)
  float maxShuntVoltage = maxExpectedCurrent * shuntResistor;

  // Hitung Current_LSB optimal (untuk presisi maksimal)
  float currentLSB = maxExpectedCurrent / 32768.0;

  // Hitung nilai kalibrasi
  uint16_t calibration = (uint16_t)(0.04096 / (currentLSB * shuntResistor));

  // Konfigurasi register
  uint16_t config = 0;
  config |= (0 << 13);  // BRNG: 16V
  config |= (3 << 11);  // PGA: ±320mV
  config |= (3 << 7);   // BADC: 12-bit 532us
  config |= (3 << 3);   // SADC: 12-bit 532us
  config |= 0x7;        // Mode: shunt + bus, continuous

  writeINARegister(sensor->address, INA219_CONFIG_REG, config);
  writeINARegister(sensor->address, INA219_CALIBRATION_REG, calibration);

  Serial.printf("INA219 Config - Cal: %d, Shunt: %.3f ohm, Max: %.2fA, LSB: %.6fA/bit\n",
                calibration, shuntResistor, maxExpectedCurrent, currentLSB);
  delay(20);
}

void readINA226Sensor(INASensor* sensor) {
  if (!sensor->isConnected) return;

  selectTCA9548AChannel(sensor->channel);

  uint16_t busVoltageRaw = readINARegister(sensor->address, INA226_BUS_VOLTAGE);
  sensor->voltage = (busVoltageRaw * 1.25) / 1000.0;

  int16_t shuntVoltageRaw = (int16_t)readINARegister(sensor->address, INA226_SHUNT_VOLTAGE);
  sensor->shuntVoltage = shuntVoltageRaw * 0.0025;

  int16_t currentRaw = (int16_t)readINARegister(sensor->address, INA226_CURRENT_REG);
  sensor->current = currentRaw * 0.0001;

  uint16_t powerRaw = readINARegister(sensor->address, INA226_POWER_REG);
  sensor->power = powerRaw * 0.0025;
}

void readINA219Sensor(INASensor* sensor) {
  if (!sensor->isConnected) return;

  selectTCA9548AChannel(sensor->channel);

  // Baca nilai raw
  int16_t shuntVoltageRaw = (int16_t)readINARegister(sensor->address, INA219_SHUNT_VOLTAGE);
  uint16_t busVoltageRaw = readINARegister(sensor->address, INA219_BUS_VOLTAGE);

  // Konversi ke nilai fisik
  sensor->shuntVoltage = shuntVoltageRaw * 0.01;   // LSB = 10μV
  sensor->voltage = (busVoltageRaw >> 3) * 0.004;  // LSB = 4mV

  // Baca nilai kalibrasi
  uint16_t calibration = readINARegister(sensor->address, INA219_CALIBRATION_REG);

  // Hitung arus berdasarkan kalibrasi
  float currentLSB = 0.04096 / (calibration * 0.1);  // 0.1 = shunt resistor
  int16_t currentRaw = (int16_t)readINARegister(sensor->address, INA219_CURRENT_REG);
  sensor->current = currentRaw * currentLSB;

  // Koreksi offset (untuk menghilangkan 0.45A saat tanpa beban)
  float zeroCurrentOffset = 0.45;  // Nilai yang diamati saat tanpa beban
  if (fabs(sensor->current) < zeroCurrentOffset * 2) {
    sensor->current = 0;  // Nullify small currents
  } else {
    sensor->current -= copysign(zeroCurrentOffset, sensor->current);
  }

  // Hitung daya
  sensor->power = sensor->voltage * sensor->current;

  sensor->lastRead = millis();
  disableTCA9548AChannels();
}

void initializeINASensors() {
  Serial.println("Initializing INA sensors...");

  for (int i = 0; i < NUM_INA_SENSORS; i++) {
    selectTCA9548AChannel(inaSensors[i].channel);

    Wire.beginTransmission(inaSensors[i].address);
    byte error = Wire.endTransmission();

    if (error == 0) {
      inaSensors[i].isConnected = true;
      Serial.print(inaSensors[i].name);
      Serial.print(" (Channel ");
      Serial.print(inaSensors[i].channel);
      Serial.print(", Address 0x");
      Serial.print(inaSensors[i].address, HEX);
      Serial.print(", Type ");
      Serial.print(inaSensors[i].type);
      Serial.println(") - Connected");

      if (inaSensors[i].type == "INA226") {
        configureINA226(&inaSensors[i]);
      } else if (inaSensors[i].type == "INA219") {
        configureINA219(&inaSensors[i]);
      }
    } else {
      inaSensors[i].isConnected = false;
      Serial.print(inaSensors[i].name);
      Serial.println(" - NOT FOUND");
    }
  }

  disableTCA9548AChannels();
  Serial.println("INA sensors initialization complete");
}

//================================================
// DEVICE CONTROL FUNCTIONS - DENGAN MODE CONTROL
//================================================
void setDevice(String deviceId, bool state) {
  // BATTERY SAFETY CHECK
  if (state && batteryMonitor.emergencyModeActive && !batteryMonitor.isCharging) {
    Serial.println("🚨 BLOCKED: Cannot turn ON " + deviceId + " (Emergency mode + not charging)");
    return;
  }

  // Log device control
  Serial.println("=== DEVICE CONTROL ===");
  Serial.println("Device: " + deviceId + " → " + (state ? "ON" : "OFF"));
  Serial.println("Emergency: " + String(batteryMonitor.emergencyModeActive ? "ACTIVE" : "OFF"));
  Serial.println("Charging: " + String(batteryMonitor.isCharging ? "YES" : "NO"));
  Serial.println("Mode: " + systemState.mode);

  // Execute device control
  if (deviceId == "L1") {
    systemState.devices.L1 = state;
    setOutputPin(PCF8574_RELAY_ADDR, RELAY_L1, state ? LOW : HIGH);
  } else if (deviceId == "L2") {
    systemState.devices.L2 = state;
    setOutputPin(PCF8574_RELAY_ADDR, RELAY_L2, state ? LOW : HIGH);
  } else if (deviceId == "L3") {
    systemState.devices.L3 = state;
    setOutputPin(PCF8574_RELAY_ADDR, RELAY_L3, state ? LOW : HIGH);
  } else if (deviceId == "L4") {
    systemState.devices.L4 = state;
    setOutputPin(PCF8574_RELAY_ADDR, RELAY_L4, state ? LOW : HIGH);
  } else if (deviceId == "L5") {
    systemState.devices.L5 = state;
    setOutputPin(PCF8574_RELAY_ADDR, RELAY_L5, state ? LOW : HIGH);
  } else if (deviceId == "K1") {
    systemState.devices.K1 = state;
    setOutputPin(PCF8574_RELAY_ADDR, RELAY_K1, state ? LOW : HIGH);
  } else if (deviceId == "K2") {
    systemState.devices.K2 = state;
    setOutputPin(PCF8574_RELAY_ADDR, RELAY_K2, state ? LOW : HIGH);
  }

  Serial.println("✅ Device " + deviceId + " set to " + (state ? "ON" : "OFF"));
  Serial.println("===================");
}

void matikanSemuaBeban() {
  setDevice("L1", false);
  setDevice("L2", false);
  setDevice("L3", false);
  setDevice("L4", false);
  setDevice("L5", false);
  setDevice("K1", false);
  setDevice("K2", false);
  if (systemState.battery.soc <= 35.0) {
    setDevice("USB", false);
  }
  Serial.println("All loads turned OFF - Battery protection");
}

void forceManualMode() {
  Serial.println("=== FORCING MANUAL MODE ===");
  systemState.mode = "MANUAL";

  // Stop semua timer auto
  for (int i = 0; i < 8; i++) {
    systemState.deviceTimers[i] = 0;
  }

  Serial.println("All auto timers reset");
  Serial.println("Mode forced to MANUAL");
  Serial.println("Devices will only follow database commands");
  Serial.println("============================");
}

//============================
// SENSOR READING FUNCTIONS
//============================
void readSensors() {
  Serial.println();
  Serial.println("-------------get_sensor_data()");

  // Baca DHT sensors
  float tempKamar = dht1.readTemperature();
  float tempRTamu = dht2.readTemperature();
  float humKamar = dht1.readHumidity();
  float humRTamu = dht2.readHumidity();

  // Check if any reads failed
  bool dhtFailed = false;
  if (!isnan(tempKamar)) {
    systemState.sensors.temperatureKamar = tempKamar;  // Kalibrasi
  } else {
    dhtFailed = true;
  }

  if (!isnan(tempRTamu)) {
    systemState.sensors.temperatureRTamu = tempRTamu;  // Kalibrasi
  } else {
    dhtFailed = true;
  }

  if (!isnan(humKamar)) {
    systemState.sensors.humidityKamar = humKamar;
  } else {
    dhtFailed = true;
  }

  if (!isnan(humRTamu)) {
    systemState.sensors.humidityRTamu = humRTamu;
  } else {
    dhtFailed = true;
  }

  if (dhtFailed) {
    systemState.statusReadSensors = "FAILED";
    Serial.println("Failed to read from DHT sensor!");
  } else {
    systemState.statusReadSensors = "SUCCEED";
  }

  // Baca LDR
  systemState.sensors.lightLevel = readLDR(LDR_PIN);

  // Baca PIR sensors
  readPIRSensors();

  //Baca tegangan baterai dari INA219
  systemState.battery.voltage = getCompensatedBatteryVoltage();

  // Baca data power dari semua INA sensors
  readPowerData();

  // Kalkulasi SOC dan estimated runtime
  calculateBatterySOC();

  Serial.printf("Temperature Kamar : %.2f °C\n", systemState.sensors.temperatureKamar);
  Serial.printf("Temperature R.Tamu : %.2f °C\n", systemState.sensors.temperatureRTamu);
  Serial.printf("Humidity Kamar : %.1f %%\n", systemState.sensors.humidityKamar);
  Serial.printf("Humidity R.Tamu : %.1f %%\n", systemState.sensors.humidityRTamu);
  Serial.printf("Light Level : %d lux\n", systemState.sensors.lightLevel);
  Serial.printf("Battery Voltage : %.2f V \n", systemState.battery.voltage);
  Serial.printf("Battery SOC : %.1f %%\n", systemState.battery.soc);
  Serial.printf("Total Power : %.2f W\n", systemState.totalPower);
  Serial.printf("Total Current : %.3f A\n", systemState.totalCurrent);
  Serial.printf("Current Mode : %s\n", systemState.mode.c_str());
  Serial.printf("Status Read Sensors : %s\n", systemState.statusReadSensors.c_str());
  Serial.println("-------------");
}

void readPIRSensors() {
  Serial.println("=== READING PIR SENSORS (DEBUG) ===");

  bool oldPirTeras = systemState.sensors.pirTeras;
  bool oldPirRuangTamu = systemState.sensors.pirRuangTamu;
  bool oldPirKamar = systemState.sensors.pirKamar;
  bool oldPirWC = systemState.sensors.pirWC;
  bool oldPirDapur = systemState.sensors.pirDapur;

  systemState.sensors.pirTeras = readPIR(PCF8574_SENSOR_ADDR, PIR_TERAS);
  systemState.sensors.pirRuangTamu = readPIR(PCF8574_SENSOR_ADDR, PIR_R_TAMU);
  systemState.sensors.pirKamar = readPIR(PCF8574_SENSOR_ADDR, PIR_KAMAR);
  systemState.sensors.pirWC = readPIR(PCF8574_SENSOR_ADDR, PIR_WC);
  systemState.sensors.pirDapur = readPIR(PCF8574_SENSOR_ADDR, PIR_DAPUR);

  Serial.printf("PIR Teras     : %d", systemState.sensors.pirTeras);
  if (oldPirTeras != systemState.sensors.pirTeras) Serial.print(" ← CHANGED!");
  Serial.println();

  Serial.printf("PIR Ruang Tamu: %d", systemState.sensors.pirRuangTamu);
  if (oldPirRuangTamu != systemState.sensors.pirRuangTamu) Serial.print(" ← CHANGED!");
  Serial.println();

  Serial.printf("PIR Kamar     : %d", systemState.sensors.pirKamar);
  if (oldPirKamar != systemState.sensors.pirKamar) Serial.print(" ← CHANGED!");
  Serial.println();

  Serial.printf("PIR WC        : %d", systemState.sensors.pirWC);
  if (oldPirWC != systemState.sensors.pirWC) Serial.print(" ← CHANGED!");
  Serial.println();

  Serial.printf("PIR Dapur     : %d", systemState.sensors.pirDapur);
  if (oldPirDapur != systemState.sensors.pirDapur) Serial.print(" ← CHANGED!");
  Serial.println();

  Serial.println("===================================");
}

void readPowerData() {
  unsigned long currentTime = millis();
  systemState.totalPower = 0;    // Reset total
  systemState.totalCurrent = 0;  // Reset total

  for (int i = 0; i < NUM_INA_SENSORS; i++) {
    if (currentTime - inaSensors[i].lastRead >= 500) {
      if (inaSensors[i].type == "INA226") {
        readINA226Sensor(&inaSensors[i]);
      } else if (inaSensors[i].type == "INA219") {
        readINA219Sensor(&inaSensors[i]);
      }
      inaSensors[i].lastRead = currentTime;
    }
  }

  // Map data INA ke struktur PowerData
  systemState.powerL1.voltage = inaSensors[0].voltage;
  systemState.powerL1.current = inaSensors[0].current;
  systemState.powerL1.power = inaSensors[0].power;

  systemState.powerL2.voltage = inaSensors[1].voltage;
  systemState.powerL2.current = inaSensors[1].current;
  systemState.powerL2.power = inaSensors[1].power;

  systemState.powerK2.voltage = inaSensors[2].voltage;
  systemState.powerK2.current = inaSensors[2].current;
  systemState.powerK2.power = inaSensors[2].power;

  systemState.powerL3.voltage = inaSensors[3].voltage;
  systemState.powerL3.current = inaSensors[3].current;
  systemState.powerL3.power = inaSensors[3].power;

  systemState.powerK1.voltage = inaSensors[4].voltage;
  systemState.powerK1.current = inaSensors[4].current;
  systemState.powerK1.power = inaSensors[4].power;

  systemState.powerL4.voltage = inaSensors[5].voltage;
  systemState.powerL4.current = inaSensors[5].current;
  systemState.powerL4.power = inaSensors[5].power;

  systemState.powerL5.voltage = inaSensors[6].voltage;
  systemState.powerL5.current = inaSensors[6].current;
  systemState.powerL5.power = inaSensors[6].power;

  systemState.powerUSB.voltage = inaSensors[7].voltage;
  systemState.powerUSB.current = inaSensors[7].current;
  systemState.powerUSB.power = inaSensors[7].power;

  // Kalkulasi total power dan current
  systemState.totalPower = systemState.powerL1.power + systemState.powerL2.power + systemState.powerL3.power + systemState.powerL4.power + systemState.powerL5.power + systemState.powerK1.power + systemState.powerK2.power + systemState.powerUSB.power;

  systemState.totalCurrent = systemState.powerL1.current + systemState.powerL2.current + systemState.powerL3.current + systemState.powerL4.current + systemState.powerL5.current + systemState.powerK1.current + systemState.powerK2.current + systemState.powerUSB.current;
}

float calculateTotalPower() {
  return systemState.totalPower;
}

void calculateBatterySOC() {
  float voltage = systemState.battery.voltage;
  systemState.battery.soc = 0;

  int tableSize = sizeof(VOLTAGE_SOC_TABLE) / sizeof(VOLTAGE_SOC_TABLE[0]);

  // Handle voltage above maximum
  if (voltage >= VOLTAGE_SOC_TABLE[0][0]) {
    systemState.battery.soc = 100.0;
  }
  // Handle voltage below minimum
  else if (voltage <= VOLTAGE_SOC_TABLE[tableSize - 1][0]) {
    systemState.battery.soc = 0.0;
  }
  // Interpolate for voltages in between
  else {
    for (int i = 0; i < tableSize - 1; i++) {
      if (voltage >= VOLTAGE_SOC_TABLE[i + 1][0] && voltage < VOLTAGE_SOC_TABLE[i][0]) {
        float v_upper = VOLTAGE_SOC_TABLE[i][0];
        float v_lower = VOLTAGE_SOC_TABLE[i + 1][0];
        float soc_upper = VOLTAGE_SOC_TABLE[i][1];
        float soc_lower = VOLTAGE_SOC_TABLE[i + 1][1];

        // Linear interpolation
        systemState.battery.soc = soc_lower + ((voltage - v_lower) * (soc_upper - soc_lower)) / (v_upper - v_lower);
        break;
      }
    }
  }

  // Ensure SOC is within 0-100% range
  systemState.battery.soc = constrain(systemState.battery.soc, 0.0, 100.0);

  calculateEstimatedRuntime();
}

void calculateEstimatedRuntime() {
  if (systemState.totalPower > 0) {
    float batteryCapacity = 200.0;
    float currentSOC = systemState.battery.soc;
    float minSOC = 35.0;  // DoD 65%

    if (currentSOC <= minSOC) {
      systemState.battery.estimatedRuntime = 0.0;
      return;
    }

    float usableCapacity = batteryCapacity * ((currentSOC - minSOC) / 100.0);
    systemState.battery.estimatedRuntime = (usableCapacity * systemState.battery.voltage) / systemState.totalPower;
  } else {
    systemState.battery.estimatedRuntime = 999.9;
  }
}

//====================
// AUTO MODE LOGIC
//====================
void executeAutoMode() {
  float soc = systemState.battery.soc;

  Serial.println("=== EXECUTING AUTO MODE ===");
  Serial.printf("Battery SOC: %.1f%% (Voltage: %.2fV - INA219 SENSOR)\n", soc, systemState.battery.voltage);

  // Tabel 3.3 Skenario Rekomendasi Beban Prioritas

  if (soc >= 90.0) {
    // Semua beban aktif (L1, L2, L3, L4, L5, K1, K2)
    executeAllDevicesLogic();
    Serial.println("Auto Mode: SOC ≥90% - All devices available");

  } else if (soc >= 80.0) {
    // L1, L2, L3, L4, L5, K1 (matikan K2)
    setDevice("K2", false);
    executePriorityDevicesLogic();
    Serial.println("Auto Mode: SOC ≥80% - K2 disabled, others available");

  } else if (soc >= 70.0) {
    // L1, L2, L3, L4, L5 (matikan kipas)
    setDevice("K1", false);
    setDevice("K2", false);
    executeLampsOnlyLogic();
    Serial.println("Auto Mode: SOC ≥70% - Only lamps available");

  } else if (soc >= 60.0) {
    // L1, L2, L3, L4 (matikan L5 dan kipas)
    setDevice("L5", false);
    setDevice("K1", false);
    setDevice("K2", false);
    executeEssentialLampsLogic();
    Serial.println("Auto Mode: SOC ≥60% - Essential lamps only");

  } else if (soc >= 50.0) {
    // PERBAIKAN: L1, L2, L3 (matikan L4, L5 dan kipas)
    setDevice("L4", false);
    setDevice("L5", false);
    setDevice("K1", false);
    setDevice("K2", false);
    executeMinimalLampsLogic();  // Hanya L1, L2, L3
    Serial.println("Auto Mode: SOC ≥50% - Minimal lamps only (L1, L2, L3)");

  } else if (soc > 35.0) {
    // PERBAIKAN: L1, L2 (matikan L3, L4, L5 dan kipas)
    setDevice("L3", false);
    setDevice("L4", false);
    setDevice("L5", false);
    setDevice("K1", false);
    setDevice("K2", false);
    executeCriticalLampsLogic();  // Hanya L1, L2
    Serial.println("Auto Mode: SOC >35% - Critical lamps only (L1, L2)");

  } else {
    // Semua beban mati (≤35%)
    matikanSemuaBeban();
    Serial.println("Auto Mode: SOC ≤35% - All loads OFF (Emergency)");
    return;
  }

  Serial.printf("Auto mode executed - SOC: %.1f%%, Priority level applied\n", soc);
  Serial.println("============================");
}

void executeAllDevicesLogic() {
  // Kontrol semua perangkat berdasarkan sensor
  executeRoomControlLogicWithSOCLimit(0.0);  // No SOC limit
}

void executePriorityDevicesLogic() {
  // Kontrol perangkat prioritas (tanpa K2)
  executeRoomControlLogicWithSOCLimit(80.0);  // K2 sudah dimatikan sebelumnya
}

void executeLampsOnlyLogic() {
  // Hanya lampu yang bisa aktif berdasarkan sensor
  executeRoomControlLogicWithSOCLimit(70.0);  // Kipas sudah dimatikan sebelumnya
}

void executeEssentialLampsLogic() {
  // Lampu esensial (L1, L2, L3, L4)
  executeRoomControlLogicWithSOCLimit(60.0);  // L1, L2, L3, L4 minimal SOC 60%
}

void executeMinimalLampsLogic() {
  // Lampu minimal (L1, L2, L3)
  executeRoomControlLogicWithSOCLimit(50.0);  // L1, L2, L3 minimal SOC 50%
}

void executeCriticalLampsLogic() {
  // Lampu kritis (L1, L2)
  executeRoomControlLogicWithSOCLimit(35.0);  // L1, L2 minimal SOC 35%
}

void executeRoomControlLogic() {
  executeRoomControlLogicWithSOCLimit(100.0);  // No limit untuk full access
}

void executeRoomControlLogicWithSOCLimit(float minSOCRequired) {
  unsigned long currentTime = millis();
  float currentSOC = systemState.battery.soc;

  Serial.println("=== ROOM CONTROL LOGIC DEBUG ===");
  Serial.printf("Current Time: %lu\n", currentTime);
  Serial.printf("Current SOC: %.1f%%\n", currentSOC);
  Serial.printf("Min SOC Required: %.1f%%\n", minSOCRequired);

  // TERAS - Lampu 1 (L1): dengan ENHANCED SMART TIMER
  if (currentSOC >= minSOCRequired && currentSOC >= 35.0) {
    Serial.println("--- CHECKING TERAS (L1) ---");
    Serial.printf("Light Level: %d (threshold: %d)\n", systemState.sensors.lightLevel, LIGHT_THRESHOLD);
    Serial.printf("PIR Teras: %d\n", systemState.sensors.pirTeras);
    Serial.printf("L1 Current State: %s\n", systemState.devices.L1 ? "ON" : "OFF");
    Serial.printf("L1 Timer: %lu\n", systemState.deviceTimers[0]);

    if (systemState.sensors.lightLevel < LIGHT_THRESHOLD && systemState.sensors.pirTeras) {
      if (!systemState.devices.L1) {
        // Nyalakan lampu pertama kali
        setDevice("L1", true);
        systemState.deviceTimers[0] = currentTime;
        Serial.println("✅ Auto: Lampu Teras ON (lux < 150 & motion)");
        Serial.printf("✅ Timer L1 SET to: %lu\n", currentTime);
      } else {
        // Reset timer jika ada gerakan baru saat lampu sudah nyala
        unsigned long oldTimer = systemState.deviceTimers[0];
        systemState.deviceTimers[0] = currentTime;
        Serial.println("🔄 Auto: Lampu Teras timer RESET (motion detected while ON)");
        Serial.printf("🔄 Timer L1 RESET: %lu → %lu\n", oldTimer, currentTime);
      }
    } else {
      // Debug mengapa kondisi tidak terpenuhi
      if (systemState.sensors.lightLevel >= LIGHT_THRESHOLD) {
        Serial.printf("❌ Light too bright: %d >= %d\n", systemState.sensors.lightLevel, LIGHT_THRESHOLD);
      }
      if (!systemState.sensors.pirTeras) {
        Serial.println("❌ No motion detected on PIR Teras");
      }
    }
  } else {
    Serial.printf("❌ SOC insufficient for L1: %.1f < %.1f\n", currentSOC, max(minSOCRequired, 35.0f));
  }

  // RUANG TAMU - Lampu 2 (L2): dengan ENHANCED SMART TIMER
  if (currentSOC >= minSOCRequired && currentSOC >= 35.0) {
    Serial.println("--- CHECKING RUANG TAMU (L2) ---");
    Serial.printf("PIR Ruang Tamu: %d\n", systemState.sensors.pirRuangTamu);
    Serial.printf("L2 Current State: %s\n", systemState.devices.L2 ? "ON" : "OFF");
    Serial.printf("L2 Timer: %lu\n", systemState.deviceTimers[1]);

    if (systemState.sensors.pirRuangTamu) {
      if (!systemState.devices.L2) {
        setDevice("L2", true);
        systemState.deviceTimers[1] = currentTime;
        Serial.println("✅ Auto: Lampu Ruang Tamu ON (motion)");
        Serial.printf("✅ Timer L2 SET to: %lu\n", currentTime);
      } else {
        // Reset timer jika ada gerakan baru
        unsigned long oldTimer = systemState.deviceTimers[1];
        systemState.deviceTimers[1] = currentTime;
        Serial.println("🔄 Auto: Lampu Ruang Tamu timer RESET (motion detected while ON)");
        Serial.printf("🔄 Timer L2 RESET: %lu → %lu\n", oldTimer, currentTime);
      }
    } else {
      Serial.println("❌ No motion detected on PIR Ruang Tamu");
    }
  }

  // KAMAR TIDUR - Lampu 3 (L3): dengan ENHANCED SMART TIMER (hanya jika SOC ≥ 50%)
  if (currentSOC >= 50.0 && currentSOC >= minSOCRequired) {
    Serial.println("--- CHECKING KAMAR (L3) ---");
    Serial.printf("PIR Kamar: %d\n", systemState.sensors.pirKamar);
    Serial.printf("L3 Current State: %s\n", systemState.devices.L3 ? "ON" : "OFF");
    Serial.printf("L3 Timer: %lu\n", systemState.deviceTimers[3]);

    if (systemState.sensors.pirKamar) {
      if (!systemState.devices.L3) {
        setDevice("L3", true);
        systemState.deviceTimers[3] = currentTime;
        Serial.println("✅ Auto: Lampu Kamar ON (motion & SOC ≥ 50%)");
        Serial.printf("✅ Timer L3 SET to: %lu\n", currentTime);
      } else {
        // Reset timer jika ada gerakan baru
        unsigned long oldTimer = systemState.deviceTimers[3];
        systemState.deviceTimers[3] = currentTime;
        Serial.println("🔄 Auto: Lampu Kamar timer RESET (motion detected while ON)");
        Serial.printf("🔄 Timer L3 RESET: %lu → %lu\n", oldTimer, currentTime);
      }
    } else {
      Serial.println("❌ No motion detected on PIR Kamar");
    }
  } else if (currentSOC < 50.0 && systemState.devices.L3) {
    setDevice("L3", false);
    Serial.println("⚠️ Auto: Lampu Kamar OFF (SOC < 50%)");
  }

  // WC - Lampu 4 (L4): dengan ENHANCED SMART TIMER (hanya jika SOC ≥ 60%)
  if (currentSOC >= 60.0 && currentSOC >= minSOCRequired) {
    Serial.println("--- CHECKING WC (L4) ---");
    Serial.printf("PIR WC: %d\n", systemState.sensors.pirWC);
    Serial.printf("L4 Current State: %s\n", systemState.devices.L4 ? "ON" : "OFF");
    Serial.printf("L4 Timer: %lu\n", systemState.deviceTimers[5]);

    if (systemState.sensors.pirWC) {
      if (!systemState.devices.L4) {
        setDevice("L4", true);
        systemState.deviceTimers[5] = currentTime;
        Serial.println("✅ Auto: Lampu WC ON (motion & SOC ≥ 60%)");
        Serial.printf("✅ Timer L4 SET to: %lu\n", currentTime);
      } else {
        // Reset timer jika ada gerakan baru
        unsigned long oldTimer = systemState.deviceTimers[5];
        systemState.deviceTimers[5] = currentTime;
        Serial.println("🔄 Auto: Lampu WC timer RESET (motion detected while ON)");
        Serial.printf("🔄 Timer L4 RESET: %lu → %lu\n", oldTimer, currentTime);
      }
    } else {
      Serial.println("❌ No motion detected on PIR WC");
    }
  } else if (currentSOC < 60.0 && systemState.devices.L4) {
    setDevice("L4", false);
    Serial.println("⚠️ Auto: Lampu WC OFF (SOC < 60%)");
  }

  // DAPUR - Lampu 5 (L5): dengan ENHANCED SMART TIMER (hanya jika SOC ≥ 70%)
  if (currentSOC >= 70.0 && currentSOC >= minSOCRequired) {
    Serial.println("--- CHECKING DAPUR (L5) ---");
    Serial.printf("PIR Dapur: %d\n", systemState.sensors.pirDapur);
    Serial.printf("L5 Current State: %s\n", systemState.devices.L5 ? "ON" : "OFF");
    Serial.printf("L5 Timer: %lu\n", systemState.deviceTimers[6]);

    if (systemState.sensors.pirDapur) {
      if (!systemState.devices.L5) {
        setDevice("L5", true);
        systemState.deviceTimers[6] = currentTime;
        Serial.println("✅ Auto: Lampu Dapur ON (motion & SOC ≥ 70%)");
        Serial.printf("✅ Timer L5 SET to: %lu\n", currentTime);
      } else {
        // Reset timer jika ada gerakan baru
        unsigned long oldTimer = systemState.deviceTimers[6];
        systemState.deviceTimers[6] = currentTime;
        Serial.println("🔄 Auto: Lampu Dapur timer RESET (motion detected while ON)");
        Serial.printf("🔄 Timer L5 RESET: %lu → %lu\n", oldTimer, currentTime);
      }
    } else {
      Serial.println("❌ No motion detected on PIR Dapur");
    }
  } else if (currentSOC < 70.0 && systemState.devices.L5) {
    setDevice("L5", false);
    Serial.println("⚠️ Auto: Lampu Dapur OFF (SOC < 70%)");
  }

  // KIPAS - dengan SOC requirement yang berbeda untuk K1 dan K2
if (currentSOC >= minSOCRequired) {
    Serial.println("--- CHECKING KIPAS ---");
    Serial.printf("Temperature R.Tamu: %.1f°C (threshold: %.1f°C)\n", 
                  systemState.sensors.temperatureRTamu, TEMP_THRESHOLD);
    Serial.printf("Temperature Kamar: %.1f°C (threshold: %.1f°C)\n", 
                  systemState.sensors.temperatureKamar, TEMP_THRESHOLD);

    // ✅ KIPAS RUANG TAMU (K2) - HANYA jika SOC ≥ 90%
    if (currentSOC >= 90.0 && systemState.sensors.pirRuangTamu && 
        systemState.sensors.temperatureRTamu > TEMP_THRESHOLD) {
        if (!systemState.devices.K2) {
            setDevice("K2", true);
            systemState.deviceTimers[2] = currentTime;
            Serial.println("✅ Auto: Kipas Ruang Tamu ON (motion & temp > 32°C & SOC ≥ 90%)");
        } else {
            unsigned long oldTimer = systemState.deviceTimers[2];
            systemState.deviceTimers[2] = currentTime;
            Serial.println("🔄 Auto: Kipas Ruang Tamu timer RESET");
        }
    }

    // ✅ KIPAS KAMAR (K1) - jika SOC ≥ 80%
    if (currentSOC >= 80.0 && systemState.sensors.pirKamar && 
        systemState.sensors.temperatureKamar > TEMP_THRESHOLD) {
        if (!systemState.devices.K1) {
            setDevice("K1", true);
            systemState.deviceTimers[4] = currentTime;
            Serial.println("✅ Auto: Kipas Kamar ON (motion & temp > 32°C & SOC ≥ 80%)");
        } else {
            unsigned long oldTimer = systemState.deviceTimers[4];
            systemState.deviceTimers[4] = currentTime;
            Serial.println("🔄 Auto: Kipas Kamar timer RESET");
        }
    }
}

// Force OFF berdasarkan SOC
if (currentSOC < 90.0 && systemState.devices.K2) {
    setDevice("K2", false);
    Serial.println("⚠️ Auto: Kipas Ruang Tamu OFF (SOC < 90%)");
}
if (currentSOC < 80.0 && systemState.devices.K1) {
    setDevice("K1", false);
    Serial.println("⚠️ Auto: Kipas Kamar OFF (SOC < 80%)");
}

  Serial.println("=================================");
}

void executeLampControlLogic() {
  // Hanya lampu yang dikontrol (tanpa kipas)
  executeRoomControlLogic();  // Kipas sudah dimatikan sebelumnya
}

void executeBasicLampControlLogic() {
  // L1, L2, L3, L4 saja
  executeRoomControlLogic();  // L5 sudah dimatikan sebelumnya
}

void executeMinimalLampControlLogic() {
  // L1, L2, L3 saja
  executeRoomControlLogic();  // L4, L5 sudah dimatikan sebelumnya
}

void executeCriticalLampControlLogic() {
  // L1, L2 saja
  executeRoomControlLogic();  // L3, L4, L5 sudah dimatikan sebelumnya
}

void checkAutoOffTimers() {
  if (systemState.mode != "AUTO") return;

  unsigned long currentTime = millis();
  float currentSOC = systemState.battery.soc;

  Serial.println("=== CHECKING AUTO OFF TIMERS (ENHANCED DEBUG) ===");
  Serial.printf("Current Time: %lu\n", currentTime);
  Serial.printf("Auto Off Duration: %d ms\n", AUTO_OFF_DURATION);

  // Check each device timer with detailed debug
  struct DeviceCheck {
    bool* device;
    unsigned long* timer;
    String name;
    int index;
  };

  DeviceCheck devices[] = {
    { &systemState.devices.L1, &systemState.deviceTimers[0], "Lampu Teras", 0 },
    { &systemState.devices.L2, &systemState.deviceTimers[1], "Lampu Ruang Tamu", 1 },
    { &systemState.devices.K2, &systemState.deviceTimers[2], "Kipas Ruang Tamu", 2 },
    { &systemState.devices.L3, &systemState.deviceTimers[3], "Lampu Kamar", 3 },
    { &systemState.devices.K1, &systemState.deviceTimers[4], "Kipas Kamar", 4 },
    { &systemState.devices.L4, &systemState.deviceTimers[5], "Lampu WC", 5 },
    { &systemState.devices.L5, &systemState.deviceTimers[6], "Lampu Dapur", 6 }
  };

  for (int i = 0; i < 7; i++) {
    if (*(devices[i].device)) {
      unsigned long elapsed = currentTime - *(devices[i].timer);
      Serial.printf("%s: ON, Timer: %lu, Elapsed: %lu ms",
                    devices[i].name.c_str(), *(devices[i].timer), elapsed);

      if (elapsed >= AUTO_OFF_DURATION) {
        String deviceCode = "";
        switch (devices[i].index) {
          case 0: deviceCode = "L1"; break;
          case 1: deviceCode = "L2"; break;
          case 2: deviceCode = "K2"; break;
          case 3: deviceCode = "L3"; break;
          case 4: deviceCode = "K1"; break;
          case 5: deviceCode = "L4"; break;
          case 6: deviceCode = "L5"; break;
        }
        setDevice(deviceCode, false);
        Serial.printf(" → ⏰ TURNED OFF (10s timeout)\n");
      } else {
        Serial.printf(" → ⏳ Still ON (%lu ms remaining)\n", AUTO_OFF_DURATION - elapsed);
      }
    } else {
      Serial.printf("%s: OFF\n", devices[i].name.c_str());
    }
  }

  // Force off perangkat berdasarkan level SOC (override timer)
  Serial.println("--- SOC BASED FORCE OFF CHECK ---");
  if (currentSOC < 50.0 && systemState.devices.L3) {
    setDevice("L3", false);
    Serial.println("⚠️ Auto: Force OFF L3 (SOC < 50%)");
  }
  if (currentSOC < 60.0 && systemState.devices.L4) {
    setDevice("L4", false);
    Serial.println("⚠️ Auto: Force OFF L4 (SOC < 60%)");
  }
  if (currentSOC < 70.0 && systemState.devices.L5) {
    setDevice("L5", false);
    Serial.println("⚠️ Auto: Force OFF L5 (SOC < 70%)");
  }
  if (currentSOC < 80.0) {
    if (systemState.devices.K1) {
      setDevice("K1", false);
      Serial.println("⚠️ Auto: Force OFF K1 (SOC < 80%)");
    }
    if (systemState.devices.K2) {
      setDevice("K2", false);
      Serial.println("⚠️ Auto: Force OFF K2 (SOC < 80%)");
    }
  }

  Serial.println("===============================================");
}

//========================================
// DATABASE COMMUNICATION FUNCTIONS
//========================================
void getDataFromDatabase() {
  Serial.println();
  Serial.println("=============== GET DATA FROM DATABASE ===============");

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi not connected!");
    return;
  }

  HTTPClient http;
  int httpCode;

  postData = "id=" + systemState.deviceId;
  payload = "";

  digitalWrite(ON_Board_LED, HIGH);

  Serial.println("🔗 Connecting to: " + getDataURL);
  Serial.println("📤 POST data: " + postData);
  Serial.println("📏 POST data length: " + String(postData.length()));

  http.begin(getDataURL);
  http.setTimeout(15000);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  Serial.println("📡 Sending POST request...");

  httpCode = http.POST(postData);

  Serial.println("📊 Response received:");
  Serial.print("   HTTP Code: ");
  Serial.println(httpCode);

  if (httpCode > 0) {
    payload = http.getString();
    Serial.print("   Payload Length: ");
    Serial.println(payload.length());
    Serial.print("   Payload Content: ");
    Serial.println(payload);
    JSONVar myObject = JSON.parse(payload);

    // Analyze response
    if (payload.indexOf("\"status\":\"success\"") >= 0) {
      Serial.println("✅ SUCCESS: Valid JSON response received");
    } else if (payload.indexOf("\"status\":\"error\"") >= 0) {
      Serial.println("⚠️ ERROR: Server returned error response");
    } else {
      Serial.println("❓ UNKNOWN: Unexpected response format");
    }
    Serial.print("==>");
    Serial.println(payload.indexOf("\"MODE\""));
    // Check for specific fields
    if (payload.indexOf("\"MODE\"") >= 0) {
      // Check for mode changes
      if (myObject.hasOwnProperty("MODE")) {
        Serial.print("Mode change command: ");
        Serial.println(myObject["MODE"]);
        if (strcmp(myObject["MODE"], "AUTO") == 0) {
          systemState.mode = "AUTO";
          Serial.println("Mode changed to AUTO from database");
        }

        if (strcmp(myObject["MODE"], "MANUAL") == 0) {
          systemState.mode = "MANUAL";
          Serial.println("Mode changed to MANUAL from database");
        }
      }
      Serial.println("✅ MODE field found in response");
    } else {
      Serial.println("❌ MODE field NOT found in response");
    }

  } else {
    Serial.print("❌ HTTP Error: ");
    Serial.println(httpCode);

    switch (httpCode) {
      case -1:
        Serial.println("   → Connection failed (server unreachable)");
        break;
      case -2:
        Serial.println("   → Send header failed");
        break;
      case -3:
        Serial.println("   → Send payload failed");
        break;
      case -4:
        Serial.println("   → Not connected");
        break;
      case -5:
        Serial.println("   → Connection lost");
        break;
      case -6:
        Serial.println("   → No stream");
        break;
      case -7:
        Serial.println("   → No HTTP server");
        break;
      case -8:
        Serial.println("   → Too less RAM");
        break;
      case -9:
        Serial.println("   → Encoding");
        break;
      case -10:
        Serial.println("   → Stream write");
        break;
      case -11:
        Serial.println("   → Timeout");
        break;
      default:
        Serial.println("   → Unknown error");
    }
  }

  http.end();
  digitalWrite(ON_Board_LED, LOW);
  Serial.println("=====================================================");
}

void controlDevicesFromDatabase() {
  Serial.println();
  Serial.println("---------------control_devices()");
  Serial.println("Current Mode: " + systemState.mode);

  // Cek payload tidak kosong
  if (payload.length() == 0) {
    Serial.println("No payload received - skipping device control");
    Serial.println("---------------");
    return;
  }

  // PERBAIKAN: Pastikan mode MANUAL
  if (systemState.mode != "MANUAL") {
    Serial.println("Not in MANUAL mode - skipping database control");
    Serial.println("Current mode: " + systemState.mode);
    Serial.println("---------------");
    return;
  }

  JSONVar myObject = JSON.parse(payload);

  if (JSON.typeof(myObject) == "undefined") {
    Serial.println("Parsing input failed!");
    Serial.println("Payload was: " + payload);
    Serial.println("---------------");
    return;
  }

  // Control devices based on database commands
  Serial.println("Processing database commands in MANUAL mode:");

  if (myObject.hasOwnProperty("L1")) {
    Serial.print("Database command L1 = ");
    Serial.println(myObject["L1"]);
    if (strcmp(myObject["L1"], "ON") == 0) {
      setDevice("L1", true);
      Serial.println("L1 ON from database (MANUAL mode)");
    }
    if (strcmp(myObject["L1"], "OFF") == 0) {
      setDevice("L1", false);
      Serial.println("L1 OFF from database (MANUAL mode)");
    }
  }

  if (myObject.hasOwnProperty("L2")) {
    Serial.print("Database command L2 = ");
    Serial.println(myObject["L2"]);
    if (strcmp(myObject["L2"], "ON") == 0) {
      setDevice("L2", true);
      Serial.println("L2 ON from database (MANUAL mode)");
    }
    if (strcmp(myObject["L2"], "OFF") == 0) {
      setDevice("L2", false);
      Serial.println("L2 OFF from database (MANUAL mode)");
    }
  }

  if (myObject.hasOwnProperty("L3")) {
    Serial.print("Database command L3 = ");
    Serial.println(myObject["L3"]);
    if (strcmp(myObject["L3"], "ON") == 0) {
      setDevice("L3", true);
      Serial.println("L3 ON from database (MANUAL mode)");
    }
    if (strcmp(myObject["L3"], "OFF") == 0) {
      setDevice("L3", false);
      Serial.println("L3 OFF from database (MANUAL mode)");
    }
  }

  if (myObject.hasOwnProperty("L4")) {
    Serial.print("Database command L4 = ");
    Serial.println(myObject["L4"]);
    if (strcmp(myObject["L4"], "ON") == 0) {
      setDevice("L4", true);
      Serial.println("L4 ON from database (MANUAL mode)");
    }
    if (strcmp(myObject["L4"], "OFF") == 0) {
      setDevice("L4", false);
      Serial.println("L4 OFF from database (MANUAL mode)");
    }
  }

  if (myObject.hasOwnProperty("L5")) {
    Serial.print("Database command L5 = ");
    Serial.println(myObject["L5"]);
    if (strcmp(myObject["L5"], "ON") == 0) {
      setDevice("L5", true);
      Serial.println("L5 ON from database (MANUAL mode)");
    }
    if (strcmp(myObject["L5"], "OFF") == 0) {
      setDevice("L5", false);
      Serial.println("L5 OFF from database (MANUAL mode)");
    }
  }

  if (myObject.hasOwnProperty("K1")) {
    Serial.print("Database command K1 = ");
    Serial.println(myObject["K1"]);
    if (strcmp(myObject["K1"], "ON") == 0) {
      setDevice("K1", true);
      Serial.println("K1 ON from database (MANUAL mode)");
    }
    if (strcmp(myObject["K1"], "OFF") == 0) {
      setDevice("K1", false);
      Serial.println("K1 OFF from database (MANUAL mode)");
    }
  }

  if (myObject.hasOwnProperty("K2")) {
    Serial.print("Database command K2 = ");
    Serial.println(myObject["K2"]);
    if (strcmp(myObject["K2"], "ON") == 0) {
      setDevice("K2", true);
      Serial.println("K2 ON from database (MANUAL mode)");
    }
    if (strcmp(myObject["K2"], "OFF") == 0) {
      setDevice("K2", false);
      Serial.println("K2 OFF from database (MANUAL mode)");
    }
  }

  // Check for mode changes
  if (myObject.hasOwnProperty("MODE")) {
    Serial.print("Mode change command: ");
    Serial.println(myObject["MODE"]);
    if (strcmp(myObject["MODE"], "AUTO") == 0) {
      systemState.mode = "AUTO";
      Serial.println("Mode changed to AUTO from database");
    }

    if (strcmp(myObject["MODE"], "MANUAL") == 0) {
      systemState.mode = "MANUAL";
      Serial.println("Mode changed to MANUAL from database");
    }
  }

  Serial.println("---------------");
}

void sendDataToDatabase() {
  Serial.println();
  Serial.println("---------------updatedata.php");

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected!");
    return;
  }

  // Get current device states
  String L1_State = systemState.devices.L1 ? "ON" : "OFF";
  String L2_State = systemState.devices.L2 ? "ON" : "OFF";
  String L3_State = systemState.devices.L3 ? "ON" : "OFF";
  String L4_State = systemState.devices.L4 ? "ON" : "OFF";
  String L5_State = systemState.devices.L5 ? "ON" : "OFF";
  String K1_State = systemState.devices.K1 ? "ON" : "OFF";
  String K2_State = systemState.devices.K2 ? "ON" : "OFF";
  String USB_State = systemState.devices.USB ? "ON" : "OFF";

  // PERBAIKAN: Prepare POST data dengan SEMUA parameter yang diperlukan
  postData = "id=" + systemState.deviceId;
  postData += "&mode=" + systemState.mode;

  // Sensor data
  postData += "&temperature_kamar=" + String(systemState.sensors.temperatureKamar, 2);
  postData += "&temperature_rtamu=" + String(systemState.sensors.temperatureRTamu, 2);
  postData += "&humidity_kamar=" + String(systemState.sensors.humidityKamar, 1);
  postData += "&humidity_rtamu=" + String(systemState.sensors.humidityRTamu, 1);
  postData += "&light_level=" + String(systemState.sensors.lightLevel);
  postData += "&battery_voltage=" + String(systemState.battery.voltage, 2);
  postData += "&battery_soc=" + String(systemState.battery.soc, 1);
  postData += "&estimated_runtime=" + String(systemState.battery.estimatedRuntime, 1);
  postData += "&status_read_sensors=" + systemState.statusReadSensors;

  // PIR data
  postData += "&pir_teras=" + String(systemState.sensors.pirTeras ? 1 : 0);
  postData += "&pir_ruang_tamu=" + String(systemState.sensors.pirRuangTamu ? 1 : 0);
  postData += "&pir_kamar=" + String(systemState.sensors.pirKamar ? 1 : 0);
  postData += "&pir_wc=" + String(systemState.sensors.pirWC ? 1 : 0);
  postData += "&pir_dapur=" + String(systemState.sensors.pirDapur ? 1 : 0);

  // PENTING: Device states (yang menyebabkan warning)
  postData += "&l1=" + L1_State;
  postData += "&l2=" + L2_State;
  postData += "&l3=" + L3_State;
  postData += "&l4=" + L4_State;
  postData += "&l5=" + L5_State;
  postData += "&k1=" + K1_State;
  postData += "&k2=" + K2_State;
  postData += "&usb=" + USB_State;

  // Power data
  postData += "&power_l1=" + String(systemState.powerL1.power, 2);
  postData += "&power_l2=" + String(systemState.powerL2.power, 2);
  postData += "&power_l3=" + String(systemState.powerL3.power, 2);
  postData += "&power_l4=" + String(systemState.powerL4.power, 2);
  postData += "&power_l5=" + String(systemState.powerL5.power, 2);
  postData += "&power_k1=" + String(systemState.powerK1.power, 2);
  postData += "&power_k2=" + String(systemState.powerK2.power, 2);
  postData += "&power_usb=" + String(systemState.powerUSB.power, 2);
  postData += "&total_power=" + String(systemState.totalPower, 2);

  // Current data
  postData += "&current_l1=" + String(systemState.powerL1.current, 3);
  postData += "&current_l2=" + String(systemState.powerL2.current, 3);
  postData += "&current_l3=" + String(systemState.powerL3.current, 3);
  postData += "&current_l4=" + String(systemState.powerL4.current, 3);
  postData += "&current_l5=" + String(systemState.powerL5.current, 3);
  postData += "&current_k1=" + String(systemState.powerK1.current, 3);
  postData += "&current_k2=" + String(systemState.powerK2.current, 3);
  postData += "&current_usb=" + String(systemState.powerUSB.current, 3);
  postData += "&current_total=" + String(systemState.totalCurrent, 3);

  // Voltage data
  postData += "&voltage_l1=" + String(systemState.powerL1.voltage, 2);
  postData += "&voltage_l2=" + String(systemState.powerL2.voltage, 2);
  postData += "&voltage_l3=" + String(systemState.powerL3.voltage, 2);
  postData += "&voltage_l4=" + String(systemState.powerL4.voltage, 2);
  postData += "&voltage_l5=" + String(systemState.powerL5.voltage, 2);
  postData += "&voltage_k1=" + String(systemState.powerK1.voltage, 2);
  postData += "&voltage_k2=" + String(systemState.powerK2.voltage, 2);
  postData += "&voltage_usb=" + String(systemState.powerUSB.voltage, 2);
  postData += "&voltage_system=" + String(systemState.battery.voltage, 2);

  // DEBUG: Print important data being sent
  Serial.println("=== SENDING DATA TO DATABASE ===");
  Serial.println("URL: " + updateDataURL);
  Serial.println("Data Length: " + String(postData.length()));
  Serial.println("Device States:");
  Serial.println("  L1=" + L1_State + ", L2=" + L2_State + ", L3=" + L3_State);
  Serial.println("  L4=" + L4_State + ", L5=" + L5_State);
  Serial.println("  K1=" + K1_State + ", K2=" + K2_State + ", USB=" + USB_State);
  Serial.println("Battery SOC: " + String(systemState.battery.soc));
  Serial.println("Battery Voltage: " + String(systemState.battery.voltage));
  Serial.println("Total Power: " + String(systemState.totalPower));
  Serial.println("==============================");

  HTTPClient http;
  int httpCode;

  digitalWrite(ON_Board_LED, HIGH);

  http.begin(updateDataURL);
  http.setTimeout(15000);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  httpCode = http.POST(postData);

  if (httpCode > 0) {
    payload = http.getString();
    Serial.print("httpCode : ");
    Serial.println(httpCode);
    Serial.print("payload  : ");
    Serial.println(payload);

    // Check for PHP warnings
    if (payload.indexOf("Warning") >= 0) {
      Serial.println("⚠️ PHP Warnings detected in response!");
    } else if (payload.indexOf("success") >= 0) {
      Serial.println("✅ Data sent successfully without warnings!");
    }
  } else {
    Serial.print("HTTP Error: ");
    Serial.println(httpCode);
  }

  http.end();
  Serial.println("---------------");
  digitalWrite(ON_Board_LED, LOW);
}

//========================================
// SETUP FUNCTIONS
//========================================
void initializeI2C() {
  Serial.println("Initializing I2C devices...");

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  if (testTCA9548A()) {
    Serial.println("TCA9548A multiplexer detected!");
  } else {
    Serial.println("ERROR: TCA9548A not found!");
    return;
  }

  initializePCF8574(PCF8574_RELAY_ADDR);
  initializePCF8574(PCF8574_SENSOR_ADDR);
  matikanSemuaBeban();

  Serial.println("I2C devices initialized");
}

void initializeSensors() {
  Serial.println("Initializing sensors...");

  // Initialize DHT sensors
  dht1.begin();
  dht2.begin();

  // Initialize ADC for LDR
  analogSetAttenuation(ADC_11db);
  analogSetWidth(12);

  // Initialize INA219 Battery Voltage Sensor
  initializeINA219Battery();

  initializeINASensors();
  Serial.println("Sensors initialized");
}

void initializeWiFi() {
  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.println();
  Serial.println("-------------");
  Serial.print("Connecting");

  // Connection timeout process
  int connecting_process_timed_out = 20;  // 20 seconds
  connecting_process_timed_out = connecting_process_timed_out * 2;

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");

    // Make the On Board LED flash during connection process
    digitalWrite(ON_Board_LED, HIGH);
    delay(250);
    digitalWrite(ON_Board_LED, LOW);
    delay(250);

    // Countdown connection timeout
    if (connecting_process_timed_out > 0) connecting_process_timed_out--;
    if (connecting_process_timed_out == 0) {
      Serial.println("WiFi timeout - retrying instead of restart");
      WiFi.disconnect();
      delay(2000);
      WiFi.begin(ssid, password);
      connecting_process_timed_out = 40;  // Reset timer
    }
  }

  digitalWrite(ON_Board_LED, LOW);  // Turn off LED when connected

  Serial.println();
  Serial.print("Successfully connected to : ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("-------------");
}

void testServerConnection() {
  Serial.println("=== TESTING SERVER CONNECTION ===");
  Serial.println("Server IP: " + serverIP);
  Serial.println("WiFi Status: " + String(WiFi.status()));
  Serial.println("ESP32 IP: " + WiFi.localIP().toString());

  HTTPClient http;
  String testURL = "http://" + serverIP + "/";

  Serial.println("Testing: " + testURL);

  http.begin(testURL);
  http.setTimeout(10000);

  int httpCode = http.GET();
  String payload = http.getString();

  Serial.println("Test Result:");
  Serial.println("HTTP Code: " + String(httpCode));
  Serial.println("Payload Length: " + String(payload.length()));

  if (httpCode == -1) {
    Serial.println("❌ SERVER UNREACHABLE!");
    Serial.println("Solutions:");
    Serial.println("1. Check if XAMPP is running");
    Serial.println("2. Verify IP address with 'ipconfig'");
    Serial.println("3. Check if both devices on same WiFi");
    Serial.println("4. Disable Windows Firewall temporarily");
  } else if (httpCode == 200) {
    Serial.println("✅ SERVER REACHABLE!");
  } else {
    Serial.println("⚠️ Server responds but with error code");
  }

  http.end();
  Serial.println("=================================");
}

void updateVoltageHistory(float newVoltage) {
  batteryMonitor.voltageHistory[batteryMonitor.historyIndex] = newVoltage;
  batteryMonitor.historyIndex = (batteryMonitor.historyIndex + 1) % 5;
}

bool detectCharging() {
  unsigned long currentTime = millis();

  // Check setiap 3 detik
  if (currentTime - batteryMonitor.lastChargingCheck < 3000) {
    return batteryMonitor.isCharging;
  }

  batteryMonitor.lastChargingCheck = currentTime;

  // Update voltage history
  updateVoltageHistory(systemState.battery.voltage);

  // Hitung trend voltage
  float voltageChange = 0;
  for (int i = 1; i <= 3; i++) {
    int curr = (batteryMonitor.historyIndex - 1 + 5) % 5;
    int prev = (batteryMonitor.historyIndex - 1 - i + 5) % 5;
    voltageChange += (batteryMonitor.voltageHistory[curr] - batteryMonitor.voltageHistory[prev]);
  }
  voltageChange = voltageChange / 3.0;

  // Deteksi charging - SIMPLE & RELIABLE
  bool chargingDetected = false;
  String reason = "";

  // Kondisi 1: Voltage naik > 0.1V dengan current rendah
  if (voltageChange > 0.1 && systemState.totalCurrent < 2.0) {
    chargingDetected = true;
    reason = "Voltage rising +0.1V";
  }
  // Kondisi 2: Voltage tinggi (definitely charging)
  else if (systemState.battery.voltage > 13.0) {
    chargingDetected = true;
    reason = "High voltage >13.0V";
  }
  // Kondisi 3: SOC sangat tinggi
  else if (systemState.battery.soc > 95.0) {
    chargingDetected = true;
    reason = "Very high SOC >95%";
  }
  // Kondisi 4: Stop charging jika voltage turun dengan beban
  else if (voltageChange < -0.1 && systemState.totalCurrent > 1.0) {
    chargingDetected = false;
    reason = "Voltage dropping with load";
  }

  // Stability check - butuh 2 reading stabil
  if (chargingDetected != batteryMonitor.lastChargingState) {
    batteryMonitor.stableCount = 0;
    batteryMonitor.lastChargingState = chargingDetected;
  } else {
    batteryMonitor.stableCount++;
  }

  // Update state setelah stabil
  if (batteryMonitor.stableCount >= 2) {
    bool wasCharging = batteryMonitor.isCharging;
    batteryMonitor.isCharging = chargingDetected;

    // Log perubahan
    if (batteryMonitor.isCharging != wasCharging) {
      Serial.println(batteryMonitor.isCharging ? "🔋 CHARGING DETECTED!" : "🔋 CHARGING STOPPED");
      Serial.printf("   Reason: %s\n", reason.c_str());
      Serial.printf("   Voltage: %.2fV, Trend: %+.3fV\n", systemState.battery.voltage, voltageChange);
    }
  }

  return batteryMonitor.isCharging;
}

void handleBatteryEmergency() {
  float currentSOC = systemState.battery.soc;
  unsigned long currentTime = millis();

  // ENTER emergency mode
  if (currentSOC <= batteryMonitor.emergencySOC && !batteryMonitor.emergencyModeActive) {
    batteryMonitor.emergencyModeActive = true;
    batteryMonitor.emergencyStartTime = currentTime;
    batteryMonitor.preEmergencyVoltage = systemState.battery.voltage;

    Serial.println("🚨 EMERGENCY MODE ACTIVATED 🚨");
    Serial.printf("   SOC: %.1f%% | Voltage: %.2fV\n", currentSOC, systemState.battery.voltage);

    // Matikan semua beban
    matikanSemuaBeban();
  }

  // CHECK for recovery (EXIT emergency)
  if (batteryMonitor.emergencyModeActive) {
    bool canRecover = false;
    String recoveryReason = "";

    // Recovery kondisi 1: SOC naik ke recovery threshold (hysteresis)
    if (currentSOC >= batteryMonitor.recoverySOC) {
      canRecover = true;
      recoveryReason = "SOC recovery " + String(batteryMonitor.recoverySOC) + "%";
    }

    // Recovery kondisi 2: Charging detected + voltage recovery
    else if (batteryMonitor.isCharging && systemState.battery.voltage > (batteryMonitor.preEmergencyVoltage + 0.2)) {
      canRecover = true;
      recoveryReason = "Charging + voltage recovery";
    }

    // Recovery kondisi 3: Very high voltage (definitely charging)
    else if (systemState.battery.voltage > 13.2) {
      canRecover = true;
      recoveryReason = "Very high voltage >13.2V";
    }

    // EXECUTE recovery
    if (canRecover) {
      batteryMonitor.emergencyModeActive = false;

      Serial.println("✅ EMERGENCY RECOVERY SUCCESS ✅");
      Serial.printf("   Reason: %s\n", recoveryReason.c_str());
      Serial.printf("   Final SOC: %.1f%% | Voltage: %.2fV\n", currentSOC, systemState.battery.voltage);
      Serial.printf("   Emergency duration: %lu seconds\n", (currentTime - batteryMonitor.emergencyStartTime) / 1000);

      // IMPORTANT: TIDAK force mode, respect user choice
      Serial.printf("   Mode stays: %s (user preference)\n", systemState.mode.c_str());
    }
  }
}

void initializeBatteryMonitoring() {
  Serial.println("=== INITIALIZING BATTERY MONITORING ===");

  // Initialize voltage history dengan current reading
  float currentVoltage = getBatteryVoltageFromINA219();
  for (int i = 0; i < 5; i++) {
    batteryMonitor.voltageHistory[i] = currentVoltage;
  }

  Serial.printf("✅ Battery monitoring initialized\n");
  Serial.printf("   Current voltage: %.2fV\n", currentVoltage);
  Serial.printf("   Emergency threshold: %.1f%% SOC\n", batteryMonitor.emergencySOC);
  Serial.printf("   Recovery threshold: %.1f%% SOC\n", batteryMonitor.recoverySOC);
  Serial.println("=========================================");
}

//====================
// SETUP & MAIN LOOP
//====================
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== Smart Home DC System - Sesuai Proposal Skripsi ===");
  Serial.println("ESP32 Smart Home DC Control System");
  Serial.println("Pin Configuration:");
  Serial.println("- I2C: SDA=GPIO21, SCL=GPIO22");
  Serial.println("- DHT1 (Kamar): GPIO18");
  Serial.println("- DHT2 (Ruang Tamu): GPIO19");
  Serial.println("- LDR (Teras): GPIO32");
  Serial.println("- BATTERY VOLTAGE SENSOR INA219");
  Serial.println("- OnBoard LED: GPIO2");

  // Initialize GPIO pins
  pinMode(ON_Board_LED, OUTPUT);

  // LED startup sequence
  digitalWrite(ON_Board_LED, HIGH);
  delay(2000);
  digitalWrite(ON_Board_LED, LOW);
  delay(1000);

  // Force manual mode at startup
  forceManualMode();

  // DEVICE ID DEBUG
  Serial.println("=== DEVICE ID DEBUG ===");
  Serial.println("Device ID: '" + systemState.deviceId + "'");
  Serial.println("Device ID Length: " + String(systemState.deviceId.length()));
  Serial.println("=======================");

  initializeI2C();
  initializeSensors();
  initializeBatteryMonitoring();
  setBatteryCompensationParameters();
  calibrateBatteryCompensation();
  initializeWiFi();

  // Test server connection
  delay(2000);
  testServerConnection();

  // USB standby selalu hidup
  systemState.devices.USB = true;

  // Debug initial battery voltage reading
  Serial.println("=== INITIAL INA219 BATTERY TEST ===");
  debugINA219Battery();
  Serial.println("===================================");

  Serial.println("=== ESP32 System Ready ===");
  Serial.println("Database URLs:");
  Serial.println("- Get Data: " + getDataURL);
  Serial.println("- Update Data: " + updateDataURL);
  Serial.println("Device ID: " + systemState.deviceId);
  Serial.println("Mode: " + systemState.mode);
  Serial.println("Threshold Settings:");
  Serial.printf("- Temperature: %.1f°C\n", TEMP_THRESHOLD);
  Serial.printf("- Light: %d lux\n", LIGHT_THRESHOLD);
  Serial.printf("- Low Battery: %.1f%%\n", LOW_BATTERY_THRESHOLD);
  Serial.printf("- Auto Off Duration: %d seconds\n", AUTO_OFF_DURATION / 1000);
  Serial.println(" Battery Voltage Sensor:");
  Serial.printf("- Type: INA219 Sensor @ 0x%02X\n", INA219_BATTERY_ADDR);
  Serial.printf("- Connection: Direct to I2C Bus (parallel with TCA)\n");
  Serial.printf("- Function: Voltage monitoring only\n");
  Serial.printf("- Status: %s\n", ina219Battery.isConnected ? "CONNECTED" : "NOT FOUND");

  delay(2000);
}

void loop() {
  // Check WiFi connection status
  if (WiFi.status() == WL_CONNECTED) {

    // STEP A: Read sensors FIRST
    readSensors();
    debugCompensation();

    // STEP B: Battery monitoring
    detectCharging();
    handleBatteryEmergency();

    // STEP C: Check if system paused (emergency + not charging)
    bool systemPaused = batteryMonitor.emergencyModeActive && !batteryMonitor.isCharging;

    if (systemPaused) {
      Serial.println("🚨 SYSTEM PAUSED: Emergency active, waiting for charging...");

      // Tetap kirim data ke database untuk monitoring
      if (millis() - lastDatabaseUpdate >= DATABASE_UPDATE_INTERVAL) {
        sendDataToDatabase();
        lastDatabaseUpdate = millis();
      }

      delay(3000);  // Wait 3 seconds lalu cek lagi
      return;       // Skip execution tapi BISA recovery jika charging
    }

    // STEP D: Get commands dari database (normal execution)
    getDataFromDatabase();

    // STEP E: Control devices berdasarkan mode + battery safety
    if (systemState.mode == "MANUAL") {
      controlDevicesFromDatabase();
      Serial.println("Mode MANUAL: Following database commands");
    } else if (systemState.mode == "AUTO") {
      Serial.println("Mode AUTO: Skipping database control");
    }

    delay(500);

    // STEP F: Execute auto mode (jika AUTO dan tidak emergency)
    if (systemState.mode == "AUTO") {
      if (!batteryMonitor.emergencyModeActive || batteryMonitor.isCharging) {
        executeAutoMode();
        checkAutoOffTimers();
        Serial.println("Mode AUTO: Logic executed");
      } else {
        Serial.println("Mode AUTO: Logic paused (emergency)");
      }
    }

    // STEP G: Debug INA219 battery sensor
    static int debugCounter = 0;
    if (debugCounter++ >= 10) {
      debugINA219Battery();
      debugCounter = 0;
    }

    // STEP H: Send data to database
    if (millis() - lastDatabaseUpdate >= DATABASE_UPDATE_INTERVAL) {
      sendDataToDatabase();
      lastDatabaseUpdate = millis();
    }

    // STEP I: Debug monitoring setiap 5 detik
    static unsigned long lastMonitorDebug = 0;
    if (millis() - lastMonitorDebug > 5000) {
      Serial.println("\n=== BATTERY MONITOR STATUS ===");
      Serial.printf("SOC: %.1f%% | Voltage: %.2fV | Charging: %s\n",
                    systemState.battery.soc, systemState.battery.voltage,
                    batteryMonitor.isCharging ? "YES" : "NO");
      Serial.printf("Emergency: %s | Mode: %s | Total Power: %.1fW\n",
                    batteryMonitor.emergencyModeActive ? "ACTIVE" : "OFF",
                    systemState.mode.c_str(), systemState.totalPower);
      Serial.println("==============================\n");
      lastMonitorDebug = millis();
    }

    delay(1500);  // Main loop delay
  } else {
    Serial.println("WiFi disconnected! Attempting to reconnect...");
    WiFi.reconnect();
    delay(5000);
  }
}