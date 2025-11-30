#define BLYNK_TEMPLATE_ID "TMPL3qtqoSZww"
#define BLYNK_TEMPLATE_NAME "hdc 1080"
#define BLYNK_AUTH_TOKEN "MXJAL1ZmzjGCt7hLMT8U-J9azt4_4XmX"

#include <Wire.h>
#include <SPI.h>
#include "ClosedCube_HDC1080.h"
#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

// WiFi credentials
char ssid[] = "Vishalaakshi";
char pass[] = "flamingocheerful761";

// I2C addresses
#define HDC1080_ADDR      0x40
#define OPT3001_ADDRESS   0x44

// OPT3001 registers
#define RESULT_REGISTER   0x00
#define CONFIG_REGISTER   0x01

ClosedCube_HDC1080 hdc1080;

// LoRa (SX1278) pins
#define SX1278_CS    5
#define SX1278_SCK   18
#define SX1278_MISO  19
#define SX1278_MOSI  27

// LoRa registers & constants
#define REG_FIFO                0x00
#define REG_OP_MODE             0x01
#define REG_FRF_MSB             0x06
#define REG_FRF_MID             0x07
#define REG_FRF_LSB             0x08
#define REG_PA_CONFIG           0x09
#define REG_FIFO_ADDR_PTR       0x0D
#define REG_FIFO_TX_BASE_ADDR   0x0E
#define REG_IRQ_FLAGS           0x12
#define REG_PAYLOAD_LENGTH      0x22
#define REG_MODEM_CONFIG1       0x1D
#define REG_MODEM_CONFIG2       0x1E
#define REG_MODEM_CONFIG3       0x26
#define REG_VERSION             0x42

#define MODE_LONG_RANGE_MODE    0x80
#define MODE_SLEEP              0x00
#define MODE_STDBY              0x01
#define MODE_TX                 0x03
#define PA_BOOST                0x80

// LoRa SPI functions
void sx1278_write(uint8_t reg, uint8_t val) {
  digitalWrite(SX1278_CS, LOW);
  SPI.transfer(reg | 0x80);
  SPI.transfer(val);
  digitalWrite(SX1278_CS, HIGH);
}

uint8_t sx1278_read(uint8_t reg) {
  digitalWrite(SX1278_CS, LOW);
  SPI.transfer(reg & 0x7F);
  uint8_t val = SPI.transfer(0x00);
  digitalWrite(SX1278_CS, HIGH);
  return val;
}

void sx1278_send_packet(const char* msg) {
  uint8_t len = strlen(msg);
  sx1278_write(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
  sx1278_write(REG_FIFO_ADDR_PTR, 0x00);

  for (uint8_t i = 0; i < len; i++) {
    sx1278_write(REG_FIFO, msg[i]);
  }

  sx1278_write(REG_PAYLOAD_LENGTH, len);
  sx1278_write(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_TX);

  while ((sx1278_read(REG_IRQ_FLAGS) & 0x08) == 0) {
    delay(1);
  }

  sx1278_write(REG_IRQ_FLAGS, 0xFF);
  Serial.println("LoRa packet sent");
}

void sx1278_init() {
  pinMode(SX1278_CS, OUTPUT);
  digitalWrite(SX1278_CS, HIGH);

  SPI.begin(SX1278_SCK, SX1278_MISO, SX1278_MOSI, SX1278_CS);
  SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));

  if (sx1278_read(REG_VERSION) != 0x12) {
    Serial.println("ERROR: SX1278 not found");
    while (1);
  }

  sx1278_write(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_SLEEP);
  delay(10);
  sx1278_write(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);

  long frf = (long)(434.0 * 1000000.0 / 61.03515625);
  sx1278_write(REG_FRF_MSB, (frf >> 16) & 0xFF);
  sx1278_write(REG_FRF_MID, (frf >> 8) & 0xFF);
  sx1278_write(REG_FRF_LSB, frf & 0xFF);

  sx1278_write(REG_PA_CONFIG, PA_BOOST | 0x0F);
  sx1278_write(REG_MODEM_CONFIG1, 0x72);
  sx1278_write(REG_MODEM_CONFIG2, 0x74);
  sx1278_write(REG_MODEM_CONFIG3, 0x04);
  sx1278_write(REG_FIFO_TX_BASE_ADDR, 0x00);
  sx1278_write(REG_FIFO_ADDR_PTR, 0x00);

  sx1278_write(REG_OP_MODE, MODE_LONG_RANGE_MODE | MODE_STDBY);
  delay(10);
}

// OPT3001 Functions
void writeOPTRegister(uint8_t reg, uint16_t value) {
  Wire1.beginTransmission(OPT3001_ADDRESS);
  Wire1.write(reg);
  Wire1.write((value >> 8) & 0xFF);
  Wire1.write(value & 0xFF);
  Wire1.endTransmission();
}

uint16_t readOPTRegister(uint8_t reg) {
  Wire1.beginTransmission(OPT3001_ADDRESS);
  Wire1.write(reg);
  Wire1.endTransmission(false);
  Wire1.requestFrom(OPT3001_ADDRESS, (uint8_t)2);
  while (Wire1.available() < 2);
  uint16_t msb = Wire1.read();
  uint16_t lsb = Wire1.read();
  return (msb << 8) | lsb;
}

float readOPT3001() {
  uint16_t result = readOPTRegister(RESULT_REGISTER);
  uint16_t exponent = (result >> 12) & 0x0F;
  uint16_t mantissa = result & 0x0FFF;
  float lux = 0.01 * (1 << exponent) * mantissa;

  Serial.print("Ambient Light: ");
  Serial.print(lux);
  Serial.println(" lux");

  Blynk.virtualWrite(V2, lux);

  char msg[32];
  snprintf(msg, sizeof(msg), "Light: %.2f lux", lux);
  sx1278_send_packet(msg);

  return lux;
}

// HDC1080 Read Function
void readHDC1080() {
  Wire.beginTransmission(HDC1080_ADDR);
  Wire.write(0x02);
  Wire.write(0x10);
  Wire.write(0x00);
  Wire.endTransmission();
  delay(15);

  Wire.beginTransmission(HDC1080_ADDR);
  Wire.write(0x00);
  Wire.endTransmission();
  delay(20);

  Wire.requestFrom(HDC1080_ADDR, 4);
  if (Wire.available() < 4) return;

  uint16_t rawTemp = (Wire.read() << 8) | Wire.read();
  uint16_t rawHum = (Wire.read() << 8) | Wire.read();

  float tempC = (rawTemp / 65536.0) * 165.0 - 40.0;
  float humidity = (rawHum / 65536.0) * 100.0;

  Serial.print("Temperature (C): ");
  Serial.println(tempC);
  Serial.print("Humidity (%): ");
  Serial.println(humidity);

  Blynk.virtualWrite(V0, tempC);
  Blynk.virtualWrite(V1, humidity);
}

// SETUP
void setup() {
  Serial.begin(115200);
  delay(1000);

  Wire.begin(26, 25);    // HDC1080 I2C
  Wire1.begin(21, 22);   // OPT3001 I2C

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  writeOPTRegister(CONFIG_REGISTER, 0xC410);  // Configure OPT3001

  sx1278_init();
  Serial.println("System initialized");
}

// LOOP
void loop() {
  Blynk.run();
  readHDC1080();
  readOPT3001();
  delay(2000);
}
