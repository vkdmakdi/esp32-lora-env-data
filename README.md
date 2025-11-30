# esp32-lora-env-data
LoRa-based environmental monitoring system using ESP32, HDC1080, and OPT3001 with cloud dashboard integration

**Code Explanation**

This project uses an ESP32 to read data from two I2C sensors (HDC1080 and OPT3001), send the readings to a cloud dashboard using Blynk, and also transmit the same data through an SX1278 LoRa module using SPI.
Here’s a breakdown of how the code works:

1. Library Imports & Credentials

The required libraries for I2C, SPI, Wi-Fi, Blynk, and the HDC1080 sensor are included.
Your Wi-Fi SSID, password, and Blynk authentication token are defined at the top.

2. Sensor and LoRa Addresses

The I2C addresses for:

HDC1080 (temperature & humidity)

OPT3001 (ambient light sensor)

and the LoRa register addresses are defined for easy reference.

3. LoRa SPI Communication Functions
sx1278_write(reg, val)

Sends a single byte to an SX1278 register over SPI.

sx1278_read(reg)

Reads a byte from an SX1278 register.

sx1278_send_packet(msg)

Writes the message into the SX1278 FIFO buffer

Sets payload length

Triggers LoRa TX mode

Waits until transmission is complete

Clears IRQ flags

This is the core function that actually sends messages through LoRa.

4. LoRa Initialization
sx1278_init()

Responsible for configuring the SX1278 module:

Initializes SPI pins

Verifies SX1278 version register

Sets frequency to 434 MHz

Configures power amplifier

Sets LoRa modulation parameters

Sets FIFO base address

Prepares the module in standby mode

This ensures LoRa is ready before the first transmission.

5. OPT3001 Light Sensor Functions
writeOPTRegister()

Writes a 16-bit value to the sensor’s configuration register.

readOPTRegister()

Reads 16-bit data from any OPT3001 register.

readOPT3001()

Reads raw light data

Splits exponent & mantissa

Converts to lux

Sends the lux value to Blynk (V2)

Sends a formatted message over LoRa

This function handles all ambient light measurement tasks.

6. HDC1080 Temperature & Humidity
readHDC1080()

Initiates a temperature+humidity measurement

Reads 4 raw bytes

Converts to °C and %RH using the sensor’s calibration formula

Uploads data to Blynk (V0 = temp, V1 = humidity)

Provides accurate environmental readings.

7. Setup Function
setup()

Runs once at startup:

Starts Serial for debugging

Initializes I2C buses

Wire → HDC1080

Wire1 → OPT3001

Connects to Wi-Fi

Connects to Blynk Cloud

Configures OPT3001

Initializes LoRa

Prints “System initialized” when successful

8. Main Loop
loop()

Runs continuously:

Keeps Blynk connected using Blynk.run()

Reads temperature & humidity (readHDC1080())

Reads light intensity (readOPT3001())

Sends data to both Cloud and LoRa

Waits 2 seconds before next reading

This creates a continuous cycle of sensing → uploading → transmitting.

9. Data Flow Summary

Sensors → ESP32 (I2C)

ESP32 → Blynk Cloud (Wi-Fi)

ESP32 → SX1278 → LoRa Transmission (SPI)

The same sensor data is available both online and over long-range wireless communication.
