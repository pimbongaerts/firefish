#include "sam.h"                           // CMSIS header for SAMD51
#include "Wire.h"                          // I2C Communication
#include "MS5837.h"                        // Bar30 depth sensor
#include "RTClib.h"                        // Real-time clock
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ST7789.h"
#include "SparkFun_ADXL345.h"              // SparkFun ADXL345 Library
#include "Adafruit_I2CDevice.h"            // TODO: Should not need to include this
#include "Adafruit_ADS1X15.h"
#include "SdFat.h"
#include "Adafruit_SPIFlash.h"
#include "ff.h"                            // SdFat - formatting
#include "diskio.h"                        // SdFat - formatting
//#include "SparkFun_u-blox_GNSS_Arduino_Library.h"
#include "ping1d.h"                        // Ping sonar / altimeter


#define TFT_DC 21
#define TFT_CS 20
#define TFT_RST 19
#define TFT_MOSI 11
#define TFT_MISO 10
#define TFT_SCLK 12
#define TOP_SDA 8
#define TOP_SCL 9
#define PIN_BUTTON1 27
#define PIN_BUTTON2 28
#define PIN_BUTTON3 29
#define PIN_CAM_FOCUS 22
#define PIN_CAM_SHUTTER 23
#define FOCUS_DELAY 300
#define SHUTTER_DELAY 200
#define INTERVAL 1000

// Constants
// Color definitions (correct)
#define BLACK 0x0000
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define WHITE 0xFFFF
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define DGRAY 0x2945
#define LGRAY 0x528A
#define ORANGE 0xFC02

#define FLUID_DENSITY 1029
#define NOT_RESPONSIVE -999
#define NOT_RUNNING -999
#define BUTTON_PRESS_DELAY 300

#define FILE_NAME      "data.csv"
#define MARKER_FILE_NAME "markerfile.txt"
#define GMT_OFFSET -4
#define DISK_LABEL    "EXT FLASH"

// Global objects
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
MS5837 depth_sensor;
// SFE_UBLOX_GNSS myGNSS;
ADXL345 adxl = ADXL345();
Adafruit_ADS1115 ads1115;
RTC_DS3231 rtc;
Adafruit_FlashTransport_QSPI flashTransport;
Adafruit_SPIFlash flash(&flashTransport);
FatFileSystem fatfs;
Uart Serial2 (&sercom2, 5, 4, SERCOM_RX_PAD_1, UART_TX_PAD_0);
static Ping1D ping1 { Serial1 };
extern "C" char* sbrk(int incr);

// Objects for formatting
FATFS elmchamFatfs;
uint8_t workbuf[4096]; // Working buffer for f_fdisk function.

// Fixed coordinates for the alternative screen
//float fixedPointsLat[] = {-17.7436248, -17.7437710, -17.7436477};
//float fixedPointsLong[] = {177.4306748, 177.4306671, 177.4305437};
//const int numFixedPoints = sizeof(fixedPointsLat) / sizeof(fixedPointsLat[0]);
float fixedPointsLat[1];
float fixedPointsLong[1];
int numFixedPoints = 0;
float minLat, maxLat, minLon, maxLon;
const int timeZoneOffsetHours = 13; // For UTC+13 Tonga
const int syncDelaySecs = 2;       // Syncing delay estimate

// Structs to group related variables
struct TimeInfo {
  int year = NOT_RESPONSIVE;
  int month = NOT_RESPONSIVE;
  int day = NOT_RESPONSIVE;
  int hour = NOT_RESPONSIVE;
  int min = NOT_RESPONSIVE;
  int sec = NOT_RESPONSIVE;
  unsigned long unix = NOT_RESPONSIVE;
};

struct SensorReadings {
  float bar30_depth = NOT_RESPONSIVE;
  float extrapolated_depth = NOT_RESPONSIVE;
  float licor_value = NOT_RESPONSIVE;
  float ping1_altitude = NOT_RESPONSIVE;
  float ping1_altitude_conf = NOT_RESPONSIVE;
  long gnss_lat = NOT_RESPONSIVE;
  long gnss_long = NOT_RESPONSIVE;
  int gnss_siv = NOT_RESPONSIVE;
};

struct VoltageReadings {
  float batv = NOT_RESPONSIVE;
  float usbv = NOT_RESPONSIVE;
};

struct DeviceStatus {
  bool bar30_active = true;
  bool ping1_active = true;
  bool gnss_active = false;
  bool licor_active = false;
  long flash_size = NOT_RESPONSIVE;
  float prop_used = NOT_RESPONSIVE;
};

struct Intervalometer {
  int number = NOT_RUNNING;
  int marker = NOT_RESPONSIVE;
  bool triggerloop = false;
  bool first_loop_of_sec = false;
};

unsigned long lastSecond = 0;
#define INTERVAL 1000  // 1000 milliseconds = 1 second

// Instances of structs
TimeInfo rtcTime;
TimeInfo gnssTime;
SensorReadings sensorReadings;
VoltageReadings voltageReadings;
DeviceStatus deviceStatus;
Intervalometer intervalometer;

String serialinput = "";
bool force_logging = false;
bool displayAltScreen = false; // Added variable to track alternative screen display

static const SPIFlash_Device_t my_flash_devices[] = {
    MX25L12872F,
};
const int flashDevices = 1;

void SERCOM2_0_Handler()
{
  Serial2.IrqHandler();
}
void SERCOM2_1_Handler()
{
  Serial2.IrqHandler();
}
void SERCOM2_2_Handler()
{
  Serial2.IrqHandler();
}
void SERCOM2_3_Handler()
{
  Serial2.IrqHandler();
}

int freeMemory() {
  char top;
  return (&top - reinterpret_cast<char*>(sbrk(0))) / 1000;
}

// Initialize TFT screen
void init_tft() {
  tft.init(240, 320);
  tft.setRotation(1);
  tft.setTextWrap(false);
  tft.invertDisplay(0);
}

void init_pins() {
  // Initialize pins
  pinMode(PIN_BUTTON1, INPUT);
  pinMode(PIN_BUTTON2, INPUT);
  pinMode(PIN_BUTTON3, INPUT);
  pinMode(PIN_CAM_FOCUS, OUTPUT);
  pinMode(PIN_CAM_SHUTTER, OUTPUT);
  Serial.println("Initialized pins");
}

void init_flash() {
  if (!flash.begin(my_flash_devices, flashDevices)) {
    Serial.println("Error, failed to mount newly formatted filesystem!");
    Serial.println("Was the flash chip formatted with the fatfs_format example?");
    while(1) delay(1);
  }
  Serial.print("Flash chip JEDEC ID: 0x"); Serial.println(flash.getJEDECID(), HEX);
  if (!fatfs.begin(&flash)) {
    Serial.println("Error, failed to mount filesystem!");
    while(1) delay(1);
  }
  Serial.println("Mounted filesystem.");
  deviceStatus.flash_size = flash.size() / 2048; // size in KB - changed from original 1024
}

void format_flash(){
  // From SdFAT_format from Tony DiCola
  Serial.println("Creating and formatting FAT filesystem (this takes ~60 seconds)...");

  // Make filesystem.
  FRESULT r = f_mkfs("", FM_FAT | FM_SFD, 0, workbuf, sizeof(workbuf));
  if (r != FR_OK) {
    Serial.print("Error, f_mkfs failed with error code: "); Serial.println(r, DEC);
    while(1) yield();
  }

  // mount to set disk label
  r = f_mount(&elmchamFatfs, "0:", 1);
  if (r != FR_OK) {
    Serial.print("Error, f_mount failed with error code: "); Serial.println(r, DEC);
    while(1) yield();
  }

  // Setting label
  Serial.println("Setting disk label to: " DISK_LABEL);
  r = f_setlabel(DISK_LABEL);
  if (r != FR_OK) {
    Serial.print("Error, f_setlabel failed with error code: "); Serial.println(r, DEC);
    while(1) yield();
  }

  // unmount
  f_unmount("0:");

  // sync to make sure all data is written to flash
  flash.syncBlocks();
  
  Serial.println("Formatted flash!");

  // Check new filesystem
  if (!fatfs.begin(&flash)) {
    Serial.println("Error, failed to mount newly formatted filesystem!");
    while(1) delay(1);
  }
  Serial.println("Flash chip successfully formatted with new empty filesystem!");
  NVIC_SystemReset();
}

// Initialize Real-Time Clock
void init_rtc() {
  if (! rtc.begin()) {
    rtcTime.hour = NOT_RESPONSIVE;
    Serial.println("RTC not responsive");
  }
  if (rtc.lostPower()) {
    Serial.println("RTC lost power");
  }
  // get_rtc_time();
}

// Sync to GNSS time
// void sync_rtc_with_gnss() {
//   uint32_t localUnixTimeWithDelay = myGNSS.getUnixEpoch() + (timeZoneOffsetHours * 3600) + syncDelaySecs;
//   rtc.adjust(DateTime(localUnixTimeWithDelay));
//   Serial.println("Syncing RTC time with GNSS time + offset + delay...");
// }

void get_rtc_time() {
  DateTime now = rtc.now();
  rtcTime.year = now.year();
  rtcTime.month = now.month();
  rtcTime.day = now.day();
  rtcTime.hour = now.hour();
  rtcTime.min = now.minute();
  rtcTime.sec = now.second();
  rtcTime.unix = now.unixtime();
}

// void get_gnss_time() {
//   gnssTime.unix = myGNSS.getUnixEpoch();
// }

// Initialize sonar
void init_ping1() {
  Serial1.begin(115200);
  int attempts = 0;
  while (!ping1.initialize()) { 
    if (attempts > 3) { 
      deviceStatus.ping1_active = false;
      break;
    }
    Serial.println("Could not connect to Ping sonar...");
    attempts++;
    delay(300);
  }
  get_ping1_altitude();
}

// Initialize sonar
void reinit_ping1() {
  ping1.set_ping_enable(false);
  delay(500);
  ping1.set_ping_enable(true);
  get_ping1_altitude();
}

// Obtain altitude measurement
void get_ping1_altitude() {
    if (ping1.update()) {
        sensorReadings.ping1_altitude = (float)ping1.distance() / 1000;
        sensorReadings.ping1_altitude_conf = ping1.confidence();
    } else {
        sensorReadings.ping1_altitude = NOT_RESPONSIVE;
        sensorReadings.ping1_altitude_conf = 0;
    }
}

void tft_home() {
  tft.fillScreen(BLACK);
  tft.setTextColor(CYAN, BLACK);
  tft.setTextSize(2);
  tft.setCursor(0,5);
  tft.println("ALTITUDE DEPTH    BOTTOM ");
  tft.setCursor(0,65);
  tft.println("MARKER  PHOTO     LICOR");
  tft.setCursor(0,125);
  tft.println("DATE          TIME   ");
  tft.setCursor(0,185);
  tft.println("BAT       GPS      FLASH ");
}

void tft_update() {
  if (displayAltScreen) {
    tft_update_alt_screen();
  } else {
    tft_update_main_screen();
  }
}

void tft_update_main_screen() {
  tft.setTextColor(WHITE, BLACK);
  tft.setTextSize(3);
  tft.setCursor(0,30);
  if (sensorReadings.ping1_altitude_conf > 70 ) {
    tft.setTextColor(WHITE, BLACK);
  } else {
    tft.setTextColor(LGRAY, BLACK);
  }
  tft_print_6char(sensorReadings.ping1_altitude);
  tft.setTextColor(WHITE, BLACK);
  if (sensorReadings.bar30_depth > 0.5) {
    tft.setTextColor(WHITE, BLACK);
  } else {
    tft.setTextColor(LGRAY, BLACK);
  }
  tft_print_6char(sensorReadings.bar30_depth);
  if (sensorReadings.ping1_altitude_conf > 70 ) {
    tft.setTextColor(WHITE, BLACK);
  } else {
    tft.setTextColor(LGRAY, BLACK);
  }
  tft_print_6char(sensorReadings.extrapolated_depth);
  tft.setTextColor(WHITE, BLACK);
  tft.setCursor(0,90);
  if (intervalometer.number != NOT_RUNNING){
    tft.setTextColor(WHITE, BLACK);
    tft_print_4char(intervalometer.marker);
    tft.print(" ");
    tft_print_4char(intervalometer.number);
    tft.setTextColor(BLUE, BLACK);
    tft.print(" R ");
  } else {
    tft.setTextColor(LGRAY, BLACK);
    tft_print_4char(intervalometer.marker);
    tft.print(" ");
    tft.print("0000");
    tft.print("   ");
  }

  tft.setTextColor(WHITE, BLACK);
  if (deviceStatus.licor_active) {  
    tft_print_4char(abs((int)sensorReadings.licor_value));
  } else {
    tft.print("      ");
  }
  
  tft.print("  ");
  tft.setTextColor(WHITE, BLACK);
  tft.setCursor(0,150);
  print_rtc_date(rtcTime.day, rtcTime.month, rtcTime.year);
  tft.print(" ");
  print_rtc_time(rtcTime.hour, rtcTime.min, rtcTime.sec);
  tft.setCursor(0,210);
  tft.print(voltageReadings.batv, 1);
  tft.print("V");
  if (voltageReadings.usbv > 4.5) {
    tft.print("*");
  } else {
    tft.print(" ");
  }
  tft.print("  ");
  if (sensorReadings.gnss_siv > 0){
    tft.setTextColor(WHITE, BLACK);
  } else {
    tft.setTextColor(LGRAY, BLACK);
  }
  tft_print_2char(sensorReadings.gnss_siv);
  //tft.print(freeMemory());
  //tft.print("kB");
  tft.print("    ");
  tft.print(deviceStatus.prop_used);
  tft.print("%");
}

// Function to compute bearing between two points (unchanged)
float computeBearing(float lat1, float lon1, float lat2, float lon2) {
  // Convert degrees to radians
  float lat1Rad = lat1 * PI / 180.0;
  float lon1Rad = lon1 * PI / 180.0;
  float lat2Rad = lat2 * PI / 180.0;
  float lon2Rad = lon2 * PI / 180.0;

  float dLon = lon2Rad - lon1Rad;
  float y = sin(dLon) * cos(lat2Rad);
  float x = cos(lat1Rad) * sin(lat2Rad) - sin(lat1Rad) * cos(lat2Rad) * cos(dLon);
  float bearingRad = atan2(y, x);
  float bearingDeg = bearingRad * 180.0 / PI;
  // Normalize to 0..360 degrees
  bearingDeg = fmod(bearingDeg + 360.0, 360.0);
  return bearingDeg;
}

// Updated function to update the alternative screen
void tft_update_alt_screen() {
  // Compute min and max latitudes and longitudes of fixed points
  float minFixedLat = fixedPointsLat[0];
  float maxFixedLat = fixedPointsLat[0];
  float minFixedLon = fixedPointsLong[0];
  float maxFixedLon = fixedPointsLong[0];
  for (int i = 1; i < numFixedPoints; i++) {
    if (fixedPointsLat[i] < minFixedLat) minFixedLat = fixedPointsLat[i];
    if (fixedPointsLat[i] > maxFixedLat) maxFixedLat = fixedPointsLat[i];
    if (fixedPointsLong[i] < minFixedLon) minFixedLon = fixedPointsLong[i];
    if (fixedPointsLong[i] > maxFixedLon) maxFixedLon = fixedPointsLong[i];
  }
  // Compute center
  float centerLat = (minFixedLat + maxFixedLat) / 2.0;
  float centerLon = (minFixedLon + maxFixedLon) / 2.0;
  // Compute deltaLat and deltaLon ensuring at least 100x100m area
  float deltaLatFixed = maxFixedLat - minFixedLat;
  float deltaLatMin = 100.0 / 111320.0; // 100 meters in degrees latitude
  float deltaLat = max(deltaLatFixed, deltaLatMin);
  float cosLat = cos(centerLat * PI / 180.0);
  float deltaLonFixed = maxFixedLon - minFixedLon;
  float deltaLonMin = 100.0 / (cosLat * 111320.0); // 100 meters in degrees longitude
  float deltaLon = max(deltaLonFixed, deltaLonMin);
  // Set minLat, maxLat, minLon, maxLon
  minLat = centerLat - deltaLat / 2.0;
  maxLat = centerLat + deltaLat / 2.0;
  minLon = centerLon - deltaLon / 2.0;
  maxLon = centerLon + deltaLon / 2.0;

  // Clear screen and set up text
  //tft.fillScreen(BLACK);
  tft.setTextSize(2);
  tft.setTextColor(CYAN, BLACK);
  tft.setCursor(0, 0);
  tft.println("GNSS Position Map");
  tft.setCursor(0, 220);
  tft.setTextColor(WHITE, BLACK);
  tft.print(String(sensorReadings.gnss_lat / 10000000.0, 6));
  tft.print(" ");
  tft.print(String(sensorReadings.gnss_long / 10000000.0, 6));
  tft.print(" [");
  tft.print(String(sensorReadings.gnss_siv));
  tft.print("]");

  // Drawing fixed points
  for (int i = 0; i < numFixedPoints; i++) {
    int x = map_longitude_to_x(fixedPointsLong[i]);
    int y = map_latitude_to_y(fixedPointsLat[i]);
    tft.fillCircle(x, y, 2, ORANGE);
  }

  // Drawing current GNSS position or direction indicator
  float sensorLat = sensorReadings.gnss_lat / 10000000.0;
  float sensorLon = sensorReadings.gnss_long / 10000000.0;

  if (sensorReadings.gnss_siv > 0) {
    if (sensorLat >= minLat && sensorLat <= maxLat && sensorLon >= minLon && sensorLon <= maxLon) {
      // Position is within map
      int x = map_longitude_to_x(sensorLon);
      int y = map_latitude_to_y(sensorLat);
      tft.fillCircle(x, y, 5, WHITE);
    } else {
      // Position is outside map
      bool north = sensorLat > maxLat;
      bool south = sensorLat < minLat;
      bool east = sensorLon > maxLon;
      bool west = sensorLon < minLon;

      // Draw direction indicator
      if (north && !east && !west) {
        // North
        tft.fillTriangle(160, 0, 150, 10, 170, 10, RED);
      } else if (south && !east && !west) {
        // South
        tft.fillTriangle(160, 240, 150, 230, 170, 230, RED);
      } else if (east && !north && !south) {
        // East
        tft.fillTriangle(320, 120, 310, 110, 310, 130, RED);
      } else if (west && !north && !south) {
        // West
        tft.fillTriangle(0, 120, 10, 110, 10, 130, RED);
      } else if (north && east) {
        // Northeast
        tft.fillTriangle(310, 10, 320, 0, 320, 10, RED);
      } else if (north && west) {
        // Northwest
        tft.fillTriangle(0, 0, 10, 10, 0, 10, RED);
      } else if (south && east) {
        // Southeast
        tft.fillTriangle(310, 230, 320, 240, 320, 230, RED);
      } else if (south && west) {
        // Southwest
        tft.fillTriangle(0, 240, 10, 230, 0, 230, RED);
      }
    }
  } else {
    tft.setCursor(0, 30);
    tft.println("No GNSS fix available.");
  }

  // Drawing the scale bar (25m) at the top-right corner
  float mapWidthInMetres = (maxLon - minLon) * cosLat * 111320.0;
  float pixelsPerMetreX = 320.0 / mapWidthInMetres;
  float scaleBarLengthInMetres = 25.0;
  float scaleBarLengthInPixels = scaleBarLengthInMetres * pixelsPerMetreX;
  int scaleBarLength = (int)scaleBarLengthInPixels;

  int scaleBarX = 320 - scaleBarLength - 10; // 10 pixels from the right edge
  int scaleBarY = 20; // 20 pixels from the top edge

  // Draw the scale bar
  tft.drawLine(scaleBarX, scaleBarY, scaleBarX + scaleBarLength, scaleBarY, WHITE);
  tft.setCursor(scaleBarX, scaleBarY - 15);
  tft.setTextSize(1);
  tft.setTextColor(WHITE, BLACK);
  tft.print("25m");

  // Drawing N E S W indicators
  tft.setTextSize(1);
  tft.setTextColor(CYAN, BLACK);

  // North
  tft.setCursor(155, 20);
  tft.print("N");

  // East
  tft.setCursor(310, 115);
  tft.print("E");

  // South
  tft.setCursor(155, 205);
  tft.print("S");

  // West
  tft.setCursor(5, 115);
  tft.print("W");
}

// Functions to map latitude and longitude to screen coordinates
int map_latitude_to_y(float lat) {
  return map(lat * 10000000, minLat * 10000000, maxLat * 10000000, 240, 0);
}

int map_longitude_to_x(float lon) {
  return map(lon * 10000000, minLon * 10000000, maxLon * 10000000, 0, 320);
}

// Print depth/altitude to LCD
void tft_print_6char(float number) {
  if (number == NOT_RESPONSIVE) {
    tft.print(F("-N/A- "));
  } else if (number >= 1000) {
    tft.print(F("100+m "));
  } else if (number > 100) {
    tft.print(number, 0);
    tft.print(F("m "));
  } else if (number > 10) {
    tft.print(number, 1);
    tft.print(F("m "));
  } else if (number > 0) {
    tft.print(F("0"));
    tft.print(number, 1);
    tft.print(F("m "));
  } else if (number < 0) {
    tft.print(F("-NEG- "));
  } else {
    tft.print(F("ERROR "));
  }
}

// Print marker number to TFT
void tft_print_4char(float number) {
  if (number == NOT_RESPONSIVE) {
    tft.print(F("N/A!"));
  } else if (number > 1000) {
    tft.print(number, 0);
  } else if (number > 100) {
    tft.print(F("0"));
    tft.print(number, 0);
  } else if (number > 10) {
    tft.print(F("00"));
    tft.print(number, 0);
  } else if (number > 0) {
    tft.print(F("000"));
    tft.print(number, 0);
  } else if (number < 0) {
    tft.print(F("NEG!"));
  } else {
    tft.print(F("NA  "));
  }
}

// Print marker number to TFT
void tft_print_2char(float number) {
  if (number == NOT_RESPONSIVE) {
    tft.print(F("N/A!"));
  } else if (number < 10) {
    tft.print(F("0"));
    tft.print(number, 0);
  } else {
    tft.print(number, 0);
  }
}

void print_rtc_date(int day, int month, int year) {
  if (day == NOT_RESPONSIVE) {
    tft.print(F("-N/A-"));
  } else {
    if (day < 10) { tft.print(F("0")); }
    tft.print(day);
    tft.print(F("/"));
    if (month < 10) { tft.print(F("0")); }
    tft.print(month);
    tft.print(F("/"));
    if ((year % 100) == 0){ 
      tft.print(F("00"));
    } else {
      tft.print(year % 100);
    }
  }
}

void print_rtc_time(int number1, int number2, int number3) {
  if (number1 == NOT_RESPONSIVE) {
    tft.print(F("-N/A-"));
  } else {
    if (number1 < 10) { tft.print(F("0")); }
    tft.print(number1);
    tft.print(F(":"));
    if (number2 < 10) { tft.print(F("0")); }
    tft.print(number2);
    tft.print(F(":"));
    if (number3 < 10) { tft.print(F("0")); }
    tft.print(number3);
  }
}

void init_depth_sensor() {
  Wire.begin();
  int attempts = 0;
  while (!depth_sensor.init(Wire)) { 
    if (attempts > 3) { 
      deviceStatus.bar30_active = false;
      break;
    }
    attempts++;
    delay(300);
  }
  if (deviceStatus.bar30_active){
    depth_sensor.setModel(MS5837::MS5837_30BA);
    depth_sensor.setFluidDensity(FLUID_DENSITY);
  } else {
    Serial.println("Could not connect to Bar30 depth sensor...");
  }
}

// void init_gnss() {
//   Wire.begin();
//   int attempts = 0;
//   while (attempts <= 3 && !myGNSS.begin()) {
//     attempts++;
//     delay(300);
//   }
//   if (myGNSS.begin()) {
//     deviceStatus.gnss_active = true;
//     myGNSS.setI2COutput(COM_TYPE_UBX | COM_TYPE_NMEA);
//     myGNSS.saveConfigSelective(VAL_CFG_SUBSEC_IOPORT);
//   }
// }

void init_adxl_sensor() {
  adxl.powerOn();                     
  adxl.setRangeSetting(16);           
  adxl.setActivityXYZ(1, 0, 0);       
  adxl.setActivityThreshold(25);      
  adxl.setInactivityXYZ(1, 0, 0);     
  adxl.setInactivityThreshold(75);    
  adxl.setTimeInactivity(10);         
  adxl.setTapDetectionOnXYZ(0, 0, 1); 
  adxl.setTapThreshold(25);           
  adxl.setTapDuration(15);            
  adxl.setDoubleTapLatency(80);       
  adxl.setDoubleTapWindow(200);       
  adxl.setFreeFallThreshold(7);       
  adxl.setFreeFallDuration(30);       
  adxl.InactivityINT(1);
  adxl.ActivityINT(1);
  adxl.FreeFallINT(1);
  adxl.doubleTapINT(1);
  adxl.singleTapINT(1);
}

void init_ads_sensor() {
  Wire1.begin();
  Wire1.setClock(400000); // Set I2C clock to 400 kHz
  if (ads1115.begin(0x48, &Wire1)) {
    Serial.println("ADS1115 initialized...");
    ads1115.setGain(GAIN_SIXTEEN); // Set gain as needed
    ads1115.setDataRate(RATE_ADS1115_860SPS); // Set the highest data rate
    deviceStatus.licor_active = true;
    
    // Start continuous differential conversion between AIN0 and AIN1
    ads1115.startADCReading(ADS1X15_REG_CONFIG_MUX_DIFF_0_1, true);
  } else {
    Serial.println("ADS1115 could not be initialized...");
    deviceStatus.licor_active = false;
  }
}

void init_marker_number() {
  File markerfile = fatfs.open(MARKER_FILE_NAME, FILE_READ);
  if (markerfile) {
    while (markerfile.available()) {
      String line = markerfile.readStringUntil('\n');
      intervalometer.marker = line.toInt();
      Serial.print(F("Current marker: "));
      Serial.println(intervalometer.marker);
    }
    markerfile.close();
    if (!fatfs.remove(MARKER_FILE_NAME)) {
      Serial.println("Error, couldn't delete logfile!");
      while(1) yield();
    }
  } else {
    intervalometer.marker = 1;
  }
  File new_markerfile = fatfs.open(MARKER_FILE_NAME, FILE_WRITE);
  new_markerfile.println(intervalometer.marker);
  new_markerfile.close();
}

void next_marker_number() {
  intervalometer.marker++;
  File new_markerfile = fatfs.open(MARKER_FILE_NAME, FILE_WRITE);
  new_markerfile.println(intervalometer.marker);
  new_markerfile.close();
}

// Obtain light measurement
void get_licor_value() {
  if (deviceStatus.licor_active) {
    sensorReadings.licor_value = ads1115.getLastConversionResults();
  } else {
    sensorReadings.licor_value = NOT_RESPONSIVE;
  }
}

// Get voltages from USB and BAT
void get_voltages() {
  int usb_raw = analogRead(A0);
  int bat_raw = analogRead(A1);
  voltageReadings.usbv = usb_raw * (3.3 / 512.0);
  voltageReadings.batv = bat_raw * (3.3 / 512.0);
}

// Obtain depth measurement
void get_bar30_depth() {
  depth_sensor.read();
  sensorReadings.bar30_depth = depth_sensor.depth(); 
}

// Obtain GNSS coords and number of satellites
// void get_gnss_coords() {
//   sensorReadings.gnss_lat = myGNSS.getLatitude();
//   sensorReadings.gnss_long = myGNSS.getLongitude();
//   sensorReadings.gnss_siv = myGNSS.getSIV();
// }

// Get extrapolated depth
void get_extrapolated_depth() {
  if (sensorReadings.ping1_altitude != NOT_RESPONSIVE) {
    sensorReadings.extrapolated_depth = sensorReadings.bar30_depth + sensorReadings.ping1_altitude;
  } else {
    sensorReadings.extrapolated_depth = NOT_RESPONSIVE;
  }
}

// Toggle intervalometer
void toggle_intervalometer() {
  Serial.println("Button interrupt");
  if (intervalometer.number == NOT_RUNNING) {
    intervalometer.number = 0;
    next_marker_number();
    get_prop_used();
    tft_update();
  } else {
    intervalometer.number = NOT_RUNNING;
    // Ensure camera pins are set LOW when stopping
    digitalWrite(PIN_CAM_FOCUS, LOW);
    digitalWrite(PIN_CAM_SHUTTER, LOW);
    tft_update();
  }
}

void camera_capture() {
  digitalWrite(PIN_CAM_FOCUS, HIGH);
  delay(FOCUS_DELAY);
  digitalWrite(PIN_CAM_SHUTTER, HIGH);
  delay(SHUTTER_DELAY);
}

void erase_flash() {
  Serial.print("Flash chip JEDEC ID: 0x"); Serial.println(flash.getJEDECID(), HEX);
  Serial.println("Deleting logging file...");
  if (!fatfs.remove(FILE_NAME)) {
    Serial.println("Error, couldn't delete logfile!");
    while(1) yield();
  }
  Serial.println("Deleted logfile!");
}

void print_logfile() {
  File logfile = fatfs.open(FILE_NAME, FILE_READ);
  if (logfile) {
    Serial.println("Opened file, printing contents below:");
    while (logfile.available()) {
      char c = logfile.read();
      Serial.print(c);
    }
    logfile.close();
  }
  else {
    Serial.println("Failed to open data file! Does it exist?");
  }
}

void check_for_adxl_taps() {
  byte interrupts = adxl.getInterruptSource();
  if(adxl.triggered(interrupts, ADXL345_DOUBLE_TAP)){
    Serial.println("*** DOUBLE TAP ***");
    toggle_intervalometer();
  } else if(adxl.triggered(interrupts, ADXL345_SINGLE_TAP)){
    Serial.println("*** TAP ***");
    if (intervalometer.number == NOT_RUNNING) { 
      Serial.println(intervalometer.marker);
      // New functionality: Toggle alternative screen if bar30_depth < 1
      // if (sensorReadings.bar30_depth < 1) {
      //   displayAltScreen = !displayAltScreen;
      //   tft.fillScreen(BLACK); // Clear the screen when switching
      //   if (displayAltScreen) {
      //     if (numFixedPoints == 0) {
      //       // Add the current position to fixedPointsLat and fixedPointsLong
      //       fixedPointsLat[0] = sensorReadings.gnss_lat / 10000000.0;
      //       fixedPointsLong[0] = sensorReadings.gnss_long / 10000000.0;
      //       numFixedPoints = 1;
      //     }
      //   } else {
      //     tft_home();
      //   }
      // }
    }
  }
}

// Interrupt through Serial command
void serial_command_interrupt() {
  serialinput = Serial.readStringUntil('\n');
  Serial.print("You typed: " );
  Serial.println(serialinput);
  if (serialinput == "timesync") {
    Serial.println("Enter current datetime as YYYY/MM/DD hh:mm:ss");
    while (!Serial.available()) delay(10);
    String datetime = Serial.readString();
    int Year = datetime.substring(0, 4).toInt();
    int Month = datetime.substring(5, 7).toInt();
    int Day = datetime.substring(8, 10).toInt();
    int Hour = datetime.substring(11, 13).toInt();
    int Minute = datetime.substring(14, 16).toInt();
    int Second = datetime.substring(17, 19).toInt();
    rtc.adjust(DateTime(Year, Month, Day, Hour, Minute, Second));
    get_rtc_time();
  } else if (serialinput == "gnsstime"){
    // sync_rtc_with_gnss();
    // get_rtc_time();
  } else if (serialinput == "download") {
    print_logfile();
  } else if (serialinput == "format") {
    Serial.println("!!! Are you sure you want to format the flash drive? (y to confirm) !!!");
    while(Serial.available() == 0){ };
    serialinput = Serial.readStringUntil('\n');
    if (serialinput == "y") {
      format_flash();
    }
  } else if (serialinput == "reset_marker") {
    intervalometer.marker = 0;
    next_marker_number();
  } else if (serialinput == "force_logging") {
    force_logging = true;
  } else if (serialinput == "stop_logging") {
    force_logging = false;
  }
}

void printTwoDigits(Print &printer, int value) {
  if (value < 10) {
    printer.print('0');
  }
  printer.print(value, DEC);
}

// Log sensor readings to SPI flash
void log_sensor_readings() {
  File logfile = fatfs.open(FILE_NAME, FILE_WRITE);
  if (logfile) {
    logfile.print(rtcTime.year, DEC);           
    logfile.print(F("/"));
    printTwoDigits(logfile, rtcTime.month);
    logfile.print(F("/"));
    printTwoDigits(logfile, rtcTime.day);
    logfile.print(F(","));
    printTwoDigits(logfile, rtcTime.hour);          
    logfile.print(F(":"));
    printTwoDigits(logfile, rtcTime.min);
    logfile.print(F(":"));
    printTwoDigits(logfile, rtcTime.sec);
    logfile.print(F(","));
    logfile.print(rtcTime.unix);            
    logfile.print(F(","));
    logfile.print(sensorReadings.bar30_depth);            
    logfile.print(F(","));
    logfile.print(sensorReadings.ping1_altitude);     
    logfile.print(F(","));
    logfile.print(sensorReadings.ping1_altitude_conf);
    logfile.print(F(","));
    logfile.print(sensorReadings.extrapolated_depth);     
    logfile.print(F(","));
    logfile.print(sensorReadings.licor_value);    
    logfile.print(F(","));
    logfile.print(intervalometer.marker);   
    logfile.print(F(","));
    if (intervalometer.number != NOT_RUNNING || force_logging) {
      logfile.print(intervalometer.number);
    } else {
      logfile.print("NA");
    }
    logfile.print(F(","));
    // if (deviceStatus.gnss_active && sensorReadings.bar30_depth < 1){
    //   logfile.print(String(sensorReadings.gnss_lat / 10000000.0, 8));
    //   logfile.print(F(","));
    //   logfile.print(String(sensorReadings.gnss_long / 10000000.0, 8));  
    //   logfile.print(F(","));
    //   logfile.print(sensorReadings.gnss_siv);    
    //   logfile.print(F(","));
    //   logfile.print(gnssTime.unix);
    //   logfile.print(F(","));
    // } else {
    logfile.print(F("NA,NA,NA,NA,"));
    //}
    logfile.print(millis());    
    logfile.print(F(","));
    logfile.print(voltageReadings.usbv);    
    logfile.print(F(","));
    logfile.print(voltageReadings.batv);
    logfile.println();
    logfile.close();
  }
  else {
    Serial.println("Failed to open data file for writing!");
  }
}

// Get free memory on flash
void get_prop_used() {
  File logfile = fatfs.open(FILE_NAME, FILE_WRITE);
  long file_size = logfile.size() / 1000;
  deviceStatus.prop_used = ((float)file_size /  (float)deviceStatus.flash_size) * 100.0;
}

void setup() {
  Serial.begin(115200);
  Serial.println("Firefish initialization...");
  init_tft();
  Serial.println("TFT home...");
  tft_home();
  Serial.println("Init pins...");
  init_pins();
  //Serial.println("Init gnss...");
  //init_gnss();
  Serial.println("Init rtc...");
  init_rtc();
  Serial.println("Init depth sensor...");
  init_depth_sensor();
  Serial.println("Init ping...");
  init_ping1();
  Serial.println("Init adxl...");
  init_adxl_sensor();
  Serial.println("Init flash...");
  init_flash();
  get_prop_used();
  //get_gnss_time();
  init_marker_number();
  init_ads_sensor(); 
}

void loop() {
  // Turn off shutter and focus
  digitalWrite(PIN_CAM_SHUTTER, LOW);
  digitalWrite(PIN_CAM_FOCUS, LOW);
  // Wait until start of new second
  get_rtc_time();
  while (rtcTime.sec == lastSecond)
  {
    get_rtc_time();
    check_for_adxl_taps();
  };
  // At the start of a new second
  lastSecond = rtcTime.sec;
  if (intervalometer.number != NOT_RUNNING) {
    camera_capture();
    intervalometer.number++;
  }
  // Get sensor readings
    get_bar30_depth();
    if (deviceStatus.ping1_active) {
      get_ping1_altitude();
      get_extrapolated_depth();
    }
  get_voltages();
  get_licor_value();
  // if (deviceStatus.gnss_active && sensorReadings.bar30_depth < 1){
  //   get_gnss_time();
  //   get_gnss_coords();
  // }
  // Only log when intervalometer is running
  if (intervalometer.number != NOT_RUNNING || force_logging) { // used to be voltageReadings.usbv < 4.5 
    log_sensor_readings();
  }
  // Check for interrupt
  if (Serial.available()) {
    serial_command_interrupt();
  }
  tft_update();
}

extern "C" {
  
  DSTATUS disk_status (BYTE pdrv) {
    (void) pdrv;
    return 0;
  }

  DSTATUS disk_initialize (BYTE pdrv) {
    (void) pdrv;
    return 0;
  }

  DRESULT disk_read (BYTE pdrv, BYTE *buff, DWORD sector, UINT count) {
    (void) pdrv;
    return flash.readBlocks(sector, buff, count) ? RES_OK : RES_ERROR;
  }

  DRESULT disk_write (BYTE pdrv, const BYTE *buff, DWORD sector, UINT count) {
    (void) pdrv;
    return flash.writeBlocks(sector, buff, count) ? RES_OK : RES_ERROR;
  }

  DRESULT disk_ioctl (BYTE pdrv, BYTE cmd, void *buff) {
    (void) pdrv;
    switch ( cmd )
    {
      case CTRL_SYNC:
        flash.syncBlocks();
        return RES_OK;

      case GET_SECTOR_COUNT:
        *((DWORD*) buff) = flash.size() / 512;
        return RES_OK;

      case GET_SECTOR_SIZE:
        *((WORD*) buff) = 512;
        return RES_OK;

      case GET_BLOCK_SIZE:
        *((DWORD*) buff) = 8;  // Erase block size in units of sector size
        return RES_OK;

      default:
        return RES_PARERR;
    }
  }
  
} // extern "C"