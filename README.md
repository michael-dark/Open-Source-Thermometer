# Open-Source-Thermometer
This project is to develop an open-source hardware and software solution for an internet-connected thermometer. Specifically, this is designed to log data to an SD card as well as send to an InfluxDB database (https://www.influxdata.com). This supports up to four thermocouples, as well as measuring room temperature and relative humidity. 

## Hardware
* Processor - [Adafruit Metro S3](https://www.adafruit.com/product/5500)
* Display - [Adafruit Monochrome 1.12" 128x128 OLED Graphic Display - STEMMA QT / Qwiic](https://www.adafruit.com/product/5297)
* Thermocouple sensor(s) - [Adafruit MCP9601 (MCP96L01) I2C Thermocouple Amplifier - STEMMA QT / Qwiic](https://www.adafruit.com/product/5165)
* Room Temp / Humidity Sensor - [Adafruit Sensirion SHT40 Temperature & Humidity Sensor - STEMMA QT / Qwiic](https://www.adafruit.com/product/4885)
* (optional) Battery - Any 3.7V lithium ion battery with a JST connector - i.e. [this](https://www.adafruit.com/product/2011)
* K-type thermocouple (unless code is changed for other thermocouples) - i.e. (this)[https://www.adafruit.com/product/3245]
* M2.5 socket-head screws and nuts (6 each)
* M3 50mm socket-head screws and M3 nuts (4 each)
* M3 socket-head screws and nuts (variable depending on sensors installed)

**NOTE:** The only soldering potentially needed for this project is to set the address if more than one thermocouple per thermometer is desired. (See [this page](https://learn.adafruit.com/adafruit-mcp9600-i2c-thermocouple-amplifier/pinouts) for more information on setting addresses.) By default, the thermocouple sensor is set to the address 0x67, which is coded as thermocouple 1. 

The code sets the thermocouple numbers based on the address:
|Address|Thermocouple number|
|:-------:|:------------------:|
|0x67|1|
|0x66|2|
|0x65|3|
|0x64|4|

However, this can be changed in the source code. 

## Case
STEP and STL files for the components of the case are included. **NOTE:** STLs may need to be rotated to print correctly. I did not need supports to print these, but that may vary depending on the 3D printer and plastic used.

## Firmware
The UF2 file can be uploaded via the Adafruit Bootloader (see instructions [here](https://learn.adafruit.com/adafruit-feather-m0-express-designed-for-circuit-python-circuitpython/uf2-bootloader-details), albeit for a different board). Press the reset button (next to the USB C connector) twice to enter bootloader mode. A new drive called "METROS3BOOT" should appear. Copy the THERMO.UF2 file to the METROS3BOOT drive, and it should load the firmware and reboot.

Alternatively, you can download the .INO Arduino file, load the libraries indicated in the #include statements, and compile the source and send it to the Metro via the [Arduino IDE](https://www.arduino.cc/en/software).

## Power
The device can be powered from the USB C power or via 6-12V power supply to the barrel jack. It requires a 5.5mm/2.1mm center-positive DC connector.

## Thermocouple
Any K-type thermocouple can be used as is. If other thermocouples are used, the thermocouple type in the code must be changed to reflect the thermocouple installed.

## Config file
The configuration file must be named thermo.cfg and be on the SD card. It is a plain text file. All lines start with the parameter, followed by an equal sign, then the parameter. In the included sample thermo.cfg, delete everything to the right of the = on each line. 

|Parameter|Legal Values|Explanation|
|---------|------------|-----------|
|WPA2Ent|0 or 1| Use WPA2 Enterprise? 0 - off, 1 - on|
|SSID||SSID of your WiFi network|
|Ident||Username for WPA2 Enterprise|
|PW||Password for WiFi|
|USECERT|0 or 1|Use certificate file for WPA2 Enterprise? 0 - no, 1 - yes|
|CERTFILE||Certificate file name|
|INFLUXDB_URL|(url beginning with http/https)|InfluxDB URL (from InfluxDB website)|
|INFLUXDB_TOKEN||InfluxDB Token (from InfluxDB website)|
|INFLUXDB_ORG||InfluxDB Org (from InfluxDB website)|
|INFLUXDB_BUCKET||InfluxDB Bucket (from InfluxDB website)|
|TZ_INFO|POSIX timezone|Current POSIX Timezone (see [here](https://support.cyberdata.net/portal/en/kb/articles/010d63c0cfce3676151e1f2d5442e311) for list|
|TC1_ENABLE|0 or 1|Enable first thermocouple (1 - yes, 0 - no)|
|TC2_ENABLE|0 or 1|Enable second thermocouple (1 - yes, 0 - no)|
|TC3_ENABLE|0 or 1|Enable third thermocouple (1 - yes, 0 - no)|
|TC4_ENABLE|0 or 1|Enable fourth thermocouple (1 - yes, 0 - no)|
|RT_ENABLE|0 or 1|Enable room temp measurement (1 - yes, 0 - no)|
|ROOM||Room device is in|
|TC1_DEVICE||First thermocouple device|
|TC1_SERVICE||First thermocouple service|
|TC2_DEVICE||Second thermocouple device|
|TC2_SERVICE||Second thermocouple service|
|TC3_DEVICE||Third thermocouple device|
|TC3_SERVICE||Third thermocouple service|
|TC4_DEVICE||Fourth thermocouple device|
|TC4_SERVICE||Fourth thermocouple service|
|TC1_OFFSET||First thermocouple offset (in degrees C)|
|TC2_OFFSET||Second thermocouple offset (in degrees C)|
|TC3_OFFSET||Third thermocouple offset (in degrees C)|
|TC4_OFFSET||Fourth thermocouple offset (in degrees C)|
|RT_OFFSET||Room temperature offset (in degrees C)|
|RH_OFFSET||Humidity offset (in degrees C)|
|FREQUENCY|<Integer>|Logging frequency (in minutes)|
|LOG_TO_SD|0 or 1|Save temps to SD card? (1 - yes, 0 - no)|

## InfluxDB Setup
I created a free account on [InfluxDB](https://www.influxdata.com) - however, you could run a local instance as well. See the InfluxDB documentation for setup. I then used [Grafana](https://grafana.com) for visualization.
