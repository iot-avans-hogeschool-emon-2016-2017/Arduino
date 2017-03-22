/*Includes*/
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#include <ESP8266HTTPClient.h>

#include <TimeLib.h> 
#include <WiFiUdp.h>

/* Time / NTP stuff */
unsigned int localUDPPort = 2390;      // local port to listen for UDP packets
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

WiFiUDP Udp; // A UDP instance to let us send and receive packets over UDP

const int timeZone = 0;     // UTC Time
//const int timeZone = 1;     // Central European Time

time_t prevDisplay = 0; // when the digital clock was displayed

/*Pins*/
const byte vInput = A0;

/*LDR values and dif margin*/
int oldValue = -1;
int newValue = -1;
int dif = 0;
const int minDif = 10;
const int maxDif = 300;

void nextValue();
void printValues();
void calcDif();
boolean isPeak();

/*RESTFul client stuff*/
String response = "";
int statusCode = 0;
char message[] = "";

void setupWiFi()
{
    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    //reset saved settings
    //wifiManager.resetSettings();
    
    //set custom ip for portal
    //wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

    //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    wifiManager.autoConnect();
    //or use this for auto generated name ESP + ChipID
    //wifiManager.autoConnect();

    
    //if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
}

/*-------- NTP / Time code ----------*/

void setupNTP() {
  Serial.println("Starting UDP");
  Udp.begin(localUDPPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());
  Serial.println("waiting for sync");

  WiFi.hostByName(ntpServerName, timeServerIP); 
  
  setSyncProvider(getNtpTime);
}

void digitalClockDisplay(){
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(".");
  Serial.print(month());
  Serial.print(".");
  Serial.print(year()); 
  Serial.println(); 
}

void printDigits(int digits){
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

time_t getNtpTime()
{
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Setup wemos");
  setupWiFi();
  setupNTP();
  calcDif();
}

void loop() {    
  if (isPeak() == true) {
    Serial.println("Is peak!");
    tickFunc();
    printValues();
  }
  //time stuff
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed
      prevDisplay = now();
      digitalClockDisplay();  
    }
  }
  //end time stuff
  //Serial.println("----------");
  //delay(1000);
}

boolean isPeak() {
  nextValue();
  calcDif();
  boolean higherThanMinDif = dif > minDif;
  boolean lowerThanMaxDif = dif < maxDif;
  boolean inMargin = higherThanMinDif && lowerThanMaxDif;
  //Serial.print(higherThanMinDif);
  //Serial.print(", ");
  //Serial.print(lowerThanMaxDif);
  //Serial.print(" : ");
  //Serial.println(inMargin);
  return inMargin;
}

void calcDif() {
  //Serial.print("Calculate difference in values: ");
  dif = newValue - oldValue;  
  //Serial.print(dif);
  if (dif < 0) {
     dif = dif * -1;
  }
  //Serial.print(" becomes: ");
  //Serial.println(dif);
}

void printValues() {
  Serial.print("Values: [");
  Serial.print(oldValue);
  Serial.print("], [");
  Serial.print(newValue);
  Serial.print("] dif:");
  Serial.println(dif);
}

void nextValue() {
  //Serial.print("Get next value from pin: ");
  //Serial.println(vInput);
  oldValue = newValue;
  newValue = analogRead(vInput);
}

int tick = 0;
void tickFunc() {
  tick++;
}

int HTTPPostFunc(char* URL, char* json) {
  HTTPClient http;
  Serial.print("[HTTP] begin...\n");
  // configure traged server and url
  //http.begin("https://192.168.1.179/test.html", "7a 9c f4 db 40 d3 62 5a 6e 21 bc 5c cc 66 c8 3e a1 45 59 38"); //HTTPS
  http.begin(URL); //HTTP

  Serial.print("[HTTP] POST...\n");
  // start connection and send HTTP header
  int httpCode;
  httpCode = http.sendRequest("POST", json);
   
  // httpCode will be negative on error
  if(httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
    Serial.printf("[HTTP] POST... code: %d\n", httpCode);
    
    // file found at server
    if(httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);
    }
  } 
  else {
    Serial.printf("[HTTP] PUT... failed, error: %s\n", http.errorToString(httpCode).c_str());
    return 1;
  }  
  http.end();
  return 0;
}

