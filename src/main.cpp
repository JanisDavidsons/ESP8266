#include <ESP8266WiFi.h> // Include the Wi-Fi library
#include <ESP8266WebServer.h>
#include <LittleFS.h> // Include the SPIFFS library
#include <WebSocketsServer.h>
#include <WiFiUdp.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "Temperature.h"

const char *ssid = "Janis_network";  // The SSID (name) of the Wi-Fi network you want to connect to
const char *password = "8378969948"; // The password of the Wi-Fi network

const int oneWireBus = 4;
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);
Temperature temperature(&sensors);

unsigned long intervalNTP = 60000; // Request NTP time every minute
unsigned long prevNTP = 0;
unsigned long lastNTPResponse = millis();
uint32_t timeUNIX = 0;
unsigned long prevActualTime = 0;

ESP8266WebServer server(80);    // Create a webserver object that listens for HTTP request on port 80
WebSocketsServer webSocket(81); // create a websocket server on port 81
WiFiUDP UDP;                    // Create an instance of the WiFiUDP class to send and receive NTP server address
File fsUploadFile;              // a File object to temporarily store the received file

String getContentType(String filename); // convert the file extension to the MIME type
const char *NTPServerName = "time.nist.gov";
IPAddress timeServerIP;           // IPAddress object to store ip
const int NTP_PACKET_SIZE = 48;   // NTP time stamp is in the first 48 bytes of the message
byte NTPBuffer[NTP_PACKET_SIZE];  // buffer to hold incoming and outgoing packets
bool handleFileRead(String path); // send the right file to the client (if it exists)
void handleFileUpload();          // upload a new file to the SPIFFS
void startServer();
void startUDP();
void sendNTPpacket(IPAddress &address);
uint32_t getTime();
inline int getHours(uint32_t actualTime);
inline int getMinutes(uint32_t actualTime);
inline int getSeconds(uint32_t actualTime);
void startWebSocket();
void startSPIFFS();
void startWiFi();
int getTemperature();

#define LED_RED 15 // specify the pins with an RGB LED connected
#define LED_GREEN 12
#define LED_BLUE 13

float currentTemperature = -127;
float previousTemp = -127;

void setup(void)
{

  pinMode(LED_RED, OUTPUT); // the pins with LEDs connected are outputs
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  Serial.begin(115200); // Start the Serial communication to send messages to the computer
  delay(10);
  Serial.println('\n');

  sensors.begin();

  startUDP();

  startWiFi(); // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection

  startSPIFFS(); // Start the SPIFFS and list all contents

  startWebSocket(); // Start a WebSocket server

  startServer(); // Start a HTTP server with a file read handler and an upload handler

  if (!WiFi.hostByName(NTPServerName, timeServerIP))
  { // Get the IP address of the NTP server
    Serial.println("DNS lookup failed. Rebooting.");
    Serial.flush();
    ESP.reset();
  }
  Serial.print("Time server IP:\t");
  Serial.println(timeServerIP);

  Serial.println("\r\nSending NTP request ...");
  sendNTPpacket(timeServerIP);

  server.on("/upload", HTTP_GET, []() {                 // if the client requests the upload page
    if (!handleFileRead("/upload.html"))                // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
  });

  server.on(
      "/upload", HTTP_POST,       // if the client posts to the upload page
      []() { server.send(200); }, // Send status 200 (OK) to tell the client we are ready to receive
      handleFileUpload            // Receive and save the file
  );

  server.onNotFound([]() {                              // If the client requests any URI
    if (!handleFileRead(server.uri()))                  // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
  });
}

bool rainbow = false; // The rainbow effect is turned off on startup

unsigned long prevMillis = millis();
int hue = 0;

void loop(void)
{
  unsigned long currentMillis = millis();
  if (currentMillis - prevNTP > intervalNTP)
  { // If a minute has passed since last NTP request
    prevNTP = currentMillis;
    Serial.println("\r\nSending NTP request ...");
    sendNTPpacket(timeServerIP); // Send an NTP request
  }

  uint32_t time = getTime(); // Check if an NTP response has arrived and get the (UNIX) time
  if (time)
  { // If a new timestamp has been received
    timeUNIX = time;
    Serial.print("NTP response:\t");
    Serial.println(timeUNIX);
    lastNTPResponse = currentMillis;
  }
  else if ((currentMillis - lastNTPResponse) > 3600000)
  {
    Serial.println("More than 1 hour since last NTP response. Rebooting.");
    Serial.flush();
    ESP.reset();
  }

  uint32_t actualTime = timeUNIX + (currentMillis - lastNTPResponse) / 1000;

  if (actualTime != prevActualTime && timeUNIX != 0)
  { // If a second has passed
    prevActualTime = actualTime;
    // Serial.printf("\rUTC time:\t%d:%d:%d   ", getHours(actualTime), getMinutes(actualTime), getSeconds(actualTime));
    // Serial.println("\n");

    bool hasChanged = temperature.requestOnBus();

    if (hasChanged)
    {
      String temp = temperature.getTempString();
      webSocket.broadcastTXT(temp);
    }
  }

  server.handleClient(); // Listen for HTTP requests from clients

  webSocket.loop();      // constantly check for websocket events
  server.handleClient(); // run the server

  if (rainbow)
  { // if the rainbow effect is turned on
    if (millis() > prevMillis + 32)
    {
      if (++hue == 360) // Cycle through the color wheel (increment by one degree every 32 ms)
        hue = 0;
      // setHue(hue); // Set the RGB LED to the right color
      prevMillis = millis();
    }
  }
}

String getContentType(String filename)
{
  if (filename.endsWith(".htm"))
    return "text/html";
  else if (filename.endsWith(".html"))
    return "text/html";
  else if (filename.endsWith(".css"))
    return "text/css";
  else if (filename.endsWith(".js"))
    return "application/javascript";
  else if (filename.endsWith(".png"))
    return "image/png";
  else if (filename.endsWith(".gif"))
    return "image/gif";
  else if (filename.endsWith(".jpg"))
    return "image/jpeg";
  else if (filename.endsWith(".ico"))
    return "image/x-icon";
  else if (filename.endsWith(".xml"))
    return "text/xml";
  else if (filename.endsWith(".pdf"))
    return "application/x-pdf";
  else if (filename.endsWith(".zip"))
    return "application/x-zip";
  else if (filename.endsWith(".gz"))
    return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path)
{ // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/"))
    path += "index.html";                    // If a folder is requested, send the index file
  String contentType = getContentType(path); // Get the MIME type
  String pathWithGz = path + ".gz";
  if (LittleFS.exists(pathWithGz) || LittleFS.exists(path))
  {                                       // If the file exists, either as a compressed archive, or normal
    if (LittleFS.exists(pathWithGz))      // If there's a compressed version available
      path += ".gz";                      // Use the compressed version
    File file = LittleFS.open(path, "r"); // Open the file
    server.streamFile(file, contentType); // Send it to the client
    file.close();                         // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path); // If the file doesn't exist, return false
  return false;
}

void handleFileUpload()
{ // upload a new file to the SPIFFS
  HTTPUpload &upload = server.upload();
  if (upload.status == UPLOAD_FILE_START)
  {
    String filename = upload.filename;
    if (!filename.startsWith("/"))
      filename = "/" + filename;
    Serial.print("handleFileUpload Name: ");
    Serial.println(filename);
    fsUploadFile = LittleFS.open(filename, "w"); // Open the file for writing in SPIFFS (create if it doesn't exist)
    filename = String();
  }
  else if (upload.status == UPLOAD_FILE_WRITE)
  {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  }
  else if (upload.status == UPLOAD_FILE_END)
  {
    if (fsUploadFile)
    {                       // If the file was successfully created
      fsUploadFile.close(); // Close the file again
      Serial.print("handleFileUpload Size: ");
      Serial.println(upload.totalSize);
      server.sendHeader("Location", "/success.html"); // Redirect the client to the success page
      server.send(303);
    }
    else
    {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t lenght)
{

  switch (type)
  {
  case WStype_DISCONNECTED:
    Serial.printf("[%u] Disconnected!\n", num);
    break;
  case WStype_CONNECTED:
  {
    IPAddress ip = webSocket.remoteIP(num);
    Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
    rainbow = false;
  }
  break;
  case WStype_TEXT: // if new text data is received
    Serial.printf("[%u] get Text: %s\n", num, payload);
    if (payload[0] == '#')
    {                                                                       // we get RGB data
      uint32_t rgb = (uint32_t)strtol((const char *)&payload[1], NULL, 16); // decode rgb data
      int r = ((rgb >> 20) & 0x3FF);                                        // 10 bits per color, so R: bits 20-29
      int g = ((rgb >> 10) & 0x3FF);                                        // G: bits 10-19
      int b = rgb & 0x3FF;                                                  // B: bits  0-9

      analogWrite(LED_RED, r); // write it to the LED output pins
      analogWrite(LED_GREEN, g);
      analogWrite(LED_BLUE, b);
    }
    else if (payload[0] == 'R')
    { // the browser sends an R when the rainbow effect is enabled
      rainbow = true;
    }
    else if (payload[0] == 'N')
    { // the browser sends an N when the rainbow effect is disabled
      rainbow = false;
    }
    else if (payload[0] == 'T')
    {
      temperature.requestOnBus();
      String tmp = temperature.getTempString();
      webSocket.broadcastTXT(tmp);
    }

    break;
  }
}

void startWiFi()
{
  WiFi.begin(ssid, password); // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(ssid);
  Serial.println(" ...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED)
  { // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i);
    Serial.print(' ');
  }

  Serial.println('\n');
  Serial.println("Connection established!");
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP()); // Send the IP address of the ESP8266 to the computer
}

void startSPIFFS()
{
  LittleFS.begin(); // Start the SPI Flash Files System
}

void startServer()
{
  server.begin(); // Actually start the server
  Serial.println("HTTP server started");
}

void startUDP()
{
  Serial.println("Starting UDP");
  UDP.begin(123); // Start listening for UDP messages on port 123
  Serial.print("Local port:\t");
  Serial.println(UDP.localPort());
  Serial.println();
}

void startWebSocket()
{
  webSocket.begin();                 // start the websocket server
  webSocket.onEvent(webSocketEvent); // if there's an incomming websocket message, go to function 'webSocketEvent'
  Serial.println("WebSocket server started.");
}

void setHue(int hue)
{                                 // Set the RGB LED to a given hue (color) (0° = Red, 120° = Green, 240° = Blue)
  hue %= 360;                     // hue is an angle between 0 and 359°
  float radH = hue * 3.142 / 180; // Convert degrees to radians
  float rf, gf, bf;

  if (hue >= 0 && hue < 120)
  { // Convert from HSI color space to RGB
    rf = cos(radH * 3 / 4);
    gf = sin(radH * 3 / 4);
    bf = 0;
  }
  else if (hue >= 120 && hue < 240)
  {
    radH -= 2.09439;
    gf = cos(radH * 3 / 4);
    bf = sin(radH * 3 / 4);
    rf = 0;
  }
  else if (hue >= 240 && hue < 360)
  {
    radH -= 4.188787;
    bf = cos(radH * 3 / 4);
    rf = sin(radH * 3 / 4);
    gf = 0;
  }
  int r = rf * rf * 1023;
  int g = gf * gf * 1023;
  int b = bf * bf * 1023;

  analogWrite(LED_RED, r); // Write the right color to the LED output pins
  analogWrite(LED_GREEN, g);
  analogWrite(LED_BLUE, b);
}

String formatBytes(size_t bytes)
{ // convert sizes in bytes to KB and MB
  if (bytes < 1024)
  {
    return String(bytes) + "B";
  }
  else if (bytes < (1024 * 1024))
  {
    return String(bytes / 1024.0) + "KB";
  }
  else if (bytes < (1024 * 1024 * 1024))
  {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  }
}

uint32_t getTime()
{
  if (UDP.parsePacket() == 0)
  { // If there's no response (yet)
    return 0;
  }
  UDP.read(NTPBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
  // Combine the 4 timestamp bytes into one 32-bit number
  uint32_t NTPTime = (NTPBuffer[40] << 24) | (NTPBuffer[41] << 16) | (NTPBuffer[42] << 8) | NTPBuffer[43];
  // Convert NTP time to a UNIX timestamp:
  // Unix time starts on Jan 1 1970. That's 2208988800 seconds in NTP time:
  const uint32_t seventyYears = 2208988800UL;
  // subtract seventy years:
  uint32_t UNIXTime = NTPTime - seventyYears;
  return UNIXTime;
}

void sendNTPpacket(IPAddress &address)
{
  memset(NTPBuffer, 0, NTP_PACKET_SIZE); // set all bytes in the buffer to 0
  // Initialize values needed to form NTP request
  NTPBuffer[0] = 0b11100011; // LI, Version, Mode
  // send a packet requesting a timestamp:
  UDP.beginPacket(address, 123); // NTP requests are to port 123
  UDP.write(NTPBuffer, NTP_PACKET_SIZE);
  UDP.endPacket();
}

inline int getSeconds(uint32_t UNIXTime)
{
  return UNIXTime % 60;
}

inline int getMinutes(uint32_t UNIXTime)
{
  return UNIXTime / 60 % 60;
}

inline int getHours(uint32_t UNIXTime)
{
  return UNIXTime / 3600 % 24;
}