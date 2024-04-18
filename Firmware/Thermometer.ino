#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "Adafruit_SHT4x.h"
#include "SdFat.h"
#include "Adafruit_MCP9601.h"
#include "RTClib.h"
#include <WiFi.h>
#include "esp_eap_client.h"  //wpa2 library for connections to Enterprise networks
#include "time.h"
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>

#define DEVICE "OpenSourceVetV1" // Device name
#define MAXNUMTC 4 // Maximum number of thermocouples
#define WIFI_TIMEOUT 1 // Try to connect for 1 minute
#define FILE_NAME "thermo.cfg" // Config file name

#define KEY_MAX_LENGTH 30     // change it if key is longer
#define VALUE_MAX_LENGTH 140  // change it if value is longer

// variables for Config file
int WPA2Ent = 0;
int RT_ENABLE = 1;
int USECERT = 0;
int TC[MAXNUMTC] = { 0 };
float TC_OFFSET[MAXNUMTC] = { 0 };
float RT_OFFSET = 0;
float RH_OFFSET = 0;
int FREQUENCY = 5;
String SSID;
String Ident;
String PW;
String INFLUXDB_URL;
String INFLUXDB_TOKEN;
String INFLUXDB_ORG;
String INFLUXDB_BUCKET;
String TZ_INFO = "UTC";
String ROOM;
String TC_DEVICE[MAXNUMTC] = { "" };
String TC_SERVICE[MAXNUMTC] = { "" };
String CERTFILE;
unsigned long logMillis;
unsigned long timeSyncMillis;
int LOG_TO_SD = 0;
bool bufEmpty = true;

int ident_count[MAXNUMTC] = { 0 };
float prev_temp[MAXNUMTC] = { 0 };

InfluxDBClient client;
// Declare Data point

int TC_ADDR[MAXNUMTC] = { 0x67, 0x66, 0x65, 0x64 };
#define SD_FAT_TYPE 3
#define SD_CS_PIN SS
#define SCREEN_WIDTH 128   // OLED display width, in pixels
#define SCREEN_HEIGHT 128  // OLED display height, in pixels
#define OLED_RESET -1      // can set an oled reset pin if desired

Adafruit_SH1107 display = Adafruit_SH1107(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET, 1000000, 100000);

SdFat SD;
File logFile;
File errFile;
Adafruit_SHT4x sht4 = Adafruit_SHT4x();
Adafruit_MCP9601 mcp[4];
RTC_PCF8523 rtc;
int RT = 1;

void setup() {

  SSID.reserve(140);
  Ident.reserve(140);
  PW.reserve(140);
  INFLUXDB_URL.reserve(140);
  INFLUXDB_TOKEN.reserve(140);
  INFLUXDB_ORG.reserve(140);
  INFLUXDB_BUCKET.reserve(140);
  TZ_INFO.reserve(140);
  ROOM.reserve(140);
  CERTFILE.reserve(140);

  bool goodWiFi = 0;

  logMillis = 0;
  timeSyncMillis = 0;

  for (int i = 0; i < MAXNUMTC; i++) {
    TC[i] = 0;
  }

  Serial.begin(9600);
  delay(500);

  display.begin(0x3D, true);  // Address 0x3D default
  display.cp437(true);
  display.setTextColor(SH110X_WHITE);
  display.clearDisplay();
  display.setTextSize(2);  // Draw 2X-scale text
  display.println("Starting Temp!");
  display.setTextSize(1);  // Draw 2X-scale text
  display.println(WiFi.macAddress());
  Serial.println(WiFi.macAddress());
  display.setTextSize(2);
  display.display();

  if (!SD.begin(SD_CS_PIN)) {
    display.setTextSize(2);  // Draw 2X-scale text
    display.setTextColor(SH110X_WHITE);
    display.println(F("SD Card"));
    display.println(F("Fail - "));
    display.println(F("Chk Card!"));
    display.display();
    while (1) delay(10);
  }

  display.setTextSize(1);
  display.println("Initialized SD card");
  display.setTextSize(2);
  display.display();

  getConfigValues();

  display.setTextSize(1);
  display.println("Read config values");
  display.display();
  display.setTextSize(2);

  Serial.println(WPA2Ent);
  Serial.println(SSID);
  Serial.println(Ident);
  Serial.println(PW);
  Serial.println(INFLUXDB_URL);
  Serial.println(INFLUXDB_TOKEN);
  Serial.println(INFLUXDB_ORG);
  Serial.println(INFLUXDB_BUCKET);
  Serial.println(TZ_INFO);
  Serial.println(ROOM);
  Serial.println(FREQUENCY);
  Serial.println(USECERT);
  Serial.println(CERTFILE);

  for (int i = 0; i < MAXNUMTC; i++) {
    char ichar[10];
    itoa((i + 1), ichar, 10);
    if (TC[i]) {
      if (!mcp[i].begin(TC_ADDR[i])) {
        Serial.print("Failed to intialize thermocouple #");
        Serial.println(i);
        TC[i] = 0;
      } else {
        mcp[i].setADCresolution(MCP9600_ADCRESOLUTION_16);
        mcp[i].setThermocoupleType(MCP9600_TYPE_K);
        mcp[i].setFilterCoefficient(3);

        char buf[200];
        strcpy(buf, "TC");
        strcat(buf, ichar);
        strcat(buf, " Device");
        //free(buf);
        strcpy(buf, "TC");
        strcat(buf, ichar);
        strcat(buf, " Service");
        //      free(buf);
        //      free(ichar);
      }
    }
  }

  display.setTextSize(1);
  display.println("Init ");
  display.println("thermocpls");
  display.setTextSize(2);
  display.display();

  if (!RT_ENABLE) {
    RT = 0;
  }
  else if (!sht4.begin()) {
    RT = 0;
  } else {
    sht4.setPrecision(SHT4X_HIGH_PRECISION);
    sht4.setHeater(SHT4X_NO_HEATER);
  }

  if (!RT) {
    display.setTextSize(1);
    display.println("Skipped room temp");
    display.setTextSize(2);
    display.display();
  }
  else {
    display.setTextSize(1);
    display.println("Initialized room temp");
    display.setTextSize(2);
    display.display();
  }
    display.setTextSize(1);
    display.print("Init WiFi (");
    display.print(WIFI_TIMEOUT);
      display.println("min max)");
    display.setTextSize(2);
    display.display();

  goodWiFi = connectWiFi();

  if (goodWiFi) {
    display.setTextSize(1);
    display.println("Initialized WiFi");
    display.setTextSize(2);
    display.display();
  }
  else {
    display.setTextSize(1);
    display.println("WiFi init fail");
    display.setTextSize(2);
    display.display();
  }
  if (!rtc.begin()) {
    display.setTextSize(2);  // Draw 2X-scale text
    display.setTextColor(SH110X_WHITE);
    display.println(F("RTC Fail"));
    display.display();
    while (1) delay(10);
  }

  display.setTextSize(1);
  display.println("Initialized RTC");
  display.setTextSize(2);
  display.display();

  if(goodWiFi) {
    initTime(TZ_INFO);
  }
  DateTime now = rtc.now();

  char filename[50];
  snprintf(filename, 50, "%d-%d-%d_%d-%d-%d.csv", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());

  delay(100);

  errFile = SD.open("Error.txt", FILE_WRITE);

  logFile = SD.open(filename, FILE_WRITE);
  if (!logFile) {
    display.setTextSize(2);  // Draw 2X-scale text
    display.setTextColor(SH110X_WHITE);
    display.println(F("Logfile Fail"));
    display.display();
    while (1) delay(10);
  }
  logFile.print("Date, Time, Room Temp, Rel Hum");
  for (int i = 0; i < MAXNUMTC; i++) {
    char ichar[10];
    itoa((i + 1), ichar, 10);
    if (TC[i]) {
      char buf[200];
      strcpy(buf, ", TC");
      strcat(buf, ichar);
      strcat(buf, ", TC");
      strcat(buf, ichar);
      strcat(buf, " status");
      logFile.print(buf);
      //      free(buf);
    }
    //    free(ichar);
  }
  logFile.println();

  display.setTextSize(1);
  display.println("Initialized log file");
  display.setTextSize(2);
  display.display();

  Serial.println("End of Setup!");
}

void displayTime() {
  DateTime now = rtc.now();

  char timeFormat[] = "hh:mm:ss";
  char *time = now.toString(timeFormat);
  char dateBuf[20];
  sprintf(dateBuf, "%02d/%02d/%02d", now.month(), now.day(), now.year());

  display.setTextSize(2);
  //  display.println(F(dateBuf));
  display.setTextSize(2);
  display.print(" ");
  display.println(time);
  display.setTextSize(1);
  display.println("--------------------");
  display.setTextSize(2);
}

void displayTemp() {

  float TCTemp;
  for (int i = 0; i < MAXNUMTC; i++) {
    char ichar[10];
    itoa((i + 1), ichar, 10);
    if (TC[i]) {
      char buf[200];
      strcpy(buf, ichar);
      strcat(buf, ":");

      strcat(buf, getTCTemp(i).c_str());
      display.print(buf);
      display.write(0xF8);
      display.println(F("C"));
    } else {
      display.println();
    }
  }

  display.setTextSize(1);
  display.println("--------------------");
  display.setTextSize(2);
}

void loop() {

  Serial.println("Loop function!");
  unsigned long logMillis = 0;
  unsigned long timeSyncMillis = 0;
  while (1) {

    if (millis() - timeSyncMillis >= (60 * 60 * 1000UL)) {
      timeSyncMillis = millis();  //get ready for the next iteration
      Serial.println("Going to sync...");
      initTime(TZ_INFO);
    }

    if ((millis() - logMillis) >= (FREQUENCY * 60 * 1000UL)) {
      logMillis = millis();  //get ready for the next iteration
      Serial.println("Going to log...");

      bufEmpty = logValues();
      WiFi.disconnect(true);
    }

    display.clearDisplay();
    display.setCursor(0, 0);

    displayTime();
    displayTemp();

    if (RT) {
      display.setTextSize(1);
      display.print(F("RT:"));
      display.printf(getRT().c_str());
      display.write(0xF8);
      display.print(F("C | "));
      display.print(F("RH:"));
      display.printf(getRH().c_str());
      display.println(F("%"));

      if (bufEmpty) {
        display.println(" Buffer Empty  ");

      } else {
        display.println(" Data Buffered ");
      }

      display.println(WiFi.macAddress());

      display.setTextSize(2);
    }
    display.display();
    delay(150);
  }
}

String getTCTemp(int i) {
  if (TC[i]) {
    char ichar[10];
    itoa((i + 1), ichar, 10);
/*    uint8_t status = mcp[i].getStatus();
          if (status & MCP9601_STATUS_OPENCIRCUIT) {
            return "Open";
          } else if (status & MCP9601_STATUS_SHORTCIRCUIT) {
            return "Short";
          } else
            if (TC[i]) {
*/
    char buf[200];
    float TCTemp;
    TCTemp = mcp[i].readThermocouple();
    Serial.print("TC");
    Serial.print(i);
    Serial.print(" Status:");
    Serial.println(mcp[i].getStatus());
    Serial.print("Cur Temp:");
    Serial.println(TCTemp);
    Serial.print("Prev Temp:");
    Serial.println(prev_temp[i]);
    if (TCTemp == prev_temp[i]) {
      ident_count[i]++;
      if(ident_count[i] > 500) {
//        ESP.restart();
      Serial.println("500 identical temp readings");
      }
    }
    else {
      prev_temp[i] = TCTemp;
      ident_count[i] = 0;
    }
    
    TCTemp = TCTemp - TC_OFFSET[i];
            char TCbuf[15];
            dtostrf(TCTemp, 0, 2, TCbuf);
    return String(TCTemp, 1);
//          }
  }
}

String getRT(void) {
  char tempBuf[15];
  sensors_event_t humidity, temp;
  sht4.getEvent(&humidity, &temp);  // populate temp and humidity objects with fresh data
                                    // dtostrf((temp.temperature-RT_OFFSET), 0, 2, tempBuf);
  return String((temp.temperature - RT_OFFSET), 1);
}

String getRH(void) {
  char humBuf[15];
  sensors_event_t humidity, temp;
  sht4.getEvent(&humidity, &temp);  // populate temp and humidity objects with fresh data
                                    //  dtostrf((humidity.relative_humidity-RH_OFFSET), 0, 2, humBuf);
  return String((humidity.relative_humidity - RH_OFFSET), 1);
}

bool logValues( void ) {

  Serial.println("logValues function");

  if (WiFi.status() != WL_CONNECTED) {
    bool goodWiFi = connectWiFi();
  }

  DateTime now = rtc.now();

  char timeFormat[] = "hh:mm:ss";
  char *time = now.toString(timeFormat);
  char dateBuf[20];
  sprintf(dateBuf, "%02d/%02d/%02d", now.month(), now.day(), now.year());

  InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());

    errFile.print(dateBuf);
    errFile.print(" - ");
    errFile.print(time);
    errFile.print(":");
    errFile.println("Connected to InfluxDB");

  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
    errFile.print(dateBuf);
    errFile.print(" - ");
    errFile.print(time);
    errFile.print(":");
    errFile.println("InfluxDB connect failed!");
    errFile.println(client.getLastErrorMessage());
  }

  Point sensor("Lab_Monitoring");
    Serial.print("Point 1");

  client.setWriteOptions(WriteOptions().batchSize(1));
  client.setWriteOptions(WriteOptions().bufferSize(100));
    Serial.print("Point 2");

  sensor.clearTags();
    Serial.print("Point 3");

  sensor.addTag("device", DEVICE);
  sensor.addTag("SSID", WiFi.SSID());
  sensor.addTag("MAC Address", WiFi.macAddress());
  sensor.addTag("Room", ROOM);

      Serial.print("Point 4");

  sensor.addField(ROOM + " Room Temp", getRT().toFloat());
  sensor.addField(ROOM + " Relative Humidity", getRH().toFloat());
  sensor.addField(ROOM + " WiFi Signal Strength", WiFi.RSSI());
      Serial.print("Point 5");

//  if (!client.writePoint(sensor)) {
//    Serial.print("InfluxDB write 1 failed: ");
//    Serial.println(client.getLastErrorMessage());
//  }

//  sensor.clearFields();

  String TCLogTemps[MAXNUMTC];
  String TCStatus[MAXNUMTC];
    Serial.print("Point 6");

  for (int i = 0; i < MAXNUMTC; i++) {

    if (TC[i]) {
    Serial.print("Point 7");
    Serial.print(i);

      sensor.addTag(TC_DEVICE[i] + " Service", TC_SERVICE[i]);
    Serial.print("Point 7a");
      sensor.addField(TC_DEVICE[i] + " Temp", getTCTemp(i).toFloat());
    Serial.print("Point 7b");
      TCLogTemps[i] = getTCTemp(i).toFloat();
    Serial.print("Point 7c");
      TCStatus[i] = mcp[i].getStatus();
    Serial.print("Point 7d");
    }
  }
    Serial.print("Point 8");
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write 2 failed: ");
    Serial.println(client.getLastErrorMessage());

    errFile.print(dateBuf);
    errFile.print(" - ");
    errFile.print(time);
    errFile.print(":");
    errFile.println("InfluxDB write failed!");
    errFile.println(client.getLastErrorMessage());

  }
  sensor.clearFields();

  char logLine[300];
  char logLine2[200];
  char logLine3[200];
  sprintf(logLine, "%s,%s,%.1f,%.1f,", dateBuf, time, getRT().toFloat(), getRH().toFloat());
  Serial.println(logLine);
  sprintf(logLine2, "%s,", TCLogTemps);
  Serial.println(logLine2);
  sprintf(logLine3, "%s,", TCStatus);
  Serial.println(logLine3);
//  logLine2[strlen(logLine2) - 1] = '\0';
  logLine3[strlen(logLine3) - 1] = '\0';
  strcat(logLine, logLine2);
  strcat(logLine, logLine3);
  Serial.println("logLine strcat");
  logFile.println(logLine);
  logFile.flush();

  return client.isBufferEmpty();
}


/*
 * This Arduino Nano code was developed by newbiely.com
 *
 * This Arduino Nano code is made available for public use without any restriction
 *
 * For comprehensive instructions and wiring diagrams, please visit:
 * https://newbiely.com/tutorials/arduino-nano/arduino-nano-read-config-from-sd-card
 */

bool SD_available(const __FlashStringHelper *key) {
  char value_string[VALUE_MAX_LENGTH];
  int value_length = SD_findKey(key, value_string);
  return value_length > 0;
}

int SD_findInt(const __FlashStringHelper *key) {
  char value_string[VALUE_MAX_LENGTH];
  int value_length = SD_findKey(key, value_string);
  return HELPER_ascii2Int(value_string, value_length);
}

float SD_findFloat(const __FlashStringHelper *key) {
  char value_string[VALUE_MAX_LENGTH];
  int value_length = SD_findKey(key, value_string);
  return HELPER_ascii2Float(value_string, value_length);
}

String SD_findString(const __FlashStringHelper *key) {
  char value_string[VALUE_MAX_LENGTH];
  int value_length = SD_findKey(key, value_string);
  return HELPER_ascii2String(value_string, value_length);
}


int SD_findKey(const __FlashStringHelper *key, char *value) {
  File configFile = SD.open(FILE_NAME);

  if (!configFile) {
    Serial.print(F("SD Card: error on opening file "));
    Serial.println(FILE_NAME);
    return (0);
  }

  char key_string[KEY_MAX_LENGTH];
  char SD_buffer[KEY_MAX_LENGTH + VALUE_MAX_LENGTH + 1];  // 1 is = character
  int key_length = 0;
  int value_length = 0;

  // Flash string to string
  PGM_P keyPoiter;
  keyPoiter = reinterpret_cast<PGM_P>(key);
  byte ch;
  do {
    ch = pgm_read_byte(keyPoiter++);
    if (ch != 0)
      key_string[key_length++] = ch;
  } while (ch != 0);

  // check line by line
  while (configFile.available()) {
    int buffer_length = configFile.readBytesUntil('\n', SD_buffer, (KEY_MAX_LENGTH + VALUE_MAX_LENGTH + 1));
    if (SD_buffer[buffer_length - 1] == '\r')
      buffer_length--;  // trim the \r

    if (buffer_length > (key_length + 1)) {                  // 1 is = character
      if (memcmp(SD_buffer, key_string, key_length) == 0) {  // equal
        if (SD_buffer[key_length] == '=') {
          value_length = buffer_length - key_length - 1;
          memcpy(value, SD_buffer + key_length + 1, value_length);
          break;
        }
      }
    }
  }

  configFile.close();  // close the file
  return value_length;
}

int HELPER_ascii2Int(char *ascii, int length) {
  int sign = 1;
  int number = 0;

  for (int i = 0; i < length; i++) {
    char c = *(ascii + i);
    if (i == 0 && c == '-')
      sign = -1;
    else {
      if (c >= '0' && c <= '9')
        number = number * 10 + (c - '0');
    }
  }

  return number * sign;
}

float HELPER_ascii2Float(char *ascii, int length) {
  int sign = 1;
  int decimalPlace = 0;
  float number = 0;
  float decimal = 0;

  for (int i = 0; i < length; i++) {
    char c = *(ascii + i);
    if (i == 0 && c == '-')
      sign = -1;
    else {
      if (c == '.')
        decimalPlace = 1;
      else if (c >= '0' && c <= '9') {
        if (!decimalPlace)
          number = number * 10 + (c - '0');
        else {
          decimal += ((float)(c - '0') / pow(10.0, decimalPlace));
          decimalPlace++;
        }
      }
    }
  }

  return (number + decimal) * sign;
}

String HELPER_ascii2String(char *ascii, int length) {
  String str;
  str.reserve(length);
  str = "";

  for (int i = 0; i < length; i++) {
    char c = *(ascii + i);
    str += String(c);
  }

  return str;
}


String get_wifi_status(int status) {
  switch (status) {
    case WL_IDLE_STATUS:
      return "WL_IDLE_STATUS";
    case WL_SCAN_COMPLETED:
      return "WL_SCAN_COMPLETED";
    case WL_NO_SSID_AVAIL:
      return "WL_NO_SSID_AVAIL";
    case WL_CONNECT_FAILED:
      return "WL_CONNECT_FAILED";
    case WL_CONNECTION_LOST:
      return "WL_CONNECTION_LOST";
    case WL_CONNECTED:
      return "WL_CONNECTED";
    case WL_DISCONNECTED:
      return "WL_DISCONNECTED";
  }
}

void setTimezone(String timezone) {
  Serial.printf("  Setting Timezone to %s\n", timezone.c_str());
  setenv("TZ", timezone.c_str(), 1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();
}

void initTime(String timezone) {
  struct tm timeinfo;

  if (WiFi.status() != WL_CONNECTED) {
    bool goodWiFi = connectWiFi();
  }

  Serial.println("Setting up time");
  configTime(0, 0, "pool.ntp.org");  // First connect to NTP server, with 0 TZ offset
  if (!getLocalTime(&timeinfo)) {
    Serial.println("  Failed to obtain time");
    return;
  }
  Serial.println("  Got the time from NTP");
  // Now we can set the real timezone
  setTimezone(TZ_INFO);
  time_t now;
  time(&now);
  localtime_r(&now, &timeinfo);

  Serial.println(&timeinfo);

  int yr;
  int mt;
  int dy;
  int hr;
  int mi;
  int se;

  yr = timeinfo.tm_year + 1900;
  mt = timeinfo.tm_mon + 1;
  dy = timeinfo.tm_mday;
  hr = timeinfo.tm_hour;
  mi = timeinfo.tm_min;
  se = timeinfo.tm_sec;

  rtc.adjust(DateTime(yr, mt, dy, hr, mi, se));
}

void getConfigValues(void) {

  // WPA2 Enterprise? 1 - yes, 0 - no
  WPA2Ent = SD_findInt(F("WPA2Ent"));

  // SSID to connect to
  SSID = SD_findString(F("SSID"));

  // Identity (for WPA2 Enterprise)
  Ident = SD_findString(F("Ident"));

  // PW - either for identity (WPA2 Ent) or for access point (WPA2)
  PW = SD_findString(F("PW"));

  // InfluxDB URL
  INFLUXDB_URL = SD_findString(F("INFLUXDB_URL"));

  // InfluxDB Access Token
  INFLUXDB_TOKEN = SD_findString(F("INFLUXDB_TOKEN"));

  // InfluxDB Org
  INFLUXDB_ORG = SD_findString(F("INFLUXDB_ORG"));

  // InfluxDB Bucket
  INFLUXDB_BUCKET = SD_findString(F("INFLUXDB_BUCKET"));

  // POSIX Timezone string (see https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv)
  TZ_INFO = SD_findString(F("TZ_INFO"));

  // Room the sensor is in
  ROOM = SD_findString(F("ROOM"));

  // Frequency to log values in minutes
  FREQUENCY = SD_findInt(F("FREQUENCY"));

  // Write out to SD card as well as InfluxDB? 1 = yes, 0 = no
  LOG_TO_SD = SD_findInt(F("LOG_TO_SD"));

  // Use certificate for WPA2 Enterprise?
  USECERT = SD_findInt(F("USECERT"));

  // Filename of certificate to use for WPA2 Enterprise
  CERTFILE = SD_findString(F("CERTFILE"));

  // Enable room temp?
  RT_ENABLE = SD_findInt(F("RT_ENABLE"));

  // Enable thermocouples 1-4? 1 = yes, 0 = no
  TC[0] = SD_findInt(F("TC1_ENABLE"));
  TC[1] = SD_findInt(F("TC2_ENABLE"));
  TC[2] = SD_findInt(F("TC3_ENABLE"));
  TC[3] = SD_findInt(F("TC4_ENABLE"));

  for (int i = 0; i < MAXNUMTC; i++) {

    char buf[200];
    char ichar[10];
    itoa((i + 1), ichar, 10);
    strcpy(buf, "TC");
    strcat(buf, ichar);
    strcat(buf, "_DEVICE");

    // Device being monitored by thermocouple

    TC_DEVICE[i] = SD_findString(F(buf));
    //free(buf);
    strcpy(buf, "TC");
    strcat(buf, ichar);
    strcat(buf, "_SERVICE");

    // Service owning device being monitored
    TC_SERVICE[i] = SD_findString(F(buf));
    //    free(buf);
    //    free(ichar);

    strcpy(buf, "TC");
    strcat(buf, ichar);
    strcat(buf, "_OFFSET");

    // Offset from temperature - used for compensation for thermocouple variance

    TC_OFFSET[i] = SD_findFloat(F(buf));
  }

  // Offset for room temperature - used to compensate for calibration
  RT_OFFSET = SD_findFloat(F("RT_OFFSET"));

  // Offset for humidity - used to compensate for calibration
  RH_OFFSET = SD_findFloat(F("RH_OFFSET"));
}

bool connectWiFi(void) {

  Serial.println(WiFi.macAddress());

  if (WPA2Ent && SSID && Ident && PW) {
    Serial.println("Using WPA2 Enterprise");
    char *certBuf;
    File certFile;
    if (USECERT) {
      Serial.println("Using certificate for connection!");
      certFile = SD.open(CERTFILE);
      unsigned int fileSize = certFile.size();
      certBuf = (char *)malloc(fileSize + 1);
      certFile.read(certBuf, fileSize);
      certBuf[fileSize] = '\0';
      certFile.close();
      WiFi.disconnect(true);
      WiFi.mode(WIFI_STA);  //init wifi mode
      WiFi.begin(SSID, WPA2_AUTH_PEAP, Ident, Ident, PW, certBuf);
      // WPA2 enterprise magic ends here
      //      free(certBuf);

    } else {
      WiFi.begin(SSID, WPA2_AUTH_PEAP, Ident, Ident, PW);
    }
  }

  if (!WPA2Ent && SSID && PW) {
    WiFi.begin(SSID, PW);
  }

  unsigned long wifiTimer = millis();
  while (WiFi.status() != WL_CONNECTED) {
    
    delay(500);
    Serial.print(".");
    Serial.println(get_wifi_status(WiFi.status()));

    if (get_wifi_status(WiFi.status()) == "WL_NO_SSID_AVAIL") {
      return false;
    }

    if ((millis() - wifiTimer) > (1000UL * 60 * WIFI_TIMEOUT)) {
      Serial.println("Did not connect Wifi!");
      return false;
    }
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  return true;
}
