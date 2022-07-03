/*
*** platform.io ***
[env:d1_mini]
platform = espressif8266
board = d1_mini
framework = arduino
monitor_speed = 115200

build_flags =
    -DSSID_NAME="xxx"
    -DPASSWORD_NAME="xxx"
    -DSTORMGLASS_KEY="xx-xx-xx"
    -DEMAIL_FROM="xxx@gmail.com"
    -DEMAIL_FROM_NAME="xx"
    -DEMAIL_FROM_PASS="xxxxxxx"
    -DEMAIL_TO="xxx@gmail.com"
    -DEMAIL_TO_NAME="xxx"

; set frequency to 160MHz
board_build.f_cpu = 160000000L
*/



#include <Arduino.h>
#include <NeoPixelBus.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266HTTPClient.h>
#include <WiFiManager.h>
#include <DNSServer.h>
#include <ESP8266WiFiMulti.h>
#include <ArduinoOTA.h>
#include <NTPClient.h>
#include <ESP_Mail_Client.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include <BlynkSimpleEsp8266.h>

/* Google Root CA can be downloaded from https://pki.goog/repository/ */
/** From the test on July 2021, GlobalSign Root CA was missing from Google server
 * when checking with https://www.sslchecker.com/sslchecker.
 * The certificate chain, GTS Root R1 can be used instead.
 */

const char rootCACert[] PROGMEM = "-----BEGIN CERTIFICATE-----\n"
                                  "MIIFVzCCAz+gAwIBAgINAgPlk28xsBNJiGuiFzANBgkqhkiG9w0BAQwFADBHMQsw\n"
                                  "CQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEU\n"
                                  "MBIGA1UEAxMLR1RTIFJvb3QgUjEwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAw\n"
                                  "MDAwWjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZp\n"
                                  "Y2VzIExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjEwggIiMA0GCSqGSIb3DQEBAQUA\n"
                                  "A4ICDwAwggIKAoICAQC2EQKLHuOhd5s73L+UPreVp0A8of2C+X0yBoJx9vaMf/vo\n"
                                  "27xqLpeXo4xL+Sv2sfnOhB2x+cWX3u+58qPpvBKJXqeqUqv4IyfLpLGcY9vXmX7w\n"
                                  "Cl7raKb0xlpHDU0QM+NOsROjyBhsS+z8CZDfnWQpJSMHobTSPS5g4M/SCYe7zUjw\n"
                                  "TcLCeoiKu7rPWRnWr4+wB7CeMfGCwcDfLqZtbBkOtdh+JhpFAz2weaSUKK0Pfybl\n"
                                  "qAj+lug8aJRT7oM6iCsVlgmy4HqMLnXWnOunVmSPlk9orj2XwoSPwLxAwAtcvfaH\n"
                                  "szVsrBhQf4TgTM2S0yDpM7xSma8ytSmzJSq0SPly4cpk9+aCEI3oncKKiPo4Zor8\n"
                                  "Y/kB+Xj9e1x3+naH+uzfsQ55lVe0vSbv1gHR6xYKu44LtcXFilWr06zqkUspzBmk\n"
                                  "MiVOKvFlRNACzqrOSbTqn3yDsEB750Orp2yjj32JgfpMpf/VjsPOS+C12LOORc92\n"
                                  "wO1AK/1TD7Cn1TsNsYqiA94xrcx36m97PtbfkSIS5r762DL8EGMUUXLeXdYWk70p\n"
                                  "aDPvOmbsB4om3xPXV2V4J95eSRQAogB/mqghtqmxlbCluQ0WEdrHbEg8QOB+DVrN\n"
                                  "VjzRlwW5y0vtOUucxD/SVRNuJLDWcfr0wbrM7Rv1/oFB2ACYPTrIrnqYNxgFlQID\n"
                                  "AQABo0IwQDAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4E\n"
                                  "FgQU5K8rJnEaK0gnhS9SZizv8IkTcT4wDQYJKoZIhvcNAQEMBQADggIBAJ+qQibb\n"
                                  "C5u+/x6Wki4+omVKapi6Ist9wTrYggoGxval3sBOh2Z5ofmmWJyq+bXmYOfg6LEe\n"
                                  "QkEzCzc9zolwFcq1JKjPa7XSQCGYzyI0zzvFIoTgxQ6KfF2I5DUkzps+GlQebtuy\n"
                                  "h6f88/qBVRRiClmpIgUxPoLW7ttXNLwzldMXG+gnoot7TiYaelpkttGsN/H9oPM4\n"
                                  "7HLwEXWdyzRSjeZ2axfG34arJ45JK3VmgRAhpuo+9K4l/3wV3s6MJT/KYnAK9y8J\n"
                                  "ZgfIPxz88NtFMN9iiMG1D53Dn0reWVlHxYciNuaCp+0KueIHoI17eko8cdLiA6Ef\n"
                                  "MgfdG+RCzgwARWGAtQsgWSl4vflVy2PFPEz0tv/bal8xa5meLMFrUKTX5hgUvYU/\n"
                                  "Z6tGn6D/Qqc6f1zLXbBwHSs09dR2CQzreExZBfMzQsNhFRAbd03OIozUhfJFfbdT\n"
                                  "6u9AWpQKXCBfTkBdYiJ23//OYb2MI3jSNwLgjt7RETeJ9r/tSQdirpLsQBqvFAnZ\n"
                                  "0E6yove+7u7Y/9waLd64NnHi/Hm3lCXRSHNboTXns5lndcEZOitHTtNCjv0xyBZm\n"
                                  "2tIMPNuzjsmhDYAPexZ3FL//2wmUspO8IFgV6dtxQ/PeEMMA3KgqlbbC1j+Qa3bb\n"
                                  "bP6MvPJwNQzcmRk13NfIRmPVNnGuV/u3gm3c\n"
                                  "-----END CERTIFICATE-----\n";


#define NUM_LEDS_PER_STRIP 12
#define PIN_LED 13  //D7
#define str(s) #s
#define xstr(s) str(s)
#define colorSaturation 128

ESP8266WiFiMulti wifiMulti; // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(NUM_LEDS_PER_STRIP, PIN_LED);
SMTPSession smtp;  // The SMTP Session object used for Email sending
WiFiUDP ntpUDP;  //// Define NTP Client to get time
NTPClient timeClient(ntpUDP, "pool.ntp.org");

//Wifi from build flaGS
#ifndef SSID_NAME
#define SSID_NAME "WIFI_SSID" //Default SSID if not build flag from PlatformIO doesn't work
#endif

#ifndef PASSWORD_NAME
#define PASSWORD_NAME "WIFI_PASSWORD" //Default WiFi Password if not build flag from PlatformIO doesn't work
#endif

//Gets SSID/PASSWORD from PlatformIO.ini build flags
const char ssid[] = xstr(SSID_NAME);      //  your network SSID (name)
const char pass[] = xstr(PASSWORD_NAME);  // your network password
const char key[] = xstr(STORMGLASS_KEY);   //Stormglass API key
const char email_from[] = xstr(EMAIL_FROM);   //e.g Bob@gmail.com
const char email_from_name[] = xstr(EMAIL_FROM_NAME);   //e.g Bobby
const char email_from_pass[] = xstr(EMAIL_FROM_PASS);   //Read this: https://randomnerdtutorials.com/esp8266-nodemcu-send-email-smtp-server-arduino/
const char email_to[] = xstr(EMAIL_TO);   //e.g Sally@gmail.com
const char email_to_name[] = xstr(EMAIL_TO_NAME);   //e.g Sally
const char blynk_key[] = xstr(BLYNK_KEY);


//For LED flashing
int dim = 10;
int dimswitch = 1;    //1 or -1
int dimstep = 5;
unsigned long flash_LEDs = 0;   //Use for millis, if email sent Flash LEDs for 60s

RgbColor red(colorSaturation, 0, 0);
RgbColor green(0, colorSaturation, 0);
RgbColor blue(0, 0, colorSaturation);
RgbColor white(colorSaturation);
RgbColor black(0);

HslColor hslRed(red);
HslColor hslGreen(green);
HslColor hslBlue(blue);
HslColor hslWhite(white);
HslColor hslBlack(black);

//Forecast location and vars:  https://www.latlong.net/degrees-minutes-seconds-to-decimal-degrees
String latitude_trench = "-41.504000";
String longitude_trench = "174.563833";
String latitude_hunters = "-40.968267";
String longitude_hunters = "174.815167";
String latitude_ward = "-41.295500";
String longitude_ward = "174.871500";

int API_times[4] = {7, 9, 20, 100};   //5am, 12pm, 7pm and 100=None
int API_hour_complete[24];    //If API hit, then mark in 0-23hrs in array as 1, else 0
int day_adder = 0  * 86400;    //86400 sec in a day.  Get forcast for 0=today or next 8am, or 1=next+1 day etc
int localUTC_offset = 12;   //NZ in winter

int forecast_hours = 10;  //How many hours of forecast to get (counting from 0)
String forecast;    //Forecast from weather website
int blynk_trigger = 0;
int z = 0;    //for loops

//Stormglass.io (maybe include precipitation)
double swellHeight[12];
String swellHeight_discription[12];
double waveHeight[12];
String waveHeight_discription[12];
double windSpeed_temp;
double windSpeed[12];
int windDirection[12];
String windDirectionNSWE[12];

String email_preamble = "";
String email_content = "";
int window = 0;


// Variable to save current epoch time
unsigned long epochTime; 
unsigned int epoch_adder;
int first_run = 1;    //first time run, get a forecast
unsigned long epochTime_millis; //millis() since last NTP
unsigned long loop_millis = millis() / 1000;      //Used for delay between API check loop

// Function that gets current epoch time
unsigned long getTime() {
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();
  return now;
}

//functions
void smtpCallback(SMTP_Status status);    //email
void ConnectToAP();   //Wifi
void gmail_send(String);      //Send email
unsigned long NTP_update();
String stormglass(unsigned long, int forecast_hours_use, String latitude_use, String longitude_use);    //hit the API
unsigned int decode_epoch(unsigned long, int);       //Turn Unix epoch time into hours:minutes
void JSON_array();      //deserialise JSON and build forecast arrray
int weather_window();


BLYNK_WRITE(V1)
{
  day_adder = param.asInt();
}

BLYNK_WRITE(V2)
{
  API_times[0] = param.asInt();
}

BLYNK_WRITE(V3)
{
  API_times[1] = param.asInt();
}

BLYNK_WRITE(V4)
{
  API_times[2] = param.asInt();
}

BLYNK_WRITE(V5)
{
  API_times[3] = param.asInt();
}


void setup()
{
  //Terminal setup
  Serial.begin(115200);

  //initiate LED
    strip.Begin();
    strip.Show();

  //Connect to Wifi
  ConnectToAP();

  WiFiClientSecure client;
  client.setInsecure(); //the magic line, use with caution
  client.connect("https://api.stormglass.io", 443);

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

  StaticJsonDocument<200> doc;

  epochTime = NTP_update();  //get time
  epochTime_millis = millis() / 1000;   // /1000 to make mills into seconds
  
}  



void loop()
{

  Blynk.run();

  if ((millis() /1000) - loop_millis >= 5 * 60  ||  first_run == 1){     //5mins between checks

      Serial.print("API times: ");
      for (z = 0; z < 4; z++){

        Serial.print(API_times[z]);
        Serial.print(", ");
      }
      Serial.println();
      Serial.print("Adder: ");
      Serial.println(day_adder);
      Serial.println();

      Serial.print("API_hour_complete: ");
      for (int x=0; x<24; x++){
        Serial.print(x);
        Serial.print(":");
        Serial.print(API_hour_complete[x]);
        Serial.print(", ");
        }
        Serial.println();

  loop_millis = millis() / 1000;      //reset to latest millis in seconds since we are checking
  int local_hour = decode_epoch(epochTime + ((millis() / 1000) - epochTime_millis), 0);  //Get local hour

  //check if API done for this hour
  if (API_hour_complete[local_hour] == 0){
    //We've not done an API for this hour, proceed to check API_times array
  
      for (z = 0; z < 4; z++){

        //Check for API for every instance in the API_times array
        if (API_times[z] == local_hour || first_run == 1) {

              Serial.print("Local hour ");
              Serial.println(local_hour);

                //clear array
                for (int y=0; y<24; y++){
                API_hour_complete[y] = 0;
                }

                //set flag in array for current hour
                API_hour_complete[local_hour] = 1;    //Set flag for this hour for a completed API hit
              
                email_preamble = "";  //reset
                email_content = "";  //reset
                window = 0;

                //Get epoch time
                epochTime = NTP_update();
                epochTime_millis = millis() / 1000;   // /1000 to make mills into seconds.  Updated with new NTP time

                //For testing (NZ time)
                //epochTime = 1655844755;    //8.52am
                //epochTime = 1655674400;    //9.33am
                //epochTime = 1655724800;    //11.33pm
                //epochTime = 1655717600;    //9.33pm
                
                epoch_adder = decode_epoch(epochTime, 1);   //epoch has been updated, work out time at next coming 8am to forecast

                  Serial.print("Epoch Time for next 8am: ");
                  Serial.println(epochTime + epoch_adder);
                  Serial.println();



                //Trench weather forecast
                //Send epoch time, hours ahead to get forecast and how many hours of forecast
                String trench = "";  //reset
                forecast = stormglass(epochTime + epoch_adder, forecast_hours, latitude_trench, longitude_trench);

                //deserialise JSON and populate forecast array for processing
                JSON_array();

                //Look for good weather
                window = weather_window();
              
                if (window != -1){
                  //send forecast via GMail, weather window has been found
                  int window_length = int(window / 1000);   //will trunc decimals
                  int window_start = window - (window_length * 1000);

                  trench = "Trench: " + String(int(windSpeed[window_start])) +" knot winds from the " + windDirectionNSWE[window_start] + " with a " + swellHeight[window_start] + "m swell (" + swellHeight_discription[window_start] +") and " + waveHeight[window_start] + "m waves (" + waveHeight_discription[window_start] + ")" + "\r" + "Start time = " + String(8 + window_start) + ":00, weather window lasts for " + String(window_length) +" hours" + "\r";
                }

                //Hunters weather forecast
                //Send epoch time, hours ahead to get forecast and how many hours of forecast
                String hunters = "";  //reset
                forecast = stormglass(epochTime + epoch_adder, forecast_hours, latitude_hunters, longitude_hunters);

                //deserialise JSON and populate forecast array for processing
                JSON_array();

                //Look for good weather
                window = weather_window();
              
                if (window != -1){
                  //send forecast via GMail, weather window has been found
                  int window_length = int(window / 1000);   //will trunc decimals
                  int window_start = window - (window_length * 1000);

                  hunters = "Hunters: " + String(int(windSpeed[window_start])) +" knot winds from the " + windDirectionNSWE[window_start] + " with a " + swellHeight[window_start] + "m swell (" + swellHeight_discription[window_start] +") and " + waveHeight[window_start] + "m waves (" + waveHeight_discription[window_start] + ")" + "\r" + "Start time = " + String(8 + window_start) + ":00, weather window lasts for " + String(window_length) +" hours" + "\r";
                }

                //Ward weather forecast
                //Send epoch time, hours ahead to get forecast and how many hours of forecast
                String ward = "";  //reset
                forecast = stormglass(epochTime + epoch_adder, forecast_hours, latitude_ward, longitude_ward);

                //deserialise JSON and populate forecast array for processing
                JSON_array();

                //Look for good weather
                window = weather_window();
              
                if (window != -1){
                  //send forecast via GMail, weather window has been found
                  int window_length = int(window / 1000);   //will trunc decimals
                  int window_start = window - (window_length * 1000);

                  ward = "Ward: " + String(int(windSpeed[window_start])) +" knot winds from the " + windDirectionNSWE[window_start] + " with a " + swellHeight[window_start] + "m swell (" + swellHeight_discription[window_start] +") and " + waveHeight[window_start] + "m waves (" + waveHeight_discription[window_start] + ")" + "\r" + "Start time = " + String(8 + window_start) + ":00, weather window lasts for " + String(window_length) +" hours" + "\r";
                }


                //Check if any weather forecasts need an email
                if (trench != ""){
                email_content = trench;

                if (hunters != "" || ward != ""){
                  email_content = email_content + "\r";
                }
                }

                if (hunters != ""){
                email_content = email_content + hunters;

                if (ward != ""){
                  email_content = email_content + "\r";
                }
                }

                if (ward != ""){
                email_content = email_content + ward + "\r";
                }
                

              if (email_content != "")
              {

              flash_LEDs = millis();    //There is email content (forecast) get ready to flash LEDs

                if (day_adder == 0)
                {
                    //No days added
                    if (local_hour > 8){
                      email_preamble = ("Favourable weather forecast for tomorrow (" + String(dayStr(weekday(epochTime + epoch_adder + (localUTC_offset * 3600)))) + "):");
                    }
                          else
                    {
                      email_preamble = ("Favourable weather forecast for today (" + String(dayStr(weekday(epochTime + epoch_adder + (localUTC_offset * 3600)))) + "):");
                    }
                }

                else 
                  //Days added
                {

                    email_preamble = ("Favourable weather forecast for " + String(dayStr(weekday(epochTime + epoch_adder + (localUTC_offset * 3600)))) + ":");
                }
              }
              
                else 
                {
                email_preamble = "No good weather coming up  :(";  
                }

              gmail_send(email_preamble + "\r\r" + email_content);
                

              //First time, forecast done regardless of time.  Break out of the for loop
              if (first_run == 1){
                Serial.println("First time run, end the loop");
                first_run = 0;    //reset
                z = 0 ;
                break;
                }     

                Serial.println("Waiting for the next round...");
                break;
          }
    }
  }
    
  else 
 
  {
    //No API hit - explain why
    if (API_hour_complete[local_hour] == 1){
      //API has been already hit for this hour.  Clear array for all hours and then put completed hour back
      Serial.println("API already hit for this hour");      
    }
    else {
      Serial.println("Not a matching hour");
    }
  }
  
  }         //end of main loop

  
    
    //Flash LEDs if email sent within 5min
    if (millis() - flash_LEDs < (5 * 60 * 1000) && email_content != "" ){
    strip.ClearTo(RgbColor(dim,0,0),6,11);
    strip.ClearTo(RgbColor(0,dim,0),0,5);
    strip.Show();

    dim = dim + (dimstep * dimswitch);

    if (dim > 255){
      dim = 255;
      dimswitch = -1;
      delay(1000);
    }

    if (dim < 10){
      dim = 10;
      dimswitch = 1;
     }    
    
    delay (30);
    }

    else

    {
    //Not with 5min of a good forecast to make LEDs solid
    strip.ClearTo(RgbColor(255,0,0),6,11);
    strip.ClearTo(RgbColor(0,255,0),0,5);
    strip.Show();
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

    //WiFi.begin(ssid, pass);
    Blynk.begin(blynk_key, ssid, pass);
    
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




String stormglass(unsigned long epoch_to_use, int forecast_hours_use, String latitude_use, String longitude_use ){

  String host = "https://api.stormglass.io/v2/weather/point?lat=" + latitude_use + "&lng=" + longitude_use + "&params=windDirection,windSpeed,waveHeight,swellHeight";    //Trench coordinates

  String host_final = host + "&start=" + String(epoch_to_use) + "&end=" + String(epoch_to_use + (forecast_hours_use * 3600));
  String payload = "";

  Serial.print("Stormglass.io request: ");
  Serial.println(host_final);
  Serial.println();

  WiFiClientSecure client;
  client.setInsecure(); //the magic line, use with caution
  client.connect(host_final, 443);

  HTTPClient https; 
  
    Serial.print("[HTTPS] begin...\n");
    if (https.begin(client, host_final)) {  // HTTPS

    https.addHeader("Authorization",  key);

      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      int httpCode = https.GET();
    
      // httpCode will be negative on error
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] GET... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          Serial.println("Success");
          payload = https.getString();
        }
      } else {
        Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }

      https.end();
    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
    }

     //For testing
     //payload = "{\"hours\":[{\"swellDirection\":{\"icon\":168.86,\"meteo\":217.76,\"noaa\":324.91,\"sg\":217.76},\"swellHeight\":{\"icon\":0.52,\"meteo\":0.71,\"noaa\":0.19,\"sg\":0.71},\"time\":\"2022-06-19T10:00:00+00:00\",\"waveHeight\":{\"icon\":2.24,\"meteo\":2.99,\"noaa\":3.07,\"sg\":2.99},\"windDirection\":{\"icon\":176.76,\"noaa\":186.55,\"sg\":176.76},\"windSpeed\":{\"icon\":13.56,\"noaa\":16.73,\"sg\":13.56}},{\"swellDirection\":{\"icon\":173.18,\"meteo\":221.76,\"noaa\":324.57,\"sg\":221.76},\"swellHeight\":{\"icon\":0.54,\"meteo\":0.77,\"noaa\":0.18,\"sg\":0.77},\"time\":\"2022-06-19T11:00:00+00:00\",\"waveHeight\":{\"icon\":2.3,\"meteo\":3.06,\"noaa\":3.1,\"sg\":3.06},\"windDirection\":{\"icon\":175.18,\"noaa\":184.6,\"sg\":175.18},\"windSpeed\":{\"icon\":13.58,\"noaa\":17.0,\"sg\":13.58}},{\"swellDirection\":{\"icon\":177.5,\"meteo\":225.76,\"noaa\":324.22,\"sg\":225.76},\"swellHeight\":{\"icon\":0.56,\"meteo\":0.82,\"noaa\":0.16,\"sg\":0.82},\"time\":\"2022-06-19T12:00:00+00:00\",\"waveHeight\":{\"icon\":2.35,\"meteo\":3.14,\"noaa\":3.12,\"sg\":3.14},\"windDirection\":{\"icon\":173.6,\"noaa\":182.66,\"sg\":173.6},\"windSpeed\":{\"icon\":13.61,\"noaa\":17.27,\"sg\":13.61}},{\"swellDirection\":{\"icon\":178.9,\"meteo\":224.38,\"noaa\":323.42,\"sg\":224.38},\"swellHeight\":{\"icon\":0.55,\"meteo\":0.69,\"noaa\":0.16,\"sg\":0.69},\"time\":\"2022-06-19T13:00:00+00:00\",\"waveHeight\":{\"icon\":2.37,\"meteo\":3.2,\"noaa\":3.19,\"sg\":3.2},\"windDirection\":{\"icon\":172.41,\"noaa\":180.42,\"sg\":172.41},\"windSpeed\":{\"icon\":13.79,\"noaa\":17.44,\"sg\":13.79}},{\"swellDirection\":{\"icon\":180.31,\"meteo\":223.01,\"noaa\":322.62,\"sg\":223.01},\"swellHeight\":{\"icon\":0.53,\"meteo\":0.56,\"noaa\":0.15,\"sg\":0.56},\"time\":\"2022-06-19T14:00:00+00:00\",\"waveHeight\":{\"icon\":2.38,\"meteo\":3.26,\"noaa\":3.25,\"sg\":3.26},\"windDirection\":{\"icon\":171.22,\"noaa\":178.19,\"sg\":171.22},\"windSpeed\":{\"icon\":13.96,\"noaa\":17.6,\"sg\":13.96}},{\"swellDirection\":{\"icon\":181.71,\"meteo\":221.63,\"noaa\":321.82,\"sg\":221.63},\"swellHeight\":{\"icon\":0.52,\"meteo\":0.43,\"noaa\":0.15,\"sg\":0.43},\"time\":\"2022-06-19T15:00:00+00:00\",\"waveHeight\":{\"icon\":2.4,\"meteo\":3.32,\"noaa\":3.32,\"sg\":3.32},\"windDirection\":{\"icon\":170.03,\"noaa\":175.95,\"sg\":170.03},\"windSpeed\":{\"icon\":14.14,\"noaa\":17.77,\"sg\":14.14}},{\"swellDirection\":{\"icon\":186.55,\"meteo\":223.42,\"noaa\":274.07,\"sg\":223.42},\"swellHeight\":{\"icon\":0.48,\"meteo\":0.62,\"noaa\":0.61,\"sg\":0.62},\"time\":\"2022-06-19T16:00:00+00:00\",\"waveHeight\":{\"icon\":2.46,\"meteo\":3.38,\"noaa\":3.34,\"sg\":3.38},\"windDirection\":{\"icon\":169.08,\"noaa\":174.4,\"sg\":169.08},\"windSpeed\":{\"icon\":14.62,\"noaa\":17.81,\"sg\":14.62}}],\"meta\":{\"cost\":1,\"dailyQuota\":10,\"end\":\"2022-06-19 16:30\",\"lat\":-41.291,\"lng\":174.524,\"params\":[\"windDirection\",\"windSpeed\",\"waveHeight\",\"swellDirection\",\"swellHeight\"],\"requestCount\":3,\"start\":\"2022-06-19 10:00\"}}";
     //payload = "{\"hours\":[{\"swellHeight\":{\"icon\":0.81,\"meteo\":1.11,\"noaa\":0.7,\"sg\":1.11},\"time\":\"2022-06-23T20:00:00+00:00\",\"waveHeight\":{\"icon\":0.85,\"meteo\":1.4,\"noaa\":1.09,\"sg\":1.4},\"windDirection\":{\"icon\":359.27,\"noaa\":335.2,\"sg\":359.27},\"windSpeed\":{\"icon\":4.7,\"noaa\":11.36,\"sg\":4.7}},{\"swellHeight\":{\"icon\":0.82,\"meteo\":1.07,\"noaa\":0.68,\"sg\":1.07},\"time\":\"2022-06-23T21:00:00+00:00\",\"waveHeight\":{\"icon\":0.88,\"meteo\":1.4,\"noaa\":1.09,\"sg\":1.4},\"windDirection\":{\"icon\":3.51,\"noaa\":336.47,\"sg\":3.51},\"windSpeed\":{\"icon\":5.15,\"noaa\":11.31,\"sg\":5.15}},{\"swellHeight\":{\"icon\":0.82,\"meteo\":1.16,\"noaa\":0.67,\"sg\":1.16},\"time\":\"2022-06-23T22:00:00+00:00\",\"waveHeight\":{\"icon\":0.94,\"meteo\":1.41,\"noaa\":1.12,\"sg\":1.41},\"windDirection\":{\"icon\":357.79,\"noaa\":337.62,\"sg\":357.79},\"windSpeed\":{\"icon\":5.73,\"noaa\":11.38,\"sg\":5.73}},{\"swellHeight\":{\"icon\":0.83,\"meteo\":1.25,\"noaa\":0.65,\"sg\":1.25},\"time\":\"2022-06-23T23:00:00+00:00\",\"waveHeight\":{\"icon\":1.0,\"meteo\":1.42,\"noaa\":1.15,\"sg\":1.42},\"windDirection\":{\"icon\":352.07,\"noaa\":338.78,\"sg\":352.07},\"windSpeed\":{\"icon\":6.31,\"noaa\":11.44,\"sg\":6.31}},{\"swellHeight\":{\"icon\":0.83,\"meteo\":1.34,\"noaa\":0.64,\"sg\":1.34},\"time\":\"2022-06-24T00:00:00+00:00\",\"waveHeight\":{\"icon\":1.06,\"meteo\":1.43,\"noaa\":1.18,\"sg\":1.43},\"windDirection\":{\"icon\":346.35,\"noaa\":339.93,\"sg\":346.35},\"windSpeed\":{\"icon\":6.89,\"noaa\":11.51,\"sg\":6.89}},{\"swellHeight\":{\"icon\":0.85,\"meteo\":1.35,\"noaa\":0.62,\"sg\":1.35},\"time\":\"2022-06-24T01:00:00+00:00\",\"waveHeight\":{\"icon\":1.07,\"meteo\":1.45,\"noaa\":1.22,\"sg\":1.45},\"windDirection\":{\"icon\":347.42,\"noaa\":339.08,\"sg\":347.42},\"windSpeed\":{\"icon\":6.78,\"noaa\":11.84,\"sg\":6.78}},{\"swellHeight\":{\"icon\":0.86,\"meteo\":1.35,\"noaa\":0.61,\"sg\":1.35},\"time\":\"2022-06-24T02:00:00+00:00\",\"waveHeight\":{\"icon\":1.08,\"meteo\":1.46,\"noaa\":1.27,\"sg\":1.46},\"windDirection\":{\"icon\":348.48,\"noaa\":338.24,\"sg\":348.48},\"windSpeed\":{\"icon\":6.68,\"noaa\":12.18,\"sg\":6.68}},{\"swellHeight\":{\"icon\":0.88,\"meteo\":1.36,\"noaa\":0.59,\"sg\":1.36},\"time\":\"2022-06-24T03:00:00+00:00\",\"waveHeight\":{\"icon\":1.09,\"meteo\":1.48,\"noaa\":1.31,\"sg\":1.48},\"windDirection\":{\"icon\":349.55,\"noaa\":337.39,\"sg\":349.55},\"windSpeed\":{\"icon\":6.57,\"noaa\":12.51,\"sg\":6.57}},{\"swellHeight\":{\"icon\":0.89,\"meteo\":1.35,\"noaa\":0.59,\"sg\":1.35},\"time\":\"2022-06-24T04:00:00+00:00\",\"waveHeight\":{\"icon\":1.07,\"meteo\":1.48,\"noaa\":1.3,\"sg\":1.48},\"windDirection\":{\"icon\":349.39,\"noaa\":334.14,\"sg\":349.39},\"windSpeed\":{\"icon\":6.33,\"noaa\":12.06,\"sg\":6.33}},{\"swellHeight\":{\"icon\":0.91,\"meteo\":1.34,\"noaa\":0.6,\"sg\":1.34},\"time\":\"2022-06-24T05:00:00+00:00\",\"waveHeight\":{\"icon\":1.05,\"meteo\":1.49,\"noaa\":1.3,\"sg\":1.49},\"windDirection\":{\"icon\":349.23,\"noaa\":330.88,\"sg\":349.23},\"windSpeed\":{\"icon\":6.08,\"noaa\":11.6,\"sg\":6.08}},{\"swellHeight\":{\"icon\":0.92,\"meteo\":1.33,\"noaa\":0.6,\"sg\":1.33},\"time\":\"2022-06-24T06:00:00+00:00\",\"waveHeight\":{\"icon\":1.03,\"meteo\":1.49,\"noaa\":1.29,\"sg\":1.49},\"windDirection\":{\"icon\":349.07,\"noaa\":327.63,\"sg\":349.07},\"windSpeed\":{\"icon\":5.84,\"noaa\":11.15,\"sg\":5.84}},{\"swellHeight\":{\"icon\":0.92,\"meteo\":1.31,\"noaa\":0.63,\"sg\":1.31},\"time\":\"2022-06-24T07:00:00+00:00\",\"waveHeight\":{\"icon\":1.02,\"meteo\":1.48,\"noaa\":1.27,\"sg\":1.48},\"windDirection\":{\"icon\":349.95,\"noaa\":332.09,\"sg\":349.95},\"windSpeed\":{\"icon\":5.74,\"noaa\":10.97,\"sg\":5.74}}],\"meta\":{\"cost\":1,\"dailyQuota\":10,\"end\":\"2022-06-24 07:00\",\"lat\":-41.291,\"lng\":174.524,\"params\":[\"windDirection\",\"windSpeed\",\"waveHeight\",\"swellHeight\"],\"requestCount\":4,\"start\":\"2022-06-23 20:00\"}}";

     return payload;
}



unsigned long NTP_update(){
  unsigned long NTPtime;
  NTPtime = getTime();
  return NTPtime;
}



void gmail_send(String email_txt){
    /** Enable the debug via Serial port
   * 0 for no debugging
   * 1 for basic level debugging
   *
   * Debug port can be changed via ESP_MAIL_DEFAULT_DEBUG_PORT in ESP_Mail_FS.h
   */
  smtp.debug(1);

  /* Set the callback function to get the sending results */
  smtp.callback(smtpCallback);

  /* Declare the session config data */
  ESP_Mail_Session session;

  /* Set the session config */
  /* The log in credentials */
  //https://randomnerdtutorials.com/esp8266-nodemcu-send-email-smtp-server-arduino/
  session.server.host_name = "smtp.gmail.com";
  session.server.port = 587;
  session.login.email = email_from;
  session.login.password = email_from_pass;
  session.login.user_domain = F("");

  /* Set the NTP config time */
  session.time.ntp_server = F("pool.ntp.org,time.nist.gov");
  session.time.gmt_offset = 3;
  session.time.day_light_offset = 0;

  // If you invoke addRecipient multiple times, it will add the given recipient to the list of recipients of the given time (TO, CC, and BCC).
  // message.addRecipient(Message.RecipientType.CC, InternetAddress.parse("abc@abc.com"));
  // message.addRecipient(Message.RecipientType.CC, InternetAddress.parse("abc@def.com"));
  // message.addRecipient(Message.RecipientType.CC, InternetAddress.parse("ghi@abc.com"));

  /** In ESP32, timezone environment will not keep after wake up boot from sleep.
   * The local time will equal to GMT time.
   *
   * To sync or set time with NTP server with the valid local time after wake up boot,
   * set both gmt and day light offsets to 0 and assign the timezone environment string e.g.
     session.time.ntp_server = F("pool.ntp.org,time.nist.gov");
     session.time.gmt_offset = 0;
     session.time.day_light_offset = 0;
     session.time.timezone_env_string = "JST-9"; // for Tokyo
   * The library will get (sync) the time from NTP server without GMT time offset adjustment
   * and set the timezone environment variable later.
   *
   * This timezone environment string will be stored to flash or SD file named "/tz_env.txt"
   * which set via session.time.timezone_file.
   *
   * See the timezone environment string list from
   * https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
   *
   */

  /* Declare the message class */
  SMTP_Message message;

  /* Set the message headers */
  message.sender.name = (email_from_name); // This witll be used with 'MAIL FROM' command and 'From' header field.
  message.sender.email = email_from; // This witll be used with 'From' header field.
  message.subject = F("Weather report");
  message.addRecipient((email_to_name), (email_to)); // This will be used with RCPT TO command and 'To' header field.
  //message.addRecipient(F("Name"), F("Email address")); // This will be used with RCPT TO command and 'To' header field.
  
  String textMsg = email_txt;
  message.text.content = textMsg;

  /** If the message to send is a large string, to reduce the memory used from internal copying  while sending,
   * you can assign string to message.text.blob by cast your string to uint8_t array like this
   *
   * String myBigString = "..... ......";
   * message.text.blob.data = (uint8_t *)myBigString.c_str();
   * message.text.blob.size = myBigString.length();
   *
   * or assign string to message.text.nonCopyContent, like this
   *
   * message.text.nonCopyContent = myBigString.c_str();
   *
   * Only base64 encoding is supported for content transfer encoding in this case.
   */

  /** The Plain text message character set e.g.
   * us-ascii
   * utf-8
   * utf-7
   * The default value is utf-8
   */
  message.text.charSet = F("us-ascii");

  /** The content transfer encoding e.g.
   * enc_7bit or "7bit" (not encoded)
   * enc_qp or "quoted-printable" (encoded)
   * enc_base64 or "base64" (encoded)
   * enc_binary or "binary" (not encoded)
   * enc_8bit or "8bit" (not encoded)
   * The default value is "7bit"
   */
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  // If this is a reply message
  // message.in_reply_to = "<parent message id>";
  // message.references = "<parent references> <parent message id>";

  /** The message priority
   * esp_mail_smtp_priority_high or 1
   * esp_mail_smtp_priority_normal or 3
   * esp_mail_smtp_priority_low or 5
   * The default value is esp_mail_smtp_priority_low
   */
  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;

  // message.response.reply_to = "someone@somemail.com";
  // message.response.return_path = "someone@somemail.com";

  /** The Delivery Status Notifications e.g.
   * esp_mail_smtp_notify_never
   * esp_mail_smtp_notify_success
   * esp_mail_smtp_notify_failure
   * esp_mail_smtp_notify_delay
   * The default value is esp_mail_smtp_notify_never
   */
  // message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

  /* Set the custom message header */
  //message.addHeader(F("Message-ID: <example@gmail.com>"));
  message.addHeader("Message-ID: <" + String(email_from) + ">");

  // For Root CA certificate verification (ESP8266 and ESP32 only)
  // session.certificate.cert_data = rootCACert;
  // or
  // session.certificate.cert_file = "/path/to/der/file";
  // session.certificate.cert_file_storage_type = esp_mail_file_storage_type_flash; // esp_mail_file_storage_type_sd
  // session.certificate.verify = true;

  // The WiFiNINA firmware the Root CA certification can be added via the option in Firmware update tool in Arduino IDE

  /* Connect to server with the session config */

  // Library will be trying to sync the time with NTP server if time is never sync or set.
  // This is 10 seconds blocking process.
  // If time synching was timed out, the error "NTP server time synching timed out" will show via debug and callback function.
  // You can manually sync time by yourself with NTP library or calling configTime in ESP32 and ESP8266.
  // Time can be set manually with provided timestamp to function smtp.setSystemTime.

  //
  if (!smtp.connect(&session))
    return;

  /* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &message))
    Serial.println("Error sending Email, " + smtp.errorReason());

  // to clear sending result log
  // smtp.sendingResult.clear();

  ESP_MAIL_PRINTF("Free Heap: %d\n", MailClient.getFreeHeap());

}



/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status)
{
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success())
  {
    // ESP_MAIL_PRINTF used in the examples is for format printing via debug Serial port
    // that works for all supported Arduino platform SDKs e.g. AVR, SAMD, ESP32 and ESP8266.
    // In ESP32 and ESP32, you can use Serial.printf directly.

    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failed: %d\n", status.failedCount());
    Serial.println("----------------\n");
    struct tm dt;

    for (size_t i = 0; i < smtp.sendingResult.size(); i++)
    {
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(i);

      // In case, ESP32, ESP8266 and SAMD device, the timestamp get from result.timestamp should be valid if
      // your device time was synched with NTP server.
      // Other devices may show invalid timestamp as the device time was not set i.e. it will show Jan 1, 1970.
      // You can call smtp.setSystemTime(xxx) to set device time manually. Where xxx is timestamp (seconds since Jan 1, 1970)
      time_t ts = (time_t)result.timestamp;
      localtime_r(&ts, &dt);

      ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %d/%d/%d %d:%d:%d\n", dt.tm_year + 1900, dt.tm_mon + 1, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");

    // You need to clear sending result as the memory usage will grow up.
    smtp.sendingResult.clear();
  }
}




//Update the time
//Use = 0 return local time, = 1 return time to next 8am
unsigned int decode_epoch(unsigned long currentTime, int use)
{
    //UTC time vars
    int hour;
    int hour_local;
    int second;
    int minute;

    //Only print when doing an API pull
    if (use == 1){
    Serial.print("The epoch UTC time is ");
    Serial.print(currentTime);
    Serial.println();

    // print the hour, minute and second:
    Serial.print("The UTC time is ");      // UTC is the time at Greenwich Meridian (GMT)
    Serial.print((currentTime % 86400L) / 3600); // print the hour (86400 equals secs per day)
    Serial.print(':');
    
    if (((currentTime % 3600) / 60) < 10)
    {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }

    Serial.print((currentTime % 3600) / 60 && use == 1 ); // print the minute (3600 equals secs per minute)
    Serial.print(':');

    if ((currentTime % 60) < 10)
    {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }

    Serial.println(currentTime % 60); // print the second
    }

  hour = (currentTime % 86400L) / 3600;

  //UTC offset applied
  hour_local = hour + localUTC_offset;
    if (hour_local > 23){
      hour_local = hour_local - 24;
    }

  minute = (currentTime % 3600) / 60;
  second = currentTime % 60;

    //Only print when doing an API pull
    //24hr clock
    // if (use == 1){
    Serial.print("The local time is ");
    Serial.print(hour_local);
    Serial.print(":");
    Serial.print(minute);
    Serial.print(":");
    Serial.print(second);
    Serial.println();
    //}

    //API was hit just to get local time
    if (use == 0){
      return hour_local;
    }


    int hours_next8AM;
    int minutes_next8AM;

    //Work out how many hours and minutes to the next 8am
    //always forecast the next 8am-8pm period unless already the hour of 8am of same day
    //If before 8am on same day (e.g it's 6am now) then forecast for same day
    hours_next8AM = ((24 - hour_local) + 7);

    //Still correct?
    if (minute == 0){
      minutes_next8AM = 0;
      }  else
      {
        minutes_next8AM = 60 - minute;
      }


    //If in small hours then hours:
    // 24 - 5(am) = 19 + 8 = 27  = 2 hours + minutes until 8am   //deducate 25 hrs
    if (hours_next8AM >= 25){
      hours_next8AM = hours_next8AM - 25;

      //if (example) 5am exactly (0 mins) then increase hour +1   
      // e.g not 2hrs plus (0) minutes  = 3 hrs + 0 minutes
      if (minute == 0){
        hours_next8AM ++;
      }
    }


    //If in the 8am hour already then forecast for now
    //Zero all as we are 8am ish already
    if (hour_local== 8){
      hours_next8AM = 0;
      minutes_next8AM = minute * -1;   //Need to adjust minutes of epoch back minutes elapsed to starts at 8am exactly
    }

    if (minutes_next8AM >= 0){
      Serial.println();
      Serial.print("Hours and minutes to next 8:00AM: ");
      Serial.print(hours_next8AM);
      Serial.print(":");
      Serial.print(minutes_next8AM);
      Serial.println();
    }
    else
    {
      Serial.println();
      Serial.print("In the current 8am 1hr window, will use 8am this morning for forecast");
      Serial.println();
    }

    unsigned int epoch_adder_calc = (hours_next8AM * 3600) + (minutes_next8AM * 60) + day_adder;

    return epoch_adder_calc;

}



void JSON_array(){
    // Deserialize the document
  DynamicJsonDocument doc(6144);

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, forecast);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }

  
  // Print the result
  serializeJsonPretty(doc, Serial);
      Serial.println();
      Serial.println();

  //Populate the array
  for (int x=0; x <= forecast_hours; x++){
  
    swellHeight[x] = doc["hours"][x]["swellHeight"]["sg"];
    waveHeight[x] = doc["hours"][x]["waveHeight"]["sg"];
    windSpeed_temp = doc["hours"][x]["windSpeed"]["sg"];
    windSpeed[x] = windSpeed_temp * 1.94384;
    windDirection[x] = doc["hours"][x]["windDirection"]["sg"];

    // Serial.print("Hour: ");
    // Serial.println(x);
    terminal.print("Hour: ");
    terminal.print(x);

    if (swellHeight[x] < 2.0){
      swellHeight_discription[x] = "Low";
    }

    if (swellHeight[x] >= 2.0 && swellHeight[x] <= 4.0){
      swellHeight_discription[x] = "Moderate";
    }

    if (swellHeight[x] > 4.0){
      swellHeight_discription[x] = "Heavy";
    }

    // Serial.print("Swell height: ");
    // Serial.print(swellHeight[x]);
    // Serial.print(" - ");
    // Serial.println(swellHeight_discription[x]);



    if (waveHeight[x] <= 0.1){
      waveHeight_discription[x] = "Calm";
    }

    if (waveHeight[x] > 0.1 && waveHeight[x] <= 0.5){
      waveHeight_discription[x] = "Smooth";
    }

    if (waveHeight[x] > 0.5 && waveHeight[x] <= 1.0){
      waveHeight_discription[x] = "Slight";
    }

    if (waveHeight[x] > 1.0 && waveHeight[x] <= 2.0){
      waveHeight_discription[x] = "Moderate";
    }

    if (waveHeight[x] > 2.0 && waveHeight[x] <= 3.0){
      waveHeight_discription[x] = "Rough";
    }

    if (waveHeight[x] > 3.0 && waveHeight[x] <= 4.5){
      waveHeight_discription[x] = "Very rough";
    }

    if (waveHeight[x] > 4.5 && waveHeight[x] <= 6.5){
      waveHeight_discription[x] = "High";
    }

    if (waveHeight[x] > 6.5 && waveHeight[x] <= 8.5){
      waveHeight_discription[x] = "Very high";
    }

    if (waveHeight[x] > 8.5){
      waveHeight_discription[x] = "Phenomenal";
    }

    // Serial.print("Wave height: ");
    // Serial.print(waveHeight[x]);
    // Serial.print(" - ");
    // Serial.println(waveHeight_discription[x]);

    // Serial.print("Wind speed: ");
    // Serial.print(windSpeed[x]);
    // Serial.print(" knots from the ");
    
    if (windDirection[x] > 336 && windDirection[x] <= 360){
      windDirectionNSWE[x] = "North";
    }

    if (windDirection[x] >= 0 && windDirection[x] <= 23){
      windDirectionNSWE[x] = "North";
    }

    if (windDirection[x] > 23 && windDirection[x] <= 68){
      windDirectionNSWE[x] = "North east";
    }

    if (windDirection[x] > 68 && windDirection[x] <= 113){
      windDirectionNSWE[x] = "East";
    }

    if (windDirection[x] > 113 && windDirection[x] <= 158){
      windDirectionNSWE[x] = "South east";
    }

    if (windDirection[x] > 158 && windDirection[x] <= 203){
      windDirectionNSWE[x] = "South";
    }

    if (windDirection[x] > 203 && windDirection[x] <= 248){
      windDirectionNSWE[x] = "South west";
    }

    if (windDirection[x] > 248 && windDirection[x] <= 293){
      windDirectionNSWE[x] = "West";
    }

    if (windDirection[x] > 293 && windDirection[x] <= 336){
      windDirectionNSWE[x] = "North west";
    }

    // Serial.println(windDirectionNSWE[x]);
    // Serial.println();

    //For testing
    // waveHeight_discription[x] = "Slight";
    // swellHeight_discription[x] = "Low";
    // windDirectionNSWE[x] = "North west";
    // windSpeed[x] = 10;
}
}

//Check for good weather windows
int weather_window(){

  //Check the array
  //Looking for 3hour window of Low swell, Smooth or Slight wave height, and below 12 knots

  int window_count = 0;     //Number of hours found with all categories passing
  int window_categories_count = 0; //count for each category passing test.
  int window__hours_target = 3;    //number of hours needed in window
  int start_hour = -1;  //Hour in the array, -1 means none found yet
  
  for (int x=0; x <= forecast_hours; x++){

    //Calm, Smooth, Slight waves
    if(waveHeight_discription[x] == "Calm" || waveHeight_discription[x] == "Smooth" || waveHeight_discription[x] == "Slight"){
        window_categories_count ++;
    }

    //Low swell
    if(swellHeight_discription[x] == "Low"){
        window_categories_count ++;
    }

    //At or below 12 knots
    if(windSpeed[x] <= 12){
        window_categories_count ++;
    }

    //Wind only fron North-ish
    if(windDirectionNSWE[x] != "South"  || windDirectionNSWE[x] != "South east"  || windDirectionNSWE[x] != "South west"){
        window_categories_count ++;
    }


    //How many passed
    if (window_categories_count >= 4){
      window_count ++;
      Serial.print("Window count passed in hour: ");
      Serial.print(8 + x);      
      Serial.println(":00");

      if (start_hour == -1){
        start_hour = x;         //Start of window found
      }
    }

    else

    {
      window_categories_count = 0;    //Reset the window
      start_hour = -1;    //Reset the window
    }
  }

  //Did we succeed in finding a 3 hour window?
  if (window_count >= window__hours_target){
    Serial.println();
    Serial.println("********* Weather window found *********");
    Serial.println();
    return ((window_count*1000) + start_hour);    //4005 = 4 hours of weather window, starting at 5hrs after 8am
    }
    else
    {
      return -1;    //No window found
    }

    
}










/*
Wind
Direction such as north-west, is the direction the wind is expected to come from.

Speed is given in knots, and 1 knot is approximately 2km/hr. This is an average speed, so always expect that gusts may be 50% higher. Also allow for funnelling between headlands, causing the wind speed to double.

Sea
Sea is a description of the waves formed by the local wind.

Swell
A swell comes from either a distant disturbance, such as a cyclone or depression, or develops from wind waves that have been blowing from the same direction for a length of time. Swells increase in height and get steeper when they reach shallow water.

The measures used for swells are:

low – under 2.0 metres
moderate – 2 –4 metres
heavy – over 4 metres
Visibility
The average visibility in New Zealand is about 15 nautical miles.

This information is typically given when visibility is expected to be less than 6 miles (10km).

The visibility distance are:

fog – less than 1.0 nautical mile
poor – 1–3 nautical miles
fair – 3–6 nautical miles
good – over 6 nautical miles
Outlook
All marine forecasts are for up to 48 hours, with the outlook for a further 3 days.

Wave height
This refers to the size of of significant waves that are generated by the wind in the area.

The approximate wind wave height measures used are:

calm – up to 0.1 metre
smooth – up to 0.5 metre
slight – up to 1.0 metre
moderate – up to 2.0 metres
rough – up to 3.0 metres
very rough – up to 4.5 metres
high – up to 6.5 metres
very high – up to 8.5 metres
phenomenal – up to 11.0 metres (or more)




heres some GPS marks i thought the wellington boateys might  like to use, im sure in hell not going to be needeing them so may as well pass them ona nd hope they can be of some use

Fishermans Rock

GPS 41-04.10 S 174-36.15 E


A dangerous spot to visit in small boats unless the wind is light.
The place is riddled with extreme dropoffs (eg 300 metres up to 50
metres). The gamefishing potential is relatively un-tapped but look
to be very good. Groper fishing is quite localised but can be very
excellent at times. You should get into big blue cod on the turn of
the tide. Warehou are common during the winter


Verns Reef
GPS 41-08.55 S 174-43.40 E


A cracker place for big tarakihi (1-3kg) and school groper. The reef
area is in 70 to 85 meters water depth. It is not a huge reef
structure and don't always expect to see fish on the sounder. There
are some good rock formations out from Verns in 100 to 150 metres
which produce huge blue cod and groper



78 Metre Rise
GPS 41-01.30 S 174.39.85 E ?????

Just north of Fishermans rock, this place can be a consistent groper
spot. Some people anchor up in 100 � 130 metres, others fish the
deep. Bass can be taken as well as groper in depths of 200 to 250
metres


GPS 40-56.28 S 174-42.60 E ????

If you steam 3 to 5 miles west from Hunters you will find quite a few
good pinnacles coming out of 150 metres and rising up to 100 metres.
The groper fishing can be very good in this area from time to time as
well as big blue cod and tarakihi. Albacore tuna and sharks are
caught regularly during the summer



The Trench
GPS 41-29.10 S 174-52.40 E

The main spot to fish is approximately 3 miles straight out from
Turakirae Head The 'patch' is in 220 metres depth but you can catch
fish from 200 to 300 metres. Groper are the main target fish but you
will catch bass and bluenose.


Five Mile Reef
GPS 41-24.95 S 174-48.40 E

Good place for big blue cod and tarakihi in 40 to 80 metres. Kingfish
can be taken during the summer and warehou in the winter around the
major pinnacles. A spot that can be a very tidal and expensive on
terminal tackle with numbers of barracouta and shark and snags.
Groper are caught in the deeper patches from 75 to 125 metres.

A Good Bluenose Spot
GPS 41-26.45 S 174-46.25 E

Fish in 160 to 220 metres � Look for fish sign on your sounder,
especially if it is up off the bottom


78 metre rise snapper puka
S41 01.300 E174 39.850

Blue nose
S41 26.450 E174 46.250

rock puka
S41 04.100 E174 36.150

5 Mile Reef
S41 24.940 E174 33.830

Hunters bank kingfish
S40 58.096 E174 48.910

Island bay
S41 21.850 E174 47.810

Karori puka trumpeter
S41 23.284 E174 41.033

Makara Snapper Etc
S41 11.390 E174 43.460

Mana snapper
S41 05.843 E174 47.871

Mana bridge Snapper
S41 05.900 E174 47.780

Mana bridge 1
S41 06.200 E174 47.710

Ohau makara puka
S41 12.320 E174 35.840

Out wide mana reef
S41 04.670 E174 42.150

Paramata reef snapper
S41 04.786 E174 50.875

Puka 150-200mtrs
S41 02.252 E174 45.201

Pukerua bay Snapper
S41 02.460 E174 50.920

Reef
S41 09.750 E174 38.898

Snapper
S41 02.460 E174 50.920

Snapper etc
S41 03.919 E174 49.844

Trench puka
S41 30.240 E174 33.830

Verns Reef
S41 08.550 E174 43.400

Verns reef2
S41 08.545 E174 43.427

Wairaka rise Snapper
S41 01.254 E174 51.401

*/

