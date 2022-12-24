
//*************************************************************************
//********************* LIBRARY INCLUSIONS - ALL **************************
//*************************************************************************
// Import required libraries
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <esp_now.h> // espnow library
#include <esp_wifi.h> // needed for resetting the mac address
#include <Arduino_JSON.h>
#include <HardwareSerial.h> // used for setting up the serial communications on non RX/TX pins
#include <EEPROM.h> // used for storing data even after reset, in flash/eeprom memory
#include <HTTPClient.h> // to update via http
#include <HTTPUpdate.h> // to update via http
#include <WiFiClientSecure.h> // security access to github
#include "cert.h" // used for github security
#include <Adafruit_DotStar.h>  // Because conditional #includes don't work w/Arduino sketches...
#include <SPI.h> // needed for oled display and dot stars
#include <Wire.h> // needed for oled display
#include <Adafruit_GFX.h> // needed for oled display
#include <Adafruit_SSD1306.h> // needed for oled display

// change based upon board version
int IRledPin =  21;    // LED connected to GPIO26, pin 21 for v5+ and pin 26 for v4 and earlier
// serial definitions for LoRa
#define SERIAL1_RXPIN 22 // TO LORA TX - 22 for v5 and 19 for 4-
#define SERIAL1_TXPIN 23 // TO LORA RX 23 all versions
const int motionSensor = 34;
#define DATAPIN    14 // DI/O for v4 boards it is 25, v5 is 14
#define CLOCKPIN   27 // CI/O
// dotstar definitions
#define NUMPIXELS 21 // Number of LEDs in strip
byte colorposession = 7;
Adafruit_DotStar strip(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BRG);
// Runs 10 LEDs at a time along strip, change by increasing the tail negative value
// This requires about 200 mA for all the 'on' pixels + 1 mA per 'off' pixel.
int      head  = 0, tail = -7; // Index of first 'on' and 'off' pixels
uint32_t color = 0xFF0000;      // 'On' color (starts red)
const byte IR_Sensor_Pin = 32; // this is int input pin for ir receiver, use pin 32 for v5 plus and 16 for v4 and earlier



// Set GPIOs for LED and PIR Motion Sensor
bool MOTIONDETECTED = false;


long Core0 = 0;
long Core1 = 0;
long PreviousCore0Check = 0;
long PreviousCore1Check = 0;

// define the number of bytes I'm using/accessing for eeprom
#define EEPROM_SIZE 102

//*************************************************************************
//******************** OTA UPDATES ****************************************
//*************************************************************************
String FirmwareVer = {"5.0"};
#define URL_fw_Version "https://raw.githubusercontent.com/LaserTagMods/autoupdate/main/jtowerversion.txt"
#define URL_fw_Bin "https://raw.githubusercontent.com/LaserTagMods/autoupdate/main/JTOWER.bin"

//#define URL_fw_Version "http://cade-make.000webhostapp.com/version.txt"
//#define URL_fw_Bin "http://cade-make.000webhostapp.com/firmware.bin"

bool OTAMODE = false;

// text inputs
const char* PARAM_INPUT_1 = "input1";
const char* PARAM_INPUT_2 = "input2";

void connect_wifi();
void firmwareUpdate();
int FirmwareVersionCheck();
bool UPTODATE = false;
int updatenotification;

//unsigned long previousMillis = 0; // will store last time LED was updated
unsigned long previousMillis_2 = 0;
const long otainterval = 1000;
const long mini_interval = 1000;

String OTAssid = "dontchangeme"; // network name to update OTA
String OTApassword = "dontchangeme"; // Network password for OTA

//*************************************************************************
//******************** DEVICE SPECIFIC DEFINITIONS ************************
//*************************************************************************
// These should be modified as applicable for your needs
int JBOXID = 101; // this is the device ID, guns are 0-63, controlle ris 98, jbox is 100-120
int DeviceSelector = JBOXID;
int TaggersOwned = 64; // how many taggers do you own or will play?
// Replace with your network credentials
String ssid = "JBOX#101";
const char* password = "123456789";

//*************************************************************************
//******************** VARIABLE DECLARATIONS ******************************
//*************************************************************************
int WebSocketData;
bool ledState = 0;
const int ledPin = 2;
int Menu[25]; // used for menu settings storage

int id = 0;
//int tempteamscore;
//int tempplayerscore;
int teamscore[4];
int playerscore[64];
int pid = 0;

// definitions for analyzing incoming brx serial data
String readStr; // the incoming data is stored as this string
char *ptr = NULL; // used for initializing and ending string break up.

//variabls for blinking an LED with Millis
const int led = 2; // ESP32 Pin to which onboard LED is connected
unsigned long previousMillis = 0;  // will store last time LED was updated
unsigned long ledpreviousMillis = 0;  // will store last time LED was updated
const long ledinterval = 1500;  // interval at which to blink (milliseconds)
unsigned long currentMillis0 = 0; // used for core 0 counter/timer.
unsigned long currentMillis1 = 0; // used for core 0 counter/timer.
unsigned long previousMillis0 = 0;  // will store last time LED was updated
unsigned long LastScoreUpdate = 0; // for score updates
long previousIRSend = 0; // for IR test

bool SCORESYNC = false; // used for initializing score syncronization
int ScoreRequestCounter = 0; // counter used for score syncronization
String ScoreTokenStrings[73]; // used for disecting incoming score string/char
int PlayerDeaths[64]; // used for score accumilation
int PlayerKills[64]; // used for score accumilation
int PlayerObjectives[64]; // used for score accumilation
int TeamKills[4]; // used for score accumilation
int TeamObjectives[4]; // used for score accumilation
int TeamDeaths[4];
String ScoreString = "0";
long ScorePreviousMillis = 0;
bool UPDATEWEBAPP = false; // used to update json scores to web app
long WebAppUpdaterTimer = 0;
int WebAppUpdaterProcessCounter = 0;

// timer settings
unsigned long CurrentMillis = 0;
int interval = 2000;
long PreviousMillis = 0;
int ESPNOWinterval = 20000;
int ESPNOWPreviousMillis = 0;

//*********************** LORA VARIABLE DELCARATIONS **********************
//*************************************************************************
String LoRaDataReceived; // string used to record data coming in
String tokenStrings[8]; // used to break out the incoming data for checks/balances
String LoRaDataToSend; // used to store data being sent from this device

bool LORALISTEN = true; // a trigger used to enable the radio in (listening)
bool ENABLELORATRANSMIT = false; // a trigger used to enable transmitting data

int sendcounter = 0;
int receivecounter = 0;

unsigned long lorapreviousMillis = 0;
const long lorainterval = 3000;

//*************************************************************************
//******************** IR RECEIVER VARIABLE DELCARATIONS ******************
//*************************************************************************
// Define Variables used for the game functions
int TeamID=99; // this is for team recognition, team 0 = red, team 1 = blue, team 2 = yellow, team 3 = green
int LastPosessionTeam = 99;
int gamestatus=0; // used to turn the ir reciever procedure on and off
int PlayerID=0; // used to identify player
int PID[6]; // used for recording player bits for ID decifering
int DamageID=0; // used to identify weapon
int ShotType=0; // used to identify shot type
int DID[8]; // used for recording weapon bits for ID decifering
int BID[4]; // used for recording shot type bits for ID decifering
int PowerID = 0; // used to identify power
int CriticalID = 0; // used to identify if critical
int BB[4]; // bullet type bits for decoding Ir
int PP[7]; // Player bits for decoding player id
int TT[2]; // team bits for decoding team 
int DD[8]; // damage bits for decoding damage
int CC[1]; // Critical hit on/off from IR
int UU[2]; // Power bits
int ZZ[3]; // parity bits depending on number of 1s in ir
int XX[1]; // bit to used to confirm brx ir is received
bool ENABLEIRRECEIVER = true; // enables and disables the ir receiver



//*************************************************************************
// ******************* GAME VARIABLE DELCARATIONS *************************
//*************************************************************************
// variables for gameplay and scoring:
int Function = 99; // set as default for basic domination game mode
int LastFunction = 0; // used to go back to function to execute an order (66)
bool ORDER66 = false; // for killing all players
int TeamScore[4]; // used to track team score
int PlayerScore[64]; // used to track individual player score
int JboxPlayerID[21]; // used for incoming jbox ids for players with posessions
int JboxTeamID[21]; // used for incoiming jbox ids for teams with posessions
int JboxInPlay[21]; // used as an identifier to verify if a jbox id is in play or not
int CapturableEmitterScore[4]; // used for accumulating points for capturability to alter team alignment
int RequiredCapturableEmitterCounter = 10;
bool FIVEMINUTEDOMINATION = false;
bool TENMINUTEDOMINATION = false;
bool FIVEMINUTETUGOWAR = false;
bool NEWTAGRECEIVED = false;
bool UPDATEWEPAPP = false;
bool GAMEOVER = false;
int CurrentDominationLeader = 99;
int PreviousDominationLeader = 99;

long PreviousDominationClock = 0;

bool MULTIBASEDOMINATION = false;
bool BASICDOMINATION = false; // a default game mode
bool CAPTURETHEFLAGMODE = false;
bool DOMINATIONCLOCK = false;
byte TempDomTagCounter = 0;
bool GasEm = false;
bool POSTDOMINATIONSCORETOBLYNK = false;
bool CAPTURABLEEMITTER = false;
bool ANYTEAM = false;

// IR LED VARIABLE DELCARATIONS
int B[4];
int P[7];
int T[3];
int D[8];
int C[1];
int X[2];
int Z[3];
int SyncLTTO = 3;
int LTTOA[7];

int BulletType = 0;
int Player = 0;
int Team = 2;
int Damage = 0;
int Critical = 0;
int Power = 0;

unsigned long irledpreviousMillis = 0;
long irledinterval = 5000;

// Function Execution
bool BASECONTINUOUSIRPULSE = false; // enables/disables continuous pulsing of ir
bool SINGLEIRPULSE = false; // enabled/disables single shot of enabled ir signal
bool TAGACTIVATEDIR = true; // enables/disables tag activated onetime IR, set as default for basic domination game
bool TAGACTIVATEDIRCOOLDOWN = false; // used to manage frequency of accessible base ir protocol features
long CoolDownStart = 0; // used for a delay timer
long CoolDownInterval = 0;

// IR Protocols
bool CONTROLPOINTLOST = true; // set as default for running basic domination game
bool CONTROLPOINTCAPTURED = false; // set as default for running basic domination game
bool MOTIONSENSOR = false;
bool SWAPBRX = false;
bool OWNTHEZONE = false;
bool HITTAG = false;
bool CAPTURETHEFLAG = false;
bool LIGHTDAMAGE = false;
bool MEDIUMDAMAGE = false;
bool HEAVYDAMAGE = false;
bool FRAG = false;
bool RESPAWNSTATION = false;
bool WRESPAWNSTATION = false;
bool MEDKIT = false;
bool LOOTBOX = false;
bool ARMORBOOST = false;
bool SHEILDS = false;
bool LTTORESPAWN = false;
bool LTTOOTZ = false;
bool LTTOHOSTED = false;
bool CUSTOMLTTOTAG = false;
bool LTTOGG = false;

//  CORE VARIABLE DELCARATIONS
unsigned long core0previousMillis = 0;
const long core0interval = 1000;

unsigned long core1previousMillis = 0;
const long core1interval = 3000;

//  RGB VARIABLE DELCARATIONS
int red = 33; // v4- = pin 17, v5+ = pin 33
int green = 25; // v4- = pin 21, v5+ = pin 25
int blue = 26; // v4- = pin 22, v5+ = pin 26
int RGBState = LOW;  // RGBState used to set the LED
bool RGBGREEN = false;
bool RGBOFF = false;
bool RGBRED = false;
bool RGBYELLOW = false;
bool RGBBLUE = false;
bool RGBPURPLE = false;
bool RGBCYAN = false;
bool RGBWHITE = true;

unsigned long rgbpreviousMillis = 0;
const long rgbinterval = 1000;

unsigned long changergbpreviousMillis = 0;
const long changergbinterval = 2000;
bool RGBDEMOMODE = true; // enables rgb demo flashing and changing of colors

//PIEZO VARIABLE DELCARATIONS
const int buzzer = 5; // buzzer pin, 5 for all versions
bool BUZZ = false; // set to true to send buzzer sound
bool LONGBUZZ = false; // set to true to sound buzzer
unsigned long buzzerpreviousMillis = 0;
byte BuzzerCounter = 5;

void StartShortBuzzer() {
  BUZZ = true; // activate buzzer/horn
  buzzerpreviousMillis = currentMillis1;
  digitalWrite(buzzer, HIGH);
}
void StartLongBuzzer() {
  LONGBUZZ = true;
  buzzerpreviousMillis = currentMillis1;
  digitalWrite(buzzer, HIGH);
}
void EndShortBuzzer() {
  BUZZ = false;
  digitalWrite(buzzer, LOW);
}
void EndLongBuzzer() {
  if (BuzzerCounter < 1) {
    LONGBUZZ = false;
    digitalWrite(buzzer, LOW);
    BuzzerCounter = 5;
  } else {
    digitalWrite(buzzer, LOW);
    delay(1000);
    digitalWrite(buzzer, HIGH);
    BuzzerCounter--;
    buzzerpreviousMillis = currentMillis1;
  }
}

// LTTO VARIABLES NEEDED
int PLTTO[2];
int TLTTO[3];
int DLTTO[2];
int LTTOTagType = 99;
int GearMod = 0;
int LTTODamage = 1;

//*****************************************************************************************
// ESP Now Objects:
//*****************************************************************************************
// for resetting mac address to custom address:
uint8_t newMACAddress[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0xff}; 

// REPLACE WITH THE MAC Address of your receiver, this is the address we will be sending to
uint8_t broadcastAddress[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x09};


// data to process espnow data
String TokenStrings[20];

long previngamemillis = 0;

// Game Settings - Config:
byte SettingsGameMode = 0; // default is team battle, 1 is royale
byte SettingsLives = 0; // default low, 1 is high
byte SettingsLighting = 0; // High is default, 1 is low
byte SettingsGameTime = 0; // off is default, 1 is on
byte SettingsRespawn = 0; // auto is default, 1 is manual
byte ObjectiveMode = 0;

String SendStartBeacon = "null";
String ConfirmBeacon = "null";


bool INGAME = false;

void formatMacAddress(const uint8_t *macAddr, char *buffer, int maxLength)
// Formats MAC Address
{
  snprintf(buffer, maxLength, "%02x:%02x:%02x:%02x:%02x:%02x", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
}

void receiveCallback(const uint8_t *macAddr, const uint8_t *data, int dataLen)
// Called when data is received
{
  // Only allow a maximum of 250 characters in the message + a null terminating byte
  char buffer[ESP_NOW_MAX_DATA_LEN + 1];
  int msgLen = min(ESP_NOW_MAX_DATA_LEN, dataLen);
  strncpy(buffer, (const char *)data, msgLen);
 
  // Make sure we are null terminated
  buffer[msgLen] = 0;
 
  // Format the MAC address
  char macStr[18];
  formatMacAddress(macAddr, macStr, 18);
 
  // Send Debug log message to the serial port
  Serial.printf("Received message from: %s - %s\n", macStr, buffer);
  
  // store and separate the buffer
  String IncomingData = String(buffer);
  char *ptr = strtok((char*)IncomingData.c_str(), ","); // looks for commas as breaks to split up the string
  int index = 0;
  while (ptr != NULL)
  {
    TokenStrings[index] = ptr; // saves the individual characters divided by commas as individual strings
    index++;
    ptr = strtok(NULL, ",");  // takes a list of delimiters
  }
  Serial.println("We have found " + String(index ) + " tokens");
  int stringnumber = 0;
  while (stringnumber < index) {
    Serial.print("Token " + String(stringnumber) + ": ");
    Serial.println(TokenStrings[stringnumber]);
    stringnumber++;
  }
  if (TokenStrings[1] == "69") {
    Serial.println("Received a game over winner Announcement");
    Serial.println("reset scores button pressed");
    Serial.println("All scores reset");
    ResetScores();
    rgbBlink();
    colorposession = 7;
  }
}
 
void sentCallback(const uint8_t *macAddr, esp_now_send_status_t status)
// Called when data is sent
{
  char macStr[18];
  formatMacAddress(macAddr, macStr, 18);
  Serial.print("Last Packet Sent to: ");
  Serial.println(macStr);
  Serial.print("Last Packet Send Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}
 
void broadcast(const String &message)
// Emulates a broadcast
{
  // Broadcast a message to every device in range
  uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  esp_now_peer_info_t peerInfo = {};
  memcpy(&peerInfo.peer_addr, broadcastAddress, 6);
  if (!esp_now_is_peer_exist(broadcastAddress))
  {
    esp_now_add_peer(&peerInfo);
  }
  // Send message
  esp_err_t result = esp_now_send(broadcastAddress, (const uint8_t *)message.c_str(), message.length());
 
  // Print results to serial monitor
  if (result == ESP_OK)
  {
    Serial.println("Broadcast message success");
  }
  else if (result == ESP_ERR_ESPNOW_NOT_INIT)
  {
    Serial.println("ESP-NOW not Init.");
  }
  else if (result == ESP_ERR_ESPNOW_ARG)
  {
    Serial.println("Invalid Argument");
  }
  else if (result == ESP_ERR_ESPNOW_INTERNAL)
  {
    Serial.println("Internal Error");
  }
  else if (result == ESP_ERR_ESPNOW_NO_MEM)
  {
    Serial.println("ESP_ERR_ESPNOW_NO_MEM");
  }
  else if (result == ESP_ERR_ESPNOW_NOT_FOUND)
  {
    Serial.println("Peer not found.");
  }
  else
  {
    Serial.println("Unknown error");
  }
}

// object to used to change esp default mac to custom mac
void ChangeMACaddress() {
  //WiFi.mode(WIFI_STA);
  
  Serial.print("[OLD] ESP32 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  
  esp_wifi_set_mac(ESP_IF_WIFI_STA, &newMACAddress[0]);
  
  Serial.print("[NEW] ESP32 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
}

void IntializeESPNOW() {
  // run the object for changing the esp default mac address
  ChangeMACaddress();
  
  // Initialize ESP-NOW
  if (esp_now_init() == ESP_OK)
  {
    Serial.println("ESP-NOW Init Success");
    esp_now_register_recv_cb(receiveCallback);
    esp_now_register_send_cb(sentCallback);
  }
  else
  {
    Serial.println("ESP-NOW Init Failed");
    delay(3000);
    ESP.restart();
  }
}

// **************************************************
// DOT STARS LED STRIP STUFF
//***************************************************
void SetupDotStars() {
  #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
    clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
  #endif

  strip.begin(); // Initialize pins for output
  strip.show();  // Turn all LEDs off ASAP
}
bool RUNTHESNAKE = true;
long SnakeMillis = 0;
void RunLEDSnake() {
  //***************************************************************************************
  if (currentMillis0 - SnakeMillis > 20) {
    SnakeMillis = currentMillis0;  
  strip.setPixelColor(head, color); // 'On' pixel at head
  strip.setPixelColor(tail, 0);     // 'Off' pixel at tail
  strip.show();                     // Refresh strip
  //delay(20);                        // Pause 20 milliseconds (~50 FPS)
  if(++head >= NUMPIXELS) {         // Increment head index.  Off end of strip?
    head = 0;                       //  Yes, reset head index to start
    //if (colorposession == 7) {
      //colorposession = 0;
    //} else {
      //colorposession++;
    //}
    if (colorposession == 0) { color = 0x00FF00;} // color set to red
    if (colorposession == 1) { color = 0x0000FF;} // color set to blue
    if (colorposession == 2) { color = 0xFFFF00;} // color set to yellow
    if (colorposession == 3) { color = 0xFF0000;} // color set to green
    if (colorposession == 4) { color = 0xFFFFFF;} // color set to white
    if (colorposession == 5) { color = 0x000FFFF;} // color set to Magenta
    if (colorposession == 6) { color = 0xFF00FF;} // color set to Cyan
    if (colorposession == 7) { color = 0x0000000;} // color set to off
    //if((color >>= 8) == 0)          //  Next color (G->R->B) ... past blue now?
      //color = 0xFF0000;             //   Yes, reset to red
  }
  if(++tail >= NUMPIXELS) tail = 0; // Increment, reset tail index
  }
}
//****************************************************
// WebServer 
//****************************************************

JSONVar board;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncEventSource events("/events");

// callback function that will be executed when data is received
void UpdateWebApp0() { 
  id = 0; // team id place holder
  pid = 0; // player id place holder
  while (id < 4) {
    board["temporaryteamobjectives"+String(id)] = TeamScore[id];
    id++;
  }
  String jsonString = JSON.stringify(board);
  events.send(jsonString.c_str(), "new_readings", millis());
}
void UpdateWebApp2() { 
  // now we calculate and send leader board information
      //Serial.println("Game Highlights:");
      //Serial.println();
      int ObjectiveLeader[3];
      int LeaderScore[3];
      // first Leader for Kills:
      int kmax=0; 
      int highest=0;
      
      // Now Leader for Objectives:
      kmax=0; highest=0;
      for (int k=0; k<64; k++)
      if (PlayerScore[k] > highest) {
        highest = PlayerScore[k];
        kmax = k;
      }
      //  max now contains the largest kills value
      // kmax contains the index to the largest kills value.
      ObjectiveLeader[0] = kmax; // setting the slot 0 for the first place holder
      LeaderScore[0] = highest; // setts the leader score as a temp stored data for now      
      PlayerScore[kmax]=0; // resetting the first place leader's score
      // we do it again for second place
      kmax=0; highest=0; // starting over again but now the highest score is gone
      for (int k=0; k<64; k++)
      if (PlayerScore[k] > highest) {
        highest = PlayerScore[k];
        kmax = k;
      }
      //  max now contains the largest kills value
      // kmax contains the index to the largest kills value.
      ObjectiveLeader[1] = kmax; // setting the slot 1 for the second place holder
      LeaderScore[1] = highest; // setts the leader score as a temp stored data for now      
      PlayerScore[kmax]=0; // resetting the second place leader's score
      // one final time for third place
      kmax=0; highest=0; // starting over again but now the highest score is gone
      for (int k=0; k<64; k++)
      if (PlayerScore[k] > highest) {
        highest = PlayerScore[k];
        kmax = k;
      }
      //  max now contains the largest kills value
      // kmax contains the index to the largest kills value.
      ObjectiveLeader[2] = kmax; // setting the slot 2 for the first place holder
      LeaderScore[2] = highest; // setts the third score as a temp stored data for now      
      PlayerScore[kmax]=0; // resetting the first place leader's score
      // now we send the updates to the blynk app
      //Serial.println("The Dominator, Players with the most objective points:");
      //Serial.println("1st place: Player " + String(ObjectiveLeader[0]) + " with " + String(LeaderScore[0]));
      //Serial.println("2nd place: Player " + String(ObjectiveLeader[1]) + " with " + String(LeaderScore[1]));
      //Serial.println("3rd place: Player " + String(ObjectiveLeader[2]) + " with " + String(LeaderScore[2]));
      //Serial.println();      
      board["ObjectiveLeader0"] = ObjectiveLeader[0];
      board["Leader0Objectives"] = LeaderScore[0];
      board["ObjectiveLeader1"] = ObjectiveLeader[1];
      board["Leader1Objectives"] = LeaderScore[1];
      board["ObjectiveLeader2"] = ObjectiveLeader[2];
      board["Leader2Objectives"] = LeaderScore[2];
      // now get the player's scores back where they should be:
      PlayerScore[ObjectiveLeader[0]] = LeaderScore[0];
      PlayerScore[ObjectiveLeader[1]] = LeaderScore[1];
      PlayerScore[ObjectiveLeader[2]] = LeaderScore[2];
        
  String jsonString = JSON.stringify(board);
  events.send(jsonString.c_str(), "new_readings", millis());
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>ESP Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="icon" href="data:,">
  <style>
  html {
    font-family: Arial;
    display: inline-block;
    text-align: center;
  }
  p {  font-size: 1.2rem;}
  h1 {
    font-size: 1.8rem;
    color: white;
  }
  h2{
    font-size: 1.5rem;
    font-weight: bold;
    color: #143642;
  }
  .topnav {
    overflow: hidden;
    background-color: #143642;
  }
  body {
    margin: 0;
  }
  .content {
    padding: 30px;
    max-width: 600px;
    margin: 0 auto;
  }
  .card {
    background-color: #F8F7F9;;
    box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5);
    padding-top:10px;
    padding-bottom:20px;
  }
  .button {
    padding: 15px 50px;
    font-size: 24px;
    text-align: center;
    outline: none;
    color: #fff;
    background-color: #0f8b8d;
    border: none;
    border-radius: 5px;
    -webkit-touch-callout: none;
    -webkit-user-select: none;
    -khtml-user-select: none;
    -moz-user-select: none;
    -ms-user-select: none;
    user-select: none;
    -webkit-tap-highlight-color: rgba(0,0,0,0);
   }
   /*.button:hover {background-color: #0f8b8d}*/
   .button:active {
     background-color: #0f8b8d;
     box-shadow: 2 2px #CDCDCD;
     transform: translateY(2px);
   }
   .state {
     font-size: 1.5rem;
     color:#8c8c8c;
     font-weight: bold;
   }
   .stopnav { overflow: hidden; background-color: #2f4468; color: white; font-size: 1.7rem; }
   .scontent { padding: 20px; }
   .scard { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
   .scards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
   .sreading { font-size: 2.8rem; }
   .spacket { color: #bebebe; }
   .scard.red { color: #FC0000; }
   .scard.blue { color: #003DFC; }
   .scard.yellow { color: #E5D200; }
   .scard.green { color: #00D02C; }
   .scard.black { color: #000000; }
  </style>
<title>JEDGE 3.0</title>
<meta name="viewport" content="width=device-width, initial-scale=1">
<link rel="icon" href="data:,">
</head>
<body>
  <div class="topnav">
    <h1>JTOWER 5.2</h1>
  </div>
  <div class="content">
    <div class="card">
      <h2>Gear Selector</h2>
      <p><select name="Gear" id="GearID">
        <option value="100">BRX</option>
        <option value="101">Evolver</option>
        <option value="102">LTTO</option>
        <option value="103">Battle Rifle Pro</option>
        <option value="104">Laser Wars</option>
        <option value="105">Ops Pro</option>
        </select>
      </p>
      <h2>Primary Function</h2>
      <p><select name="primary" id="primaryid">
        <option value="0">Basic Domination</option>
        <option value="1">5 Minute Domination</option>
        <option value="2">10 Minute Domination</option>
        <option value="0c">Domination Node - LoRa</option>
        <option value="0a">Domination Node - ESPNOW</option>
        <option value="0b">Domination Master</option>
        <option value="3">Shots Accumulator</option>
        <option value="4a">Evolver Capture The Flag - Red</option>
        <option value="4b">Evolver Capture The Flag - Blue</option>
        <option value="4c">Evolver Capture The Flag - Yellow</option>
        <option value="4d">Evolver Capture The Flag - Green</option>
        <option value="4e">Evolver Respawn - Cover Station</option>
        <option value="4f">Evolver Own The Zone</option>
        <option value="4g">Evolver Medic - BRoyale Checkpoint</option>
        <option value="4h">Evolver Upgrade Station</option>
        <option value="16">Evolver IR Test Mode</option>
        <option value="11">Own The Zone</option>
        <option value="5a">BRX Respawn Station - Red</option>
        <option value="5b">BRX Respawn Station - Blue</option>
        <option value="5d">BRX Respawn Station - Green</option>
        <option value="5c">BRX Respawn Station - Yellow</option>
        <option value="9">Capturable Respawn Station</option>
        <option value="10">Loot Box</option>
        <option value="11">Own The Zone</option>
        <option value="12">Gas Drone - 5 Minute</option>
        <option value="13">BRX Heavy Damage</option>
        <option value="14">BRX Boost Box</option>
        <option value="5e">BRP Respawn Station - All</option>
        <option value="IRDebug">IR Debug</option>        
        </select>
      </p>
      <h2>Motion Activation</h2>
      <p><select name="motion" id="motionid">
        <option value="xyz">Select</option>
        <option value="20">On</option>
        <option value="21">Off</option>
        </select>
      </p>
      <p><button id="resetscores" class="button">Reset Domination Scores</button></p>
      <p><button id="killplayers" class="button">End Game For All</button></p>
      <p><button id="playball" class="button">Start New Game</button></p>
    </div>
  </div>
  <div class="stopnav">
    <h3>Score Reporting</h3>
  </div>
  <div class="scontent">
    <div class="scards">
      <div class="scard red">
        <h4>Alpha Team</h4>
        <p><span class="reading"> Points:<span id="to0"></span></p>
      </div>
      <div class="scard blue">
        <h4>Bravo Team</h4><p>
        <p><span class="reading"> Points:<span id="to1"></span></p>
      </div>
      <div class="scard yellow">
        <h4>Charlie Team</h4>
        <p><span class="reading"> Points:<span id="to2"></span></p>
      </div>
      <div class="scard green">
        <h4>Delta Team</h4>
        <p><span class="reading"> Points:<span id="to3"></span></p>
      </div>
    </div>
  </div>
  <div class="stopnav">
    <h3>Game Highlights</h3>
  </div>
  <div class="scontent">
    <div class="scards">
      <div class="scard black">
      <h2>Most Points</h2>
        <p><span class="reading">Player <span id="MO0"></span><span class="reading"> : <span id="MO10"></span></p>
        <p><span class="reading">Player <span id="MO1"></span><span class="reading"> : <span id="MO11"></span></p>
        <p><span class="reading">Player <span id="MO2"></span><span class="reading"> : <span id="MO12"></span></p>
      </div>
    </div>
  </div>
  <div class="topnav">
    <h1>JBOX DEBUG</h1>
  </div>
  <div class="content">
    <div class="card">
    <p><button id="loratest" class="button">Test Lora</button></p>
    <h2>Device ID Assignment</h2>
      <p><select name="devicename" id="devicenameid">
        <option value="965">Select JBOX ID</option>
        <option value="900">JBOX 100</option>
        <option value="901">JBOX 101</option>
        <option value="902">JBOX 102</option>
        <option value="903">JBOX 103</option>
        <option value="904">JBOX 104</option>
        <option value="905">JBOX 105</option>
        <option value="906">JBOX 106</option>
        <option value="907">JBOX 107</option>
        <option value="908">JBOX 108</option>
        <option value="909">JBOX 109</option>
        <option value="910">JBOX 110</option>
        <option value="911">JBOX 111</option>
        <option value="912">JBOX 112</option>
        <option value="913">JBOX 113</option>
        <option value="914">JBOX 114</option>
        <option value="915">JBOX 115</option>
        <option value="916">JBOX 116</option>
        <option value="917">JBOX 117</option>
        <option value="918">JBOX 118</option>
        <option value="919">JBOX 119</option>
        <option value="920">JBOX 120</option>
        </select>
      </p>
      <form action="/get">
        wifi ssid: <input type="text" name="input1">
        <input type="submit" value="Submit">
      </form><br>
      <form action="/get">
        wifi pass: <input type="text" name="input2">
        <input type="submit" value="Submit">
      </form><br>
      <p><button id="otaupdate" class="button">OTA Firmware Refresh</button></p>
    </div>
  </div>
  
<script>
if (!!window.EventSource) {
 var source = new EventSource('/events');
 
 source.addEventListener('open', function(e) {
  console.log("Events Connected");
 }, false);
 source.addEventListener('error', function(e) {
  if (e.target.readyState != EventSource.OPEN) {
    console.log("Events Disconnected");
  }
 }, false);
 
 source.addEventListener('message', function(e) {
  console.log("message", e.data);
 }, false);
 
 source.addEventListener('new_readings', function(e) {
  console.log("new_readings", e.data);
  var obj = JSON.parse(e.data);
  document.getElementById("to0").innerHTML = obj.temporaryteamobjectives0;
  document.getElementById("to1").innerHTML = obj.temporaryteamobjectives1;
  document.getElementById("to2").innerHTML = obj.temporaryteamobjectives2; 
  document.getElementById("to3").innerHTML = obj.temporaryteamobjectives3;
  
  document.getElementById("MO0").innerHTML = obj.ObjectiveLeader0;
  document.getElementById("MO10").innerHTML = obj.Leader0Objectives;
  document.getElementById("MO1").innerHTML = obj.ObjectiveLeader1;
  document.getElementById("MO11").innerHTML = obj.Leader1Objectives;
  document.getElementById("MO2").innerHTML = obj.ObjectiveLeader2;
  document.getElementById("MO12").innerHTML = obj.Leader2Objectives;

 }, false);
}
  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;
  window.addEventListener('load', onLoad);
  function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage; // <-- add this line
  }
  function onOpen(event) {
    console.log('Connection opened');
  }
  function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
  }
  function onMessage(event) {
    var state;
    
  }
  
  function onLoad(event) {
    initWebSocket();
    initButton();
  }
  function initButton() {
    document.getElementById('GearID').addEventListener('change', handlegear, false);
    document.getElementById('primaryid').addEventListener('change', handleprimary, false);
    document.getElementById('motionid').addEventListener('change', handlemotion, false);
    document.getElementById('resetscores').addEventListener('click', toggle14s);
    document.getElementById('otaupdate').addEventListener('click', toggle14o);
    document.getElementById('loratest').addEventListener('click', toggle14c);
    document.getElementById('killplayers').addEventListener('click', toggle14a);
    document.getElementById('playball').addEventListener('click', toggle14b);
    document.getElementById('devicenameid').addEventListener('change', handledevicename, false);
    
  }
  function toggle14s(){
    websocket.send('toggle14s');
  }
  function toggle14o(){
    websocket.send('toggle14o');
  }
  function toggle14a(){
    websocket.send('toggle14a');
  }
  function toggle14b(){
    websocket.send('toggle14b');
  }
  function toggle14c(){
    websocket.send('toggle14c');
  }
  function handlegear() {
    var xb = document.getElementById("GearID").value;
    websocket.send(xb);
  }
  function handleprimary() {
    var xa = document.getElementById("primaryid").value;
    websocket.send(xa);
  }
  function handlemotion() {
    var xc = document.getElementById("motionid").value;
    websocket.send(xc);
  }
  function handledevicename() {
    var xp = document.getElementById("devicenameid").value;
    websocket.send(xp);
  }
</script>
</body>
</html>
)rawliteral";


void notifyClients() {
  ws.textAll(String(ledState));
}
void notifyClients1() {
  ws.textAll(String(WebSocketData));
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  Serial.println("handling websocket message");
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "toggle14s") == 0) { // game reset
      Serial.println("reset scores button pressed");
      Serial.println("All scores reset");
      ResetScores();
      rgbBlink();
      colorposession = 7;
      // need to send a command to stop and reset other domination boxes as well
    }
    if (strcmp((char*)data, "toggle14b") == 0) { // 
      Serial.println("Return to normal functions");
      // reset esp
      Serial.println("Sent Data Via ESPNOW");
      ORDER66 = false;
      LoRaDataToSend = "ENDORDER66";
      TransmitLoRa();
      ESP.restart();      
    }
    if (strcmp((char*)data, "toggle14c") == 0) { // 
      Serial.println("Test Lora");
      LoRaDataToSend = "TEST";
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("Test Listen LoRa");
      rgbBlink();
      LORALISTEN = true;
      ShowGameMode();
      if (colorposession == 7) {
        colorposession = 0;
      } else {
        colorposession++;
      }
      TransmitLoRa();     
    }
    if (strcmp((char*)data, "toggle14o") == 0) { // update firmware
      Serial.println("OTA Update Mode");
      rgbBlink();
      EEPROM.write(0, 1); // reset OTA attempts counter
      EEPROM.commit(); // Store in Flash
      // have other boxes in proximity update as well:
      delay(2000);
      ESP.restart();
      //INITIALIZEOTA = true;
    }
    if (strcmp((char*)data, "0") == 0) {
      Function = 0;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("Basic Domination");
      rgbBlink();
       
      colorposession = 7;
      ShowGameMode();
    }
    if (strcmp((char*)data, "0c") == 0) {
      Function = 4;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("Domination Node - LoRa");
      rgbBlink();
      LORALISTEN = true;
      colorposession = 7;
      ShowGameMode();
    }
    if (strcmp((char*)data, "0a") == 0) {
      Function = 5;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("Domination Node - ESPNOW");
      rgbBlink();
       
      colorposession = 7;
      ShowGameMode();
    }
    if (strcmp((char*)data, "0b") == 0) {
      Function = 6;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("Domination Master");
      rgbBlink();
      LORALISTEN = true;
      colorposession = 7;
      ShowGameMode();
    }
    if (strcmp((char*)data, "1") == 0) {
      Function = 1;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("5 Minute Basic Domination");
      rgbBlink();
       
      colorposession = 7;
      ShowGameMode();
    }
    if (strcmp((char*)data, "2") == 0) {
      Function = 2;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("10 Minute Basic Domination");
      rgbBlink();
       
      colorposession = 7;
      ShowGameMode();
    }
    if (strcmp((char*)data, "3") == 0) {
      Function = 3;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("Shot Accumulator Mode");
      rgbBlink();
      colorposession = 7;
      ShowGameMode();
    }
    if (strcmp((char*)data, "4a") == 0) {
      Function = 40;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("Evolver Capture The Flag - red");
      BulletType = 14;
      Damage = 0; // Damage
      Team = 0;
      Critical = 0;
      resetRGB();
      RGBRED = true; colorposession = 0;
      rgbBlink();
       
      ShowGameMode();
    }
    if (strcmp((char*)data, "4b") == 0) {
      Function = 41;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("Evolver Capture The Flag - blue");
      BulletType = 14;
      Critical = 0;
      Damage = 0;
      Team = 1;
      resetRGB();
      RGBBLUE = true; colorposession = 1;
      rgbBlink();
       
      
      ShowGameMode();
    }
    if (strcmp((char*)data, "4c") == 0) {
      Function = 42;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("Evolver Capture The Flag - yellow");
      BulletType = 14;
      Critical = 0;
      Damage = 0;
      Team = 2;
      resetRGB();
      RGBYELLOW = true; colorposession = 2;
      rgbBlink();
       
      
      ShowGameMode();
    }
    if (strcmp((char*)data, "4d") == 0) {
      Function = 43;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("Evolver Capture The Flag - green");
      BulletType = 14;
      Damage = 0;
      Team = 3;  
      Critical = 0;
      resetRGB(); 
      RGBGREEN = true; colorposession = 3;   
      rgbBlink();
       
      
      ShowGameMode();
    }
    if (strcmp((char*)data, "4e") == 0) {
      Function = 44;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("Evolver Respawn Station");
      BulletType = 10;
      Damage = 0;
      Team = 2;  
      resetRGB(); 
      RGBWHITE = true; colorposession = 4;   
      rgbBlink();
       
      
      ShowGameMode();
    }
    if (strcmp((char*)data, "4f") == 0) {
      Function = 45;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("Evolver OTZ Station");
      BulletType = 12;
      Damage = 0;
      Team = 2;  
      resetRGB(); 
      RGBPURPLE = true; colorposession = 5;   
      rgbBlink();
       
      
      ShowGameMode();
    }
    if (strcmp((char*)data, "4g") == 0) {
      Function = 46;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("Evolver Medic Station");
      BulletType = 13;
      Damage = 100;
      Team = 2;  
      resetRGB(); 
      RGBCYAN = true; colorposession = 6;   
      rgbBlink();
       
      
      ShowGameMode();
    }
    if (strcmp((char*)data, "4h") == 0) {
      Function = 47;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("Evolver Upgrade Station");
      BulletType = 11;
      Damage = 0;
      Team = 2;  
      resetRGB(); 
      RGBYELLOW = true; colorposession = 2;   
      rgbBlink();
       
      
      ShowGameMode();
    }
    if (strcmp((char*)data, "5a") == 0) {
      Function = 50;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("BRX Red Respawn Station");
      BulletType = 15;
      Damage = 6;
      Team = 0; 
      Player = 63;
      Critical = 1;
      Power = 0;
      resetRGB(); 
      RGBRED = true; colorposession = 0;   
      rgbBlink();
       
      
      ShowGameMode();
    }
    if (strcmp((char*)data, "5b") == 0) {
      Function = 51;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("BRX Blue Respawn Station");
      BulletType = 15;
      Damage = 6;
      Team = 1; 
      Player = 63;
      Critical = 1;
      Power = 0;
      resetRGB(); 
      RGBBLUE = true; colorposession = 1;   
      rgbBlink();
       
      
      ShowGameMode();
    }
    if (strcmp((char*)data, "5c") == 0) {
      Function = 52;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("BRX Yellow Respawn Station");
      BulletType = 15;
      Damage = 6;
      Team = 2; 
      Player = 63;
      Critical = 1;
      Power = 0;
      resetRGB(); 
      RGBYELLOW = true; colorposession = 2;   
      rgbBlink();
       
      
      ShowGameMode();
    }
    if (strcmp((char*)data, "5d") == 0) {
      Function = 53;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("BRX Green Respawn Station");
      BulletType = 15;
      Damage = 6;
      Team = 3; 
      Player = 63;
      Critical = 1;
      Power = 0;
      resetRGB(); 
      RGBGREEN = true; colorposession = 3;   
      rgbBlink();
       
      
      ShowGameMode();
    }
    if (strcmp((char*)data, "5e") == 0) {
      Function = 54;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("BRP ALL Respawn Station");
      BulletType = 15;
      Damage = 0;
      Team = 6; 
      Player = 1;
      Critical = 0;
      //Power = 0;
      resetRGB(); 
      RGBWHITE = true; colorposession = 4;   
      rgbBlink();
      ShowGameMode();
    }
    if (strcmp((char*)data, "6") == 0) {
      Function = 6;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("Respawn Station Blue ");
      rgbBlink();
       
      
      ShowGameMode();
    }
    if (strcmp((char*)data, "7") == 0) {
      Function = 8;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("Respawn Station Green ");
      rgbBlink();
       
      
      ShowGameMode();
    }
    if (strcmp((char*)data, "8") == 0) {
      Function = 8;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("Respawn Station Yellow");
      rgbBlink();
       
      
      ShowGameMode();
    }
    if (strcmp((char*)data, "9") == 0) {
      Function = 9;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("Capturable Respawn Station");
      rgbBlink();
       
      
      ShowGameMode();
    }
    if (strcmp((char*)data, "10") == 0) {
      Function = 10;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("Own The Zone");
      rgbBlink();
       
      
      ShowGameMode();
    }
    if (strcmp((char*)data, "11") == 0) {
      Function = 11;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("Gas Drone");
      rgbBlink();
       
      
      ShowGameMode();
    }
    if (strcmp((char*)data, "12") == 0) {
      Function = 12;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("Gas Drone - 5 minutes");
      rgbBlink();
       
      
      ShowGameMode();
    }
    if (strcmp((char*)data, "13") == 0) {
      Function = 13;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("BRX pROXIMITY mINE");
      BulletType = 0;
      Damage = 100;
      Team = 2; 
      Player = 63;
      Critical = 1;
      Power = 0;
      resetRGB(); 
      RGBCYAN = true; colorposession = 6;   
      rgbBlink();
       
      
      ShowGameMode();
    }
    if (strcmp((char*)data, "14") == 0) {
      Function = 14;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("BRX Perk Box");
      BulletType = 3;
      Damage = 10;
      Team = 3; 
      Player = 63;
      Critical = 0;
      Power = 1;
      resetRGB(); 
      RGBPURPLE = true; colorposession = 5;   
      rgbBlink();
       
      
      ShowGameMode();
    }
    if (strcmp((char*)data, "15") == 0) {
      Function = 15;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("Test Listen LoRa");
      rgbBlink();
      LORALISTEN = true;
      ShowGameMode();
    }
    if (strcmp((char*)data, "16") == 0) {
      Function = 16;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("Evolver Capture The Flag - red");
      BulletType = 14;
      Damage = 0;
      Team = 0;
      Critical = 0;
      resetRGB();
      RGBRED = true; colorposession = 0;
      rgbBlink();
       
      
      ShowGameMode();
    }
    if (strcmp((char*)data, "100") == 0) {
      GearMod = 0;
      ResetScores();
      EEPROM.write(2, GearMod);
      EEPROM.commit();
      Serial.println("Set Gear Selection to BRX");
      rgbBlink();
      ShowGameMode();
    }
    if (strcmp((char*)data, "101") == 0) {
      GearMod = 1;
      ResetScores();
      EEPROM.write(2, GearMod);
      EEPROM.commit();
      Serial.println("Set Gear Selection to Evolver");
      rgbBlink();
      ShowGameMode();
    }
    if (strcmp((char*)data, "102") == 0) {
      GearMod = 2;
      ResetScores();
      EEPROM.write(2, GearMod);
      EEPROM.commit();
      Serial.println("Set Gear Selection to LTTO");
      rgbBlink();
      ShowGameMode();
    }
    if (strcmp((char*)data, "103") == 0) {
      GearMod = 3;
      ResetScores();
      EEPROM.write(2, GearMod);
      EEPROM.commit();
      Serial.println("Set Gear Selection to Battle Rifle Pro");
      rgbBlink();
      ShowGameMode();
    }    
    if (strcmp((char*)data, "IRDebug") == 0) {
      Function = 99;
      ResetScores();
      EEPROM.write(1, Function);
      EEPROM.commit();
      Serial.println("IR Debug");
      resetRGB(); 
      RGBWHITE = true; colorposession = 4;   
      rgbBlink();
      ShowGameMode();
    }
    if (strcmp((char*)data, "104") == 0) {
      GearMod = 4;
      ResetScores();
      EEPROM.write(2, GearMod);
      EEPROM.commit();
      Serial.println("Set Gear Selection to LaserWars");
      rgbBlink();
      ShowGameMode();
    }
    if (strcmp((char*)data, "105") == 0) {
      GearMod = 5;
      ResetScores();
      EEPROM.write(2, GearMod);
      EEPROM.commit();
      Serial.println("Set Gear Selection to Ops Pro");
      rgbBlink();
      ShowGameMode();
    }
    if (strcmp((char*)data, "900") == 0) {
      JBOXID = 100;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
      rgbBlink();
    }
    if (strcmp((char*)data, "901") == 0) {
      JBOXID = 101;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "902") == 0) {
      JBOXID = 102;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
      rgbBlink();
    }
    if (strcmp((char*)data, "903") == 0) {
      JBOXID = 103;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
      rgbBlink();
    }
    if (strcmp((char*)data, "904") == 0) {
      JBOXID = 104;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
      rgbBlink();
    }
    if (strcmp((char*)data, "905") == 0) {
      JBOXID = 105;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
      rgbBlink();
    }
    if (strcmp((char*)data, "906") == 0) {
      JBOXID = 106;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
      rgbBlink();
    }
    if (strcmp((char*)data, "907") == 0) {
      JBOXID = 107;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
      rgbBlink();
    }
    if (strcmp((char*)data, "908") == 0) {
      JBOXID = 108;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
      rgbBlink();
    }
    if (strcmp((char*)data, "909") == 0) {
      JBOXID = 109;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
      rgbBlink();
    }
    if (strcmp((char*)data, "910") == 0) {
      JBOXID = 110;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
      rgbBlink();
    }
    if (strcmp((char*)data, "911") == 0) {
      JBOXID = 111;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
      rgbBlink();
    }
    if (strcmp((char*)data, "912") == 0) {
      JBOXID = 112;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
      rgbBlink();
    }
    if (strcmp((char*)data, "913") == 0) {
      JBOXID = 113;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
      rgbBlink();
    }
    if (strcmp((char*)data, "914") == 0) {
      JBOXID = 114;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
      rgbBlink();
    }
    if (strcmp((char*)data, "915") == 0) {
      JBOXID = 115;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
      rgbBlink();
    }
    if (strcmp((char*)data, "916") == 0) {
      JBOXID = 116;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
      rgbBlink();
    }
    if (strcmp((char*)data, "917") == 0) {
      JBOXID = 117;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
      rgbBlink();
    }
    if (strcmp((char*)data, "918") == 0) {
      JBOXID = 118;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
      rgbBlink();
    }
    if (strcmp((char*)data, "919") == 0) {
      JBOXID = 119;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
      rgbBlink();
    }
    if (strcmp((char*)data, "920") == 0) {
      JBOXID = 120;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
      rgbBlink();
    }
    if (strcmp((char*)data, "21") == 0) {
      Serial.println("No Motion Sensors");
      EEPROM.write(101, 0);
      EEPROM.commit();
      rgbBlink();
      ESP.restart();
    }
    if (strcmp((char*)data, "20") == 0) {
      Serial.println("Mostion Sensensor Activated");
      EEPROM.write(101, 1);
      EEPROM.commit();
      rgbBlink();
      ESP.restart();
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}
String processor(const String& var){
  Serial.println(var);
  if(var == "STATE"){
    if (ledState){
      return "ON";
    }
    else{
      return "OFF";
    }
  }
}
//*************************************************************************
//*************************** RGB OBJECTS *********************************
//*************************************************************************
void resetRGB() {
  Serial.println("Resetting RGBs");
  RGBGREEN = false;
  RGBOFF = false;
  RGBRED = false;
  RGBYELLOW = false;
  RGBBLUE = false;
  RGBPURPLE = false;
  RGBCYAN = false;
  RGBWHITE = false;
}
void AlignRGBWithTeam() {
  Serial.println("running AlignRGBWithTeam Function");
  resetRGB();
  if (TeamID == 0) {
    RGBRED = true; colorposession = 0;
  }
  if (TeamID == 1) {
    RGBBLUE = true; colorposession = 1;
  }
  if (TeamID == 2) {
    RGBYELLOW = true; colorposession = 2;
  }
  if (TeamID == 3) {
    RGBGREEN = true; colorposession = 3;
  }
}
void changergbcolor() {
  int statuschange = 99;
  if(RGBGREEN) {RGBGREEN = false; RGBWHITE = true; colorposession = 4; statuschange = 0;}
  if(RGBRED) {RGBRED = false; RGBGREEN = true; colorposession = 3;}
  if(RGBYELLOW) {RGBYELLOW = false; RGBRED = true; colorposession = 0;}
  if(RGBBLUE) {RGBBLUE = false; RGBYELLOW = true; colorposession = 2;}
  if(RGBPURPLE) {RGBPURPLE = false; RGBBLUE = true; colorposession = 1;}
  if(RGBCYAN) {RGBCYAN = false; RGBPURPLE = true; colorposession = 5;}
  if(RGBWHITE && statuschange == 99) {RGBWHITE = false; RGBCYAN = true; colorposession = 6;}
}
void rgbgreen(){
    digitalWrite(red, LOW); // turn the LED on 
    //digitalWrite(red, HIGH); // turn the LED off by making the voltage LOW
    //digitalWrite(green, LOW); // turn the LED on 
    digitalWrite(green, HIGH); // turn the LED off by making the voltage LOW
    digitalWrite(blue, LOW); // turn the LED on
    //digitalWrite(blue, HIGH); // turn the LED off by making the voltage LOW 
}
void rgbred(){
    //digitalWrite(red, LOW); // turn the LED on 
    digitalWrite(red, HIGH); // turn the LED off by making the voltage LOW
    digitalWrite(green, LOW); // turn the LED on 
    //digitalWrite(green, HIGH); // turn the LED off by making the voltage LOW
    digitalWrite(blue, LOW); // turn the LED on
    //digitalWrite(blue, HIGH); // turn the LED off by making the voltage LOW
}
void rgbyellow(){
    //digitalWrite(red, LOW); // turn the LED on 
    digitalWrite(red, HIGH); // turn the LED off by making the voltage LOW
    //digitalWrite(green, LOW); // turn the LED on 
    digitalWrite(green, HIGH); // turn the LED off by making the voltage LOW
    digitalWrite(blue, LOW); // turn the LED on
    //digitalWrite(blue, HIGH); // turn the LED off by making the voltage LOW
}
void rgbpurple(){
    //digitalWrite(red, LOW); // turn the LED on 
    digitalWrite(red, HIGH); // turn the LED off by making the voltage LOW
    digitalWrite(green, LOW); // turn the LED on 
    //digitalWrite(green, HIGH); // turn the LED off by making the voltage LOW
    //digitalWrite(blue, LOW); // turn the LED on
    digitalWrite(blue, HIGH); // turn the LED off by making the voltage LOW
}
void rgbblue(){
    digitalWrite(red, LOW); // turn the LED on 
    //digitalWrite(red, HIGH); // turn the LED off by making the voltage LOW
    digitalWrite(green, LOW); // turn the LED on 
    //digitalWrite(green, HIGH); // turn the LED off by making the voltage LOW
    //digitalWrite(blue, LOW); // turn the LED on
    digitalWrite(blue, HIGH); // turn the LED off by making the voltage LOW
}
void rgboff(){
    digitalWrite(red, LOW); // turn the LED on 
    //digitalWrite(red, HIGH); // turn the LED off by making the voltage LOW
    digitalWrite(green, LOW); // turn the LED on 
    //digitalWrite(green, HIGH); // turn the LED off by making the voltage LOW
    digitalWrite(blue, LOW); // turn the LED on
    //digitalWrite(blue, HIGH); // turn the LED off by making the voltage LOW
}
void rgbcyan(){
    digitalWrite(red, LOW); // turn the LED on 
    //digitalWrite(red, HIGH); // turn the LED off by making the voltage LOW
    //digitalWrite(green, LOW); // turn the LED on 
    digitalWrite(green, HIGH); // turn the LED off by making the voltage LOW
    //digitalWrite(blue, LOW); // turn the LED on
    digitalWrite(blue, HIGH); // turn the LED off by making the voltage LOW
}
void rgbwhite(){
    //digitalWrite(red, LOW); // turn the LED on 
    digitalWrite(red, HIGH); // turn the LED off by making the voltage LOW
    //digitalWrite(green, LOW); // turn the LED on 
    digitalWrite(green, HIGH); // turn the LED off by making the voltage LOW
    //digitalWrite(blue, LOW); // turn the LED on
    digitalWrite(blue, HIGH); // turn the LED off by making the voltage LOW
}
void rgbBlink() {
  rgboff();
  delay(100);
  rgbcyan();
  delay(100);
  rgboff();
  delay(100);
  rgbcyan();
  delay(100);
  rgboff();
}
void rgbblink() {
  if (RGBState == LOW) {
    RGBState = HIGH;
    if(RGBGREEN) {
      rgbgreen();
      //////Serial.println("Blinking RGB");
      }
    if(RGBRED) {
      rgbred();
      ////Serial.println("Blinking RGB");
      }
    if(RGBYELLOW) {
      rgbyellow();
      ////Serial.println("Blinking RGB");
      }
    if(RGBBLUE) {
      rgbblue();
      ////Serial.println("Blinking RGB");
      }
    if(RGBPURPLE) {
      rgbpurple();
      ////Serial.println("Blinking RGB");
      }
    if(RGBCYAN) {
      rgbcyan();
      ////Serial.println("Blinking RGB");
      }
    if(RGBWHITE) {
      rgbwhite();
      //////Serial.println("Blinking RGB");
      }
  } else {
    rgboff();
    RGBState = LOW;
  }
}

//*************************************************************************
//************************ IR RECEIVER OBJECTS ****************************
//*************************************************************************
void PrintReceivedTag() {
  // prints each individual bit on serial monitor
  // typically not used but used for troubleshooting
  Serial.println("TagReceived!!");
  Serial.print("B0: "); Serial.println(BB[0]);
  Serial.print("B1: "); Serial.println(BB[1]);
  Serial.print("B2: "); Serial.println(BB[2]);
  Serial.print("B3: "); Serial.println(BB[3]);
  Serial.print("P0: "); Serial.println(PP[0]);
  Serial.print("P1: "); Serial.println(PP[1]);
  Serial.print("P2: "); Serial.println(PP[2]);
  Serial.print("P3: "); Serial.println(PP[3]);
  Serial.print("P4: "); Serial.println(PP[4]);
  Serial.print("P5: "); Serial.println(PP[5]);
  Serial.print("T0: "); Serial.println(TT[0]);
  Serial.print("T1: "); Serial.println(TT[1]);
  Serial.print("D0: "); Serial.println(DD[0]);
  Serial.print("D1: "); Serial.println(DD[1]);
  Serial.print("D2: "); Serial.println(DD[2]);
  Serial.print("D3: "); Serial.println(DD[3]);
  Serial.print("D4: "); Serial.println(DD[4]);
  Serial.print("D5: "); Serial.println(DD[5]);
  Serial.print("D6: "); Serial.println(DD[6]);
  Serial.print("D7: "); Serial.println(DD[7]);
  Serial.print("C0: "); Serial.println(CC[0]);
  Serial.print("U0: "); Serial.println(UU[0]);
  Serial.print("U1: "); Serial.println(UU[1]);
  Serial.print("Z0: "); Serial.println(ZZ[0]);
  Serial.print("Z1: "); Serial.println(ZZ[1]);
}
// this procedure breaksdown each Weapon bit of the brx ir signal recieved and assigns the applicable bit value then adds them together to identify the player ID (1-64)
void IDDamage() {
      // determining indivudual protocol values for Weapon ID bits
      if (DD[7] > 750) {
        DID[0] = 1;
      } else {
        DID[0] = 0;
      }
      if (DD[6] > 750) {
        DID[1]=2;
      } else {
        DID[1]=0;
      }
      if (DD[5] > 750) {
        DID[2]=4;
        } else {
        DID[2]=0;
      }
      if (DD[4] > 750) {
        DID[3]=8;
      } else {
        DID[3]=0;
      }
      if (DD[3] > 750) {
        DID[4]=16;
      } else {
        DID[4]=0;
      }
      if (DD[2] > 750) {
        DID[5]=32;
      } else {
        DID[5]=0;
      }
      if (DD[1] > 750) {
        DID[6]=64;
      } else {
        DID[6]=0;
      }
      if (DD[0] > 750) {
        DID[7]=128;
      } else {
        DID[7]=0;
      }
      // ID Damage by summing assigned values above based upon protocol values (1-64)
      DamageID=DID[0]+DID[1]+DID[2]+DID[3]+DID[4]+DID[6]+DID[7]+DID[5];
      if (GearMod == 4) {
        DamageID = DID[4]+DID[6]+DID[7]+DID[5];
      }
      Serial.print("Damage ID = ");
      Serial.println(DamageID);     
}
// this procedure breaksdown each bullet bit of the brx ir signal recieved and assigns the applicable bit value then adds them together to identify the player ID (1-64)
void IDShot() {
      // determining indivudual protocol values for Weapon ID bits
      if (BB[3] > 750) {
        BID[0] = 1;
      } else {
        BID[0] = 0;
      }
      if (BB[2] > 750) {
        BID[1]=2;
      } else {
        BID[1]=0;
      }
      if (BB[1] > 750) {
        BID[2]=4;
        } else {
        BID[2]=0;
      }
      if (BB[0] > 750) {
        BID[3]=8;
      } else {
        BID[3]=0;
      }
      // ID Player by summing assigned values above based upon protocol values (1-64)
      ShotType=BID[0]+BID[1]+BID[2]+BID[3];
      Serial.print("Shot Type = ");
      Serial.println(ShotType);      
}
// this procedure breaksdown each player bit of the brx ir signal recieved and assigns the applicable bit value then adds them together to identify the player ID (1-64)
void IDEvolverPlayer() {
      // determining indivudual protocol values for player ID bits
      // Also assign IR values for sending player ID with device originated tags
      int brpid = 0;
      if (PP[3] > 750) {
        PID[0] = 1;
      } else {
        PID[0] = 0;
      }
      if (PP[2] > 750) {
        PID[1]=2;
      } else {
        PID[1]=0;
      }
      if (PP[1] > 750) {
        PID[2]=4;
        } else {
        PID[2]=0;
      }
      if (PP[0] > 750) {
        PID[3]=8;
      } else {
        PID[3]=0;
      }
      // ID Player by summing assigned values above based upon protocol values (1-64)
      PlayerID=PID[0]+PID[1]+PID[2]+PID[3];
      Serial.print("Player ID = ");
      Serial.println(PlayerID);
      
}
void IDplayer() {
      // determining indivudual protocol values for player ID bits
      // Also assign IR values for sending player ID with device originated tags
      int brpid = 0;
      if (TT[0] > 800) {
        brpid = brpid + 1;
      }
      if (PP[5] > 800) {
        PID[0] = 1;
        brpid = brpid + 2;
      } else {
        PID[0] = 0;
      }
      if (PP[4] > 800) {
        PID[1]=2;
        brpid = brpid + 4;
      } else {
        PID[1]=0;
      }
      if (PP[3] > 800) {
        PID[2]=4;
        brpid = brpid + 8;
        } else {
        PID[2]=0;
      }
      if (PP[2] > 800) {
        PID[3]=8;
        brpid = brpid + 16;
      } else {
        PID[3]=0;
      }
      if (PP[1] > 800) {
        PID[4]=16;
        brpid = brpid + 32;
      } else {
        PID[4]=0;
      }
      if (PP[0] > 800) {
        PID[5]=32;
        brpid = brpid + 64;
      } else {
        PID[5]=0;
      }
      if (PP[6] > 800 && GearMod == 4) {
        PID[5] = PID[5] + 64;
      }
      // ID Player by summing assigned values above based upon protocol values (1-64)
      PlayerID=PID[0]+PID[1]+PID[2]+PID[3]+PID[4]+PID[5];
      if (GearMod == 3) {
        PlayerID = brpid;
      }
      Serial.print("Player ID = ");
      Serial.println(PlayerID);      
}
void teamID() {
      // check if the IR is from Red team
      if (TT[0] < 800 && TT[1] < 800) {
      // sets the current team as red
      TeamID = 0;
      Serial.print("team = Red = ");
      Serial.println(TeamID);
      }
      // check if the IR is from blue team 
      if (TT[0] < 800 && TT[1] > 800) {
      // sets the current team as blue
      TeamID = 1;
      Serial.print("team = Blue = ");
      Serial.println(TeamID);
      }
      // check if the IR is from green team 
      if (TT[0] > 800 && TT[1] > 800) {
      // sets the current team as green
      TeamID = 3;
      Serial.print("team = Green = ");
      Serial.println(TeamID);
      }
      if (TT[0] > 800 && TT[1] < 800) {
      // sets the current team as red
      TeamID = 2;
      Serial.print("team = Yellow = ");
      Serial.println(TeamID);
      }
      if (GearMod == 3) {
        TeamID = 0;
        if (DD[1] > 800) {
          TeamID = TeamID + 1;
        }
        if (DD[0] > 800) {
          TeamID = TeamID + 2;
        }
        if (TT[1] > 800) {
          TeamID = TeamID + 4;
        }
        Serial.print("team = Blue = ");
        Serial.println(TeamID);
      }
}
void IDPower() {
  if (UU[0] < 750 & UU[1] < 750) {PowerID = 0;}
  if (UU[0] < 750 & UU[1] > 750) {PowerID = 1;}
  if (UU[0] > 750 & UU[1] < 750) {PowerID = 2;}
  if (UU[0] > 750 & UU[1] > 750) {PowerID = 3;}
  Serial.println("Power = "+String(PowerID));
}
void IDCritical() {
  if (CC[0] < 750) {CriticalID = 0;}
  else {CriticalID = 1;}
  Serial.println("Criical = "+String(CriticalID));
}
void BasicDomination() {
  RGBDEMOMODE = false;
  StartShortBuzzer();
  AlignRGBWithTeam();
  if (GearMod == 0) {
    CONTROLPOINTCAPTURED = true;
  }
  if (!DOMINATIONCLOCK) {
    DOMINATIONCLOCK = true;
    Serial.println("enabled domination clock/counter");
  }
}
void ChangeBaseAlignment() {
  if (TeamID != Team) { // checks to see if tag hitting base is unfriendly or friendly
    CapturableEmitterScore[TeamID]++; // add one point to the team that shot the base
    if (CapturableEmitterScore[TeamID] >= RequiredCapturableEmitterCounter) { // check if the team scored enough points/shots to capture base
      Team = TeamID; // sets the ir protocol used for the respawn ir tag
      // resetting all team capture points
      CapturableEmitterScore[0] = 0;
      CapturableEmitterScore[1] = 0;
      CapturableEmitterScore[2] = 0;
      CapturableEmitterScore[3] = 0;
      AlignRGBWithTeam(); // sets team RGB to new friendly team
      BASECONTINUOUSIRPULSE = true;
      ResetAllIRProtocols();
      SINGLEIRPULSE = true; // allows for a one time pulse of an IR
      CONTROLPOINTCAPTURED = true;
      BUZZ = true;
    } else { // if team tagging base did not meet minimum to control base
      int tempscore = CapturableEmitterScore[TeamID]; // temporarily stores the teams score that just shot the base
      // resets all scores
      CapturableEmitterScore[0] = 0;
      CapturableEmitterScore[1] = 0;
      CapturableEmitterScore[2] = 0;
      CapturableEmitterScore[3] = 0;
      CapturableEmitterScore[TeamID] = tempscore; // resaves the temporarily stored score over the team's score that just shot the base, just easier than a bunch of if statements
    }
  }
}
void RunIRDebug() {
  // makes the action below happen as it cycles through the 25 bits as was delcared above
  for (byte x = 0; x < 4; x++) BB[x]=0;
  for (byte x = 0; x < 6; x++) PP[x]=0;
  for (byte x = 0; x < 2; x++) TT[x]=0;
  for (byte x = 0; x < 8; x++) DD[x]=0;
  for (byte x = 0; x < 1; x++) CC[x]=0;
  for (byte x = 0; x < 2; x++) UU[x]=0;
  for (byte x = 0; x < 2; x++) ZZ[x]=0;
  // checks for a 2 millisecond sync pulse signal with a tollerance of 500 microsecons
  // Serial.println("IR input set up.... Ready!...");
  if (pulseIn(IR_Sensor_Pin, LOW, 150000) > 200) {
      // stores each pulse or bit, individually
      BB[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // B1
      BB[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // B2
      BB[2] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // B3
      BB[3] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // B4
      PP[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P1 32
      PP[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P2 16
      PP[2] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P3 8 
      PP[3] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P4 4
      PP[4] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P5 2
      PP[5] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P6 0
      TT[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // T1
      TT[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // T2
      DD[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D1
      DD[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D2
      DD[2] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D3
      DD[3] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D4
      DD[4] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D5
      DD[5] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D6
      DD[6] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D7
      DD[7] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D8
      CC[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // C1
      UU[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // ?1
      UU[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // ?2
      ZZ[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // Z1
      ZZ[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // Z2
      XX[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // X1
      Serial.println("Tag Received");
      PrintReceivedTag();
    } else {
      //Serial.println("Error Reading Protocol, Adjust Parameters in Code if issue persists");
    }
}

void ReceiveOpsProIR() {
  // makes the action below happen as it cycles through the 25 bits as was delcared above
  for (byte x = 0; x < 4; x++) BB[x]=0;
  for (byte x = 0; x < 6; x++) PP[x]=0;
  for (byte x = 0; x < 2; x++) TT[x]=0;
  for (byte x = 0; x < 8; x++) DD[x]=0;
  for (byte x = 0; x < 1; x++) CC[x]=0;
  for (byte x = 0; x < 2; x++) UU[x]=0;
  for (byte x = 0; x < 2; x++) ZZ[x]=0;
  // checks for a 2 millisecond sync pulse signal with a tollerance of 500 microsecons
  // Serial.println("IR input set up.... Ready!...");
  if (pulseIn(IR_Sensor_Pin, LOW, 150000) > 200) {
      // stores each pulse or bit, individually
      BB[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // B1
      BB[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // B2
      BB[2] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // B3
      BB[3] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // B4
      PP[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P1 32
      PP[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P2 16
      PP[2] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P3 8 
      PP[3] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P4 4
      PP[4] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P5 2
      PP[5] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P6 0
      TT[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // T1
      TT[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // T2
      DD[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D1
      DD[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D2
      DD[2] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D3
      DD[3] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D4
      DD[4] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D5
      DD[5] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D6
      DD[6] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D7
      DD[7] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D8
      CC[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // C1
      UU[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // ?1
      UU[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // ?2
      ZZ[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // Z1
      ZZ[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // Z2
      XX[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // X1
      Serial.println("Tag Received");
      PrintReceivedTag();
      if (BB[0] < 1000 && BB[1] > 2500) {
        Serial.println("Received Ops Pro");
        NEWTAGRECEIVED = true;
        if (TT[0] > 1750) {
          Serial.println("Purple Tagger");
          TeamID = 2;
        } else {
          if (TT[1] > 1750) { 
            Serial.println("Blue Tagger");
            TeamID = 1;
          } else {
            Serial.println("Red Tagger");
            TeamID = 0;
          }
        }
      }
    } else {
      //Serial.println("Error Reading Protocol, Adjust Parameters in Code if issue persists");
    }
}
// This procedure uses the preset IR_Sensor_Pin to determine if an ir received is BRX, if so it records the protocol received
void receiveBRXir() {
  // makes the action below happen as it cycles through the 25 bits as was delcared above
  for (byte x = 0; x < 4; x++) BB[x]=0;
  for (byte x = 0; x < 6; x++) PP[x]=0;
  for (byte x = 0; x < 2; x++) TT[x]=0;
  for (byte x = 0; x < 8; x++) DD[x]=0;
  for (byte x = 0; x < 1; x++) CC[x]=0;
  for (byte x = 0; x < 2; x++) UU[x]=0;
  for (byte x = 0; x < 2; x++) ZZ[x]=0;
  // checks for a 2 millisecond sync pulse signal with a tollerance of 500 microsecons
  // Serial.println("IR input set up.... Ready!...");
  if (pulseIn(IR_Sensor_Pin, LOW, 150000) > 1500) {
      // stores each pulse or bit, individually
      BB[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // B1
      BB[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // B2
      BB[2] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // B3
      BB[3] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // B4
      PP[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P1 32
      PP[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P2 16
      PP[2] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P3 8 
      PP[3] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P4 4
      PP[4] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P5 2
      PP[5] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P6 0
      TT[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // T1
      TT[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // T2
      DD[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D1
      DD[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D2
      DD[2] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D3
      DD[3] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D4
      DD[4] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D5
      DD[5] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D6
      DD[6] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D7
      DD[7] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D8
      CC[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // C1
      UU[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // ?1
      UU[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // ?2
      ZZ[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // Z1
      ZZ[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // Z2
      XX[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // X1
      if (ZZ[1] > 250 && XX[0] < 250 && BB[0] < 1250) {
        Serial.println("BRX Tag Confirmed");
        PrintReceivedTag();
        teamID();
        IDplayer();
        IDShot();
        IDDamage();
        IDPower();
        IDCritical();
        NEWTAGRECEIVED = true;
      } else {
        Serial.println("Protocol not Recognized");
      }
    } else {
      //Serial.println("Incorrect Sync Bits from Protocol, not BRX");
    }
}
void receiveLaserWarsir() {
  // makes the action below happen as it cycles through the 25 bits as was delcared above
  for (byte x = 0; x < 1; x++) BB[x]=0;
  for (byte x = 0; x < 7; x++) PP[x]=0;
  for (byte x = 0; x < 2; x++) TT[x]=0;
  for (byte x = 0; x < 4; x++) DD[x]=0;
  // checks for a 2 millisecond sync pulse signal with a tollerance of 500 microsecons
  // Serial.println("IR input set up.... Ready!...");
  if (pulseIn(IR_Sensor_Pin, LOW, 150000) > 2100) {
      // stores each pulse or bit, individually
      BB[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // B1
      PP[6] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P7
      PP[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P1
      PP[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P2
      PP[2] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P3
      PP[3] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P4
      PP[4] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P5
      PP[5] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P6
      TT[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // T1
      TT[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // T2
      DD[4] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D1
      DD[5] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D2
      DD[6] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D3
      DD[7] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D4
      XX[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // X1
      if (XX[0] < 250 && BB[0] < 800) {
        Serial.println("LaserWars Tag Confirmed");
        //PrintLWReceivedTag();
        teamID(); // same as br 
        IDplayer(); // same as brx, except... add 64 if pp6 is high
        //IDShot();
        IDDamage();
        //IDPower();
        //IDCritical();
        NEWTAGRECEIVED = true;
      } else {
        Serial.println("Protocol not Recognized");
      }
    } else {
      //Serial.println("Incorrect Sync Bits from Protocol, not LWars");
    }
}
// This procedure uses the preset IR_Sensor_Pin to determine if an ir received is BRX, if so it records the protocol received
void ReceiveEvolverIR() {
  // makes the action below happen as it cycles through the 25 bits as was delcared above
  for (byte x = 0; x < 4; x++) BB[x]=0;
  for (byte x = 0; x < 4; x++) PP[x]=0;
  for (byte x = 0; x < 2; x++) TT[x]=0;
  for (byte x = 0; x < 8; x++) DD[x]=0;
  for (byte x = 0; x < 1; x++) CC[x]=0;
  //for (byte x = 0; x < 2; x++) UU[x]=0;
  for (byte x = 0; x < 3; x++) ZZ[x]=0;
  // checks for a 2 millisecond sync pulse signal with a tollerance of 500 microsecons
  // Serial.println("IR input set up.... Ready!...");
  if (pulseIn(IR_Sensor_Pin, LOW, 150000) > 2250) {
      // stores each pulse or bit, individually
      BB[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // B1
      BB[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // B2
      BB[2] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // B3
      BB[3] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // B4
      PP[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P1
      PP[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P2
      PP[2] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P3
      PP[3] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P4
      TT[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // T1
      TT[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // T2
      DD[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D1
      DD[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D2
      DD[2] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D3
      DD[3] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D4
      DD[4] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D5
      DD[5] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D6
      DD[6] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D7
      DD[7] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D8
      CC[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // C1
      ZZ[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // Z1
      ZZ[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // Z2
      ZZ[2] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // Z3
      XX[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // X1
      if (ZZ[2] > 250 && XX[0] < 250 && BB[0] < 1250) {
        Serial.println("Evolver Tag Confirmed");
        PrintReceivedTag();
        teamID();
        IDEvolverPlayer();
        IDShot();
        IDDamage();
        //IDPower();
        IDCritical();
        NEWTAGRECEIVED = true;
      } else {
        Serial.println("Protocol not Recognized");
      }
    } else {
      //Serial.println("Incorrect Sync Bits from Protocol, not BRX");
    }
}
void PrintLTTOTag() {
  // prints each individual bit on serial monitor
  Serial.println("TagReceived!!");
  Serial.println(PLTTO[0]);
  Serial.println(PLTTO[1]);
  Serial.println(TLTTO[0]);
  Serial.println(TLTTO[1]);
  Serial.println(TLTTO[2]);
  Serial.println(DLTTO[0]);
  Serial.println(DLTTO[1]);
}
//******************************************************************************************************************************************************************************************
void IDLTTOTeam() {
  int PID[2];
  int TID[3];
  if (PLTTO[1] > 1750) {
    PID[1] = 1;
  } else {
    PID[1] = 0;
  }
  if (PLTTO[0] > 1750) {
    PID[0] = 2;
  } else {
    PID[0] = 0;
  }
  if (TLTTO[2] > 1750) {
    TID[2] = 1;
  } else {
    TID[2] = 0;
  }
  if (TLTTO[1] > 1750) {
    TID[1] = 2;
  } else {
    TID[1] = 0;
  }
  if (TLTTO[0] > 1750) {
    TID[0] = 4;
  } else {
    TID[0] = 0;
  }
  if (LTTOTagType == 1) {
    Serial.println("Grab and Go Game");
    Serial.print("Team identified as: ");
    TeamID = TID[2] + TID[1] + TID[0];
    if (TeamID == 0) {
      Serial.println("Solo");
    }
    if (TeamID == 1) {
      Serial.println("Team 1");
    }
    if (TeamID == 2) {
      Serial.println("Team 2");
    }
    if (TeamID == 3) {
      Serial.println("Team 3");
    }
  } else {
    Serial.println("Hosted Game");
    PlayerID = TID[2] + TID[1] + TID[0] + 1;
    TeamID = PID[0] + PID[1];
    Serial.print("Team identified as: ");
    if (TeamID == 0) {
      Serial.println("Solo");
    }
    if (TeamID == 1) {
      Serial.println("Team 1");
    }
    if (TeamID == 2) {
      Serial.println("Team 2");
    }
    if (TeamID == 3) {
      Serial.println("Team 3");
    }
    Serial.print("Player identified as: ");
    Serial.println(PlayerID);
  }
}

void IDLTTODamage() {
  int DID[3];
  if (DLTTO[1] > 1750) {
    DID[1] = 1;
  } else {
    DID[1] = 0;
  }
  if (DLTTO[0] > 1750) {
    DID[0] = 2;
  } else {
    DID[0] = 0;
  }
  Serial.print("Tag damage = ");
  DamageID = DID[1] + DID[0] + 1;
  Serial.println(DamageID);
}
// This procedure uses the preset IR_Sensor_Pin to determine if an ir received is BRX, if so it records the protocol received
void ReceiveLTTOIR() {
  // makes the action below happen as it cycles through the 25 bits as was delcared above
  for (byte x = 0; x < 2; x++) PLTTO[x]=0;
  for (byte x = 0; x < 3; x++) TLTTO[x]=0;
  for (byte x = 0; x < 2; x++) DLTTO[x]=0;
  // checks for a 3 millisecond sync pulse signal with a tollerance of 500 microsecons
  if (pulseIn(IR_Sensor_Pin, LOW, 250000) > 2500) {
    if (pulseIn(IR_Sensor_Pin, LOW) < 3200) {
      // stores each pulse or bit, individually
      PLTTO[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P0
      PLTTO[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P1
      TLTTO[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // T0
      TLTTO[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // T1
      TLTTO[2] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // T2
      DLTTO[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D0
      DLTTO[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // D1
      // run a test on the tag before anything else:
      bool ISLTTO = true;
      if (PLTTO[0] < 750 || PLTTO[0] > 2250) {ISLTTO = false;}
      if (PLTTO[1] < 750 || PLTTO[1] > 2250) {ISLTTO = false;}
      if (TLTTO[0] < 750 || TLTTO[0] > 2250) {ISLTTO = false;}
      if (TLTTO[1] < 750 || TLTTO[1] > 2250) {ISLTTO = false;}
      if (TLTTO[2] < 750 || TLTTO[2] > 2250) {ISLTTO = false;}
      if (DLTTO[0] < 750 || DLTTO[0] > 2250) {ISLTTO = false;}
      if (DLTTO[1] < 750 || DLTTO[1] > 2250) {ISLTTO = false;} 
      if (ISLTTO) { // this means this is a good tag and we are most likely using it
        // now lets check what kind of tag it is...
        if (PLTTO[0] > 1750 || PLTTO[1] > 1750) { // this is a hosted game
          LTTOTagType = 0;
        } else {
          LTTOTagType = 1; // use 1 as a grab and go tag
          // so we know it is a grab and go, but it still could be a zone tag, lets be sure...
          if (TLTTO[0] > 1750) {
            LTTOTagType = 2; // it is a zone tag, ignore!
          }
        }
        if (LTTOTagType < 2) {
          PrintLTTOTag(); // runs a proceedure, see proceedure for details
          IDLTTOTeam(); // check team
          IDLTTODamage(); // check the damageif (CAPTURABLEEMITTER) {
          NEWTAGRECEIVED = true;
        }
      }
    } else {
      Serial.println("protocol not recognized as ltto");
    }
  } else {
    
  }  
}

//*************************************************************************
//************************** IR LED OBJECTS *******************************
//*************************************************************************
// This procedure sends a 38KHz pulse to the IRledPin
// for a certain # of microseconds. We'll use this whenever we need to send codes
void pulseIR(long microsecs) {
  // we'll count down from the number of microseconds we are told to wait
  cli();  // this turns off any background interrupts
  while (microsecs > 0) {
    // 38 kHz is about 13 microseconds high and 13 microseconds low
    digitalWrite(IRledPin, HIGH);  // this takes about 3 microseconds to happen
    delayMicroseconds(11);         // hang out for 10 microseconds, you can also change this to 9 if its not working
    digitalWrite(IRledPin, LOW);   // this also takes about 3 microseconds
    delayMicroseconds(11);         // hang out for 10 microseconds, you can also change this to 9 if its not working
    // so 26 microseconds altogether
    microsecs -= 26;
  }
  sei();  // this turns them back on
}
void ExecuteOrder66() {
  if (GearMod == 0) { // BRX
    Player = 64;
    BulletType = 10;
    Team = 2;
    Damage = 200;
    Critical = 0;
    Power = 0;  
    SetIRProtocol();
  }
  if (GearMod == 1) { // Evolver
    Player = 15;
    BulletType = 0;
    Team = 2;
    Damage = 200;
    Critical = 1;
    SetEvolverIRProtocol();
  }
  if (GearMod == 2) { // LTTO
    
  }
  if (GearMod == 3) { //BRP
    Player = 1;
    BulletType = 0;
    Team = 4;
    Damage = 200;
    Critical = 0;
    SetBRPIRProtocol();
  }
  if (GearMod == 4) { // Laserwar
    
  }
}
void SendLTTOIR() {
  
}
void SendBRPIR() {
  Serial.println("Sending IR signal");
  pulseIR(2000); // sync
  delayMicroseconds(500); // delay
  pulseIR(B[0]); // B1
  delayMicroseconds(500); // delay
  pulseIR(B[1]); // B2
  delayMicroseconds(500); // delay
  pulseIR(B[2]); // B3
  delayMicroseconds(500); // delay
  pulseIR(B[3]); // B4
  delayMicroseconds(500); // delay
  pulseIR(P[0]); // P1
  delayMicroseconds(500); // delay
  pulseIR(P[1]); // P2
  delayMicroseconds(500); // delay
  pulseIR(P[2]); // P3
  delayMicroseconds(500); // delay
  pulseIR(P[3]); // P4
  delayMicroseconds(500); // delay
  pulseIR(P[4]); // P5
  delayMicroseconds(500); // delay
  pulseIR(P[5]); // P6
  delayMicroseconds(500); // delay
  pulseIR(P[6]); // P7
  delayMicroseconds(500); // delay
  pulseIR(T[0]); // T1
  delayMicroseconds(500); // delay
  pulseIR(T[1]); // T2
  delayMicroseconds(500); // delay
  pulseIR(T[2]); // T3
  delayMicroseconds(500); // delay
  pulseIR(D[0]); // D1
  delayMicroseconds(500); // delay
  pulseIR(D[1]); // D2
  delayMicroseconds(500); // delay
  pulseIR(D[2]); // D3
  delayMicroseconds(500); // delay
  pulseIR(D[3]); // D4
  delayMicroseconds(500); // delay
  pulseIR(D[4]); // D5
  delayMicroseconds(500); // delay
  pulseIR(D[5]); // D6
  delayMicroseconds(500); // delay
  pulseIR(D[6]); // D7
  delayMicroseconds(500); // delay
  pulseIR(D[7]); // D8
  delayMicroseconds(500); // delay
  pulseIR(C[0]); // C1
  delayMicroseconds(500); // delay
  pulseIR(Z[0]); // Z1
  delayMicroseconds(500); // delay
  pulseIR(Z[1]); // Z1
}
void SendIR() {
  Serial.println("Sending IR signal");
  pulseIR(2000); // sync
  delayMicroseconds(500); // delay
  pulseIR(B[0]); // B1
  delayMicroseconds(500); // delay
  pulseIR(B[1]); // B2
  delayMicroseconds(500); // delay
  pulseIR(B[2]); // B3
  delayMicroseconds(500); // delay
  pulseIR(B[3]); // B4
  delayMicroseconds(500); // delay
  pulseIR(P[0]); // P1
  delayMicroseconds(500); // delay
  pulseIR(P[1]); // P2
  delayMicroseconds(500); // delay
  pulseIR(P[2]); // P3
  delayMicroseconds(500); // delay
  pulseIR(P[3]); // P4
  delayMicroseconds(500); // delay
  pulseIR(P[4]); // P5
  delayMicroseconds(500); // delay
  pulseIR(P[5]); // P6
  delayMicroseconds(500); // delay
  pulseIR(T[0]); // T1
  delayMicroseconds(500); // delay
  pulseIR(T[1]); // T2
  delayMicroseconds(500); // delay
  pulseIR(D[0]); // D1
  delayMicroseconds(500); // delay
  pulseIR(D[1]); // D2
  delayMicroseconds(500); // delay
  pulseIR(D[2]); // D3
  delayMicroseconds(500); // delay
  pulseIR(D[3]); // D4
  delayMicroseconds(500); // delay
  pulseIR(D[4]); // D5
  delayMicroseconds(500); // delay
  pulseIR(D[5]); // D6
  delayMicroseconds(500); // delay
  pulseIR(D[6]); // D7
  delayMicroseconds(500); // delay
  pulseIR(D[7]); // D8
  delayMicroseconds(500); // delay
  pulseIR(C[0]); // C1
  delayMicroseconds(500); // delay
  pulseIR(X[0]); // X1
  delayMicroseconds(500); // delay
  pulseIR(X[1]); // X2
  delayMicroseconds(500); // delay
  pulseIR(Z[0]); // Z1
  delayMicroseconds(500); // delay
  pulseIR(Z[1]); // Z1
}
void SendEvolverIR() {
  Serial.println("Sending IR signal");
  pulseIR(2500); // sync
  delayMicroseconds(500); // delay
  pulseIR(B[0]); // B1
  delayMicroseconds(500); // delay
  pulseIR(B[1]); // B2
  delayMicroseconds(500); // delay
  pulseIR(B[2]); // B3
  delayMicroseconds(500); // delay
  pulseIR(B[3]); // B4
  delayMicroseconds(500); // delay
  pulseIR(P[2]); // P3
  delayMicroseconds(500); // delay
  pulseIR(P[3]); // P4
  delayMicroseconds(500); // delay
  pulseIR(P[4]); // P5
  delayMicroseconds(500); // delay
  pulseIR(P[5]); // P6
  delayMicroseconds(500); // delay
  pulseIR(T[0]); // T1
  delayMicroseconds(500); // delay
  pulseIR(T[1]); // T2
  delayMicroseconds(500); // delay
  pulseIR(D[0]); // D1
  delayMicroseconds(500); // delay
  pulseIR(D[1]); // D2
  delayMicroseconds(500); // delay
  pulseIR(D[2]); // D3
  delayMicroseconds(500); // delay
  pulseIR(D[3]); // D4
  delayMicroseconds(500); // delay
  pulseIR(D[4]); // D5
  delayMicroseconds(500); // delay
  pulseIR(D[5]); // D6
  delayMicroseconds(500); // delay
  pulseIR(D[6]); // D7
  delayMicroseconds(500); // delay
  pulseIR(D[7]); // D8
  delayMicroseconds(500); // delay
  pulseIR(C[0]); // C1
  delayMicroseconds(500); // delay
  pulseIR(Z[0]); // Z1
  delayMicroseconds(500); // delay
  pulseIR(Z[1]); // Z1
  delayMicroseconds(500); // delay
  pulseIR(Z[2]); // Z2
}
void RespawnStation() {
  BulletType = 15;
  Player = 63;
  Damage = 6;
  Critical = 1;
  Power = 0;
  SetIRProtocol();
}
void HeavyDamage() { // not set properly yet
  BulletType = 0;
  Player = 63;
  if (Player == PlayerID) {
    Player = random(63);
  }
  Damage = 100;
  Critical = 0;
  Power = 0;
  SetIRProtocol();
}
void LightDamage() { // not set properly yet
  BulletType = 0;
  Player = 63;
  if (Player == PlayerID) {
    Player = random(63);
  }
  Damage = 15;
  Critical = 0;
  Power = 0;
  SetIRProtocol();
}
void SwapBRX() {
  
}
void ControlPointLost() {
  BulletType = 15;
  Player = 63;
  if (Player == PlayerID) {
    Player = random(63);
  }
  if (TeamID == 2) {
    Team = 0;
  } else {
    Team = 2;
  }
  Damage = 50;
  Critical = 0;
  Power = 0;
  SetIRProtocol();
}
void ControlPointCaptured() {
  BulletType = 15;
  Player = 0;
  if (Player == PlayerID) {
    Player = random(64);
  }
  Team = TeamID;
  Damage = 50;
  Critical = 0;
  Power = 0;
  SetIRProtocol();
}
void GasPlayers() {
  BulletType = 11;
  Player = 64;
  Team = TeamID;
  Damage = 1;
  Critical = 0;
  Power = 1;
  SetIRProtocol();
}
void OwnTheZone() {
  
}
void SetBRPIRProtocol() {
// Set Player ID
if (Player==0) {P[0]=500; P[1]=500; P[2]=500; P[3]=500; P[4]=500; P[5]=500; P[6]=500;}
if (Player==1) {P[0]=500; P[1]=500; P[2]=500; P[3]=500; P[4]=500; P[5]=500; P[6]=1000;}
if (Player==2) {P[0]=500; P[1]=500; P[2]=500; P[3]=500; P[4]=500; P[5]=1000; P[6]=500;}
if (Player==3) {P[0]=500; P[1]=500; P[2]=500; P[3]=500; P[4]=500; P[5]=1000; P[6]=1000;}
if (Player==4) {P[0]=500; P[1]=500; P[2]=500; P[3]=500; P[4]=1000; P[5]=500; P[6]=500;}
if (Player==5) {P[0]=500; P[1]=500; P[2]=500; P[3]=500; P[4]=1000; P[5]=500; P[6]=1000;}
if (Player==6) {P[0]=500; P[1]=500; P[2]=500; P[3]=500; P[4]=1000; P[5]=1000; P[6]=500;}
if (Player==7) {P[0]=500; P[1]=500; P[2]=500; P[3]=500; P[4]=1000; P[5]=1000; P[6]=1000;}
if (Player==8) {P[0]=500; P[1]=500; P[2]=500; P[3]=1000; P[4]=500; P[5]=500; P[6]=500;}
if (Player==9) {P[0]=500; P[1]=500; P[2]=500; P[3]=1000; P[4]=500; P[5]=500; P[6]=1000;}
if (Player==10) {P[0]=500; P[1]=500; P[2]=500; P[3]=1000; P[4]=500; P[5]=1000; P[6]=500;}
if (Player==11) {P[0]=500; P[1]=500; P[2]=500; P[3]=1000; P[4]=500; P[5]=1000; P[6]=1000;}
if (Player==12) {P[0]=500; P[1]=500; P[2]=500; P[3]=1000; P[4]=1000; P[5]=500; P[6]=500;}
if (Player==13) {P[0]=500; P[1]=500; P[2]=500; P[3]=1000; P[4]=1000; P[5]=500; P[6]=1000;}
if (Player==14) {P[0]=500; P[1]=500; P[2]=500; P[3]=1000; P[4]=1000; P[5]=1000; P[6]=500;}
if (Player==15) {P[0]=500; P[1]=500; P[2]=500; P[3]=1000; P[4]=1000; P[5]=1000; P[6]=1000;}
if (Player==16) {P[0]=500; P[1]=500; P[2]=1000; P[3]=500; P[4]=500; P[5]=500; P[6]=500;}
if (Player==17) {P[0]=500; P[1]=500; P[2]=1000; P[3]=500; P[4]=500; P[5]=500; P[6]=1000;}
if (Player==18) {P[0]=500; P[1]=500; P[2]=1000; P[3]=500; P[4]=500; P[5]=1000; P[6]=500;}
if (Player==19) {P[0]=500; P[1]=500; P[2]=1000; P[3]=500; P[4]=500; P[5]=1000; P[6]=1000;}
if (Player==20) {P[0]=500; P[1]=500; P[2]=1000; P[3]=500; P[4]=1000; P[5]=500; P[6]=500;}
if (Player==21) {P[0]=500; P[1]=500; P[2]=1000; P[3]=500; P[4]=1000; P[5]=500; P[6]=1000;}
if (Player==22) {P[0]=500; P[1]=500; P[2]=1000; P[3]=500; P[4]=1000; P[5]=1000; P[6]=500;}
if (Player==23) {P[0]=500; P[1]=500; P[2]=1000; P[3]=500; P[4]=1000; P[5]=1000; P[6]=1000;}
if (Player==24) {P[0]=500; P[1]=500; P[2]=1000; P[3]=1000; P[4]=500; P[5]=500; P[6]=500;}
if (Player==25) {P[0]=500; P[1]=500; P[2]=1000; P[3]=1000; P[4]=500; P[5]=500; P[6]=1000;}
if (Player==26) {P[0]=500; P[1]=500; P[2]=1000; P[3]=1000; P[4]=500; P[5]=1000; P[6]=500;}
if (Player==27) {P[0]=500; P[1]=500; P[2]=1000; P[3]=1000; P[4]=500; P[5]=1000; P[6]=1000;}
if (Player==28) {P[0]=500; P[1]=500; P[2]=1000; P[3]=1000; P[4]=1000; P[5]=500; P[6]=500;}
if (Player==29) {P[0]=500; P[1]=500; P[2]=1000; P[3]=1000; P[4]=1000; P[5]=500; P[6]=1000;}
if (Player==30) {P[0]=500; P[1]=500; P[2]=1000; P[3]=1000; P[4]=1000; P[5]=1000; P[6]=500;}
if (Player==31) {P[0]=500; P[1]=500; P[2]=1000; P[3]=1000; P[4]=1000; P[5]=1000; P[6]=1000;}
if (Player==32) {P[0]=500; P[1]=1000; P[2]=500; P[3]=500; P[4]=500; P[5]=500; P[6]=500;}
if (Player==33) {P[0]=500; P[1]=1000; P[2]=500; P[3]=500; P[4]=500; P[5]=500; P[6]=1000;}
if (Player==34) {P[0]=500; P[1]=1000; P[2]=500; P[3]=500; P[4]=500; P[5]=1000; P[6]=500;}
if (Player==35) {P[0]=500; P[1]=1000; P[2]=500; P[3]=500; P[4]=500; P[5]=1000; P[6]=1000;}
if (Player==36) {P[0]=500; P[1]=1000; P[2]=500; P[3]=500; P[4]=1000; P[5]=500; P[6]=500;}
if (Player==37) {P[0]=500; P[1]=1000; P[2]=500; P[3]=500; P[4]=1000; P[5]=500; P[6]=1000;}
if (Player==38) {P[0]=500; P[1]=1000; P[2]=500; P[3]=500; P[4]=1000; P[5]=1000; P[6]=500;}
if (Player==39) {P[0]=500; P[1]=1000; P[2]=500; P[3]=500; P[4]=1000; P[5]=1000; P[6]=1000;}
if (Player==40) {P[0]=500; P[1]=1000; P[2]=500; P[3]=1000; P[4]=500; P[5]=500; P[6]=500;}
if (Player==41) {P[0]=500; P[1]=1000; P[2]=500; P[3]=1000; P[4]=500; P[5]=500; P[6]=1000;}
if (Player==42) {P[0]=500; P[1]=1000; P[2]=500; P[3]=1000; P[4]=500; P[5]=1000; P[6]=500;}
if (Player==43) {P[0]=500; P[1]=1000; P[2]=500; P[3]=1000; P[4]=500; P[5]=1000; P[6]=1000;}
if (Player==44) {P[0]=500; P[1]=1000; P[2]=500; P[3]=1000; P[4]=1000; P[5]=500; P[6]=500;}
if (Player==45) {P[0]=500; P[1]=1000; P[2]=500; P[3]=1000; P[4]=1000; P[5]=500; P[6]=1000;}
if (Player==46) {P[0]=500; P[1]=1000; P[2]=500; P[3]=1000; P[4]=1000; P[5]=1000; P[6]=500;}
if (Player==47) {P[0]=500; P[1]=1000; P[2]=500; P[3]=1000; P[4]=1000; P[5]=1000; P[6]=1000;}
if (Player==48) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=500; P[4]=500; P[5]=500; P[6]=500;}
if (Player==49) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=500; P[4]=500; P[5]=500; P[6]=1000;}
if (Player==50) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=500; P[4]=500; P[5]=1000; P[6]=500;}
if (Player==51) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=500; P[4]=500; P[5]=1000; P[6]=1000;}
if (Player==52) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=500; P[4]=1000; P[5]=500; P[6]=500;}
if (Player==53) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=500; P[4]=1000; P[5]=500; P[6]=1000;}
if (Player==54) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=500; P[4]=1000; P[5]=1000; P[6]=500;}
if (Player==55) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=500; P[4]=1000; P[5]=1000; P[6]=1000;}
if (Player==56) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=1000; P[4]=500; P[5]=500; P[6]=500;}
if (Player==57) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=1000; P[4]=500; P[5]=500; P[6]=1000;}
if (Player==58) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=1000; P[4]=500; P[5]=1000; P[6]=500;}
if (Player==59) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=1000; P[4]=500; P[5]=1000; P[6]=1000;}
if (Player==60) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=1000; P[4]=1000; P[5]=500; P[6]=500;}
if (Player==61) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=1000; P[4]=1000; P[5]=500; P[6]=1000;}
if (Player==62) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=1000; P[4]=1000; P[5]=1000; P[6]=500;}
if (Player==63) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=1000; P[4]=1000; P[5]=1000; P[6]=1000;}
if (Player==64) {P[0]=500; P[1]=500; P[2]=500; P[3]=500; P[4]=500; P[5]=500; P[6]=500;}
if (Player==65) {P[0]=500; P[1]=500; P[2]=500; P[3]=500; P[4]=500; P[5]=500; P[6]=1000;}
if (Player==66) {P[0]=500; P[1]=500; P[2]=500; P[3]=500; P[4]=500; P[5]=1000; P[6]=500;}
if (Player==67) {P[0]=500; P[1]=500; P[2]=500; P[3]=500; P[4]=500; P[5]=1000; P[6]=1000;}
if (Player==68) {P[0]=500; P[1]=500; P[2]=500; P[3]=500; P[4]=1000; P[5]=500; P[6]=500;}
if (Player==69) {P[0]=500; P[1]=500; P[2]=500; P[3]=500; P[4]=1000; P[5]=500; P[6]=1000;}
if (Player==70) {P[0]=500; P[1]=500; P[2]=500; P[3]=500; P[4]=1000; P[5]=1000; P[6]=500;}
if (Player==71) {P[0]=500; P[1]=500; P[2]=500; P[3]=500; P[4]=1000; P[5]=1000; P[6]=1000;}
if (Player==72) {P[0]=500; P[1]=500; P[2]=500; P[3]=1000; P[4]=500; P[5]=500; P[6]=500;}
if (Player==73) {P[0]=500; P[1]=500; P[2]=500; P[3]=1000; P[4]=500; P[5]=500; P[6]=1000;}
if (Player==74) {P[0]=500; P[1]=500; P[2]=500; P[3]=1000; P[4]=500; P[5]=1000; P[6]=500;}
if (Player==75) {P[0]=500; P[1]=500; P[2]=500; P[3]=1000; P[4]=500; P[5]=1000; P[6]=1000;}
if (Player==76) {P[0]=500; P[1]=500; P[2]=500; P[3]=1000; P[4]=1000; P[5]=500; P[6]=500;}
if (Player==77) {P[0]=500; P[1]=500; P[2]=500; P[3]=1000; P[4]=1000; P[5]=500; P[6]=1000;}
if (Player==78) {P[0]=500; P[1]=500; P[2]=500; P[3]=1000; P[4]=1000; P[5]=1000; P[6]=500;}
if (Player==79) {P[0]=500; P[1]=500; P[2]=500; P[3]=1000; P[4]=1000; P[5]=1000; P[6]=1000;}
if (Player==80) {P[0]=500; P[1]=500; P[2]=1000; P[3]=500; P[4]=500; P[5]=500; P[6]=500;}
if (Player==81) {P[0]=500; P[1]=500; P[2]=1000; P[3]=500; P[4]=500; P[5]=500; P[6]=1000;}
if (Player==82) {P[0]=500; P[1]=500; P[2]=1000; P[3]=500; P[4]=500; P[5]=1000; P[6]=500;}
if (Player==83) {P[0]=500; P[1]=500; P[2]=1000; P[3]=500; P[4]=500; P[5]=1000; P[6]=1000;}
if (Player==84) {P[0]=500; P[1]=500; P[2]=1000; P[3]=500; P[4]=1000; P[5]=500; P[6]=500;}
if (Player==85) {P[0]=500; P[1]=500; P[2]=1000; P[3]=500; P[4]=1000; P[5]=500; P[6]=1000;}
if (Player==86) {P[0]=500; P[1]=500; P[2]=1000; P[3]=500; P[4]=1000; P[5]=1000; P[6]=500;}
if (Player==87) {P[0]=500; P[1]=500; P[2]=1000; P[3]=500; P[4]=1000; P[5]=1000; P[6]=1000;}
if (Player==88) {P[0]=500; P[1]=500; P[2]=1000; P[3]=1000; P[4]=500; P[5]=500; P[6]=500;}
if (Player==89) {P[0]=500; P[1]=500; P[2]=1000; P[3]=1000; P[4]=500; P[5]=500; P[6]=1000;}
if (Player==90) {P[0]=500; P[1]=500; P[2]=1000; P[3]=1000; P[4]=500; P[5]=1000; P[6]=500;}
if (Player==91) {P[0]=500; P[1]=500; P[2]=1000; P[3]=1000; P[4]=500; P[5]=1000; P[6]=1000;}
if (Player==92) {P[0]=500; P[1]=500; P[2]=1000; P[3]=1000; P[4]=1000; P[5]=500; P[6]=500;}
if (Player==93) {P[0]=500; P[1]=500; P[2]=1000; P[3]=1000; P[4]=1000; P[5]=500; P[6]=1000;}
if (Player==94) {P[0]=500; P[1]=500; P[2]=1000; P[3]=1000; P[4]=1000; P[5]=1000; P[6]=500;}
if (Player==95) {P[0]=500; P[1]=500; P[2]=1000; P[3]=1000; P[4]=1000; P[5]=1000; P[6]=1000;}
if (Player==96) {P[0]=500; P[1]=1000; P[2]=500; P[3]=500; P[4]=500; P[5]=500; P[6]=500;}
if (Player==97) {P[0]=500; P[1]=1000; P[2]=500; P[3]=500; P[4]=500; P[5]=500; P[6]=1000;}
if (Player==98) {P[0]=500; P[1]=1000; P[2]=500; P[3]=500; P[4]=500; P[5]=1000; P[6]=500;}
if (Player==99) {P[0]=500; P[1]=1000; P[2]=500; P[3]=500; P[4]=500; P[5]=1000; P[6]=1000;}
if (Player==100) {P[0]=500; P[1]=1000; P[2]=500; P[3]=500; P[4]=1000; P[5]=500; P[6]=500;}
if (Player==101) {P[0]=500; P[1]=1000; P[2]=500; P[3]=500; P[4]=1000; P[5]=500; P[6]=1000;}
if (Player==102) {P[0]=500; P[1]=1000; P[2]=500; P[3]=500; P[4]=1000; P[5]=1000; P[6]=500;}
if (Player==103) {P[0]=500; P[1]=1000; P[2]=500; P[3]=500; P[4]=1000; P[5]=1000; P[6]=1000;}
if (Player==104) {P[0]=500; P[1]=1000; P[2]=500; P[3]=1000; P[4]=500; P[5]=500; P[6]=500;}
if (Player==105) {P[0]=500; P[1]=1000; P[2]=500; P[3]=1000; P[4]=500; P[5]=500; P[6]=1000;}
if (Player==106) {P[0]=500; P[1]=1000; P[2]=500; P[3]=1000; P[4]=500; P[5]=1000; P[6]=500;}
if (Player==107) {P[0]=500; P[1]=1000; P[2]=500; P[3]=1000; P[4]=500; P[5]=1000; P[6]=1000;}
if (Player==108) {P[0]=500; P[1]=1000; P[2]=500; P[3]=1000; P[4]=1000; P[5]=500; P[6]=500;}
if (Player==109) {P[0]=500; P[1]=1000; P[2]=500; P[3]=1000; P[4]=1000; P[5]=500; P[6]=1000;}
if (Player==110) {P[0]=500; P[1]=1000; P[2]=500; P[3]=1000; P[4]=1000; P[5]=1000; P[6]=500;}
if (Player==111) {P[0]=500; P[1]=1000; P[2]=500; P[3]=1000; P[4]=1000; P[5]=1000; P[6]=1000;}
if (Player==112) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=500; P[4]=500; P[5]=500; P[6]=500;}
if (Player==113) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=500; P[4]=500; P[5]=500; P[6]=1000;}
if (Player==114) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=500; P[4]=500; P[5]=1000; P[6]=500;}
if (Player==115) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=500; P[4]=500; P[5]=1000; P[6]=1000;}
if (Player==116) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=500; P[4]=1000; P[5]=500; P[6]=500;}
if (Player==117) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=500; P[4]=1000; P[5]=500; P[6]=1000;}
if (Player==118) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=500; P[4]=1000; P[5]=1000; P[6]=500;}
if (Player==119) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=500; P[4]=1000; P[5]=1000; P[6]=1000;}
if (Player==120) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=1000; P[4]=500; P[5]=500; P[6]=500;}
if (Player==121) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=1000; P[4]=500; P[5]=500; P[6]=1000;}
if (Player==122) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=1000; P[4]=500; P[5]=1000; P[6]=500;}
if (Player==123) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=1000; P[4]=500; P[5]=1000; P[6]=1000;}
if (Player==124) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=1000; P[4]=1000; P[5]=500; P[6]=500;}
if (Player==125) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=1000; P[4]=1000; P[5]=500; P[6]=1000;}
if (Player==126) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=1000; P[4]=1000; P[5]=1000; P[6]=500;}
if (Player==127) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=1000; P[4]=1000; P[5]=1000; P[6]=1000;}
// Set Bullet Type
if (BulletType==0) {B[0]=500; B[1]=500; B[2]=500; B[3]=500;}
if (BulletType==1) {B[0]=500; B[1]=500; B[2]=500; B[3]=1000;}
if (BulletType==2) {B[0]=500; B[1]=500; B[2]=1000; B[3]=500;}
if (BulletType==3) {B[0]=500; B[1]=500; B[2]=1000; B[3]=1000;}
if (BulletType==4) {B[0]=500; B[1]=1000; B[2]=500; B[3]=500;}
if (BulletType==5) {B[0]=500; B[1]=1000; B[2]=500; B[3]=1000;}
if (BulletType==6) {B[0]=500; B[1]=1000; B[2]=1000; B[3]=500;}
if (BulletType==7) {B[0]=500; B[1]=1000; B[2]=1000; B[3]=1000;}
if (BulletType==8) {B[0]=1000; B[1]=500; B[2]=500; B[3]=500;}
if (BulletType==9) {B[0]=1000; B[1]=500; B[2]=500; B[3]=1000;}
if (BulletType==10) {B[0]=1000; B[1]=500; B[2]=1000; B[3]=500;}
if (BulletType==11) {B[0]=1000; B[1]=500; B[2]=1000; B[3]=1000;}
if (BulletType==12) {B[0]=1000; B[1]=1000; B[2]=500; B[3]=500;}
if (BulletType==13) {B[0]=1000; B[1]=1000; B[2]=500; B[3]=1000;}
if (BulletType==14) {B[0]=1000; B[1]=1000; B[2]=1000; B[3]=500;}
if (BulletType==15) {B[0]=1000; B[1]=1000; B[2]=1000; B[3]=1000;}
// SETS PLAYER TEAM
if (Team==0) {T[0]=500; T[1]=500; T[2]=500;} // RED
if (Team==1) {T[0]=500; T[1]=500; T[2]=1000;} // BLUE
if (Team==2) {T[0]=500; T[1]=1000; T[2]=500;} // YELLOW
if (Team==3) {T[0]=500; T[1]=1000; T[0]=1000;} // GREEN
if (Team==4) {T[0]=1000; T[1]=500; T[0]=500;} // ?
if (Team==5) {T[0]=1000; T[1]=500; T[0]=1000;} // ?
if (Team==6) {T[0]=1000; T[1]=1000; T[0]=500;} // ?
if (Team==7) {T[0]=1000; T[1]=1000; T[0]=1000;} // ?

// SETS Damage
if (Damage==0) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==1) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==2) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==3) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==4) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==5) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==6) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==7) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==8) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==9) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==10) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==11) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==12) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==13) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==14) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==15) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==16) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==17) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==18) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==19) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==20) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==21) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==22) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==23) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==24) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==25) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==26) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==27) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==28) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==29) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==30) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==31) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==32) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==33) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==34) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==35) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==36) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==37) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==38) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==39) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==40) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==41) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==42) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==43) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==44) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==45) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==46) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==47) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==48) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==49) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==50) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==51) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==52) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==53) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==54) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==55) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==56) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==57) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==58) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==59) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==60) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==61) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==62) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==63) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==64) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==65) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==66) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==67) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==68) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==69) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==70) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==71) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==72) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==73) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==74) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==75) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==76) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==77) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==78) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==79) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==80) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==81) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==82) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==83) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==84) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==85) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==86) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==87) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==88) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==89) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==90) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==91) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==92) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==93) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==94) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==95) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==96) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==97) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==98) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==99) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==100) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==101) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==102) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==103) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==104) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==105) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==106) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==107) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==108) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==109) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==110) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==111) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==112) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==113) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==114) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==115) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==116) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==117) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==118) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==119) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==120) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==121) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==122) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==123) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==124) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==125) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==126) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==127) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==128) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==129) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==130) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==131) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==132) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==133) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==134) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==135) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==136) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==137) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==138) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==139) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==140) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==141) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==142) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==143) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==144) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==145) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==146) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==147) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==148) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==149) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==150) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==151) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==152) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==153) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==154) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==155) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==156) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==157) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==158) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==159) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==160) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==161) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==162) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==163) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==164) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==165) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==166) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==167) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==168) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==169) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==170) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==171) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==172) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==173) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==174) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==175) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==176) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==177) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==178) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==179) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==180) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==181) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==182) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==183) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==184) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==185) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==186) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==187) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==188) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==189) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==190) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==191) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==192) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==193) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==194) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==195) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==196) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==197) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==198) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==199) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==200) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==201) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==202) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==203) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==204) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==205) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==206) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==207) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==208) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==209) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==210) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==211) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==212) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==213) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==214) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==215) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==216) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==217) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==218) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==219) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==220) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==221) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==222) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==223) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==224) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==225) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==226) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==227) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==228) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==229) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==230) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==231) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==232) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==233) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==234) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==235) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==236) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==237) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==238) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==239) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==240) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==241) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==242) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==243) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==244) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==245) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==246) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==247) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==248) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==249) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==250) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==251) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==252) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==253) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==254) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==255) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
// Sets Critical
if (Critical==1){C[0]=1000;}
  else {C[0]=500;}
brpparitycheck();
//PrintTag();
SendBRPIR();
}
void SetIRProtocol() {
// Set Player ID
if (Player==0) {P[0]=500; P[1]=500; P[2]=500; P[3]=500; P[4]=500; P[5]=500;}
if (Player==1) {P[0]=500; P[1]=500; P[2]=500; P[3]=500; P[4]=500; P[5]=1000;}
if (Player==2) {P[0]=500; P[1]=500; P[2]=500; P[3]=500; P[4]=1000; P[5]=500;}
if (Player==3) {P[0]=500; P[1]=500; P[2]=500; P[3]=500; P[4]=1000; P[5]=1000;}
if (Player==4) {P[0]=500; P[1]=500; P[2]=500; P[3]=1000; P[4]=500; P[5]=500;}
if (Player==5) {P[0]=500; P[1]=500; P[2]=500; P[3]=1000; P[4]=500; P[5]=1000;}
if (Player==6) {P[0]=500; P[1]=500; P[2]=500; P[3]=1000; P[4]=1000; P[5]=500;}
if (Player==7) {P[0]=500; P[1]=500; P[2]=500; P[3]=1000; P[4]=1000; P[5]=1000;}
if (Player==8) {P[0]=500; P[1]=500; P[2]=1000; P[3]=500; P[4]=500; P[5]=500;}
if (Player==9) {P[0]=500; P[1]=500; P[2]=1000; P[3]=500; P[4]=500; P[5]=1000;}
if (Player==10) {P[0]=500; P[1]=500; P[2]=1000; P[3]=500; P[4]=1000; P[5]=500;}
if (Player==11) {P[0]=500; P[1]=500; P[2]=1000; P[3]=500; P[4]=1000; P[5]=1000;}
if (Player==12) {P[0]=500; P[1]=500; P[2]=1000; P[3]=1000; P[4]=500; P[5]=500;}
if (Player==13) {P[0]=500; P[1]=500; P[2]=1000; P[3]=1000; P[4]=500; P[5]=1000;}
if (Player==14) {P[0]=500; P[1]=500; P[2]=1000; P[3]=1000; P[4]=1000; P[5]=500;}
if (Player==15) {P[0]=500; P[1]=500; P[2]=1000; P[3]=1000; P[4]=1000; P[5]=1000;}
if (Player==16) {P[0]=500; P[1]=1000; P[2]=500; P[3]=500; P[4]=500; P[5]=500;}
if (Player==17) {P[0]=500; P[1]=1000; P[2]=500; P[3]=500; P[4]=500; P[5]=1000;}
if (Player==18) {P[0]=500; P[1]=1000; P[2]=500; P[3]=500; P[4]=1000; P[5]=500;}
if (Player==19) {P[0]=500; P[1]=1000; P[2]=500; P[3]=500; P[4]=1000; P[5]=1000;}
if (Player==20) {P[0]=500; P[1]=1000; P[2]=500; P[3]=1000; P[4]=500; P[5]=500;}
if (Player==21) {P[0]=500; P[1]=1000; P[2]=500; P[3]=1000; P[4]=500; P[5]=1000;}
if (Player==22) {P[0]=500; P[1]=1000; P[2]=500; P[3]=1000; P[4]=1000; P[5]=500;}
if (Player==23) {P[0]=500; P[1]=1000; P[2]=500; P[3]=1000; P[4]=1000; P[5]=1000;}
if (Player==24) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=500; P[4]=500; P[5]=500;}
if (Player==25) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=500; P[4]=500; P[5]=1000;}
if (Player==26) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=500; P[4]=1000; P[5]=500;}
if (Player==27) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=500; P[4]=1000; P[5]=1000;}
if (Player==28) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=1000; P[4]=500; P[5]=500;}
if (Player==29) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=1000; P[4]=500; P[5]=1000;}
if (Player==30) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=1000; P[4]=1000; P[5]=500;}
if (Player==31) {P[0]=500; P[1]=1000; P[2]=1000; P[3]=1000; P[4]=1000; P[5]=1000;}
if (Player==32) {P[0]=1000; P[1]=500; P[2]=500; P[3]=500; P[4]=500; P[5]=500;}
if (Player==33) {P[0]=1000; P[1]=500; P[2]=500; P[3]=500; P[4]=500; P[5]=1000;}
if (Player==34) {P[0]=1000; P[1]=500; P[2]=500; P[3]=500; P[4]=1000; P[5]=500;}
if (Player==35) {P[0]=1000; P[1]=500; P[2]=500; P[3]=500; P[4]=1000; P[5]=1000;}
if (Player==36) {P[0]=1000; P[1]=500; P[2]=500; P[3]=1000; P[4]=500; P[5]=500;}
if (Player==37) {P[0]=1000; P[1]=500; P[2]=500; P[3]=1000; P[4]=500; P[5]=1000;}
if (Player==38) {P[0]=1000; P[1]=500; P[2]=500; P[3]=1000; P[4]=1000; P[5]=500;}
if (Player==39) {P[0]=1000; P[1]=500; P[2]=500; P[3]=1000; P[4]=1000; P[5]=1000;}
if (Player==40) {P[0]=1000; P[1]=500; P[2]=1000; P[3]=500; P[4]=500; P[5]=500;}
if (Player==41) {P[0]=1000; P[1]=500; P[2]=1000; P[3]=500; P[4]=500; P[5]=1000;}
if (Player==42) {P[0]=1000; P[1]=500; P[2]=1000; P[3]=500; P[4]=1000; P[5]=500;}
if (Player==43) {P[0]=1000; P[1]=500; P[2]=1000; P[3]=500; P[4]=1000; P[5]=1000;}
if (Player==44) {P[0]=1000; P[1]=500; P[2]=1000; P[3]=1000; P[4]=500; P[5]=500;}
if (Player==45) {P[0]=1000; P[1]=500; P[2]=1000; P[3]=1000; P[4]=500; P[5]=1000;}
if (Player==46) {P[0]=1000; P[1]=500; P[2]=1000; P[3]=1000; P[4]=1000; P[5]=500;}
if (Player==47) {P[0]=1000; P[1]=500; P[2]=1000; P[3]=1000; P[4]=1000; P[5]=1000;}
if (Player==48) {P[0]=1000; P[1]=1000; P[2]=500; P[3]=500; P[4]=500; P[5]=500;}
if (Player==49) {P[0]=1000; P[1]=1000; P[2]=500; P[3]=500; P[4]=500; P[5]=1000;}
if (Player==50) {P[0]=1000; P[1]=1000; P[2]=500; P[3]=500; P[4]=1000; P[5]=500;}
if (Player==51) {P[0]=1000; P[1]=1000; P[2]=500; P[3]=500; P[4]=1000; P[5]=1000;}
if (Player==52) {P[0]=1000; P[1]=1000; P[2]=500; P[3]=1000; P[4]=500; P[5]=500;}
if (Player==53) {P[0]=1000; P[1]=1000; P[2]=500; P[3]=1000; P[4]=500; P[5]=1000;}
if (Player==54) {P[0]=1000; P[1]=1000; P[2]=500; P[3]=1000; P[4]=1000; P[5]=500;}
if (Player==55) {P[0]=1000; P[1]=1000; P[2]=500; P[3]=1000; P[4]=1000; P[5]=1000;}
if (Player==56) {P[0]=1000; P[1]=1000; P[2]=1000; P[3]=500; P[4]=500; P[5]=500;}
if (Player==57) {P[0]=1000; P[1]=1000; P[2]=1000; P[3]=500; P[4]=500; P[5]=1000;}
if (Player==58) {P[0]=1000; P[1]=1000; P[2]=1000; P[3]=500; P[4]=1000; P[5]=500;}
if (Player==59) {P[0]=1000; P[1]=1000; P[2]=1000; P[3]=500; P[4]=1000; P[5]=1000;}
if (Player==60) {P[0]=1000; P[1]=1000; P[2]=1000; P[3]=1000; P[4]=500; P[5]=500;}
if (Player==61) {P[0]=1000; P[1]=1000; P[2]=1000; P[3]=1000; P[4]=500; P[5]=1000;}
if (Player==62) {P[0]=1000; P[1]=1000; P[2]=1000; P[3]=1000; P[4]=1000; P[5]=500;}
if (Player==63) {P[0]=1000; P[1]=1000; P[2]=1000; P[3]=1000; P[4]=1000; P[5]=1000;}
// Set Bullet Type
if (BulletType==0) {B[0]=500; B[1]=500; B[2]=500; B[3]=500;}
if (BulletType==1) {B[0]=500; B[1]=500; B[2]=500; B[3]=1000;}
if (BulletType==2) {B[0]=500; B[1]=500; B[2]=1000; B[3]=500;}
if (BulletType==3) {B[0]=500; B[1]=500; B[2]=1000; B[3]=1000;}
if (BulletType==4) {B[0]=500; B[1]=1000; B[2]=500; B[3]=500;}
if (BulletType==5) {B[0]=500; B[1]=1000; B[2]=500; B[3]=1000;}
if (BulletType==6) {B[0]=500; B[1]=1000; B[2]=1000; B[3]=500;}
if (BulletType==7) {B[0]=500; B[1]=1000; B[2]=1000; B[3]=1000;}
if (BulletType==8) {B[0]=1000; B[1]=500; B[2]=500; B[3]=500;}
if (BulletType==9) {B[0]=1000; B[1]=500; B[2]=500; B[3]=1000;}
if (BulletType==10) {B[0]=1000; B[1]=500; B[2]=1000; B[3]=500;}
if (BulletType==11) {B[0]=1000; B[1]=500; B[2]=1000; B[3]=1000;}
if (BulletType==12) {B[0]=1000; B[1]=1000; B[2]=500; B[3]=500;}
if (BulletType==13) {B[0]=1000; B[1]=1000; B[2]=500; B[3]=1000;}
if (BulletType==14) {B[0]=1000; B[1]=1000; B[2]=1000; B[3]=500;}
if (BulletType==15) {B[0]=1000; B[1]=1000; B[2]=1000; B[3]=1000;}
// SETS PLAYER TEAM
if (Team==0) {T[0]=500; T[1]=500;} // RED
if (Team==1) {T[0]=500; T[1]=1000;} // BLUE
if (Team==2) {T[0]=1000; T[1]=500;} // YELLOW
if (Team==3) {T[0]=1000; T[1]=1000;} // GREEN
// SETS Damage
if (Damage==0) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==1) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==2) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==3) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==4) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==5) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==6) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==7) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==8) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==9) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==10) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==11) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==12) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==13) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==14) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==15) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==16) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==17) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==18) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==19) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==20) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==21) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==22) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==23) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==24) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==25) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==26) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==27) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==28) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==29) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==30) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==31) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==32) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==33) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==34) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==35) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==36) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==37) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==38) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==39) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==40) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==41) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==42) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==43) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==44) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==45) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==46) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==47) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==48) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==49) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==50) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==51) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==52) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==53) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==54) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==55) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==56) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==57) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==58) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==59) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==60) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==61) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==62) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==63) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==64) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==65) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==66) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==67) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==68) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==69) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==70) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==71) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==72) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==73) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==74) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==75) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==76) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==77) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==78) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==79) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==80) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==81) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==82) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==83) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==84) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==85) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==86) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==87) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==88) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==89) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==90) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==91) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==92) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==93) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==94) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==95) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==96) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==97) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==98) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==99) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==100) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==101) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==102) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==103) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==104) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==105) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==106) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==107) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==108) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==109) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==110) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==111) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==112) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==113) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==114) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==115) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==116) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==117) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==118) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==119) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==120) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==121) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==122) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==123) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==124) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==125) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==126) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==127) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==128) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==129) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==130) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==131) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==132) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==133) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==134) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==135) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==136) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==137) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==138) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==139) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==140) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==141) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==142) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==143) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==144) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==145) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==146) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==147) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==148) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==149) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==150) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==151) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==152) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==153) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==154) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==155) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==156) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==157) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==158) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==159) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==160) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==161) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==162) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==163) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==164) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==165) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==166) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==167) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==168) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==169) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==170) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==171) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==172) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==173) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==174) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==175) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==176) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==177) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==178) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==179) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==180) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==181) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==182) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==183) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==184) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==185) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==186) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==187) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==188) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==189) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==190) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==191) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==192) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==193) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==194) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==195) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==196) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==197) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==198) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==199) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==200) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==201) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==202) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==203) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==204) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==205) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==206) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==207) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==208) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==209) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==210) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==211) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==212) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==213) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==214) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==215) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==216) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==217) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==218) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==219) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==220) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==221) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==222) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==223) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==224) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==225) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==226) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==227) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==228) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==229) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==230) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==231) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==232) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==233) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==234) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==235) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==236) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==237) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==238) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==239) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==240) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==241) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==242) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==243) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==244) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==245) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==246) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==247) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==248) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==249) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==250) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==251) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==252) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==253) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==254) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==255) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
// Sets Critical
if (Critical==1){C[0]=1000;}
  else {C[0]=500;}
// Sets Power
if (Power==0){X[0] = 500; X[1] = 500;}
if (Power==1){X[0] = 500; X[1] = 1000;}
if (Power==2){X[0] = 1000; X[1] = 500;}
if (Power==3){X[0] = 1000; X[1] = 1000;}
paritycheck();
PrintTag();
SendIR();
}
void SetEvolverIRProtocol() {
Serial.println("IR Data Being Sent ");
// Set Player ID
Serial.println("Player: " + String(Player));
if (Player==0) {P[0]=500; P[1]=500; P[2]=500; P[3]=500; P[4]=500; P[5]=500;}
if (Player==1) {P[0]=500; P[1]=500; P[2]=500; P[3]=500; P[4]=500; P[5]=1000;}
if (Player==2) {P[0]=500; P[1]=500; P[2]=500; P[3]=500; P[4]=1000; P[5]=500;}
if (Player==3) {P[0]=500; P[1]=500; P[2]=500; P[3]=500; P[4]=1000; P[5]=1000;}
if (Player==4) {P[0]=500; P[1]=500; P[2]=500; P[3]=1000; P[4]=500; P[5]=500;}
if (Player==5) {P[0]=500; P[1]=500; P[2]=500; P[3]=1000; P[4]=500; P[5]=1000;}
if (Player==6) {P[0]=500; P[1]=500; P[2]=500; P[3]=1000; P[4]=1000; P[5]=500;}
if (Player==7) {P[0]=500; P[1]=500; P[2]=500; P[3]=1000; P[4]=1000; P[5]=1000;}
if (Player==8) {P[0]=500; P[1]=500; P[2]=1000; P[3]=500; P[4]=500; P[5]=500;}
if (Player==9) {P[0]=500; P[1]=500; P[2]=1000; P[3]=500; P[4]=500; P[5]=1000;}
if (Player==10) {P[0]=500; P[1]=500; P[2]=1000; P[3]=500; P[4]=1000; P[5]=500;}
if (Player==11) {P[0]=500; P[1]=500; P[2]=1000; P[3]=500; P[4]=1000; P[5]=1000;}
if (Player==12) {P[0]=500; P[1]=500; P[2]=1000; P[3]=1000; P[4]=500; P[5]=500;}
if (Player==13) {P[0]=500; P[1]=500; P[2]=1000; P[3]=1000; P[4]=500; P[5]=1000;}
if (Player==14) {P[0]=500; P[1]=500; P[2]=1000; P[3]=1000; P[4]=1000; P[5]=500;}
if (Player==15) {P[0]=500; P[1]=500; P[2]=1000; P[3]=1000; P[4]=1000; P[5]=1000;}
// Set Bullet Type
Serial.println("Bullet: " + String(BulletType));
if (BulletType==0) {B[0]=500; B[1]=500; B[2]=500; B[3]=500;}
if (BulletType==1) {B[0]=500; B[1]=500; B[2]=500; B[3]=1000;}
if (BulletType==2) {B[0]=500; B[1]=500; B[2]=1000; B[3]=500;}
if (BulletType==3) {B[0]=500; B[1]=500; B[2]=1000; B[3]=1000;}
if (BulletType==4) {B[0]=500; B[1]=1000; B[2]=500; B[3]=500;}
if (BulletType==5) {B[0]=500; B[1]=1000; B[2]=500; B[3]=1000;}
if (BulletType==6) {B[0]=500; B[1]=1000; B[2]=1000; B[3]=500;}
if (BulletType==7) {B[0]=500; B[1]=1000; B[2]=1000; B[3]=1000;}
if (BulletType==8) {B[0]=1000; B[1]=500; B[2]=500; B[3]=500;}
if (BulletType==9) {B[0]=1000; B[1]=500; B[2]=500; B[3]=1000;}
if (BulletType==10) {B[0]=1000; B[1]=500; B[2]=1000; B[3]=500;}
if (BulletType==11) {B[0]=1000; B[1]=500; B[2]=1000; B[3]=1000;}
if (BulletType==12) {B[0]=1000; B[1]=1000; B[2]=500; B[3]=500;}
if (BulletType==13) {B[0]=1000; B[1]=1000; B[2]=500; B[3]=1000;}
if (BulletType==14) {B[0]=1000; B[1]=1000; B[2]=1000; B[3]=500;}
if (BulletType==15) {B[0]=1000; B[1]=1000; B[2]=1000; B[3]=1000;}
// SETS PLAYER TEAM
Serial.println("Team: " + String(Team));
if (Team==0) {T[0]=500; T[1]=500;} // RED
if (Team==1) {T[0]=500; T[1]=1000;} // BLUE
if (Team==2) {T[0]=1000; T[1]=500;} // YELLOW
if (Team==3) {T[0]=1000; T[1]=1000;} // GREEN
// SETS Damage
Serial.println("Damage: " + String(Damage));
if (Damage==0) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==1) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==2) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==3) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==4) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==5) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==6) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==7) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==8) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==9) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==10) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==11) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==12) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==13) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==14) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==15) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==16) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==17) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==18) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==19) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==20) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==21) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==22) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==23) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==24) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==25) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==26) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==27) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==28) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==29) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==30) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==31) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==32) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==33) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==34) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==35) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==36) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==37) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==38) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==39) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==40) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==41) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==42) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==43) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==44) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==45) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==46) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==47) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==48) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==49) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==50) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==51) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==52) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==53) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==54) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==55) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==56) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==57) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==58) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==59) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==60) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==61) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==62) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==63) {D[0] = 500; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==64) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==65) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==66) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==67) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==68) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==69) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==70) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==71) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==72) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==73) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==74) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==75) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==76) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==77) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==78) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==79) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==80) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==81) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==82) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==83) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==84) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==85) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==86) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==87) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==88) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==89) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==90) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==91) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==92) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==93) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==94) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==95) {D[0] = 500; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==96) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==97) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==98) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==99) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==100) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==101) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==102) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==103) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==104) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==105) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==106) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==107) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==108) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==109) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==110) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==111) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==112) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==113) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==114) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==115) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==116) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==117) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==118) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==119) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==120) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==121) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==122) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==123) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==124) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==125) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==126) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==127) {D[0] = 500; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==128) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==129) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==130) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==131) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==132) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==133) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==134) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==135) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==136) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==137) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==138) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==139) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==140) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==141) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==142) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==143) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==144) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==145) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==146) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==147) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==148) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==149) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==150) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==151) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==152) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==153) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==154) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==155) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==156) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==157) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==158) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==159) {D[0] = 1000; D[1] = 500; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==160) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==161) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==162) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==163) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==164) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==165) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==166) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==167) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==168) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==169) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==170) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==171) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==172) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==173) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==174) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==175) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==176) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==177) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==178) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==179) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==180) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==181) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==182) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==183) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==184) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==185) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==186) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==187) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==188) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==189) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==190) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==191) {D[0] = 1000; D[1] = 500; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==192) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==193) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==194) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==195) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==196) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==197) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==198) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==199) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==200) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==201) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==202) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==203) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==204) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==205) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==206) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==207) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==208) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==209) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==210) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==211) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==212) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==213) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==214) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==215) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==216) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==217) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==218) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==219) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==220) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==221) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==222) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==223) {D[0] = 1000; D[1] = 1000; D[2] = 500; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==224) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==225) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==226) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==227) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==228) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==229) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==230) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==231) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==232) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==233) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==234) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==235) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==236) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==237) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==238) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==239) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 500; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==240) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==241) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==242) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==243) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==244) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==245) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==246) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==247) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 500; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
if (Damage==248) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==249) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 500; D[7] = 1000;}
if (Damage==250) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 500;}
if (Damage==251) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 500; D[6] = 1000; D[7] = 1000;}
if (Damage==252) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 500;}
if (Damage==253) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 500; D[7] = 1000;}
if (Damage==254) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 500;}
if (Damage==255) {D[0] = 1000; D[1] = 1000; D[2] = 1000; D[3] = 1000; D[4] = 1000; D[5] = 1000; D[6] = 1000; D[7] = 1000;}
// Sets Critical
if (Critical==1){C[0]=1000; Serial.println("Is Critical");}
  else {C[0]=500;Serial.println("Not Critical");}
Evolverparitycheck();
//PrintTag();
SendEvolverIR();
}
void PrintTag() {
  // prints each individual bit on serial monitor
  Serial.println("Tag!!");
  Serial.print("B0: "); Serial.println(B[0]);
  Serial.print("B1: "); Serial.println(B[1]);
  Serial.print("B2: "); Serial.println(B[2]);
  Serial.print("B3: "); Serial.println(B[3]);
  Serial.print("P0: "); Serial.println(P[0]);
  Serial.print("P1: "); Serial.println(P[1]);
  Serial.print("P2: "); Serial.println(P[2]);
  Serial.print("P3: "); Serial.println(P[3]);
  Serial.print("P4: "); Serial.println(P[4]);
  Serial.print("P5: "); Serial.println(P[5]);
  Serial.print("T0: "); Serial.println(T[0]);
  Serial.print("T1: "); Serial.println(T[1]);
  Serial.print("D0: "); Serial.println(D[0]);
  Serial.print("D1: "); Serial.println(D[1]);
  Serial.print("D2: "); Serial.println(D[2]);
  Serial.print("D3: "); Serial.println(D[3]);
  Serial.print("D4: "); Serial.println(D[4]);
  Serial.print("D5: "); Serial.println(D[5]);
  Serial.print("D6: "); Serial.println(D[6]);
  Serial.print("D7: "); Serial.println(D[7]);
  Serial.print("C0: "); Serial.println(C[0]);
  Serial.print("X0: "); Serial.println(X[0]);
  Serial.print("X1: "); Serial.println(X[1]);
  Serial.print("Z0: "); Serial.println(Z[0]);
  Serial.print("Z1: "); Serial.println(Z[1]);
  Serial.println();
}
void brpparitycheck() {
  // parity changes, parity starts as even as if zero 1's
  // are present in the protocol recieved and being sent
  int parity = 0;
  if (B[0] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (B[1] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (B[2] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (B[3] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (P[0] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (P[1] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (P[2] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (P[3] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (P[4] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (P[5] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (P[6] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (T[0] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (T[1] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (T[2] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (D[0] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (D[1] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (D[2] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (D[3] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (D[4] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (D[5] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (D[6] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (D[7] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (C[0] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (parity == 0) {
    Z[0]=500;
    Z[1]=1000;
  }
  else {
    Z[0]=1000;
    Z[1]=500;
  }
  Serial.println("Modified Parity Bits:");
  Serial.println(Z[0]);
  Serial.println(Z[1]);
}
void paritycheck() {
  // parity changes, parity starts as even as if zero 1's
  // are present in the protocol recieved and being sent
  int parity = 0;
  if (B[0] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (B[1] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (B[2] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (B[3] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (P[0] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (P[1] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (P[2] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (P[3] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (P[4] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (P[5] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (T[0] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (T[1] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (D[0] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (D[1] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (D[2] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (D[3] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (D[4] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (D[5] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (D[6] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (D[7] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (C[0] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (X[0] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (X[1] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (parity == 0) {
    Z[0]=1000;
    Z[1]=500;
  }
  else {
    Z[0]=500;
    Z[1]=1000;
  }
  Serial.println("Modified Parity Bits:");
  Serial.println(Z[0]);
  Serial.println(Z[1]);
}
void Evolverparitycheck() {
  // parity changes, parity starts as even as if zero 1's
  // are present in the protocol recieved and being sent
  int parity = 0;
  if (B[0] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (B[1] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (B[2] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (B[3] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (P[2] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (P[3] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (P[4] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (P[5] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (T[0] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (T[1] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (D[0] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (D[1] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (D[2] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (D[3] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (D[4] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (D[5] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (D[6] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (D[7] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (C[0] > 750) {
    if (parity == 0) {parity = 1;}
    else {parity = 0;}
  }
  if (parity == 0) {
    Z[0]=1000;
    Z[1]=500;
    Z[2]=1000;
  }
  else {
    Z[0]=500;
    Z[1]=1000;
    Z[2]=500;
  }
  Serial.println("Modified Parity Bits:");
  Serial.println(Z[0]);
  Serial.println(Z[1]);
  Serial.println(Z[2]);
}
void VerifyCurrentIRTagSelection() {
  if (LIGHTDAMAGE) {
    LightDamage(); // sets ir protocol to motion sensor yellow and sends it
  }
  if (HEAVYDAMAGE) {
    HeavyDamage(); // sets ir protocol to motion sensor yellow and sends it
  }
  if (RESPAWNSTATION) {
    RespawnStation(); // sets ir protocol to motion sensor yellow and sends it
  }
  if (LOOTBOX) {
    SwapBRX(); // sets ir protocol to motion sensor yellow and sends it
  }
  if (CONTROLPOINTLOST) {
    ControlPointLost();
  }
  if (CONTROLPOINTCAPTURED) {
    ControlPointCaptured();
  }
  if (OWNTHEZONE) {
    OwnTheZone();
  }
}
void ResetAllIRProtocols() {
  CONTROLPOINTLOST = false;
  CONTROLPOINTCAPTURED = false;
  OWNTHEZONE = false;  
  LIGHTDAMAGE = false;
  HEAVYDAMAGE = false;
  RESPAWNSTATION = false;
  LOOTBOX = false;
}

//*************************************************************************
//*************************** LORA OBJECTS ********************************
//*************************************************************************
void ReceiveTransmission() {
  // this is an object used to listen to the serial inputs from the LoRa Module
  if(Serial1.available()>0){ // checking to see if there is anything incoming from the LoRa module
    Serial.println("Got Data"); // prints a notification that data was received
    LoRaDataReceived = Serial1.readString(); // stores the incoming data to the pre set string
    Serial.print("LoRaDataReceived: "); // printing data received
    Serial.println(LoRaDataReceived); // printing data received
    if(LoRaDataReceived.startsWith("+RCV")){ // checking if the data was from another LoRa module
      // convert the received data into a string array for comparing data
      char *ptr = strtok((char*)LoRaDataReceived.c_str(), ",");
      int index = 0;
      while (ptr != NULL){
        tokenStrings[index] = ptr;
        index++;
        ptr = strtok(NULL, ",");  // takes a list of delimiters
        }
        rgboff();
        rgbcyan();
        // the data is now stored into individual strings under tokenStrings[]
        // print out the individual string arrays separately
        Serial.println("received LoRa communication");
        Serial.print("Token 0: ");
        Serial.println(tokenStrings[0]);
        Serial.print("Token 1: ");
        Serial.println(tokenStrings[1]);
        Serial.print("Token 2: ");
        Serial.println(tokenStrings[2]);
        Serial.print("Token 3: ");
        Serial.println(tokenStrings[3]);
        Serial.print("Token 4: ");
        Serial.println(tokenStrings[4]);
        Serial.print("Token 5: ");
        Serial.println(tokenStrings[5]);
        Serial.print("Token 6: ");
        Serial.println(tokenStrings[6]);
        Serial.print("Token 7: ");
        Serial.println(tokenStrings[7]);
        Serial.print("Token 8: ");
        Serial.println(tokenStrings[8]);
        Serial.print("Token 9: ");
        Serial.println(tokenStrings[9]);
        if (tokenStrings[2] = "GAMEOVER") {
          Serial.println("reset scores button pressed");
          Serial.println("All scores reset");
          ResetScores();
          rgbBlink();
          colorposession = 7;
          String LeadPlayerID = tokenStrings[3];
          String LeadTeamID = tokenStrings[4];
          broadcast("36,69,"+LeadPlayerID+","+LeadTeamID+",42");
          Serial.println("Sent Game Ending ESPNOW Command");
        }
        digitalWrite(LED_BUILTIN, HIGH); // iluminte the onboard led
        receivercounter();
        rgboff();
        //TRANSMIT = true; // setting the transmission trigger to enable transmission
      }
    }
}
void transmissioncounter() {
  sendcounter++;
  Serial.println("Added 1 to sendcounter");
  Serial.println("counter value = "+String(sendcounter));
}
void receivercounter() {
  receivecounter++;
  Serial.println("Added 1 to receivercounter and sent to blynk app");
  Serial.println("counter value = "+String(receivecounter));
  digitalWrite(LED_BUILTIN, LOW); // turn onboard led off
}
void TransmitLoRa() {
  // LETS SEND SOMETHING:
  // delay(5000);
  // String LoRaDataToSend = (String(valueA)+String(valueB)+String(valueC)+String(valueD)+String(valueE)); 
  // If needed to send a response to the sender, this function builds the needed string
  Serial.println("Transmitting Via LoRa"); // printing to serial monitor
  Serial1.print("AT+SEND=0,"+String(LoRaDataToSend.length())+","+LoRaDataToSend+"\r\n"); // used to send a data packet
  //Serial1.print("AT+SEND=0,1,5\r\n"); // used to send data to the specified recipeint
  //  Serial1.print("$PLAY,VA20,4,6,,,,,*");
  //  Serial.println("Sent the '5' to device 0");
  transmissioncounter();
}

//*************************************************************************
//**************************** GAME PLAY OBJECTS **************************
//*************************************************************************
void AddPointToTeamWithPossession() {
  PlayerScore[PlayerID]++;
  TeamScore[TeamID]++;
  // do a gained lead check to determin leader and previous leader and notify players:
  if (TeamScore[0] > TeamScore[1] && TeamScore[0] > TeamScore[2] && TeamScore[0] > TeamScore[3]) {
    // Team 0 is in the lead.
    CurrentDominationLeader = 0;
  }
  if (TeamScore[1] > TeamScore[0] && TeamScore[1] > TeamScore[2] && TeamScore[1] > TeamScore[3]) {
    // Team 0 is in the lead.
    CurrentDominationLeader = 1;
  }
  if (TeamScore[2] > TeamScore[1] && TeamScore[2] > TeamScore[0] && TeamScore[2] > TeamScore[3]) {
    // Team 0 is in the lead.
    CurrentDominationLeader = 2;
  }
  if (TeamScore[3] > TeamScore[1] && TeamScore[3] > TeamScore[2] && TeamScore[3] > TeamScore[0]) {
    // Team 0 is in the lead.
    CurrentDominationLeader = 3;
  }
  if (CurrentDominationLeader != PreviousDominationLeader) { // We just had a change of score board leader
    PreviousDominationLeader = CurrentDominationLeader; // reset the leader board with new leader
    // Notifies players of leader change:
    Serial.println("A team has reached the goal, ending game");
  }
} 
void ResetScores() {
  TeamID = 99;
  LastPosessionTeam = 99;
  MULTIBASEDOMINATION = false;
  DOMINATIONCLOCK = false;
  int playercounter = 0;
  int teamcounter = 0;
  while (playercounter < 64) {
    PlayerScore[playercounter] = 0;
    playercounter++;
    delay(1);
  }
  while (teamcounter < 4) {
    TeamScore[teamcounter] = 0;
    teamcounter++;
  }
}
void PostDominationScore() {
  // Clear the scoreterminal content
  // This will print score updates to Terminal Widget when
  // your hardware gets connected to Blynk Server
  Serial.println("*******************************");
  Serial.println("Domination Score Updated!");
  Serial.println("*******************************");
  int teamcounter = 0;
  while (teamcounter < 4) {
    //Serial.println(teamcounter);
    if (TeamScore[teamcounter] > 0) {
      if (teamcounter == 0) {
        Serial.print("Red Team Score: ");
        Serial.println(TeamScore[teamcounter]);
      }
      if (teamcounter == 1) {
        Serial.print("Blue Team Score: ");
        Serial.println(TeamScore[teamcounter]);
      }
      if (teamcounter == 2) {
        Serial.print("Yellow Team Score: ");
        Serial.println(TeamScore[teamcounter]);
      }
      if (teamcounter == 3) {
        Serial.print("Green Team Score: ");
        Serial.println(TeamScore[teamcounter]);
      }
    }
    teamcounter++;
    delay(1);
  }
  int playercounter = 0;
  while (playercounter < 64) {
    //Serial.println(playercounter);
    if (PlayerScore[playercounter] > 0) {
      Serial.print("Player ");
      Serial.print(playercounter);
      Serial.print(" Score: ");
      Serial.println(PlayerScore[playercounter]);
    }
    playercounter++;
    delay(1);
  }
}
void RGBtimer() {
  if (currentMillis1 - previousIRSend > 2000) {
    previousIRSend = currentMillis1;
    rgbblink();
  }
  
}
void RunEvolverCaptureTheFlag() {
  ReceiveEvolverIR();
  RGBtimer();
  if (NEWTAGRECEIVED == true || MOTIONDETECTED == true) {
    NEWTAGRECEIVED = false; MOTIONDETECTED = false;
    delay(1000);
    //Serial.println("Damag: " + String(Damage) + "; Critical: " + String(Critical) + "; Bullet Type: " + String(BulletType));
    
    SetEvolverIRProtocol();
    Damage++;
  }
}
void RunEvolverRespawnCover() {
  if (currentMillis1 - previousIRSend > 3000) {
    previousIRSend = currentMillis1;
    SetEvolverIRProtocol();
    rgbblink();
  }
}
void RunBRXRespawn() {
  if (currentMillis1 - previousIRSend > 3000) {
    previousIRSend = currentMillis1;
    SetIRProtocol();
    rgbblink();
  }
}
void RunBRPRespawn() {
  if (currentMillis1 - previousIRSend > 3000) {
    previousIRSend = currentMillis1;
    SetBRPIRProtocol();
    rgbblink();
  }
}
void RunBRXHeavyDamage() {
  if (currentMillis1 - previousIRSend > 1000) {
    previousIRSend = currentMillis1;
    SetIRProtocol();
    rgbblink();
  }
}
void RunEvolverOTZ() {
  ReceiveEvolverIR();
  if (NEWTAGRECEIVED == true || MOTIONDETECTED == true) {
    NEWTAGRECEIVED = false; MOTIONDETECTED = false;
    delay(1000);
    //Serial.println("Damag: " + String(Damage) + "; Critical: " + String(Critical) + "; Bullet Type: " + String(BulletType));
    SetEvolverIRProtocol();
  }
  if (currentMillis1 - previousIRSend > 5000) {
    previousIRSend = currentMillis1;
    rgbblink();
  }
}
void RunEvolverMedic() {
  ReceiveEvolverIR();
  RGBtimer();
  if (NEWTAGRECEIVED == true || MOTIONDETECTED == true) {
    NEWTAGRECEIVED = false; MOTIONDETECTED = false;
    delay(1000);
    //Serial.println("Damag: " + String(Damage) + "; Critical: " + String(Critical) + "; Bullet Type: " + String(BulletType));
    SetEvolverIRProtocol();
  }
}
void RunBRXPerkBox() {
  receiveBRXir();
  RGBtimer();
  if (NEWTAGRECEIVED == true || MOTIONDETECTED == true) {
    NEWTAGRECEIVED = false; MOTIONDETECTED = false;
    Team = TeamID;
    delay(1000);
    //Serial.println("Damag: " + String(Damage) + "; Critical: " + String(Critical) + "; Bullet Type: " + String(BulletType));
    SetIRProtocol();
  }
}
void RunEvolverUpgrade() {
  ReceiveEvolverIR();
  RGBtimer();
  if (NEWTAGRECEIVED == true || MOTIONDETECTED == true) {
    NEWTAGRECEIVED = false; MOTIONDETECTED = false;
    delay(1000);
    //Serial.println("Damag: " + String(Damage) + "; Critical: " + String(Critical) + "; Bullet Type: " + String(BulletType));
    SetEvolverIRProtocol();
  }
}
void RunDominationGame() {
  if (GearMod == 0) {
    receiveBRXir(); // runs the ir receiver, looking for brx ir protocols
  }
  if (GearMod == 1) {
    ReceiveEvolverIR(); // runs the ir receiver, looking for brx ir protocols
  }
  if (GearMod == 2) {
    ReceiveLTTOIR(); // runs the ir receiver, looking for brx ir protocols
  }
  if (GearMod == 3) {
    receiveBRXir(); // runs the ir receiver, looking for brx ir protocols
  }
  if (GearMod == 4) {
    receiveLaserWarsir(); // runs the ir receiver, looking for LaserWars ir protocols
  }
  if (GearMod == 5) {
    ReceiveOpsProIR();
  }
  if (BUZZ) {
    if (currentMillis1 - buzzerpreviousMillis  > 4000) {
      EndShortBuzzer();
    }
  }
  if (LONGBUZZ) {
    if (currentMillis1 - buzzerpreviousMillis  > 8000) {
      EndLongBuzzer();
    }
  }
  if (NEWTAGRECEIVED) {
    NEWTAGRECEIVED = false; MOTIONDETECTED = false;
    // need to check if the new tag is from the same team as the last one received
    if (TeamID != LastPosessionTeam) {
      // we had a turnover...
      LastPosessionTeam = TeamID;
      BasicDomination();
      if (Function == 5) {
        SharePointsWithOthers();
      }
      if (Function == 4) {
        SharePointsWithOthersLoRa();
      }
    }
  }
  if (DOMINATIONCLOCK) {
    if (currentMillis1 - PreviousDominationClock > 1000) {
      Serial.println("1 second lapsed under counter object");
      PreviousDominationClock = currentMillis1;
      Serial.println("Run points accumulator");
      AddPointToTeamWithPossession();
      Serial.println("Enable Score Posting");
      UPDATEWEPAPP = true;
      rgbblink();
      ShowScores();
      if (GearMod == 0) {
        TempDomTagCounter++;
        if (TempDomTagCounter == 3) {
          TempDomTagCounter = 0;
          GasEm = true;
        }
      }
    }
  }
}
void FiveMinuteScoreCheck() {
  if (TeamScore[0] > 300 || TeamScore[1] > 300 || TeamScore[2] > 300 || TeamScore[3] > 300) {
    // one team has 5 minutes
    DOMINATIONCLOCK = false;
    MULTIBASEDOMINATION = false;
    // ends the game for everyone
    StartLongBuzzer();
    Serial.println("A team has reached the goal, ending game");
  }
  if (TeamScore[0] == 270 || TeamScore[1] == 270 || TeamScore[2] == 270 || TeamScore[3] == 270) {
    // one team has 4.5 minutes
    Serial.println("A team is about to win");
  }
}
void TenMinuteScoreCheck() {
  if (TeamScore[0] > 600 || TeamScore[1] > 600 || TeamScore[2] > 600 || TeamScore[3] > 600) {
    // one team has 10 minutes
    DOMINATIONCLOCK = false;
    MULTIBASEDOMINATION = false;
    // ends the game for everyone
    GAMEOVER = true;
    StartLongBuzzer();
    Serial.println("A team has reached the goal, ending game");
  }
  if (TeamScore[0] == 570 || TeamScore[1] == 570 || TeamScore[2] == 570 || TeamScore[3] == 570) {
    // one team has 9.5 minutes
    GAMEOVER = true;
    Serial.println("A team is about to win");
  }
}
void RunByShotAccumulator() {
  // need to be continuously awaiting a tag from ir sensor
  // need to change alignment when a new team has the base by shooting it and do nothing if the same team shoots it that has posession
  // need to accumulate a point for every time a new shot comes in
  // need to broadcast a notification via espnow in case other domination bases are near by that are also keeping track of points for players, every second so that each base is current on scores
  // need to change the color of the rgb led when a team has the base or is winning rather
  // need to also broadcast what player has the base and award a point to the player that has the base in case playing free for all and to recognize mvps
  if (GearMod == 0) {
    receiveBRXir(); // runs the ir receiver, looking for brx ir protocols
  }
  if (GearMod == 1) {
    ReceiveEvolverIR(); // runs the ir receiver, looking for brx ir protocols
  }
  if (GearMod == 2) {
    ReceiveLTTOIR(); // runs the ir receiver, looking for brx ir protocols
  }
  if (GearMod == 1) {
    receiveBRXir(); // runs the ir receiver, looking for brx ir protocols
  }
  if (NEWTAGRECEIVED) {
    NEWTAGRECEIVED = false; MOTIONDETECTED = false;
    Serial.println("Run points accumulator");
    AddPointToTeamWithPossession();
    Serial.println("Enable Score Posting");
    UPDATEWEPAPP = true;
    // need to check if the new tag is from the same team as the leader or not
    // lastposessionteam is being used as current team winner for this game mode
    if (TeamID != LastPosessionTeam) {
      // SINCE WE COULD HAVE A NEW LEADER, LETS CHECK TO BE SURE
      // if it is a new leader, we need to alter the team id before 
      if (TeamScore[0] > TeamScore[1] && TeamScore[0] > TeamScore[3]) {
        TeamID = 0;  
      }
      if (TeamScore[1] > TeamScore[0] && TeamScore[1] > TeamScore[3]) {
        TeamID = 1; 
      }
      if (TeamScore[3] > TeamScore[1] && TeamScore[3] > TeamScore[0]) {
        TeamID = 3;     
      }
      LastPosessionTeam = TeamID;
      RGBDEMOMODE = false;
      AlignRGBWithTeam();
    }
  }
}
void SharePointsWithOthers() {
  int teamvalue = 100 * TeamID; // 0 for red, 100 for blue, 200 for yellow, 300 for green
  
}
void SharePointsWithOthersLoRa() {
  String FirstToken = "CAPTURE"; // 0 for red, 100 for blue, 200 for yellow, 300 for green, example team blue with player 16, 10116
  byte SecondToken= JBOXID; // for directing who it is From
  byte ThirdToken = TeamID; // What team captured base
  byte FourthToken = PlayerID;  // Player ID
  Serial.println("sending team and player alignment to other jboxes, here is the value being sent: ");
  LoRaDataToSend = FirstToken + "," + String(SecondToken) + "," + String(ThirdToken) + "," + String(FourthToken);
  Serial.println(LoRaDataToSend);
  ENABLELORATRANSMIT = true;
}
void connect_wifi() {
  Serial.println("Waiting for WiFi");
  WiFi.begin(OTAssid.c_str(), OTApassword.c_str());
  int attemptcounter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    attemptcounter++;
    if (attemptcounter > 20) {
      ESP.restart();
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
void firmwareUpdate(void) {
  WiFiClientSecure client;
  client.setCACert(rootCACertificate);
  httpUpdate.setLedPin(LED_BUILTIN, LOW);
  t_httpUpdate_return ret = httpUpdate.update(client, URL_fw_Bin);

  switch (ret) {
  case HTTP_UPDATE_FAILED:
    Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
    break;

  case HTTP_UPDATE_NO_UPDATES:
    Serial.println("HTTP_UPDATE_NO_UPDATES");
    break;

  case HTTP_UPDATE_OK:
    Serial.println("HTTP_UPDATE_OK");
    break;
  }
}
int FirmwareVersionCheck(void) {
  String payload;
  int httpCode;
  String fwurl = "";
  fwurl += URL_fw_Version;
  fwurl += "?";
  fwurl += String(rand());
  Serial.println(fwurl);
  WiFiClientSecure * client = new WiFiClientSecure;

  if (client) 
  {
    client -> setCACert(rootCACertificate);

    // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
    HTTPClient https;

    if (https.begin( * client, fwurl)) 
    { // HTTPS      
      Serial.print("[HTTPS] GET...\n");
      // start connection and send HTTP header
      delay(100);
      httpCode = https.GET();
      delay(100);
      if (httpCode == HTTP_CODE_OK) // if version received
      {
        payload = https.getString(); // save received version
      } else {
        Serial.print("error in downloading version file:");
        Serial.println(httpCode);
      }
      https.end();
    }
    delete client;
  }
      
  if (httpCode == HTTP_CODE_OK) // if version received
  {
    payload.trim();
    if (payload.equals(FirmwareVer)) {
      UPTODATE = true;
      Serial.printf("\nDevice already on latest firmware version:%s\n", FirmwareVer);
      EEPROM.write(0, 0); // set up for upgrade confirmation
      EEPROM.commit(); // store data
      return 0;} 
    else 
    {
      EEPROM.write(0, 0); // set up for upgrade confirmation
      EEPROM.commit(); // store data
      Serial.println(payload);
      Serial.println("New firmware detected");
      
      return 1;
    }
  } 
  return 0;  
}
//*****************************************************************************
//*****************  Motion Sensor *********************************************
//*****************************************************************************

// Checks if motion was detected, sets LED HIGH and starts a timer
void IRAM_ATTR detectsMovement() {
  Serial.println("MOTION DETECTED!!!");
  MOTIONDETECTED = true;
}
void SetupMotionSensor() {
  // PIR Motion Sensor mode INPUT_PULLUP
  pinMode(motionSensor, INPUT_PULLUP);
  // Set motionSensor pin as interrupt, assign interrupt function and set RISING mode
  attachInterrupt(digitalPinToInterrupt(motionSensor), detectsMovement, RISING);
}



//*****************************************************************************
//*****************  OLED Display *********************************************
//*****************************************************************************
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define NUMFLAKES     10 // Number of snowflakes in the animation example

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16

String GameModeText = "Domination";

static const unsigned char PROGMEM logo_bmp[] =
{ 0b00000000, 0b11000000,
  0b00000001, 0b11000000,
  0b00000001, 0b11000000,
  0b00000011, 0b11100000,
  0b11110011, 0b11100000,
  0b11111110, 0b11111000,
  0b01111110, 0b11111111,
  0b00110011, 0b10011111,
  0b00011111, 0b11111100,
  0b00001101, 0b01110000,
  0b00011011, 0b10100000,
  0b00111111, 0b11100000,
  0b00111111, 0b11110000,
  0b01111100, 0b11110000,
  0b01110000, 0b01110000,
  0b00000000, 0b00110000 };

void testfilltriangle(void) {
  display.clearDisplay();

  for(int16_t i=max(display.width(),display.height())/2; i>0; i-=5) {
    // The INVERSE color is used so triangles alternate white/black
    display.fillTriangle(
      display.width()/2  , display.height()/2-i,
      display.width()/2-i, display.height()/2+i,
      display.width()/2+i, display.height()/2+i, SSD1306_INVERSE);
    display.display();
    delay(1);
  }

  delay(2000);
}

void testdrawstyles(void) {
  display.clearDisplay();

  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(F("Initializing JBOX...."));

  //display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
  //display.println("Initializing JBOX....");

//  display.setTextSize(2);             // Draw 2X-scale text
//  display.setTextColor(SSD1306_WHITE);
  //display.println(F("Initializing JBOX....")); 
  //display.println(0xDEADBEEF, HEX);

  display.display();
  delay(2000);
}

void ShowScores(void) {
  display.clearDisplay();
  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,6);             // Start at top-left corner
  display.println("RED  BLUE GRN  YLW");  
  int RedTxt = TeamScore[0]; // TeamScore[TeamID]
  int BlueTxt = TeamScore[1];
  int GreenTxt = TeamScore[3];
  int YellowTxt = TeamScore[2];
  String CurrentLeader = "null";
  if (RedTxt > BlueTxt && RedTxt > GreenTxt && RedTxt > YellowTxt) { CurrentLeader = "RED";}
  else {
    if (BlueTxt > RedTxt && BlueTxt > GreenTxt && BlueTxt > YellowTxt) { CurrentLeader = "BLUE";}
    else {
      if (GreenTxt > RedTxt && GreenTxt > BlueTxt && GreenTxt > YellowTxt) { CurrentLeader = "GREEN";}
      else {CurrentLeader = "YELLOW";}
    }
  }
  display.setCursor(0,20);
  display.print(String(RedTxt));
  display.setCursor(30,20);
  display.print(String(BlueTxt));
  display.setCursor(60,20);
  display.print(String(GreenTxt));
  display.setCursor(90,20);
  display.println(String(YellowTxt));
  display.setCursor(0,30);
  display.println("Current Leader:");
  display.setCursor(0,40);
  display.setTextSize(3); // Normal 1:1 pixel scale
  display.println(CurrentLeader);
  display.display();
}

void ShowGameMode(void) {
  if (Function == 0) {GameModeText = "Basic Domination";}
  if (Function == 4) {GameModeText = "Domination NodeLR";}
  if (Function == 5) {GameModeText = "Domination NodeSR";}
  if (Function == 6) {GameModeText = "Domination Master";}
  if (Function == 1) {GameModeText = "Domination 5Min";}
  if (Function == 2) {GameModeText = "Domination 10Min";}
  if (Function == 3) {GameModeText = "Shot Accumulator";}
  if (Function == 40) {GameModeText = "CTF-Evolver-Red";}
  if (Function == 41) {GameModeText = "CTF-Evover-Blue";}
  if (Function == 42) {GameModeText = "CTF-Evolver-Yellow";}
  if (Function == 43) {GameModeText = "CTF-Evolver-Green";}
  if (Function == 44) {GameModeText = "Respawn-Evolver";}
  if (Function == 45) {GameModeText = "OTZ-Evolver";}
  if (Function == 46) {GameModeText = "Medic-Evolver";} 
  if (Function == 47) {GameModeText = "Upgrades-Evolver";}
  if (Function == 50) {GameModeText = "Resawn-BRX-Red";}
  if (Function == 51) {GameModeText = "Respawn-BRX-Blue";}
  if (Function == 52) {GameModeText = "Respawn-BRX-Yellow";}
  if (Function == 53) {GameModeText = "Respawn-BRX-Green";}
  if (Function == 15 || Function == 16) {GameModeText = "LORA Test";}
  if (Function == 99) {GameModeText = "IR Debug";}
  display.clearDisplay();
  display.setTextSize(2);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(GameModeText);
  display.setCursor(0,20);             // Start at top-left corner
  if (GearMod == 1) {
    display.println("Evolver");
  }
  if (GearMod == 0) {
    display.println("BRX");
  }
  display.display();
}

#define XPOS   0 // Indexes into the 'icons' array in function below
#define YPOS   1
#define DELTAY 2


//******************************************************************************************
// ***********************************  GAME LOOP  ****************************************
// *****************************************************************************************
void loop0(void *pvParameters) {
Serial.print("Comms loop running on core ");
Serial.println(xPortGetCoreID());
for(;;) { // starts the forever loop 
  // put all the serial activities in here for communication with the brx
  currentMillis0 = millis(); // sets the timer for timed operations
  // Core 0 Speed Test:
  Core0++;
  if (currentMillis0 - PreviousCore0Check > 10000) {
    PreviousCore0Check = currentMillis0;
    int CurrentSeconds0 = currentMillis0/1000;
    int SpeedRate0 = Core0/CurrentSeconds0;
    Serial.println();
    Serial.println("Core " + String(xPortGetCoreID()) + " Loop Counts: " + String(Core0));
    Serial.println("Core 0 Cycles per Second: " + String(SpeedRate0));
    Serial.println();
  }
  RunLEDSnake();
  ws.cleanupClients();
  if (MULTIBASEDOMINATION) {
    if (currentMillis0 - previousMillis0 > 1000) {  // will store last time LED was updated
      // NEED TO APPLY POINTS FOR OTHER DOMINATION BOXES POSESSIONS
      previousMillis0 = currentMillis0;
      int processcounter = 0;
      while (processcounter < 21) {
        if (JboxInPlay[processcounter] == 1) {
          PlayerScore[JboxPlayerID[processcounter]]++;
          TeamScore[JboxTeamID[processcounter]]++;
          Serial.println("added a point for a multi base domination JBOX");
        }
        processcounter++;
      }
    }
  }
  if (GAMEOVER) {
    GAMEOVER = false;
    ResetScores();
  }
  if (DOMINATIONCLOCK == true || MULTIBASEDOMINATION == true) {
    if (currentMillis0 - LastScoreUpdate > 1000) {
      LastScoreUpdate = currentMillis0;
      UpdateWebApp0();
      UpdateWebApp2();
    }
  }
  if (GasEm) {
    GasEm = false;
    GasPlayers();
  }
  if (CONTROLPOINTCAPTURED) {
    CONTROLPOINTCAPTURED = false;
    ControlPointCaptured();
  }
  //**************************************************************************************
  // LoRa Receiver Main Functions:
  if (LORALISTEN) {
     ReceiveTransmission();
  }
  if (ENABLELORATRANSMIT) {
    TransmitLoRa();
    ENABLELORATRANSMIT = false;
  }
  delay(1); // this has to be here or it will just continue to restart the esp32
  }
}
// *****************************************************************************************
// **************************************  Setup  ******************************************
// *****************************************************************************************
void setup(){
  // Serial port for debugging purposes
  Serial.begin(115200);
  Serial.println("Serial Monitor Started");
  //***********************************************************************
  //***********************************************************************
  // initialize EEPROM and stored sattings
  EEPROM.begin(EEPROM_SIZE);
  JBOXID = EEPROM.read(10);
  ssid = "JBOX#" + String(JBOXID);
  int bootstatus = EEPROM.read(0);
  Serial.print("boot status = ");
  Serial.println(bootstatus);
  if (bootstatus > 0 && bootstatus < 4) {
    Serial.println("Enabling OTA Update Mode");
    bootstatus++;
    if (bootstatus == 4) {
      bootstatus = 0;
    }
    EEPROM.write(0, bootstatus);
    EEPROM.commit();
    OTAMODE = true;
  }
  int tempsetting = EEPROM.read(1);
  if (tempsetting < 255) {
    Function = tempsetting;
  }
  if (Function == 0) {
      Serial.println("Basic Domination");
      rgbBlink();
       
      colorposession = 7;
  }
  if (Function == 4) {
    Serial.println("Domination Node - LoRa");
      rgbBlink();
      LORALISTEN = true;
      colorposession = 7;
  }
  if (Function == 5) {
    Serial.println("Domination Node - ESPNOW");
      rgbBlink();
       
      colorposession = 7;
  }
  if (Function == 6) {
    Serial.println("Domination Master");
      rgbBlink();
      LORALISTEN = true;
      colorposession = 7;
  }
  if (Function == 1) {
    Serial.println("5 Minute Basic Domination");
      rgbBlink();
       
      colorposession = 7;
  }
  if (Function == 2) {
    Serial.println("10 Minute Basic Domination");
      rgbBlink();
       
      colorposession = 7;
  }
  if (Function == 3) {
    Serial.println("Shot Accumulator Mode");
      rgbBlink();
      colorposession = 7;
  }
  if (Function == 40) {
    Serial.println("Evolver Capture The Flag - red");
    BulletType = 14;
    Damage = 0;
    Team = 0;
    resetRGB();
    RGBRED = true; colorposession = 0;
    
  }
  if (Function == 41) {
    Serial.println("Evolver Capture The Flag - blue");
    BulletType = 14;
    Damage = 0;
    Team = 1;
    resetRGB();
    RGBBLUE = true; colorposession = 1;
    
  }
  if (Function == 42) {
    Serial.println("Evolver Capture The Flag - yellow");
    BulletType = 14;
    Damage = 0;
    Team = 2;
    resetRGB();
    RGBYELLOW = true; colorposession = 2;
    
  }
  if (Function == 43) {
    Serial.println("Evolver Capture The Flag - green");
    BulletType = 14;
    Damage = 0;
    Team = 3;
    resetRGB();
    RGBGREEN = true; colorposession = 3;
    
  }
  if (Function == 44) {
    BulletType = 10;
    Damage = 0;
    Team = 2;  
    resetRGB(); 
    RGBWHITE = true; colorposession = 4; colorposession = 4;  
     
  }
  if (Function == 45) {
    Serial.println("Evolver OTZ Station");
    BulletType = 12;
    Damage = 0;
    Team = 2;  
    resetRGB(); 
    RGBPURPLE = true; colorposession = 5;  
    
  }
  if (Function == 46) {
    Serial.println("Evolver Medic Station");
    BulletType = 13;
    Damage = 100;
    Team = 2;  
    resetRGB(); 
    RGBCYAN = true; colorposession = 6;  
    
  }
  if (Function == 47) {
    Serial.println("Evolver Upgrade Station");
    BulletType = 11;
    Damage = 0;
    Team = 2;  
    resetRGB(); 
    RGBYELLOW = true; colorposession = 2;
    
  }
  if (Function == 50) {
    Serial.println("BRX Red Respawn Station");
    BulletType = 15;
    Damage = 6;
    Team = 0; 
    Player = 63;
    Critical = 1;
    Power = 0;
    resetRGB(); 
    RGBRED = true; colorposession = 0;   
    rgbBlink();
     
    
  }
  if (Function == 51) {
    Serial.println("BRX Blue Respawn Station");
    BulletType = 15;
    Damage = 6;
    Team = 1; 
    Player = 63;
    Critical = 1;
    Power = 0;
    resetRGB(); 
    RGBBLUE = true; colorposession = 1;   
    rgbBlink();
     
    
  }
  if (Function == 52) {
    Serial.println("BRX Yellow Respawn Station");
    BulletType = 15;
    Damage = 6;
    Team = 2; 
    Player = 63;
    Critical = 1;
    Power = 0;
    resetRGB(); 
    RGBYELLOW = true; colorposession = 2;   
    rgbBlink();
     
    
  }
  if (Function == 53) {
    Serial.println("BRX gREEN Respawn Station");
    BulletType = 15;
    Damage = 6;
    Team = 3; 
    Player = 63;
    Critical = 1;
    Power = 0;
    resetRGB(); 
    RGBGREEN = true; colorposession = 3;   
    rgbBlink();
     
    
  } 
  if (Function == 99) {
    Serial.println("IR Debug");
    resetRGB();
    RGBWHITE = true; colorposession = 4;
    
  }
  if (Function == 15) {
    Serial.println("Test Listen LoRa");
    LORALISTEN = true;
  }
  if (Function == 16) {
    Serial.println("Test Send LoRa");
    LORALISTEN = true;
  }
  tempsetting = EEPROM.read(2);
  if (tempsetting < 255) {
    GearMod = tempsetting;
  }
  tempsetting = EEPROM.read(101);
  if (tempsetting == 1) {
    SetupMotionSensor();
    Serial.println("Motion Sensor Activated");
  } else {
    Serial.println("No Motion Sensor Function");
  }
  // setting up eeprom based SSID:
  String esid;
  for (int i = 11; i < 43; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  Serial.println();
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading EEPROM pass");
  // Setting up EEPROM Password
  String epass = "";
  for (int i = 44; i < 100; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  Serial.print("PASS: ");
  Serial.println(epass);
  // now updating the OTA credentials to match
  OTAssid = esid;
  OTApassword = epass;
  //************************************************************************
  // Connect to Wi-Fi
  Serial.println("Starting AP");
  WiFi.mode(WIFI_AP_STA);
  // SET THE RIGHT WIRELESS PROTOCOL
  //esp_wifi_set_protocol(WIFI_IF_AP,WIFI_PROTOCOL_LR);
  //esp_wifi_set_protocol(WIFI_IF_STA,WIFI_PROTOCOL_LR);
  esp_wifi_set_protocol(WIFI_IF_AP, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
  esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
  WiFi.softAP(ssid.c_str(), password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  //*************************************************************************
  // setting up wifi settings for OTA Update
  if (OTAMODE) {
    Serial.print("Active firmware version:");
    Serial.println(FirmwareVer);
    pinMode(LED_BUILTIN, OUTPUT);
    // Connect to Wi-Fi network with SSID and password
    Serial.print("Setting AP (Access Point)…");
    // Remove the password parameter, if you want the AP (Access Point) to be open
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ssid.c_str(), password);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    Serial.print("ESP Board MAC Address:  ");
    Serial.println(WiFi.macAddress());
    Serial.println("Starting ESPNOW");
    Serial.println("wifi credentials");
    Serial.println(OTAssid);
    Serial.println(OTApassword);
    connect_wifi();
  } else {
  // setting up all nececessary functions for running JBOX:
  //***********************************************************************
  // initialize the RGB pins
  Serial.println("Initializing RGB Pins");
  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(blue, OUTPUT);
  rgboff();
  //***********************************************************************
  // initialize piezo buzzer pin
  Serial.println("Initializing Piezo Buzzer");
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);
  //***********************************************************************
  // initialize the IR digital pin as an output:
  Serial.println("Initializing IR Output");
  pinMode(IRledPin, OUTPUT);
  //***********************************************************************
  // initialize the IR receiver pin as an output:
  Serial.println("Initializing IR Sensor");
  pinMode(IR_Sensor_Pin, INPUT);
  Serial.println("Ready to recieve BRX IR");
  //***********************************************************************
  // initialize LoRa Serial Comms and network settings:
  Serial.println("Initializing LoRa");
  Serial1.begin(115200, SERIAL_8N1, SERIAL1_RXPIN, SERIAL1_TXPIN); // setting up the LoRa pins
  Serial1.print("AT\r\n"); // checking that serial is working with LoRa module
  delay(100);
  //Serial1.print("AT+PARAMETER=10,7,1,7\r\n");    //For Less than 3Kms
  Serial1.print("AT+PARAMETER= 12,4,1,7\r\n");    //For More than 3Kms
  delay(100);   //wait for module to respond
  Serial1.print("AT+BAND=868500000\r\n");    //Bandwidth set to 868.5MHz
  delay(100);   //wait for module to respond
  Serial1.print("AT+ADDRESS="+String(JBOXID)+"\r\n");   //needs to be unique
  delay(100);   //wait for module to respond
  Serial1.print("AT+NETWORKID=0\r\n");   //needs to be same for receiver and transmitter
  delay(100);   //wait for module to respond
  Serial1.print("AT+PARAMETER?\r\n");    //For Less than 3Kms
  delay(100); // 
  Serial1.print("AT+BAND?\r\n");    //Bandwidth set to 868.5MHz
  delay(100);   //wait for module to respond
  Serial1.print("AT+NETWORKID?\r\n");   //needs to be same for receiver and transmitter
  delay(100);   //wait for module to respond
  Serial1.print("AT+ADDRESS?\r\n");   //needs to be unique
  delay(100);   //wait for module to respond
  pinMode(LED_BUILTIN, OUTPUT); // setting up the onboard LED to be used
  digitalWrite(LED_BUILTIN, LOW); // turn off onboard led
  digitalWrite(LED_BUILTIN, HIGH); // turn on onboard led
  delay(500);
  digitalWrite(LED_BUILTIN, LOW); // turn off onboard led
  Serial.println("LoRa Module Initialized");
  //***********************************************************************
  // initialize blinking led onboard
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
  //***********************************************************************
  // Connect to Wi-Fi
  Serial.println("Starting AP");
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid.c_str(), password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  //***********************************************************************
  // initialize web server
  initWebSocket();
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });
  // json events
    events.onConnect([](AsyncEventSourceClient *client){
      if(client->lastId()){
        Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
      }
      // send event with message "hello!", id current millis
      // and set reconnect delay to 1 second
      client->send("hello!", NULL, millis(), 10000);
    });
    server.addHandler(&events);
  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // adding in interupt for pulling data from web server for text input for SSID for OTA updates
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      OTAssid = inputMessage;
      Serial.println("clearing eeprom");
      for (int i = 11; i < 44; ++i) {
        EEPROM.write(i, 0);
      }
      Serial.println("writing eeprom ssid:");
      int j = 11;
      for (int i = 0; i < OTAssid.length(); ++i)
      {
        EEPROM.write(j, OTAssid[i]);
        Serial.print("Wrote: ");
        Serial.println(OTAssid[i]);
        j++;
      }
      EEPROM.commit();
    }
    // adding in interupt for pulling data from web server for text input for password for OTA updates
    // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_2)->value();
      inputParam = PARAM_INPUT_2;
      OTApassword = inputMessage;
      Serial.println("clearing eeprom");
      for (int i = 44; i < 100; ++i) {
        EEPROM.write(i, 0);
      }
      Serial.println("writing eeprom Password:");
      int k = 44;
      for (int i = 0; i < OTApassword.length(); ++i)
      {
        EEPROM.write(k, OTApassword[i]);
        Serial.print("Wrote: ");
        Serial.println(OTApassword[i]);
        k++;
      }
      EEPROM.commit();
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    Serial.print("OTA SSID = ");
    Serial.println(OTAssid);
    Serial.print("OTA Password = ");
    Serial.println(OTApassword);
    request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" 
                                     + inputParam + ") with value: " + inputMessage +
                                     "<br><a href=\"/\">Return to Home Page</a>");
  });
  //***********************************************************************
  // Start server
  server.begin();
  //***********************************************************************
  // Start ESP Now
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  Serial.println("Starting ESPNOW");
  IntializeESPNOW();
  //***********************************************************************
  // start up dot stars 
  SetupDotStars();
  // Initialize the OLED display:
  Wire.begin(19, 18); // Wire.begin(SDA, SCK); // changes pinout for display 19 and 18 for v5 33 and  and 32 for v4
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  testfilltriangle();  // Draw triangles (filled)
  testdrawstyles();    // Draw 'stylized' characters
  ShowGameMode();
  //***********************************************************************
  // initialize dual cores and dual loops
  Serial.println(xPortGetCoreID());
  xTaskCreatePinnedToCore(loop0, "loop0", 10000, NULL, 1, NULL, 0);
  }
  //RUNTHESNAKE = false;
} // End of setup.

void loop() {
//**********************************************************************
  // OTA update loop, runs if in OTA Update Mode
  while (OTAMODE) {
    static int num=0;
    unsigned long currentMillis = millis();
    if ((currentMillis - previousMillis) >= otainterval) {
      // save the last time you blinked the LED
      previousMillis = currentMillis;
      if (FirmwareVersionCheck()) {
        firmwareUpdate();
      }
    }
    if (UPTODATE) {
      Serial.println("all good, up to date, lets restart");
      ESP.restart(); // we confirmed there is no update available, just reset and get ready to play 
    }
  }
  ClockSpeedCheck();
  if (Function < 7) { // Domination Node - LoRa
    RunDominationGame();
    if (Function == 1) {
      if (DOMINATIONCLOCK) {
        FiveMinuteScoreCheck();
      }
    }
    if (Function == 2) {
      if (DOMINATIONCLOCK) {
        TenMinuteScoreCheck();
      }
    }
  }
  if (Function == 3) {
    RunByShotAccumulator();
  }
  if (Function == 13) {
    RunBRXHeavyDamage();
  }
  if (Function == 14) {
    RunBRXPerkBox();
    //vTaskDelay(0);
  }
  if (Function > 49) {
    if (Function < 54) {
      RunBRXRespawn();
    }
  }
  if (Function > 39) {
    if (Function < 44) {
      RunEvolverCaptureTheFlag();
    }
  }
  if (Function == 44){
    RunEvolverRespawnCover();
  }
  if (Function == 45){
    RunEvolverOTZ();
  }
  if (Function == 46){
    RunEvolverMedic();
  }
  if (Function == 47){
    RunEvolverUpgrade();
  }
  if (Function > 53 && Function < 60) {
    RunBRPRespawn();
  }
  if (Function == 1000) { // running Order66
    ExecuteOrder66();
  }
  if (Function == 16) {
    if (currentMillis1 - previousIRSend > 3000) {
      previousIRSend = currentMillis1;
      Damage++;
      SetEvolverIRProtocol();
    }
  }
  if (Function == 99) {
    RunIRDebug();
  }
}

void ClockSpeedCheck() {
  // CORE 1 SPEED TEST:
  currentMillis1 = millis(); // sets the timer for timed operations
  Core1++;
  if (currentMillis1 - PreviousCore1Check > 10000) {
    PreviousCore1Check = currentMillis1;
    int CurrentSeconds1 = currentMillis1/1000;
    int SpeedRate1 = Core1/CurrentSeconds1;
    Serial.println();
    Serial.println("Function = " + String(Function));
    Serial.println("Core " + String(xPortGetCoreID()) + " Loop Counts: " + String(Core1));
    Serial.println("Core 1 Cycles per Second: " + String(SpeedRate1));
    Serial.println();
  }
}
