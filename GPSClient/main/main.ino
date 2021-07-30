#include <SPI.h> // Serial Peripheral Interface // TX RX communication
#include <Wire.h> // communicate with I2C / TWI devices
#include <Adafruit_GFX.h> // OLED Display
#include <Adafruit_SSD1306.h> // OLED Display
#include <TinyGPS++.h> // GPS main library
#include <ESP8266WiFi.h> // WIFI connection
#include <SoftwareSerial.h> // Serial
#include <WiFiUdp.h> // Sending data

#define SCREEN_WIDTH 128 // PX
#define SCREEN_HEIGHT 64 // PX

// Declaration for a SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library.
#define OLED_RESET  LED_BUILTIN // Reseting pin
#define SCREEN_ADDRESS 0x3C // Screen Type
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); // INIT

static const int RXGPSPIN = 13, TXGPSPIN = 12;
static const uint32_t GPSBaud = 9600;

TinyGPSPlus gps;
SoftwareSerial ss(TXGPSPIN, RXGPSPIN);
WiFiUDP Udp;

const char *ssid = "40130-MS";
const char *password = "zMX6kq?+b!4-i4r=3";

WiFiServer server(80);

String date_str, time_str, lat_str, lng_str, empty_str;

float latitude, longitude;

int year, month, date, hour, minute, second;

struct Package {
  String lat = "";
  String lng = "";
  String time = "2:0:0";
  String date = "0.0.2000";
};

struct Package prevPackage;

void startingDevice() {
  Serial.println("Starting device");
  display.display();
  delay(2000);
  display.clearDisplay();
};

void connectingToWifi() {
  Serial.println("----------------");
  Serial.println("Device trying to connect to wifi..");
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 5);
  display.println("Connecting");
  display.print("   to the WiFi ");
  display.display();

  WiFi.begin(ssid, password);
  short count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    if(count >= 3) {
      display.clearDisplay();
      display.setCursor(0, 5);
      display.println("Connecting");
      display.print("   to the WiFi ");
      display.display();
      count = 0;
      delay(500);
    }
    display.print(".");
    display.display();
    delay(500);
    count++;
  }

  Serial.println("WiFi connected");
  display.clearDisplay();
  display.setCursor(0, 5);
  display.println("WiFi connected");
  display.display();
  Serial.println("----------------");
};

void startWebServer() {
  Serial.println("----------------");
  Serial.print("Server started! on port: ");
  Serial.println(WiFi.localIP());
  server.begin();
  display.println("Server started!");
  display.display();
  display.println(WiFi.localIP());
  display.display();
  delay(2000);
  Serial.println("----------------");
}

void loadingGPSDataLabel() {
  Serial.println("----------------");
  display.clearDisplay();
  Serial.println("Device trying to load GPS data.");
  display.setCursor(0, 15);
  display.print("Server: ");
  display.println(WiFi.localIP());
  display.println("\nConnecting to a satellite..");
  display.display();
  Serial.println("----------------");
}

void setup() {
  Serial.begin(115200);
  ss.begin(9600);
  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Killing the program
  } else {
    Serial.println("SSD1306 allocation success");
  }

  startingDevice();
  connectingToWifi();
  startWebServer();
  loadingGPSDataLabel();
}

void loop() {
  while (ss.available() > 0) {
    if (gps.encode(ss.read())) {
      Serial.println(String(gps.location.lat()) + "\n");
      if (gps.location.isValid()) {
        latitude = gps.location.lat();
        lat_str = String(latitude, 5);
        longitude = gps.location.lng();
        lng_str = String(longitude, 5);
        
      } else if(lat_str != empty_str) {
        lat_str += "*";
      } else if(lng_str != empty_str) {
        lng_str += "*";
      }
      
      if (gps.date.isValid()) {
        date = gps.date.day();
        month = gps.date.month();
        year = gps.date.year();
      }

      if (gps.time.isValid()) {
        hour = gps.time.hour() + 2; // UTC +2
        minute = gps.time.minute();
        second = gps.time.second();
      }
      
      String message = createMessageForDisplay(lat_str, lng_str);
      date_str = getDateString(date, month, year);
      time_str = getTimeString();
      message += "\n" + date_str;
      message += "\n" + time_str;
      sendGPSDataToDisplay(message);
      sendGPSDataToWebserver();
      printGPSDataInSerial(gps);

      if(lat_str != prevPackage.lat || lng_str != prevPackage.lng || date_str != prevPackage.date || time_str != prevPackage.time ) {
        sendUDP();
        prevPackage.lat = lat_str;
        prevPackage.lng = lng_str;
        prevPackage.time = time_str;
        prevPackage.date = date_str;
      }
      delay(2000);
    }
  }
  delay(100);
}

void printGPSDataInSerial(TinyGPSPlus gps) {
  Serial.print("Latitude: ");
  Serial.println(String(gps.location.lat(), 6));
  Serial.print("Longitude: ");
  Serial.println(String(gps.location.lng(), 6));
  Serial.println("----------------");
}

void sendGPSDataToDisplay(String message) {
  display.clearDisplay();
  display.setCursor(0, 10);
  display.println(message);
  display.display();
}

String getDateString(int date, int month, int year) {
  String result = "Date: ";
  
  if (date < 10) result += '0';
  result += String(date);
  result += "/";
  
  if (month < 10) result += '0';
  result += String(month);
  result += "/";
  
  if (year < 10) result += '0';
  result += String(year);

  Serial.println("----------------");
  Serial.println(result);
  Serial.println("----------------");
  
  return result;
}

String getTimeString() {
  String result = "Time: ";

  if(hour < 10) result += "0" + String(hour);
  else result += String(hour);
  result += ":";

  if(minute < 10) result += "0" + String(minute);
  else result += String(minute);
  result += ":";

  if(second < 10) result += "0" + String(second);
  else result += String(second);

  return result;
}

String createMessageForDisplay(String lat, String lng) {
  String message = "Server: ";
  message += IpAddress2String(WiFi.localIP());
  message += "\nYour location:\n";
  message += "Lat: ";
  message += lat_str;
  message += "\nLong: ";
  message += lng_str;

  return message;
}

String IpAddress2String(const IPAddress& ipAddress)
{
  return String(ipAddress[0]) + String(".") +
    String(ipAddress[1]) + String(".") +
    String(ipAddress[2]) + String(".") +
    String(ipAddress[3]);
}

void sendGPSDataToWebserver() {
  /* Check if a client has connected */
  WiFiClient client = server.available();
  if (!client) { 
    return;
  }
  
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
  s += "<!DOCTYPE html><html><head><title>Client(GPS)</title>";
  s += "<style>body{font-size:30px;background-color:#eee;}</style>";
  s += "</head><body><h1 align='center'>GPS Client</h1>";
  s += "<table align='center'><tr><th align='right'>Latitude</th><td align='left'>";
  s += lat_str;
  s += "</td></tr><tr><th align='right'>Longitude: </th><td align='left' >";
  s += lng_str;
  s += "</td></tr><tr><th align='right'>Date: </th><td align='left'>";
  s += String(date) + String("/") + String(month)  + String("/") + String(year);
  s += "</td></tr><tr><th align='right'>Time: </th><td align='left'>";
  s += String(hour) + String(":") + String(minute) + String(":") + String(second);
  s += "</td></tr></table>";
  s += "<script type='text/javascript'>window.setTimeout(function(){window.location.reload();},10000);</script>";
  s += "</body></html>";

  client.print(s);
}

void sendUDP() {
  String udp_package = "";
  //udp_package += "Format: LAT;LNG;DATE(date.month.year);TIME(hour:minute:second)\n";
  //udp_package += IpAddress2String(WiFi.localIP());
  //udp_package += ";";
  udp_package += lat_str + ";";
  udp_package += lng_str + ";";
  udp_package += String(date) + String(".") + String(month)  + String(".") + String(year)   + String(";");
  udp_package += String(hour) + String(":") + String(minute) + String(":") + String(second) + String(";");
  
  Serial.println("--------------------------------------------UPD Packet--------------------------------------------");
  Serial.println(udp_package);
  Serial.println("--------------------------------------------------------------------------------------------------\n\n");

  Udp.beginPacket(IPAddress(192, 168, 0, 104), 1722);
  Udp.write(udp_package.c_str());
  Udp.endPacket();
}
