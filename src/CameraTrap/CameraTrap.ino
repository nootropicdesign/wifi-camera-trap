
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include "memorysaver.h"
#include <Adafruit_NeoPixel.h>
#if !(defined ESP8266 )
#error Please select the ArduCAM ESP8266 UNO board in the Tools/Board
#endif

void blink(uint32_t color, int ms);
boolean useLED = false;

#define BOOT 0
#define SENSOR 3 // RX pin
#define LED_PIN 15
Adafruit_NeoPixel led = Adafruit_NeoPixel(1, LED_PIN, NEO_RGB + NEO_KHZ800);

const char* moduleName = "CameraTrap";
byte lastSensorReading = LOW;

//This demo can only work on OV2640_MINI_2MP or ARDUCAM_SHIELD_V2 platform.
#if !(defined (OV2640_MINI_2MP)||(defined (ARDUCAM_SHIELD_V2) && defined (OV2640_CAM)))
#error Please select the hardware platform and camera module in the ../libraries/ArduCAM/memorysaver.h file
#endif
// set GPIO16 as the slave select :
const int CS = 16;

const char* host = "yourimageserver"; // provide the hostname of the machine running the image server.
const int port = 8000;
String url = "/upload";

long captureTime= 0;
boolean shouldCapture = false;
WiFiClient client;

IPAddress defaultIP = IPAddress(10,0,0,200);


ArduCAM camera(OV2640, CS);

void capture(){
  if (useLED) {
    led.setPixelColor(0, led.Color(255, 255, 255));
    led.show();
  }

  long total_time = millis();
  camera.clear_fifo_flag();
  camera.start_capture();
  Serial.println("capturing...");

  while (!camera.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK));
  led.clear();
  led.show();
  total_time = millis() - total_time;
  Serial.print("image capture = ");
  Serial.print(total_time, DEC);
  Serial.println(" ms");

  total_time = millis();
  
  Serial.print("connecting to ");
  Serial.print(host);
  Serial.print("...");
  if (!client.connect(host, port)) {
    Serial.println("connection failed");
    return;
  } else {
    Serial.println("connected.");
  }

  size_t len = camera.read_fifo_length();
  if (len >= 0x07ffff){
    Serial.print("image too large: ");
    Serial.println(len);
    return;
  }

  Serial.print("image len = ");
  Serial.println(len);
  if (len == 0) {
    return;
  }
  camera.CS_LOW();
  camera.set_fifo_burst();
  #if !(defined (ARDUCAM_SHIELD_V2) && defined (OV2640_CAM))
  SPI.transfer(0xFF);
  #endif

  if (useLED) {
    led.setPixelColor(0, led.Color(100, 0, 0));
    led.show();
  }

  client.print("POST ");
  client.print(url);
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.println(host);
  client.print("Content-Length: "); client.println(len);
  client.println("Content-Type: image/jpeg");
  client.println("Content-Transfer-Encoding: binary");
  client.println();

  static const size_t bufferSize = 1024;
  static uint8_t buffer[bufferSize] = {0xFF};
  while (len) {
    size_t n = (len < bufferSize) ? len : bufferSize;
    SPI.transferBytes(&buffer[0], &buffer[0], n);
    if (!client.connected()) {
      Serial.println("connection lost!");
      break;
    }
    client.write(&buffer[0], n);
    len -= n;
    Serial.print(".");
  }

  camera.CS_HIGH();
  Serial.println("done");

  client.stop();

  total_time = millis() - total_time;
  Serial.print("upload time = ");
  Serial.print(total_time, DEC);
  Serial.println(" ms");

  led.clear();
  led.show();
}

void connectWiFi() {
  WiFiManager wifiManager;
  wifiManager.setAPStaticIPConfig(defaultIP, defaultIP, IPAddress(255,255,255,0));
  wifiManager.autoConnect(moduleName, NULL);
}

void setup() {
  uint8_t vid, pid;
  uint8_t temp;
  led.begin();
  led.show(); // Initialize all pixels to 'off'

#if defined(__SAM3X8E__)
  Wire1.begin();
#else
  Wire.begin();
#endif
  Serial.begin(115200);
  Serial.println("ArduCAM Start!");

  // set the CS as an output:
  pinMode(BOOT, INPUT_PULLUP); // digital input on BOOT button
  pinMode(SENSOR, INPUT);
  pinMode(2, OUTPUT); // ESP8266 LED
  digitalWrite(2, HIGH);
  pinMode(CS, OUTPUT);

  // initialize SPI:
  SPI.begin();
  SPI.setFrequency(4000000); //4MHz

  //Check if the ArduCAM SPI bus is OK
  camera.write_reg(ARDUCHIP_TEST1, 0x55);
  temp = camera.read_reg(ARDUCHIP_TEST1);
  if (temp != 0x55){
    Serial.println("SPI1 interface Error!");
    while(1);
  }

  //Check if the camera module type is OV2640
  camera.wrSensorReg8_8(0xff, 0x01);
  camera.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
  camera.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
  if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 ))) {
    Serial.println("Can't find OV2640 module!");
  } else {
    Serial.println("OV2640 detected.");
  }
 

  //Change to JPEG capture mode and initialize the OV2640 module
  camera.set_format(JPEG);
  camera.InitCAM();
  // possible resolutions:
  // OV2640_320x240
  // OV2640_352x288
  // OV2640_640x480
  // OV2640_800x600
  // OV2640_1024x768
  // OV2640_1280x1024
  // OV2640_1600x1200
  camera.OV2640_set_JPEG_size(OV2640_1024x768);

  camera.clear_fifo_flag();

  Serial.println("connect to wifi...");
  connectWiFi();
  Serial.println("WiFi connected");
  Serial.println("");
  Serial.println(WiFi.localIP());
  blink(led.Color(0, 255, 0), 50);
}

void loop() {

  if (((digitalRead(BOOT) == LOW) && ((millis() - captureTime) > 1000)) || (shouldCapture)) {
    shouldCapture = false;
    captureTime = millis();
    capture();
    lastSensorReading = HIGH;
    if (useLED) {
      led.setPixelColor(0, led.Color(100, 75, 0));
      led.show();
    }
  }
  if ((lastSensorReading == LOW) && (digitalRead(SENSOR) == HIGH)) {
    // new movement
    lastSensorReading = HIGH;
    capture();
    if (useLED) {
      led.setPixelColor(0, led.Color(100, 75, 0));
      led.show();
    }
  }
  if ((lastSensorReading == HIGH) && (digitalRead(SENSOR) == LOW)) {
    // all quiet now
    if (useLED) {
      led.setPixelColor(0, led.Color(0, 100, 0));
      led.show();
    }
  }
  lastSensorReading = digitalRead(SENSOR);
  delay(20);
}

void blink(uint32_t color, int ms) {
  led.setPixelColor(0, color);
  led.show();
  delay(ms);
  led.clear();
  led.show();
}
