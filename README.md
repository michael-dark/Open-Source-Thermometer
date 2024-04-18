# Open-Source-Thermometer
This project is to develop an open-source hardware and software solution for an internet-connected thermometer. Specifically, this is designed to log data to an SD card as well as send to an InfluxDB database (https://www.influxdata.com). This supports up to four thermocouples, as well as measuring room temperature and relative humidity. 

## Hardware
* Processor - [Adafruit Metro S3](https://www.adafruit.com/product/5500)
* Display - [Adafruit Monochrome 1.12" 128x128 OLED Graphic Display - STEMMA QT / Qwiic](https://www.adafruit.com/product/5297)
* Thermocouple sensor(s) - [Adafruit MCP9601 (MCP96L01) I2C Thermocouple Amplifier - STEMMA QT / Qwiic](https://www.adafruit.com/product/5165)
* Room Temp / Humidity Sensor - [Adafruit Sensirion SHT40 Temperature & Humidity Sensor - STEMMA QT / Qwiic](https://www.adafruit.com/product/4885)
* (optional) Battery - Any 3.7V lithium ion battery with a JST connector - i.e. [this](https://www.adafruit.com/product/2011)
* M2.5 socket-head screws and nuts (6 each)
* M3 50mm socket-head screws and M3 nuts (4 each)
* M3 socket-head screws and nuts (variable depending on sensors installed)

## Case
STEP and STL files for the components of the case are included.
