
#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <BlynkSimpleEsp8266.h>
#include <WiFiManager.h>
#include <DNSServer.h>
#include <ESP8266WiFiMulti.h>
#include <ArduinoOTA.h>
#include <BlynkSimpleEsp8266.h>

#define NUM_LEDS_PER_STRIP 44
#define PIN_LED 13
#define BLYNK_PRINT Serial
#define str(s) #s
#define xstr(s) str(s)

ESP8266WiFiMulti wifiMulti; // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'
CRGB leds[NUM_LEDS_PER_STRIP];

//build_flags =
//    -DSSID_NAME="SSID"
//    -DPASSWORD_NAME="password"
//    -DBLYNKCERT_NAME="1234567890"

//Wifi - BLYNK
#ifndef BLYNKCERT_NAME
#define BLYNKCERT_NAME "1234567890" //Default BLYNK Cert if not build flag from PlatformIO doesn't work
#endif

#ifndef SSID_NAME
#define SSID_NAME "WIFI_SSID" //Default SSID if not build flag from PlatformIO doesn't work
#endif

#ifndef PASSWORD_NAME
#define PASSWORD_NAME "WIFI_PASSWORD" //Default WiFi Password if not build flag from PlatformIO doesn't work
#endif

//Gets SSID/PASSWORD from PlatformIO.ini build flags
const char ssid[] = xstr(SSID_NAME);      //  your network SSID (name)
const char pass[] = xstr(PASSWORD_NAME);  // your network password
const char auth[] = xstr(BLYNKCERT_NAME); // your BLYNK Cert

unsigned int localPort = 2390; // local port to listen for UDP packets
WiFiUDP udp;                   // A UDP instance to let us send and receive packets over UDP
const char *geiger = "http://192.168.1.105/j";
//const char* geiger = "http://meteorite.co.nz:8082/j";
String geigerresponse;

int red = 128;
int green = 128;
int blue = 128;

long previousMillis;
long interval = 1000;

int rangehops = 10;
int maxrange = rangehops * 6; //Max CPM count to reach max range (RED) after that LEDs go bright
int percenttemp = 0;          //used by each range as a temp variable
int percentrange = 0;         //used by each range as a temp variable.  Top of range
int percentrangeprev = 0;     //used by each range as a temp variable.  Botton of range
int cpm = 0;                  //Counts per min
int temp = 0;                 //Temperature
int fails = 0;                //Failed times to connect to Geiger counter
int tempmin = 10000;          //Min temp - set high initially
int tempmax = 0;              //Max temp
int cpmmin = 10000;           //Min cpm - set high initially
int cpmmax = 0;               //Max cpm
int restmaxmin = 0;

void checkreset();
void ConnectToAP();
void API_Request();
String JSON_Extract(String);
void LEDrange();
void displayLEDs();

void setup()
{
  //Terminal setup
  Serial.begin(9600);
  Serial.print("temp = ");

  pinMode(0, INPUT);

  //initiate FastLED
  FastLED.addLeds<WS2812B, PIN_LED, GRB>(leds, NUM_LEDS_PER_STRIP);

  //Connect to Wifi
  ConnectToAP();

  WiFiManager wifiManager;
  wifiManager.autoConnect("TARDIS");

  //Blynk setup
  Blynk.begin(auth, ssid, pass);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
    {
      type = "sketch";
    }
    else
    { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR)
    {
      Serial.println("Auth Failed");
    }
    else if (error == OTA_BEGIN_ERROR)
    {
      Serial.println("Begin Failed");
    }
    else if (error == OTA_CONNECT_ERROR)
    {
      Serial.println("Connect Failed");
    }
    else if (error == OTA_RECEIVE_ERROR)
    {
      Serial.println("Receive Failed");
    }
    else if (error == OTA_END_ERROR)
    {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();

  Serial.println("OTA Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");
}

void loop()
{
  ArduinoOTA.handle();
  Blynk.run();
  checkreset();

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis > interval)
  {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    API_Request(); // Get data from Geiger counter

    cpm = JSON_Extract("cpm").toInt();

    if (cpm < cpmmin && cpm > 0)
    {
      cpmmin = cpm;
    }

    if (cpm > cpmmax)
    {
      cpmmax = cpm;
    }

    temp = JSON_Extract("temperature").toInt();

    if (temp < tempmin && temp > 0)
    {
      tempmin = temp;
    }

    if (temp > tempmax)
    {
      tempmax = temp;
    }

    Blynk.virtualWrite(V1, cpm);
    Blynk.virtualWrite(V2, cpmmax);
    Blynk.virtualWrite(V3, temp);
    Blynk.virtualWrite(V4, tempmin);
    Blynk.virtualWrite(V5, tempmax);

    //Display
    LEDrange(); // Calculate LED colour range based on CPM

    Serial.print("cpm = ");
    Serial.println(cpm);

    Serial.print("temp = ");
    Serial.println(temp);

    Serial.print("cpm min / max = ");
    Serial.print(cpmmin);
    Serial.print("  /  ");
    Serial.print(cpmmax);
    Serial.println();
    Serial.print("temp min / max = ");
    Serial.print(tempmin);
    Serial.print("  /  ");
    Serial.println(tempmax);
    Serial.print("maxrange = ");
    Serial.println(maxrange);
    Serial.println();
    Serial.print("fails = ");
    Serial.println(fails);
    Serial.println();
    Serial.print("red = ");
    Serial.println(red);
    Serial.print("blue = ");
    Serial.println(blue);
    Serial.print("green = ");
    Serial.println(green);
    Serial.println();
    Serial.println("****************");
    Serial.println();

    displayLEDs();
  }
}

//Check if reset button pressed
// void WiFiManager::resetSettings() {
//  WiFi.disconnect(true);
void checkreset()
{
  if (digitalRead(0) == 0)
  {
    Serial.println("*RESET*");
    WiFi.disconnect();
    ESP.restart();
  }
}

//Connect to access point
void ConnectToAP()
{
  Serial.println("Attempting to Connect");
  randomSeed(analogRead(6));
  while (true)
  {
    delay(1000);
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, pass);
    for (int x = 0; x < 5; x++)
    {
      delay(1000);
      if (WiFi.status() == WL_CONNECTED)
      {
        Serial.print("WiFi connected in ");
        Serial.print(x);
        Serial.println(" seconds");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        Serial.println();
        return;
      }
    }
  }
}

//JSON Function
String JSON_Extract(String lookfor)
{
  DynamicJsonBuffer jsonBuffer;
  JsonObject &root = jsonBuffer.parseObject(geigerresponse);
  JsonObject &data = root["data"];
  return data[lookfor];
}

//BLYNK Definition to capture virtual pin to start prank
BLYNK_WRITE(V6)
{
  restmaxmin = param.asInt();
  if (restmaxmin == 1)
  {                  // assigning incoming value from pin V6 to a variable
    tempmin = 10000; //Min temp - set high initially
    tempmax = 0;     //Max temp
    cpmmin = 10000;  //Min cpm - set high initially
    cpmmax = 0;      //Max cpm
    Serial.println("*** RESET ***");
  }
}

//Get data from Geiger counter
void API_Request()
{
  if ((WiFi.status() == WL_CONNECTED))
  {
    yield();
    HTTPClient http;
    char buff[400];

    //Serial.print("Getting Geiger data...");
    //Serial.println();

    http.begin(geiger);

    //Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    int httpCode = http.GET();

    // httpCode will be negative on error
    if (httpCode > 0)
    {
      // HTTP header has been send and Server response header has been handled
      //Serial.printf("[HTTP] GET... code: %d\n", httpCode);

      // file found at server
      if (httpCode == HTTP_CODE_OK)
      {
        String payload = http.getString();
        payload.toCharArray(buff, 400);
        geigerresponse = payload;
      }
    }
    else
    {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      //geigerresponse = "";
      fails = fails + 1;
    }
    http.end();
  }
}

void LEDrange()
{
  //R 0>0, G 255>255, B0>255
  percentrange = rangehops * 1;     //top of this range
  percentrangeprev = rangehops * 0; //bottom of this range

  if (cpm <= percentrange)
  {
    red = 255;
    green = 255;
    blue = 0;

    percenttemp = ((cpm - percentrangeprev) * 100) / (percentrange - percentrangeprev); //% of full range / this subrange = % within this range to apply to chaning LED
    red = (255 - (percenttemp * 255 / 100));                                            //100 - percenttemp = declining

    Serial.print("1 percenttemp = ");
    Serial.println(percenttemp);
  }

  //R 0>0, G 255>255, B0>255
  percentrange = rangehops * 2;           //top of this range
  percentrangeprev = (rangehops * 1) - 1; //bottom of this range

  if (cpm <= percentrange && cpm > percentrangeprev)
  {
    red = 0;
    green = 255;
    blue = 255;
    percenttemp = ((cpm - percentrangeprev) * 100) / (percentrange - percentrangeprev); //% of full range / this subrange = % within this range to apply to chaning LED
    blue = percenttemp * 255 / 100;
    Serial.print("2 percenttemp = ");
    Serial.println(percenttemp);
  }

  //R 0>0, G 255>0, B255>255
  percentrange = rangehops * 3;           //top of this range
  percentrangeprev = (rangehops * 2) - 1; //bottom of this range

  if (cpm <= percentrange && cpm > percentrangeprev)
  {
    red = 0;
    green = 255;
    blue = 255;

    percenttemp = ((cpm - percentrangeprev) * 100) / (percentrange - percentrangeprev); //% of full range / this subrange = % within this range to apply to chaning LED
    green = (255 - (percenttemp * 255 / 100));                                          //100 - percenttemp = declining
    Serial.print("3 percenttemp = ");
    Serial.println(percenttemp);
  }

  //R 0>255, G 0>0, B255>255
  percentrange = rangehops * 4;           //top of this range
  percentrangeprev = (rangehops * 3) - 1; //bottom of this range

  if (cpm <= percentrange && cpm > percentrangeprev)
  {
    red = 255;
    green = 0;
    blue = 255;

    percenttemp = ((cpm - percentrangeprev) * 100) / (percentrange - percentrangeprev); //% of full range / this subrange = % within this range to apply to chaning LED
    red = percenttemp * 255 / 100;
    Serial.print("4 percenttemp = ");
    Serial.println(percenttemp);
  }

  //R 255>255, G 0>0, B255>0
  percentrange = rangehops * 5;           //top of this range
  percentrangeprev = (rangehops * 4) - 1; //bottom of this range

  if (cpm > percentrangeprev)
  {
    red = 255;
    green = 0;
    blue = 255;

    percenttemp = ((cpm - percentrangeprev) * 100) / (percentrange - percentrangeprev); //% of full range / this subrange = % within this range to apply to chaning LED
    blue = (255 - (percenttemp * 255 / 100));                                           //100 - percenttemp = declining
    if (blue < 0)
    {
      blue = 0;
    }

    Serial.print("5 percenttemp = ");
    Serial.println(percenttemp);
  }

  if (cpm > rangehops * 5)
  {
    red = 255;
    green = 0;
    blue = 0;

    FastLED.setBrightness(255);
  }
  else
  {
    FastLED.setBrightness(64);
  }
}

void displayLEDs()
{
  fill_solid(&(leds[0]), NUM_LEDS_PER_STRIP /*number of leds*/, CRGB(0, 0, 0));
  fill_solid(&(leds[0]), NUM_LEDS_PER_STRIP /*number of leds*/, CRGB(red, green, blue));
  FastLED.show();
}