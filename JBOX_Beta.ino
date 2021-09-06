/*
 * 
 * JBOX LOG:
 * 
 * 1/27/21    - testing out circuits for performance/functionality and importation of individual INO sketchs
 *            - Imported ESP32 RGB INO and tested to be functional - still need lond duration performance test w/o resistors for circuit
 *            - Imported ESP32 Piezo Buzzer INO and tested to be functional - still need long duration performance testing w/o resistors for circuit
 *            - Imported Dual Core ESP32 base INO for operating both cores - tested and proven to be running smoothly
 *            - Imported IRLED BRX Send Protocol INO and tested to be functional with existing circuit in direct sunlight, 3' range of emitters - need to test long duration for emitter circuit durability
 *            - Imported IR Receiver BRX protocol INO and tested to be functional with circuitry - should run long duration test for durability
 * 1/27/21    - Started Test on Circuitry and components to let run for a couple days
 *            - buzzer and emitters pulse every five seconds
 *            - RGB LED goes on and off every seconds
 *            - IR receiver runs continuously.
 *            - Started at 1:25 PM pacific time
 *            - Finished 1/28 at 7:30 AM - results, worked without any issue
 * 1/29/21    - Integrate LoRa Module INO base code, integration successfull
 * 2/1/21     - Added in ESPNOW comms for communicating to taggers over wifi
 * 3/28/21    - Integrated most code portions together
 *            - added some blynk command items over BLE
 *            - Built out the Blynk App a little bit for some primary functions
 *            - Need to add a lot of IR LED Protocol Commands but put place holder bools in place for enabling them later
 *            - Tested out basic domination station mode and it worked well, adjusted score reporting as well and put on separate tab
 *            - Still need to engage a bit more some items like buzzer and rgb functionality when captured under basic domination base mode
 * 3/29/21    - Added Dual mode for continuous IR and capturability of base
 *            - Added/Fixed Additional IR Protocol Outputs
 * 3/30/21    - Added Confirmation Tag when capturing a base
 *            = Added buzzer sound when activating a function
 *            - Cleaned up RGB Indicators a Bit for completed modes thus far
 * 5/13/21    - Migrated everything to web server controls versus blynk and moved the lora to the comms loop 
 * 5/13/21    - Added a couple other game modes for domination base capture play
 * 5/14/21    - Successfully integrated LTTO tags into the game so you cn select LTTO for domination game play as well.
 *            = Added in a check for Zone tags for LTTO
 *            - Added LTTO IR tags to send
 * 5/28/21    - Added in ability to program all JBOXes from one JBOX
 * 6/27/21    - Added in state save for settings so on reset, settings are retained. Uses Flash/EEPROM, so this means that your board may be no good after 100,000 to 1,000,000 setting changes/uses.
 * 6/30/21    - Added a second web server for scoring only, access 192.168.4.1:80/scores
 * 7/1/21     - Added in BRP compatibility for teams red, blue, yellow, green, purple and cyan
 * 9/6/21     - fixed a few bugs
 *            - added in device assignment for jbox-jedge device id list
 *            - added in eeprom for device id assignment so not lost upon updates
 *            - added in espnow based respawn function for full radius respawn range in proximity
 *            - added in wifi credentials inputs for retaining in eeprom
*/

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

// define the number of bytes I'm using/accessing for eeprom
#define EEPROM_SIZE 101

// serial definitions for LoRa
#define SERIAL1_RXPIN 19 // TO LORA TX
#define SERIAL1_TXPIN 23 // TO LORA RX

//*************************************************************************
//******************** OTA UPDATES ****************************************
//*************************************************************************
String FirmwareVer = {"3.10"};
#define URL_fw_Version "https://raw.githubusercontent.com/LaserTagMods/autoupdate/main/bin_version.txt"
#define URL_fw_Bin "https://raw.githubusercontent.com/LaserTagMods/autoupdate/main/fw.bin"

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
const char* ssid = "JBOX#101";
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
String tokenStrings[5]; // used to break out the incoming data for checks/balances
String LoRaDataToSend; // used to store data being sent from this device

bool LORALISTEN = false; // a trigger used to enable the radio in (listening)
bool ENABLELORATRANSMIT = false; // a trigger used to enable transmitting data

int sendcounter = 0;
int receivecounter = 0;

unsigned long lorapreviousMillis = 0;
const long lorainterval = 3000;

// IR RECEIVER VARIABLE DELCARATIONS 
// Define Variables used for the game functions
int TeamID=0; // this is for team recognition, team 0 = red, team 1 = blue, team 2 = yellow, team 3 = green
int gamestatus=0; // used to turn the ir reciever procedure on and off
int PlayerID=0; // used to identify player
int PID[6]; // used for recording player bits for ID decifering
int DamageID=0; // used to identify weapon
int ShotType=0; // used to identify shot type
int DID[8]; // used for recording weapon bits for ID decifering
int BID[4]; // used for recording shot type bits for ID decifering
int PowerID = 0; // used to identify power
int CriticalID = 0; // used to identify if critical
const byte IR_Sensor_Pin = 16; // this is int input pin for ir receiver
int BB[4]; // bullet type bits for decoding Ir
int PP[6]; // Player bits for decoding player id
int TT[2]; // team bits for decoding team 
int DD[8]; // damage bits for decoding damage
int CC[1]; // Critical hit on/off from IR
int UU[2]; // Power bits
int ZZ[2]; // parity bits depending on number of 1s in ir
int XX[1]; // bit to used to confirm brx ir is received
bool ENABLEIRRECEIVER = true; // enables and disables the ir receiver

// GAME VARIABLE DELCARATIONS
// variables for gameplay and scoring:
int Function = 1; // set as default for basic domination game mode
int TeamScore[4]; // used to track team score
int PlayerScore[64]; // used to track individual player score
int CapturableEmitterScore[4]; // used for accumulating points for capturability to alter team alignment
int RequiredCapturableEmitterCounter = 10;
bool FIVEMINUTEDOMINATION = false;
bool TENMINUTEDOMINATION = false;
bool FIVEMINUTETUGOWAR = false;
int CurrentDominationLeader = 99;
int PreviousDominationLeader = 99;

long PreviousDominationClock = 0;

bool MULTIBASEDOMINATION = false;
bool BASICDOMINATION = true; // a default game mode
bool CAPTURETHEFLAGMODE = false;
bool DOMINATIONCLOCK = false;
bool POSTDOMINATIONSCORETOBLYNK = false;
bool CAPTURABLEEMITTER = false;
bool ANYTEAM = false;

// IR LED VARIABLE DELCARATIONS
int IRledPin =  26;    // LED connected to GPIO26
int B[4];
int P[6];
int T[2];
int D[8];
int C[1];
int X[2];
int Z[2];
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
bool GASGRENADE = false;
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
int red = 17;
int green = 21;
int blue = 22;
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
const int buzzer = 5; // buzzer pin
bool BUZZ = false; // set to true to send buzzer sound
bool SOUNDBUZZER = false; // set to true to sound buzzer
int buzzerduration = 250; // used for setting cycle length of buzz
const int fixedbuzzerduration = buzzerduration; // assigns a constant buzzer duration

unsigned long buzzerpreviousMillis = 0;
const long buzzerinterval = 5000;

// LTTO VARIABLES NEEDED
int PLTTO[2];
int TLTTO[3];
int DLTTO[2];
//int PlayerID = 99;
//int TeamID = 99;
//int DamageID = 99;
int LTTOTagType = 99;
int GearMod = 0;
int LTTODamage = 1;

//*****************************************************************************************
// ESP Now Objects:
//*****************************************************************************************
// for resetting mac address to custom address:
uint8_t newMACAddress[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x09}; 

// REPLACE WITH THE MAC Address of your receiver, this is the address we will be sending to
uint8_t broadcastAddress[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x09};


// Register peer
esp_now_peer_info_t peerInfo;

// Define variables to store and to be sent
int datapacket1 = 99; // INTENDED RECIPIENT - 99 is all - 0-63 for player id - 100-199 for bases - 200 - 203 for teams 0-3
int datapacket2 = 32700; // FUNCTION/COMMAND - range from 0 to 32,767 - 327 different settings - 99 different options
int datapacket3 = JBOXID; // From - device ID
String datapacket4 = "null"; // used for score reporting only


// Define variables to store incoming readings
int incomingData1; // INTENDED RECIPIENT - 99 is all - 0-63 for player id - 100-199 for bases
int incomingData2; // FUNCTION/COMMAND - range from 0 to 32,767 - 327 different settings - 99 different options
int incomingData3; // From - device ID
String incomingData4; // used for score reporting only

// Variable to store if sending data was successful
String success;

//Structure example to send data
//Must match the receiver structure
typedef struct struct_message {
    int DP1; // INTENDED RECIPIENT - 99 is all - 0-63 for player id - 100-199 for bases - 200 - 203 for teams 0-3
    int DP2; // FUNCTION/COMMAND - range from 0 to 32,767 - 327 different settings - 99 different options
    int DP3; // From - device ID
    char DP4[200]; // used for score reporting
} struct_message;

// Create a struct_message called DataToBroadcast to hold sensor readings
struct_message DataToBroadcast;

// Create a struct_message to hold incoming sensor readings
struct_message incomingReadings;

// trigger for activating data broadcast
bool BROADCASTESPNOW = false; 

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status ==0){
    success = "Delivery Success :)";
  }
  else{
    success = "Delivery Fail :(";
  }
}
// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  //digitalWrite(2, HIGH);
  Serial.print("Bytes received: ");
  Serial.println(len);
  incomingData1 = incomingReadings.DP1; // INTENDED RECIPIENT - 99 is all - 0-63 for player id - 100-199 for bases - 200 - 203 for teams 0-3
  incomingData2 = incomingReadings.DP2; // FUNCTION/COMMAND - range from 0 to 32,767 - 327 different settings - 99 different options
  incomingData3 = incomingReadings.DP3; // From
  //incomingData4 = incomingReadings.DP4; // From
  Serial.println("DP1: " + String(incomingData1)); // INTENDED RECIPIENT
  Serial.println("DP2: " + String(incomingData2)); // FUNCTION/COMMAN
  Serial.println("DP3: " + String(incomingData3)); // From - device ID
  Serial.print("DP4: "); // used for scoring
  //Serial.println(incomingData4);
  Serial.write(incomingReadings.DP4);
  Serial.println();
  incomingData4 = String(incomingReadings.DP4);
  Serial.println(incomingData4);
  ProcessIncomingCommands();
  //Serial.print("cOMMS loop running on core ");
  //Serial.println(xPortGetCoreID());
}

// object to generate random numbers to send
void getReadings(){
  // Set values to send
  DataToBroadcast.DP1 = datapacket1;
  DataToBroadcast.DP2 = datapacket2;
  DataToBroadcast.DP3 = datapacket3;
  datapacket4.toCharArray(DataToBroadcast.DP4, 200);
  Serial.println(datapacket1);
  Serial.println(datapacket2);
  Serial.println(datapacket3);
  Serial.println(datapacket4);
}

void ResetReadings() {
  //datapacket1 = 99; // INTENDED RECIPIENT - 99 is all - 0-63 for player id - 100-199 for bases - 200 - 203 for teams 0-3
  datapacket2 = 32700; // FUNCTION/COMMAND - range from 0 to 32,767 - 327 different settings - 99 different options
  datapacket3 = JBOXID; // From - device ID
  datapacket4 = "null";
}

// object for broadcasting the data packets
void BroadcastData() {
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &DataToBroadcast, sizeof(DataToBroadcast));
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
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
    // Set up the onboard LED
  pinMode(2, OUTPUT);
  
  // run the object for changing the esp default mac address
  ChangeMACaddress();
  
  // Set device as a Wi-Fi Station
  //WiFi.mode(WIFI_STA);
  
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);  
  
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;   // this is the channel being used
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
}


//****************************************************
// WebServer 
//****************************************************

JSONVar board;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
//AsyncWebServer server1(81);
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
      Serial.println("Game Highlights:");
      Serial.println();
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
      Serial.println("The Dominator, Players with the most objective points:");
      Serial.println("1st place: Player " + String(ObjectiveLeader[0]) + " with " + String(LeaderScore[0]));
      Serial.println("2nd place: Player " + String(ObjectiveLeader[1]) + " with " + String(LeaderScore[1]));
      Serial.println("3rd place: Player " + String(ObjectiveLeader[2]) + " with " + String(LeaderScore[2]));
      Serial.println();      
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
    <h1>JBOX Settings</h1>
  </div>
  <div class="content">
    <div class="card">
      <h2>Device Selector</h2>
      <p><select name="device" id="deviceid">
        <option value="899">This JBOX</option>
        <option value="898">All JBOXes</option>
        <option value="800">JBOX00</option>
        <option value="801">JBOX01</option>
        <option value="802">JBOX02</option>
        <option value="803">JBOX03</option>
        <option value="804">JBOX04</option>
        <option value="805">JBOX05</option>
        <option value="806">JBOX06</option>
        <option value="807">JBOX07</option>
        <option value="808">JBOX08</option>
        <option value="809">JBOX09</option>
        </select>
      </p>
      <h2>Primary Function</h2>
      <p><select name="primary" id="primaryid">
        <option value="1">Basic Domination</option>
        <option value="5">5 Minute Domination</option>
        <option value="6">10 Minute Domination</option>
        <option value="7">5 Minute Tug of War</option>
        <option value="8">Capture The Flag</option>
        <option value="9">10 Minute Domination (multi base)</option>
        <option value="10">Own The Zone</option>
        <option value="2">Continuous IR Emitter</option>
        <option value="3">Tag Activated IR Emitter</option>
        <option value="4">Capturable Continuous IR Emitter</option>
        </select>
      </p>
      <h2>IR Emitter Type</h2>
      <p><select name="secondary" id="secondaryid">
        <option value="101">Motion Sensor</option>
        <option value="102">Capture the Flag</option>
        <option value="103">Control Point Lost</option>
        <option value="104">Gas Grenade</option>
        <option value="105">Light Damage</option>
        <option value="106">Medium Damage</option>
        <option value="107">Heavy Damage</option>
        <option value="108">Explosive Damage</option>
        <option value="109">IR Respawn Station</option>
        <option value="117">ESPNOW Respawn Station</option>
        <option value="110">Med Kit</option>
        <option value="111">Loot Box</option>
        <option value="112">Armor Boost</option>
        <option value="113">Sheilds Boost</option>
        <option value="114">Own the Zone</option>
        <option value="115">Control Point Captured</option>
        <option value="116">Customizable LTTO Tag(settings below)</option>
        </select>
      </p>
      <h2>Team Friendly/Alignment Settings</h2>
      <p><select name="ammo" id="ammoid">
        <option value="201">Unspecified</option>
        <option value="202">Red/Alpha</option>
        <option value="203">Blue/Bravo</option>
        <option value="204">Yellow/Charlie</option>
        <option value="205">Green/Delta</option>
        </select>
      </p>
      <h2>Continuous IR Emitter Frequency</h2>
      <p><select name="MeleeSetting" id="meleesettingid">
        <option value="301">1 Sec.</option>
        <option value="302">2 Sec.</option>
        <option value="303">3 Sec.</option>
        <option value="304">5 Sec.</option>
        <option value="305">10 Sec.</option>
        <option value="306">15 Sec.</option>
        <option value="307">30 Sec.</option>
        <option value="308">60 Sec.</option>
        </select>
      </p>
      <h2>Tag Activated IR Cool Down</h2>
      <p><select name="playerlives" id="playerlivesid">
        <option value="401">Off/NA</option>
        <option value="402">5 Sec.</option>
        <option value="403">10 Sec.</option>
        <option value="404">15 Sec.</option>
        <option value="405">30 Sec.</option>
        <option value="406">60 Sec.</option>
        <option value="407">2 Min.</option>
        <option value="408">3 Min.</option>
        <option value="409">5 Min.</option>
        <option value="410">10 Min.</option>
        <option value="411">15 Min.</option>
        <option value="412">20 Min.</option>
        <option value="413">30 Min.</option>
        </select>
      </p>
      <h2>Capturable IR Base Tag Count</h2>
      <p><select name="gametime" id="gametimeid">
        <option value="501">1 Shots</option>
        <option value="502">10 Shots</option>
        <option value="503">15 Shots</option>
        <option value="504">30 Shots</option>
        <option value="505">60 Shots</option>
        <option value="506">100 Shots</option>
        <option value="507">150 Shots</option>
        <option value="508">300 Shots</option>
        <option value="509">500 Shots</option>
        <option value="510">1000 Shots</option>
        </select>
      </p>
      <h2>Gear Compatibility Selector</h2>
      <p><select name="ambience" id="ambienceid">
        <option value="601">BRX</option>
        <option value="602">LTTO</option>
        <option value="603">Battle Rifle Pro</option>
        </select>
      </p>
      <h2>Customized LTTO Tag</h2>
      <p><select name="lttotype" id="lttotypeid">
        <option value="700">Select Custom LTTO Tag</option>
        <option value="701">Hosted Game</option>
        <option value="702">Grab & Go Game</option>
        <option value="703">Respawn</option>
        <option value="704">Own the Zone</option>
        </select>
        <select name="lttoteam" id="lttoteamid">
        <option value="206">Select Team</option>
        <option value="202">Solo</option>
        <option value="203">Team 1</option>
        <option value="204">Team 2</option>
        <option value="205">Team 3</option>
        </select>
        <select name="lttodamage" id="lttodamageid">
        <option value="t20">Tag Count</option>
        <option value="t21">One</option>
        <option value="t22">Two</option>
        <option value="t23">Three</option>
        <option value="t24">Four</option>
        </select>
      </p>
      <p><button id="gameover" class="button">End Game</button></p>
      <p><button id="resetscores" class="button">Reset Scores</button></p>
    </div>
  </div>
  <div class="stopnav">
    <h3>Score Reporting</h3>
  </div>
  <div class="scontent">
    <div class="scards">
      <div class="scard red">
        <h4>Alpha Team - LTTO Solo</h4>
        <p><span class="reading"> Points:<span id="to0"></span></p>
      </div>
      <div class="scard blue">
        <h4>Bravo Team - LTTO Team 1</h4><p>
        <p><span class="reading"> Points:<span id="to1"></span></p>
      </div>
      <div class="scard yellow">
        <h4>Charlie Team - LTTO Team 2</h4>
        <p><span class="reading"> Points:<span id="to2"></span></p>
      </div>
      <div class="scard green">
        <h4>Delta Team - LTTO Team 3</h4>
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
    <p><button id="otaupdate" class="button">OTA Firmware Refresh</button></p>
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
    document.getElementById('primaryid').addEventListener('change', handleprimary, false);
    document.getElementById('secondaryid').addEventListener('change', handlesecondary, false);
    document.getElementById('meleesettingid').addEventListener('change', handlemelee, false);
    document.getElementById('playerlivesid').addEventListener('change', handlelives, false);
    document.getElementById('gametimeid').addEventListener('change', handlegametime, false);
    document.getElementById('ambienceid').addEventListener('change', handleambience, false);
    document.getElementById('ammoid').addEventListener('change', handleammo, false);
    document.getElementById('resetscores').addEventListener('click', toggle14s);
    document.getElementById('otaupdate').addEventListener('click', toggle14o);
    document.getElementById('gameover').addEventListener('click', toggle14a);
    document.getElementById('lttoteamid').addEventListener('change', handlelttoteam, false);
    document.getElementById('lttodamageid').addEventListener('change', handlelttodamage, false);
    document.getElementById('lttotypeid').addEventListener('change', handlelttotype, false);
    document.getElementById('deviceid').addEventListener('change', handledevice, false);
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
  function handleprimary() {
    var xa = document.getElementById("primaryid").value;
    websocket.send(xa);
  }
  function handlesecondary() {
    var xb = document.getElementById("secondaryid").value;
    websocket.send(xb);
  }
  function handlemelee() {
    var xc = document.getElementById("meleesettingid").value;
    websocket.send(xc);
  }
  function handledevice() {
    var xd = document.getElementById("deviceid").value;
    websocket.send(xd);
  }
  function handlelives() {
    var xe = document.getElementById("playerlivesid").value;
    websocket.send(xe);
  }
  function handlegametime() {
    var xf = document.getElementById("gametimeid").value;
    websocket.send(xf);
  }
  function handleambience() {
    var xg = document.getElementById("ambienceid").value;
    websocket.send(xg);
  }
  function handleammo() {
    var xl = document.getElementById("ammoid").value;
    websocket.send(xl);
  }
  function handlelttoteam() {
    var xm = document.getElementById("lttoteamid").value;
    websocket.send(xm);
  }
  function handlelttodamage() {
    var xn = document.getElementById("lttodamageid").value;
    websocket.send(xn);
  }
  function handlelttotype() {
    var xo = document.getElementById("lttotypeid").value;
    websocket.send(xo);
  }
  function handledevicename() {
    var xp = document.getElementById("devicenameid").value;
    websocket.send(xp);
  }
</script>
</body>
</html>
)rawliteral";

const char index1_html[] PROGMEM = R"rawliteral(
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
  <div class="stopnav">
    <h3>Score Reporting</h3>
  </div>
  <div class="scontent">
    <div class="scards">
      <div class="scard red">
        <h4>Alpha Team - LTTO Solo</h4>
        <p><span class="reading"> Points:<span id="to0"></span></p>
      </div>
      <div class="scard blue">
        <h4>Bravo Team - LTTO Team 1</h4><p>
        <p><span class="reading"> Points:<span id="to1"></span></p>
      </div>
      <div class="scard yellow">
        <h4>Charlie Team - LTTO Team 2</h4>
        <p><span class="reading"> Points:<span id="to2"></span></p>
      </div>
      <div class="scard green">
        <h4>Delta Team - LTTO Team 3</h4>
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
    document.getElementById('primaryid').addEventListener('change', handleprimary, false);
    document.getElementById('secondaryid').addEventListener('change', handlesecondary, false);
    document.getElementById('meleesettingid').addEventListener('change', handlemelee, false);
    document.getElementById('playerlivesid').addEventListener('change', handlelives, false);
    document.getElementById('gametimeid').addEventListener('change', handlegametime, false);
    document.getElementById('ambienceid').addEventListener('change', handleambience, false);
    document.getElementById('ammoid').addEventListener('change', handleammo, false);
    document.getElementById('resetscores').addEventListener('click', toggle14s);
    document.getElementById('gameover').addEventListener('click', toggle14a);
    document.getElementById('lttoteamid').addEventListener('change', handlelttoteam, false);
    document.getElementById('lttodamageid').addEventListener('change', handlelttodamage, false);
    document.getElementById('lttotypeid').addEventListener('change', handlelttotype, false);
    document.getElementById('deviceid').addEventListener('change', handledevice, false);
    
  }
  function toggle14s(){
    websocket.send('toggle14s');
  }
  function toggle14a(){
    websocket.send('toggle14a');
  }
  function handleprimary() {
    var xa = document.getElementById("primaryid").value;
    websocket.send(xa);
  }
  function handlesecondary() {
    var xb = document.getElementById("secondaryid").value;
    websocket.send(xb);
  }
  function handlemelee() {
    var xc = document.getElementById("meleesettingid").value;
    websocket.send(xc);
  }
  function handledevice() {
    var xd = document.getElementById("deviceid").value;
    websocket.send(xd);
  }
  function handlelives() {
    var xe = document.getElementById("playerlivesid").value;
    websocket.send(xe);
  }
  function handlegametime() {
    var xf = document.getElementById("gametimeid").value;
    websocket.send(xf);
  }
  function handleambience() {
    var xg = document.getElementById("ambienceid").value;
    websocket.send(xg);
  }
  function handleammo() {
    var xl = document.getElementById("ammoid").value;
    websocket.send(xl);
  }
  function handlelttoteam() {
    var xm = document.getElementById("lttoteamid").value;
    websocket.send(xm);
  }
  function handlelttodamage() {
    var xn = document.getElementById("lttodamageid").value;
    websocket.send(xn);
  }
  function handlelttotype() {
    var xo = document.getElementById("lttotypeid").value;
    websocket.send(xo);
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
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
        ResetScores();
      }
      if (DeviceSelector != JBOXID) {
        datapacket2 = 10801;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "toggle14o") == 0) { // game reset
      Serial.println("OTA Update Mode");
      EEPROM.write(0, 1); // reset OTA attempts counter
      EEPROM.commit(); // Store in Flash
      delay(2000);
      ESP.restart();
      //INITIALIZEOTA = true;
    }
    if (strcmp((char*)data, "toggle14a") == 0) { // game end
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
        DOMINATIONCLOCK = false;
        UpdateWebApp0();
        UpdateWebApp2();
      }
      if (DeviceSelector != JBOXID) {
        datapacket2 = 10802;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "1") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      EEPROM.write(1, 1);
      EEPROM.commit();
      // default domination mode that provides both player scoring and team scoring
      // Scoring reports over BLE to paired mobile device and refreshes every time
      // the score changes. To deactivate, select this option a second time to pause
      // the game/score. Recommended as a standalone base use only. Each time base is
      // shot, the team or player who shot takes posession and the base emitts a tag
      // that notifies player that the base (control point) was captured.
      Serial.println("Basic Domination Mode - Default!!!");
      //debugmonitor.println("Basic Domination Mode - Default!!!");
      Function = 1;
      RGBWHITE = true;
      CAPTURABLEEMITTER = false; // enables the team freindly to be toggled via IR
      DOMINATIONCLOCK = false; // stops the game from going if already running
      BASICDOMINATION = true; // enables this game mode
      ResetAllIRProtocols(); // resets all ir protocols
      CONTROLPOINTCAPTURED = true; // enables control point captured call out
      TAGACTIVATEDIR = true; // enables ir pulsing when base is shot
      BASECONTINUOUSIRPULSE = false;
      ENABLEIRRECEIVER = true;
      FIVEMINUTEDOMINATION = false;
      TENMINUTEDOMINATION = false;
      FIVEMINUTETUGOWAR = false;
      CAPTURETHEFLAGMODE = false;
      MULTIBASEDOMINATION = false;
      }
      if (DeviceSelector != JBOXID) {
        datapacket2 = 10001;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "2") == 0) {
      EEPROM.write(1, 2);
      EEPROM.commit();
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      // Continuous IR Emitter Mode, Clears out any existing IR Tag Settings
      // Then activates a default interval spaced broadcast of a tag of choice.
      // The tag of choice will need to be selected otherwise motion sensor is broadcasted.
      // This is good for use as a respawn station that requires no button or trigger
      // mechanism to activate. The default delay is two seconds but can be modified
      // by another setting option in the application. Another use is a proximity detector or mine.
      Serial.println("Continuous IR Emitter!!!");
      //debugmonitor.println("Continuous IR Emitter!!!");
      Function = 2;
      Team = 2;
      RGBGREEN = true;
      CAPTURABLEEMITTER = false; // enables the team freindly to be toggled via IR
      BASECONTINUOUSIRPULSE = true;
      DOMINATIONCLOCK = false; // stops the game from going if already running
      BASICDOMINATION = false; // enables this game mode
      ResetAllIRProtocols(); // Resets all ir protocols
      TAGACTIVATEDIR = false; // enables ir pulsing when base is shot
      ENABLEIRRECEIVER = false; // deactivates the ir receiver to avoid unnecessary processes
      FIVEMINUTEDOMINATION = false;
      TENMINUTEDOMINATION = false;
      FIVEMINUTETUGOWAR = false;
      CAPTURETHEFLAGMODE = false;
      MULTIBASEDOMINATION = false;
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10002;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "3") == 0) {
      EEPROM.write(1, 3);
      EEPROM.commit();
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      // Clears out any existing IR and Base Game Settings
      // Sets Default for tag activation to send an IR emitter protocol for "motion sensor" as default
      // 
      Serial.println("Tage Activated IR Emitter!!!");
      //debugmonitor.println("Tage Activated IR Emitter!!!");
      Function = 3;
      Team = 2;
      RGBWHITE = true;
      CAPTURABLEEMITTER = false; // enables the team freindly to be toggled via IR
      BASECONTINUOUSIRPULSE = false;
      DOMINATIONCLOCK = false; // stops the game from going if already running
      BASICDOMINATION = false; // enables this game mode
      TAGACTIVATEDIR = true; // enables ir pulsing when base is shot
      ENABLEIRRECEIVER = true; // activates the ir receiver to receive tags
      ResetAllIRProtocols(); // resets all ir protocols
      MOTIONSENSOR = true; // enables the alarm sound tag protocol 
      FIVEMINUTEDOMINATION = false;
      TENMINUTEDOMINATION = false;
      FIVEMINUTETUGOWAR = false;
      CAPTURETHEFLAGMODE = false;
      MULTIBASEDOMINATION = false;
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10003;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "4") == 0) {
      EEPROM.write(1, 4);
      EEPROM.commit();
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      // Clears out existing settings
      // Sets default Continuous Emitter Tag to Respwan Station As Default
      // Creates A Counter for Capturability by Shots (default is 10, change default to desired count) 
      // Enable IR Activated Capturability to switch team friendly setting to last captured    
      // bases start out as default red team
      // recommend doing a delay for respawn time to limit players ability to respawn right away on a base they may be guarding
      Serial.println("Dual Mode - Capturable Continuous IR Emitter!!!");
      //debugmonitor.println("Dual Mode - Capturable Continuous IR Emitter!!!");
      Function = 4;
      Team = 2; // sets all bases to default to red team
      RGBWHITE = true;
      BASECONTINUOUSIRPULSE = true;
      DOMINATIONCLOCK = false; // stops the game from going if already running
      BASICDOMINATION = false; // enables this game mode
      TAGACTIVATEDIR = false; // enables ir pulsing when base is shot
      ENABLEIRRECEIVER = true; // activates the ir receiver to receive tags
      ResetAllIRProtocols(); // reset all ir protocols
      RESPAWNSTATION = true; // enables the alarm sound tag protocol
      CAPTURABLEEMITTER = true; // enables the team freindly to be toggled via IR  
      FIVEMINUTEDOMINATION = false;
      TENMINUTEDOMINATION = false;
      FIVEMINUTETUGOWAR = false;
      CAPTURETHEFLAGMODE = false;
      MULTIBASEDOMINATION = false;
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10004;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "5") == 0) {
      EEPROM.write(1, 5);
      EEPROM.commit();
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      // 5 Minute domination mode that provides both player scoring and team scoring
      // Scoring reports over webserver to paired mobile device and refreshes every time
      // the score changes. To deactivate, select this option a second time to pause
      // the game/score. Recommended as a standalone base use only. Each time base is
      // shot, the team or player who shot takes posession and the base emitts a tag
      // that notifies player that the base (control point) was captured.
      Serial.println("5 Min Domination Mode - Default!!!");
      //debugmonitor.println("Basic Domination Mode - Default!!!");
      Function = 1;
      RGBWHITE = true;
      CAPTURABLEEMITTER = false; // enables the team freindly to be toggled via IR
      DOMINATIONCLOCK = false; // stops the game from going if already running
      BASICDOMINATION = true; // enables this game mode
      ResetAllIRProtocols(); // resets all ir protocols
      CONTROLPOINTCAPTURED = true; // enables control point captured call out
      TAGACTIVATEDIR = true; // enables ir pulsing when base is shot
      BASECONTINUOUSIRPULSE = false;
      ENABLEIRRECEIVER = true;
      FIVEMINUTEDOMINATION = true;
      TENMINUTEDOMINATION = false;
      FIVEMINUTETUGOWAR = false; 
      CAPTURETHEFLAGMODE = false; 
      MULTIBASEDOMINATION = false;
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10005;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "6") == 0) {
      EEPROM.write(1, 6);
      EEPROM.commit();
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      // 10 Minute domination mode that provides both player scoring and team scoring
      // Scoring reports over webserver to paired mobile device and refreshes every time
      // the score changes. To deactivate, select this option a second time to pause
      // the game/score. Recommended as a standalone base use only. Each time base is
      // shot, the team or player who shot takes posession and the base emitts a tag
      // that notifies player that the base (control point) was captured.
      Serial.println("10 min Domination Mode - Default!!!");
      //debugmonitor.println("Basic Domination Mode - Default!!!");
      Function = 1;
      RGBWHITE = true;
      CAPTURABLEEMITTER = false; // enables the team freindly to be toggled via IR
      DOMINATIONCLOCK = false; // stops the game from going if already running
      BASICDOMINATION = true; // enables this game mode
      ResetAllIRProtocols(); // resets all ir protocols
      CONTROLPOINTCAPTURED = true; // enables control point captured call out
      TAGACTIVATEDIR = true; // enables ir pulsing when base is shot
      BASECONTINUOUSIRPULSE = false;
      ENABLEIRRECEIVER = true;
      FIVEMINUTEDOMINATION = false;
      TENMINUTEDOMINATION = true;
      FIVEMINUTETUGOWAR = false;
      CAPTURETHEFLAGMODE = false;
      MULTIBASEDOMINATION = false;
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10006;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "7") == 0) {
      EEPROM.write(1, 7);
      EEPROM.commit();
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      // 5 Minute tug o war domination mode that provides both player scoring and team scoring
      // Scoring reports over webserver to paired mobile device and refreshes every time
      // the score changes. To deactivate, select this option a second time to pause
      // the game/score. Recommended as a standalone base use only. Each time base is
      // shot, the team or player who shot takes posession and the base emitts a tag
      // that notifies player that the base (control point) was captured.
      Serial.println("5 min Tug of War Domination Mode - Default!!!");
      //debugmonitor.println("Basic Domination Mode - Default!!!");
      Function = 1;
      RGBWHITE = true;
      CAPTURABLEEMITTER = false; // enables the team freindly to be toggled via IR
      DOMINATIONCLOCK = false; // stops the game from going if already running
      BASICDOMINATION = true; // enables this game mode
      ResetAllIRProtocols(); // resets all ir protocols
      CONTROLPOINTCAPTURED = true; // enables control point captured call out
      TAGACTIVATEDIR = true; // enables ir pulsing when base is shot
      BASECONTINUOUSIRPULSE = false;
      ENABLEIRRECEIVER = true;
      FIVEMINUTEDOMINATION = true;
      TENMINUTEDOMINATION = false;
      FIVEMINUTETUGOWAR = true;
      CAPTURETHEFLAGMODE = false;
      MULTIBASEDOMINATION = false;
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10007;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "8") == 0) {
      EEPROM.write(1, 8);
      EEPROM.commit();
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      // 5 Minute tug o war domination mode that provides both player scoring and team scoring
      // Scoring reports over webserver to paired mobile device and refreshes every time
      // the score changes. To deactivate, select this option a second time to pause
      // the game/score. Recommended as a standalone base use only. Each time base is
      // shot, the team or player who shot takes posession and the base emitts a tag
      // that notifies player that the base (control point) was captured.
      Serial.println("Capture the flag mode!!!");
      //debugmonitor.println("Basic Domination Mode - Default!!!");
      Function = 1;
      RGBWHITE = true;
      CAPTURABLEEMITTER = false; // enables the team freindly to be toggled via IR
      DOMINATIONCLOCK = false; // stops the game from going if already running
      BASICDOMINATION = false; // enables this game mode
      ResetAllIRProtocols(); // resets all ir protocols
      CONTROLPOINTCAPTURED = false; // enables control point captured call out
      TAGACTIVATEDIR = false; // enables ir pulsing when base is shot
      BASECONTINUOUSIRPULSE = false;
      ENABLEIRRECEIVER = true;
      FIVEMINUTEDOMINATION = true;
      TENMINUTEDOMINATION = false;
      FIVEMINUTETUGOWAR = false;
      CAPTURETHEFLAGMODE = true;
      MULTIBASEDOMINATION = false;
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10008;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "9") == 0) {
      EEPROM.write(1, 9);
      EEPROM.commit();
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      // 10 Minute domination mode that provides both player scoring and team scoring
      // Scoring reports over webserver to paired mobile device and refreshes every time
      // the score changes. To deactivate, select this option a second time to pause
      // the game/score. Recommended as a standalone base use only. Each time base is
      // shot, the team or player who shot takes posession and the base emitts a tag
      // that notifies player that the base (control point) was captured.
      Serial.println("10 min Domination Mode - Multi Bases!!!");
      //debugmonitor.println("Basic Domination Mode - Default!!!");
      Function = 1;
      RGBWHITE = true;
      CAPTURABLEEMITTER = false; // enables the team freindly to be toggled via IR
      DOMINATIONCLOCK = false; // stops the game from going if already running
      BASICDOMINATION = true; // enables this game mode
      ResetAllIRProtocols(); // resets all ir protocols
      CONTROLPOINTCAPTURED = true; // enables control point captured call out
      TAGACTIVATEDIR = true; // enables ir pulsing when base is shot
      BASECONTINUOUSIRPULSE = false;
      ENABLEIRRECEIVER = true;
      FIVEMINUTEDOMINATION = false;
      TENMINUTEDOMINATION = true;
      FIVEMINUTETUGOWAR = false;
      CAPTURETHEFLAGMODE = false;
      MULTIBASEDOMINATION = true;
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10009;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "10") == 0) {
      EEPROM.write(1, 10);
      EEPROM.commit();
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      // Own The Zone Mode sends the IR and zone espnow beacon to add objective points to player/teams
      // Continuous IR Emitter Mode, Clears out any existing IR Tag Settings
      // Clears out existing settings
      // Sets default Continuous Emitter Tag to Respwan Station As Default
      // Creates A Counter for Capturability by Shots (default is 10, change default to desired count) 
      // Enable IR Activated Capturability to switch team friendly setting to last captured    
      // bases start out as default red team
      // recommend doing a delay for respawn time to limit players ability to respawn right away on a base they may be guarding
      Serial.println("Dual Mode - Capturable Continuous IR Emitter!!!");
      //debugmonitor.println("Dual Mode - Capturable Continuous IR Emitter!!!");
      Function = 4;
      Team = 2; // sets all bases to default to red team
      RGBWHITE = true;
      BASECONTINUOUSIRPULSE = false;
      DOMINATIONCLOCK = false; // stops the game from going if already running
      BASICDOMINATION = false; // enables this game mode
      TAGACTIVATEDIR = false; // enables ir pulsing when base is shot
      ENABLEIRRECEIVER = true; // activates the ir receiver to receive tags
      ResetAllIRProtocols(); // reset all ir protocols
      OWNTHEZONE = true; // enables the ZONE TAG IR protocol
      CAPTURABLEEMITTER = true; // enables the team freindly to be toggled via IR  
      FIVEMINUTEDOMINATION = false;
      TENMINUTEDOMINATION = false;
      FIVEMINUTETUGOWAR = false;
      CAPTURETHEFLAGMODE = false;
      MULTIBASEDOMINATION = false;
      irledinterval = 15000;
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10004;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "101") == 0) {
      EEPROM.write(2, 1);
      EEPROM.commit();
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      ResetAllIRProtocols();
      MOTIONSENSOR = true;
      Serial.println("Motion Sensor Enabled!!!");
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10101;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "102") == 0) {
      EEPROM.write(2, 2);
      EEPROM.commit();
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      ResetAllIRProtocols();
      CAPTURETHEFLAG = true;
      Serial.println("Capture the Flag Enabled!!!");
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10102;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "103") == 0) {
      EEPROM.write(2, 3);
      EEPROM.commit();
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      ResetAllIRProtocols();
      CONTROLPOINTLOST = true;
      Serial.println("Control Point Lost Enabled!!!");
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10103;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "104") == 0) {
      EEPROM.write(2, 4);
      EEPROM.commit();
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      ResetAllIRProtocols();
      GASGRENADE = true;
      Serial.println("Gas Grenade Enabled!!!");
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10104;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "105") == 0) {
      EEPROM.write(2, 5);
      EEPROM.commit();
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      ResetAllIRProtocols();
      LIGHTDAMAGE = true;
      Serial.println("Light Damage Enabled!!!");
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10105;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "106") == 0) {
      EEPROM.write(2, 6);
      EEPROM.commit();
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      ResetAllIRProtocols();
      MEDIUMDAMAGE = true;
      Serial.println("Medium Damage Enabled!!!");
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10106;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "107") == 0) {
      EEPROM.write(2, 7);
      EEPROM.commit();
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      ResetAllIRProtocols();
      HEAVYDAMAGE = true;
      Serial.println("Heavy Damage Enabled!!!");
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10107;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "108") == 0) {
      EEPROM.write(2, 8);
      EEPROM.commit();
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      ResetAllIRProtocols();
      FRAG = true;
      Serial.println("Explosive Damage Enabled!!!");
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10108;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "109") == 0) {
      EEPROM.write(2, 9);
      EEPROM.commit();
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      ResetAllIRProtocols();
      RESPAWNSTATION = true;
      Serial.println("Respawn Station Enabled!!!");
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10109;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "110") == 0) {
      EEPROM.write(2, 10);
      EEPROM.commit();
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      ResetAllIRProtocols();
      MEDKIT = true;
      Serial.println("Medkit Enabled!!!");
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10110;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "112") == 0) {
      EEPROM.write(2, 12);
      EEPROM.commit();
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      ResetAllIRProtocols();
      ARMORBOOST = true;
      Serial.println("Armor Boost Enabled!!!");
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10112;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "113") == 0) {
      EEPROM.write(2, 13);
      EEPROM.commit();
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      ResetAllIRProtocols();
      SHEILDS = true;
      Serial.println("Sheilds Enabled!!!");
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10113;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "111") == 0) {
      EEPROM.write(2, 11);
      EEPROM.commit();
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      ResetAllIRProtocols();
      LOOTBOX = true;
      Serial.println("Loot Box Enabled!!!");
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10111;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "114") == 0) {
      EEPROM.write(2, 14);
      EEPROM.commit();
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      ResetAllIRProtocols();
      OWNTHEZONE = true;
      Serial.println("Own The Zone Enabled!!!");
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10114;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "115") == 0) {
      EEPROM.write(2, 15);
      EEPROM.commit();
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      ResetAllIRProtocols();
      CONTROLPOINTCAPTURED = true;
      Serial.println("Control Point Captured Enabled!!!");
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10115;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "116") == 0) {
      EEPROM.write(2, 16);
      EEPROM.commit();
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      ResetAllIRProtocols();
      CUSTOMLTTOTAG = true;
      Serial.println("LTTO Customizer Enabled!!!");
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10116;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "117") == 0) {
      EEPROM.write(2, 17);
      EEPROM.commit();
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      ResetAllIRProtocols();
      WRESPAWNSTATION = true;
      Serial.println("ESPNOW Respawn Station Enabled!!!");
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10117;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "t21") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      //ResetAllIRProtocols();
      LTTODamage = 1;
      EEPROM.write(9, 1);
      EEPROM.commit();
      Serial.println("Tags set to 1!!!");
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10151;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "t22") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      //ResetAllIRProtocols();
      LTTODamage = 1;
      EEPROM.write(9, 2);
      EEPROM.commit();
      Serial.println("Tags set to 2!!!");
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10152;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "t23") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      //ResetAllIRProtocols();
      LTTODamage = 1;
      EEPROM.write(9, 3);
      EEPROM.commit();
      Serial.println("Tags set to 3!!!");
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10153;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "t24") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      //ResetAllIRProtocols();
      LTTODamage = 1;
      EEPROM.write(9, 4);
      EEPROM.commit();
      Serial.println("Tags set to 4!!!");
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10154;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "201") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      Team = 2;
      RGBWHITE = true;
      EEPROM.write(3, 1);
      EEPROM.commit();
      Serial.println("Team Set to Nuetral!!!");
      // debugmonitor.println("Team Set to Nuetral!!!");
      ANYTEAM = true;
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10201;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "202") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      Team = 0;
      RGBRED = true;
      EEPROM.write(3, 2);
      EEPROM.commit();
      Serial.println("Team Set to Red!!!");
      //debugmonitor.println("Team Set to Red!!!");
      ANYTEAM = false;
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10202;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "203") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      Team = 1;
      RGBBLUE = true;
      EEPROM.write(3, 3);
      EEPROM.commit();
      Serial.println("Team Set to Blue!!!");
      //debugmonitor.println("Team Set to Red!!!");
      ANYTEAM = false;
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10203;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "204") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      Team = 2;
      RGBYELLOW = true;
      EEPROM.write(3, 4);
      EEPROM.commit();
      Serial.println("Team Set to Yellow!!!");
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10204;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "205") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      Team = 3;
      RGBGREEN = true;
      EEPROM.write(3, 5);
      EEPROM.commit();
      Serial.println("Team Set to Green!!!");
      //debugmonitor.println("Team Set to Green!!!");
      ANYTEAM = false;
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10205;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "301") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      irledinterval = 1000;
      EEPROM.write(4, 1);
      EEPROM.commit();
      Serial.println(irledinterval);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10301;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "302") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      irledinterval = 2000;
      EEPROM.write(4, 2);
      EEPROM.commit();
      Serial.println(irledinterval);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10302;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "303") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      irledinterval = 3000;
      EEPROM.write(4, 3);
      EEPROM.commit();
      Serial.println(irledinterval);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10303;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "304") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      irledinterval = 5000;
      EEPROM.write(4, 4);
      EEPROM.commit();
      Serial.println(irledinterval);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10304;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "305") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      irledinterval = 10000;
      EEPROM.write(4, 5);
      EEPROM.commit();
      Serial.println(irledinterval);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10305;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "306") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      irledinterval = 15000;
      EEPROM.write(4, 6);
      EEPROM.commit();
      Serial.println(irledinterval);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10306;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "307") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      irledinterval = 30000;
      EEPROM.write(4, 7);
      EEPROM.commit();
      Serial.println(irledinterval);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10307;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "308") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      irledinterval = 60000;
      EEPROM.write(4, 8);
      EEPROM.commit();
      Serial.println(irledinterval);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10308;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "401") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      TAGACTIVATEDIRCOOLDOWN = false;
      EEPROM.write(5, 1);
      EEPROM.commit();
      Serial.println("Cool Down Deactivated"); 
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10401;
        BROADCASTESPNOW = true;
      } 
    }
    if (strcmp((char*)data, "402") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      TAGACTIVATEDIRCOOLDOWN = true;
      CoolDownInterval = 5000;
      EEPROM.write(5, 2);
      EEPROM.commit();
      Serial.println("Cool Down Activated"); 
      Serial.print("Cool Down Timer Set to: ");
      Serial.println(CoolDownInterval);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10402;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "403") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      TAGACTIVATEDIRCOOLDOWN = true;
      CoolDownInterval = 10000;
      EEPROM.write(5, 3);
      EEPROM.commit();
      Serial.println("Cool Down Activated"); 
      Serial.print("Cool Down Timer Set to: ");
      Serial.println(CoolDownInterval);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10403;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "404") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      TAGACTIVATEDIRCOOLDOWN = true;
      CoolDownInterval = 15000;
      EEPROM.write(5, 4);
      EEPROM.commit();
      Serial.println("Cool Down Activated"); 
      Serial.print("Cool Down Timer Set to: ");
      Serial.println(CoolDownInterval);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10404;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "405") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      TAGACTIVATEDIRCOOLDOWN = true;
      CoolDownInterval = 30000;
      EEPROM.write(5, 5);
      EEPROM.commit();
      Serial.println("Cool Down Activated"); 
      Serial.print("Cool Down Timer Set to: ");
      Serial.println(CoolDownInterval);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10405;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "406") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      TAGACTIVATEDIRCOOLDOWN = true;
      CoolDownInterval = 60000;
      EEPROM.write(5, 6);
      EEPROM.commit();
      Serial.println("Cool Down Activated"); 
      Serial.print("Cool Down Timer Set to: ");
      Serial.println(CoolDownInterval);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10406;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "407") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      TAGACTIVATEDIRCOOLDOWN = true;
      CoolDownInterval = 120000;
      EEPROM.write(5, 7);
      EEPROM.commit();
      Serial.println("Cool Down Activated"); 
      Serial.print("Cool Down Timer Set to: ");
      Serial.println(CoolDownInterval);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10407;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "408") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      TAGACTIVATEDIRCOOLDOWN = true;
      CoolDownInterval = 180000;
      EEPROM.write(5, 8);
      EEPROM.commit();
      Serial.println("Cool Down Activated"); 
      Serial.print("Cool Down Timer Set to: ");
      Serial.println(CoolDownInterval);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10408;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "409") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      TAGACTIVATEDIRCOOLDOWN = true;
      CoolDownInterval = 300000;
      EEPROM.write(5, 9);
      EEPROM.commit();
      Serial.println("Cool Down Activated"); 
      Serial.print("Cool Down Timer Set to: ");
      Serial.println(CoolDownInterval);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10409;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "410") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      TAGACTIVATEDIRCOOLDOWN = true;
      CoolDownInterval = 600000;
      EEPROM.write(5, 10);
      EEPROM.commit();
      Serial.println("Cool Down Activated"); 
      Serial.print("Cool Down Timer Set to: ");
      Serial.println(CoolDownInterval);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10410;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "411") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      TAGACTIVATEDIRCOOLDOWN = true;
      EEPROM.write(5, 11);
      EEPROM.commit();
      CoolDownInterval = 900000;
      Serial.println("Cool Down Activated"); 
      Serial.print("Cool Down Timer Set to: ");
      Serial.println(CoolDownInterval);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10411;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "412") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      TAGACTIVATEDIRCOOLDOWN = true;
      CoolDownInterval = 1200000;
      EEPROM.write(5, 12);
      EEPROM.commit();
      Serial.println("Cool Down Activated"); 
      Serial.print("Cool Down Timer Set to: ");
      Serial.println(CoolDownInterval);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10412;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "413") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      TAGACTIVATEDIRCOOLDOWN = true;
      CoolDownInterval = 1800000;
      EEPROM.write(5, 13);
      EEPROM.commit();
      Serial.println("Cool Down Activated"); 
      Serial.print("Cool Down Timer Set to: ");
      Serial.println(CoolDownInterval);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10413;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "501") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      RequiredCapturableEmitterCounter = 1;
      EEPROM.write(6, 1);
      EEPROM.commit();
      Serial.print("Required Capturable Emitter Counter Set to: ");
      Serial.println(RequiredCapturableEmitterCounter);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10501;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "502") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      RequiredCapturableEmitterCounter = 10;
      EEPROM.write(6, 2);
      EEPROM.commit();
      Serial.print("Required Capturable Emitter Counter Set to: ");
      Serial.println(RequiredCapturableEmitterCounter);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10502;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "503") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      RequiredCapturableEmitterCounter = 15;
      EEPROM.write(6, 3);
      EEPROM.commit();
      Serial.print("Required Capturable Emitter Counter Set to: ");
      Serial.println(RequiredCapturableEmitterCounter);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10503;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "504") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      RequiredCapturableEmitterCounter = 30;
      EEPROM.write(6, 4);
      EEPROM.commit();
      Serial.print("Required Capturable Emitter Counter Set to: ");
      Serial.println(RequiredCapturableEmitterCounter);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10504;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "505") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      RequiredCapturableEmitterCounter = 60;
      EEPROM.write(6, 5);
      EEPROM.commit();
      Serial.print("Required Capturable Emitter Counter Set to: ");
      Serial.println(RequiredCapturableEmitterCounter);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10505;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "506") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      RequiredCapturableEmitterCounter = 100;
      EEPROM.write(6, 6);
      EEPROM.commit();
      Serial.print("Required Capturable Emitter Counter Set to: ");
      Serial.println(RequiredCapturableEmitterCounter);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10506;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "507") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      RequiredCapturableEmitterCounter = 150;
      EEPROM.write(6, 7);
      EEPROM.commit();
      Serial.print("Required Capturable Emitter Counter Set to: ");
      Serial.println(RequiredCapturableEmitterCounter);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10507;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "508") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      RequiredCapturableEmitterCounter = 300;
      EEPROM.write(6, 8);
      EEPROM.commit();
      Serial.print("Required Capturable Emitter Counter Set to: ");
      Serial.println(RequiredCapturableEmitterCounter);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10508;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "509") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      RequiredCapturableEmitterCounter = 500;
      EEPROM.write(6, 9);
      EEPROM.commit();
      Serial.print("Required Capturable Emitter Counter Set to: ");
      Serial.println(RequiredCapturableEmitterCounter);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10509;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "510") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      RequiredCapturableEmitterCounter = 1000;
      EEPROM.write(6, 10);
      EEPROM.commit();
      Serial.print("Required Capturable Emitter Counter Set to: ");
      Serial.println(RequiredCapturableEmitterCounter);
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10510;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "601") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      Serial.println("BRX Mode");
      GearMod = 0;
      EEPROM.write(7, 1);
      EEPROM.commit();
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10601;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "602") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      Serial.println("LTTO Mode");
      GearMod = 1;
      EEPROM.write(7, 2);
      EEPROM.commit();
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10602;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "603") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      Serial.println("BRP Mode");
      GearMod = 2;
      EEPROM.write(7, 3);
      EEPROM.commit();
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10603;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "700") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      //ResetAllIRProtocols();
      EEPROM.write(8, 0);
      EEPROM.commit();
      LTTOOTZ = false;
      LTTORESPAWN = false;
      LTTOHOSTED = false;
      LTTOGG = false;
      Serial.println("Hosted Tag!!!");
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10700;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "701") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      //ResetAllIRProtocols();
      EEPROM.write(8, 1);
      EEPROM.commit();
      LTTOOTZ = false;
      LTTORESPAWN = false;
      LTTOHOSTED = true;
      LTTOGG = false;
      Serial.println("Hosted Tag!!!");
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10701;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "702") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      //ResetAllIRProtocols();
      EEPROM.write(8, 2);
      EEPROM.commit();
      LTTOOTZ = false;
      LTTORESPAWN = false;
      LTTOHOSTED = false;
      LTTOGG = true;
      Serial.println("G&G Tag!!!");
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10702;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "703") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      //ResetAllIRProtocols();
      EEPROM.write(8, 3);
      EEPROM.commit();
      LTTOOTZ = false;
      LTTORESPAWN = true;
      LTTOHOSTED = false;
      LTTOGG = false;
      Serial.println("LTTO Respawns Enabled!!!");
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10703;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "704") == 0) {
      if (DeviceSelector == JBOXID || DeviceSelector == 199) {
      //ResetAllIRProtocols();
      EEPROM.write(8, 4);
      EEPROM.commit();
      LTTOOTZ = true;
      LTTORESPAWN = false;
      LTTOHOSTED = false;
      LTTOGG = false;
      Serial.println("LTTO Own The Zone!!!");
      //debugmonitor.println("Control Point Captured Enabled!!!");
      }if (DeviceSelector != JBOXID) {
        datapacket2 = 10704;
        BROADCASTESPNOW = true;
      }
    }
    if (strcmp((char*)data, "800") == 0) {
      DeviceSelector = 100;
      Serial.println("Device Selector Change!!!");
      datapacket1 = DeviceSelector;
    }
    if (strcmp((char*)data, "801") == 0) {
      DeviceSelector = 101;
      Serial.println("Device Selector Change!!!");
      datapacket1 = DeviceSelector;
    }
    if (strcmp((char*)data, "802") == 0) {
      DeviceSelector = 102;
      Serial.println("Device Selector Change!!!");
      datapacket1 = DeviceSelector;
    }
    if (strcmp((char*)data, "803") == 0) {
      DeviceSelector = 103;
      Serial.println("Device Selector Change!!!");
      datapacket1 = DeviceSelector;
    }
    if (strcmp((char*)data, "804") == 0) {
      DeviceSelector = 104;
      Serial.println("Device Selector Change!!!");
      datapacket1 = DeviceSelector;
    }
    if (strcmp((char*)data, "805") == 0) {
      DeviceSelector = 105;
      Serial.println("Device Selector Change!!!");
      datapacket1 = DeviceSelector;
    }
    if (strcmp((char*)data, "806") == 0) {
      DeviceSelector = 106;
      Serial.println("Device Selector Change!!!");
      datapacket1 = DeviceSelector;
    }
    if (strcmp((char*)data, "807") == 0) {
      DeviceSelector = 107;
      Serial.println("Device Selector Change!!!");
      datapacket1 = DeviceSelector;
    }
    if (strcmp((char*)data, "808") == 0) {
      DeviceSelector = 108;
      Serial.println("Device Selector Change!!!");
      datapacket1 = DeviceSelector;
    }
    if (strcmp((char*)data, "809") == 0) {
      DeviceSelector = 109;
      Serial.println("Device Selector Change!!!");
      datapacket1 = DeviceSelector;
    }
    if (strcmp((char*)data, "898") == 0) {
      DeviceSelector = 199;
      Serial.println("Device Selector Change!!!");
      datapacket1 = DeviceSelector;
    }
    if (strcmp((char*)data, "899") == 0) {
      DeviceSelector = JBOXID;
      Serial.println("Device Selector Change!!!");
      datapacket1 = DeviceSelector;
    }
    if (strcmp((char*)data, "900") == 0) {
      JBOXID = 100;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
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
    }
    if (strcmp((char*)data, "903") == 0) {
      JBOXID = 103;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "904") == 0) {
      JBOXID = 104;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "905") == 0) {
      JBOXID = 105;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "906") == 0) {
      JBOXID = 106;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "907") == 0) {
      JBOXID = 107;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "908") == 0) {
      JBOXID = 108;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "909") == 0) {
      JBOXID = 109;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "910") == 0) {
      JBOXID = 110;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "911") == 0) {
      JBOXID = 111;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "912") == 0) {
      JBOXID = 112;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "913") == 0) {
      JBOXID = 113;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "914") == 0) {
      JBOXID = 114;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "915") == 0) {
      JBOXID = 115;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "916") == 0) {
      JBOXID = 116;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "917") == 0) {
      JBOXID = 117;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "918") == 0) {
      JBOXID = 118;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "919") == 0) {
      JBOXID = 119;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
    }
    if (strcmp((char*)data, "920") == 0) {
      JBOXID = 120;
      Serial.println("JBOX ID changed to: " + String(JBOXID));
      EEPROM.write(10, JBOXID);
      EEPROM.commit();
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
void ProcessIncomingCommands() { 
  //Serial.print("cOMMS loop running on core ");
  //Serial.println(xPortGetCoreID());
  int IncomingTeam = 99;
  if (incomingData1 > 199) {
    IncomingTeam = incomingData1 - 200;
  }
  if (incomingData1 == JBOXID || incomingData1 == 199) { // data checked out to be intended for this player ID or for all players
    digitalWrite(2, HIGH);
    if (incomingData2  ==  10001) {
      // default domination mode that provides both player scoring and team scoring
      // Scoring reports over BLE to paired mobile device and refreshes every time
      // the score changes. To deactivate, select this option a second time to pause
      // the game/score. Recommended as a standalone base use only. Each time base is
      // shot, the team or player who shot takes posession and the base emitts a tag
      // that notifies player that the base (control point) was captured.
      Serial.println("Basic Domination Mode - Default!!!");
      //debugmonitor.println("Basic Domination Mode - Default!!!");
      Function = 1;
      RGBWHITE = true;
      CAPTURABLEEMITTER = false; // enables the team freindly to be toggled via IR
      DOMINATIONCLOCK = false; // stops the game from going if already running
      BASICDOMINATION = true; // enables this game mode
      ResetAllIRProtocols(); // resets all ir protocols
      CONTROLPOINTCAPTURED = true; // enables control point captured call out
      TAGACTIVATEDIR = true; // enables ir pulsing when base is shot
      BASECONTINUOUSIRPULSE = false;
      ENABLEIRRECEIVER = true;
      FIVEMINUTEDOMINATION = false;
      TENMINUTEDOMINATION = false;
      FIVEMINUTETUGOWAR = false;
      CAPTURETHEFLAGMODE = false;
      MULTIBASEDOMINATION = false;
    }
    if (incomingData2  ==  10002) {
      // Continuous IR Emitter Mode, Clears out any existing IR Tag Settings
      // Then activates a default interval spaced broadcast of a tag of choice.
      // The tag of choice will need to be selected otherwise motion sensor is broadcasted.
      // This is good for use as a respawn station that requires no button or trigger
      // mechanism to activate. The default delay is two seconds but can be modified
      // by another setting option in the application. Another use is a proximity detector or mine.
      Serial.println("Continuous IR Emitter!!!");
      //debugmonitor.println("Continuous IR Emitter!!!");
      Function = 2;
      Team = 2;
      RGBGREEN = true;
      CAPTURABLEEMITTER = false; // enables the team freindly to be toggled via IR
      BASECONTINUOUSIRPULSE = true;
      DOMINATIONCLOCK = false; // stops the game from going if already running
      BASICDOMINATION = false; // enables this game mode
      ResetAllIRProtocols(); // Resets all ir protocols
      TAGACTIVATEDIR = false; // enables ir pulsing when base is shot
      ENABLEIRRECEIVER = false; // deactivates the ir receiver to avoid unnecessary processes
      FIVEMINUTEDOMINATION = false;
      TENMINUTEDOMINATION = false;
      FIVEMINUTETUGOWAR = false;
      CAPTURETHEFLAGMODE = false;
      MULTIBASEDOMINATION = false;
    }
    if (incomingData2  ==  10003) {
      // Clears out any existing IR and Base Game Settings
      // Sets Default for tag activation to send an IR emitter protocol for "motion sensor" as default
      // 
      Serial.println("Tage Activated IR Emitter!!!");
      //debugmonitor.println("Tage Activated IR Emitter!!!");
      Function = 3;
      Team = 2;
      RGBWHITE = true;
      CAPTURABLEEMITTER = false; // enables the team freindly to be toggled via IR
      BASECONTINUOUSIRPULSE = false;
      DOMINATIONCLOCK = false; // stops the game from going if already running
      BASICDOMINATION = false; // enables this game mode
      TAGACTIVATEDIR = true; // enables ir pulsing when base is shot
      ENABLEIRRECEIVER = true; // activates the ir receiver to receive tags
      ResetAllIRProtocols(); // resets all ir protocols
      MOTIONSENSOR = true; // enables the alarm sound tag protocol 
      FIVEMINUTEDOMINATION = false;
      TENMINUTEDOMINATION = false;
      FIVEMINUTETUGOWAR = false;
      CAPTURETHEFLAGMODE = false;
      MULTIBASEDOMINATION = false;
    }
    if (incomingData2  ==  10004) {
      // Clears out existing settings
      // Sets default Continuous Emitter Tag to Respwan Station As Default
      // Creates A Counter for Capturability by Shots (default is 10, change default to desired count) 
      // Enable IR Activated Capturability to switch team friendly setting to last captured    
      // bases start out as default red team
      // recommend doing a delay for respawn time to limit players ability to respawn right away on a base they may be guarding
      Serial.println("Dual Mode - Capturable Continuous IR Emitter!!!");
      //debugmonitor.println("Dual Mode - Capturable Continuous IR Emitter!!!");
      Function = 4;
      Team = 2; // sets all bases to default to red team
      RGBWHITE = true;
      BASECONTINUOUSIRPULSE = true;
      DOMINATIONCLOCK = false; // stops the game from going if already running
      BASICDOMINATION = false; // enables this game mode
      TAGACTIVATEDIR = false; // enables ir pulsing when base is shot
      ENABLEIRRECEIVER = true; // activates the ir receiver to receive tags
      ResetAllIRProtocols(); // reset all ir protocols
      RESPAWNSTATION = true; // enables the alarm sound tag protocol
      CAPTURABLEEMITTER = true; // enables the team freindly to be toggled via IR  
      FIVEMINUTEDOMINATION = false;
      TENMINUTEDOMINATION = false;
      FIVEMINUTETUGOWAR = false;
      CAPTURETHEFLAGMODE = false;
      MULTIBASEDOMINATION = false;
    }
    if (incomingData2  ==  10005) {
      // 5 Minute domination mode that provides both player scoring and team scoring
      // Scoring reports over webserver to paired mobile device and refreshes every time
      // the score changes. To deactivate, select this option a second time to pause
      // the game/score. Recommended as a standalone base use only. Each time base is
      // shot, the team or player who shot takes posession and the base emitts a tag
      // that notifies player that the base (control point) was captured.
      Serial.println("5 Min Domination Mode - Default!!!");
      //debugmonitor.println("Basic Domination Mode - Default!!!");
      Function = 1;
      RGBWHITE = true;
      CAPTURABLEEMITTER = false; // enables the team freindly to be toggled via IR
      DOMINATIONCLOCK = false; // stops the game from going if already running
      BASICDOMINATION = true; // enables this game mode
      ResetAllIRProtocols(); // resets all ir protocols
      CONTROLPOINTCAPTURED = true; // enables control point captured call out
      TAGACTIVATEDIR = true; // enables ir pulsing when base is shot
      BASECONTINUOUSIRPULSE = false;
      ENABLEIRRECEIVER = true;
      FIVEMINUTEDOMINATION = true;
      TENMINUTEDOMINATION = false;
      FIVEMINUTETUGOWAR = false; 
      CAPTURETHEFLAGMODE = false; 
      MULTIBASEDOMINATION = false;
    }
    if (incomingData2  ==  10006) {
      // 10 Minute domination mode that provides both player scoring and team scoring
      // Scoring reports over webserver to paired mobile device and refreshes every time
      // the score changes. To deactivate, select this option a second time to pause
      // the game/score. Recommended as a standalone base use only. Each time base is
      // shot, the team or player who shot takes posession and the base emitts a tag
      // that notifies player that the base (control point) was captured.
      Serial.println("10 min Domination Mode - Default!!!");
      //debugmonitor.println("Basic Domination Mode - Default!!!");
      Function = 1;
      RGBWHITE = true;
      CAPTURABLEEMITTER = false; // enables the team freindly to be toggled via IR
      DOMINATIONCLOCK = false; // stops the game from going if already running
      BASICDOMINATION = true; // enables this game mode
      ResetAllIRProtocols(); // resets all ir protocols
      CONTROLPOINTCAPTURED = true; // enables control point captured call out
      TAGACTIVATEDIR = true; // enables ir pulsing when base is shot
      BASECONTINUOUSIRPULSE = false;
      ENABLEIRRECEIVER = true;
      FIVEMINUTEDOMINATION = false;
      TENMINUTEDOMINATION = true;
      FIVEMINUTETUGOWAR = false;
      CAPTURETHEFLAGMODE = false;
      MULTIBASEDOMINATION = false;
    }
    if (incomingData2  ==  10007) {
      // 5 Minute tug o war domination mode that provides both player scoring and team scoring
      // Scoring reports over webserver to paired mobile device and refreshes every time
      // the score changes. To deactivate, select this option a second time to pause
      // the game/score. Recommended as a standalone base use only. Each time base is
      // shot, the team or player who shot takes posession and the base emitts a tag
      // that notifies player that the base (control point) was captured.
      Serial.println("5 min Tug of War Domination Mode - Default!!!");
      //debugmonitor.println("Basic Domination Mode - Default!!!");
      Function = 1;
      RGBWHITE = true;
      CAPTURABLEEMITTER = false; // enables the team freindly to be toggled via IR
      DOMINATIONCLOCK = false; // stops the game from going if already running
      BASICDOMINATION = true; // enables this game mode
      ResetAllIRProtocols(); // resets all ir protocols
      CONTROLPOINTCAPTURED = true; // enables control point captured call out
      TAGACTIVATEDIR = true; // enables ir pulsing when base is shot
      BASECONTINUOUSIRPULSE = false;
      ENABLEIRRECEIVER = true;
      FIVEMINUTEDOMINATION = true;
      TENMINUTEDOMINATION = false;
      FIVEMINUTETUGOWAR = true;
      CAPTURETHEFLAGMODE = false;
      MULTIBASEDOMINATION = false;
    }
    if (incomingData2  ==  10008) {
      // 5 Minute tug o war domination mode that provides both player scoring and team scoring
      // Scoring reports over webserver to paired mobile device and refreshes every time
      // the score changes. To deactivate, select this option a second time to pause
      // the game/score. Recommended as a standalone base use only. Each time base is
      // shot, the team or player who shot takes posession and the base emitts a tag
      // that notifies player that the base (control point) was captured.
      Serial.println("Capture the flag mode!!!");
      //debugmonitor.println("Basic Domination Mode - Default!!!");
      Function = 1;
      RGBWHITE = true;
      CAPTURABLEEMITTER = false; // enables the team freindly to be toggled via IR
      DOMINATIONCLOCK = false; // stops the game from going if already running
      BASICDOMINATION = false; // enables this game mode
      ResetAllIRProtocols(); // resets all ir protocols
      CONTROLPOINTCAPTURED = false; // enables control point captured call out
      TAGACTIVATEDIR = false; // enables ir pulsing when base is shot
      BASECONTINUOUSIRPULSE = false;
      ENABLEIRRECEIVER = true;
      FIVEMINUTEDOMINATION = true;
      TENMINUTEDOMINATION = false;
      FIVEMINUTETUGOWAR = false;
      CAPTURETHEFLAGMODE = true;
      MULTIBASEDOMINATION = false;
    }
    if (incomingData2  ==  10009) {
      // 10 Minute domination mode that provides both player scoring and team scoring
      // Scoring reports over webserver to paired mobile device and refreshes every time
      // the score changes. To deactivate, select this option a second time to pause
      // the game/score. Recommended as a standalone base use only. Each time base is
      // shot, the team or player who shot takes posession and the base emitts a tag
      // that notifies player that the base (control point) was captured.
      Serial.println("10 min Domination Mode - Multi Bases!!!");
      //debugmonitor.println("Basic Domination Mode - Default!!!");
      Function = 1;
      RGBWHITE = true;
      CAPTURABLEEMITTER = false; // enables the team freindly to be toggled via IR
      DOMINATIONCLOCK = false; // stops the game from going if already running
      BASICDOMINATION = true; // enables this game mode
      ResetAllIRProtocols(); // resets all ir protocols
      CONTROLPOINTCAPTURED = true; // enables control point captured call out
      TAGACTIVATEDIR = true; // enables ir pulsing when base is shot
      BASECONTINUOUSIRPULSE = false;
      ENABLEIRRECEIVER = true;
      FIVEMINUTEDOMINATION = false;
      TENMINUTEDOMINATION = true;
      FIVEMINUTETUGOWAR = false;
      CAPTURETHEFLAGMODE = false;
      MULTIBASEDOMINATION = true;
    }
    if (incomingData2  ==  10010) {
      // Own The Zone Mode sends the IR and zone espnow beacon to add objective points to player/teams
      // Continuous IR Emitter Mode, Clears out any existing IR Tag Settings
      // Clears out existing settings
      // Sets default Continuous Emitter Tag to Respwan Station As Default
      // Creates A Counter for Capturability by Shots (default is 10, change default to desired count) 
      // Enable IR Activated Capturability to switch team friendly setting to last captured    
      // bases start out as default red team
      // recommend doing a delay for respawn time to limit players ability to respawn right away on a base they may be guarding
      Serial.println("Dual Mode - Capturable Continuous IR Emitter!!!");
      //debugmonitor.println("Dual Mode - Capturable Continuous IR Emitter!!!");
      Function = 4;
      Team = 2; // sets all bases to default to red team
      RGBWHITE = true;
      BASECONTINUOUSIRPULSE = false;
      DOMINATIONCLOCK = false; // stops the game from going if already running
      BASICDOMINATION = false; // enables this game mode
      TAGACTIVATEDIR = false; // enables ir pulsing when base is shot
      ENABLEIRRECEIVER = true; // activates the ir receiver to receive tags
      ResetAllIRProtocols(); // reset all ir protocols
      OWNTHEZONE = true; // enables the ZONE TAG IR protocol
      CAPTURABLEEMITTER = true; // enables the team freindly to be toggled via IR  
      FIVEMINUTEDOMINATION = false;
      TENMINUTEDOMINATION = false;
      FIVEMINUTETUGOWAR = false;
      CAPTURETHEFLAGMODE = false;
      MULTIBASEDOMINATION = false;
      irledinterval = 15000;
    }
    if (incomingData2  ==  10101) {
      ResetAllIRProtocols();
      MOTIONSENSOR = true;
      Serial.println("Motion Sensor Enabled!!!");
      //debugmonitor.println("Motion Sensor Enabled!!!");
    }
    if (incomingData2  ==  10102) {
      ResetAllIRProtocols();
      CAPTURETHEFLAG = true;
      Serial.println("Capture the Flag Enabled!!!");
      //debugmonitor.println("Capture the Flag Enabled!!!");
    }
    if (incomingData2  ==  10103) {
      ResetAllIRProtocols();
      CONTROLPOINTLOST = true;
      Serial.println("Control Point Lost Enabled!!!");
      //debugmonitor.println("Control Point Lost Enabled!!!");
    }
    if (incomingData2  ==  10104) {
      ResetAllIRProtocols();
      GASGRENADE = true;
      Serial.println("Gas Grenade Enabled!!!");
      //debugmonitor.println("Gas Grenade Enabled!!!");
    }
    if (incomingData2  ==  10105) {
      ResetAllIRProtocols();
      LIGHTDAMAGE = true;
      Serial.println("Light Damage Enabled!!!");
      //debugmonitor.println("Light Damage Enabled!!!");
    }
    if (incomingData2  ==  10106) {
      ResetAllIRProtocols();
      MEDIUMDAMAGE = true;
      Serial.println("Medium Damage Enabled!!!");
      //debugmonitor.println("Medium Damage Enabled!!!");
    }
    if (incomingData2  ==  10107) {
      ResetAllIRProtocols();
      HEAVYDAMAGE = true;
      Serial.println("Heavy Damage Enabled!!!");
      //debugmonitor.println("Heavy Damage Enabled!!!");
    }
    if (incomingData2  ==  10108) {
      ResetAllIRProtocols();
      FRAG = true;
      Serial.println("Explosive Damage Enabled!!!");
      //debugmonitor.println("Explosive Damage Enabled!!!");
    }
    if (incomingData2  ==  10109) {
      ResetAllIRProtocols();
      RESPAWNSTATION = true;
      Serial.println("Respawn Station Enabled!!!");
      //debugmonitor.println("Respawn Station Enabled!!!");
    }
    if (incomingData2  ==  10110) {
      ResetAllIRProtocols();
      MEDKIT = true;
      Serial.println("Medkit Enabled!!!");
      //debugmonitor.println("Medkit Enabled!!!");
    }
    if (incomingData2  ==  10112) {
      ResetAllIRProtocols();
      ARMORBOOST = true;
      Serial.println("Armor Boost Enabled!!!");
      //debugmonitor.println("Armor Boost Enabled!!!");
    }
    if (incomingData2  ==  10113) {
      ResetAllIRProtocols();
      SHEILDS = true;
      Serial.println("Sheilds Enabled!!!");
      //debugmonitor.println("Sheilds Enabled!!!");
    }
    if (incomingData2  ==  10111) {
      ResetAllIRProtocols();
      LOOTBOX = true;
      Serial.println("Loot Box Enabled!!!");
      //debugmonitor.println("Loot Box Enabled!!!");
    }
    if (incomingData2  ==  10114) {
      ResetAllIRProtocols();
      OWNTHEZONE = true;
      Serial.println("Own The Zone Enabled!!!");
      //debugmonitor.println("Own The Zone Enabled!!!");
    }
    if (incomingData2  ==  10115) {
      ResetAllIRProtocols();
      CONTROLPOINTCAPTURED = true;
      Serial.println("Control Point Captured Enabled!!!");
      //debugmonitor.println("Control Point Captured Enabled!!!");
    }
    if (incomingData2  ==  10116) {
      ResetAllIRProtocols();
      CUSTOMLTTOTAG = true;
      Serial.println("LTTO Customizer Enabled!!!");
    }
    if (incomingData2  ==  10117) {
      ResetAllIRProtocols();
      WRESPAWNSTATION = true;
      Serial.println("ESPNOW Respawn Station Enabled!!!");
    }
    if (incomingData2  ==  10151) {
      //ResetAllIRProtocols();
      LTTODamage = 1;
      Serial.println("Tags set to 1!!!");
      //debugmonitor.println("Control Point Captured Enabled!!!");
    }
    if (incomingData2  ==  10152) {
      //ResetAllIRProtocols();
      LTTODamage = 1;
      Serial.println("Tags set to 2!!!");
      //debugmonitor.println("Control Point Captured Enabled!!!");
    }
    if (incomingData2  ==  10153) {
      //ResetAllIRProtocols();
      LTTODamage = 1;
      Serial.println("Tags set to 3!!!");
      //debugmonitor.println("Control Point Captured Enabled!!!");
    }
    if (incomingData2  ==  10154) {
      //ResetAllIRProtocols();
      LTTODamage = 1;
      Serial.println("Tags set to 4!!!");
      //debugmonitor.println("Control Point Captured Enabled!!!");
    }
    if (incomingData2  ==  10201) {
      Team = 2;
      RGBWHITE = true;
      Serial.println("Team Set to Nuetral!!!");
      // debugmonitor.println("Team Set to Nuetral!!!");
      ANYTEAM = true;
    }
    if (incomingData2  ==  10202) {
      Team = 0;
      RGBRED = true;
      Serial.println("Team Set to Red!!!");
      //debugmonitor.println("Team Set to Red!!!");
      ANYTEAM = false;
    }
    if (incomingData2  ==  10203) {
      Team = 1;
      RGBBLUE = true;
      Serial.println("Team Set to Blue!!!");
      //debugmonitor.println("Team Set to Red!!!");
      ANYTEAM = false;
    }
    if (incomingData2  ==  10204) {
      Team = 2;
      RGBYELLOW = true;
      Serial.println("Team Set to Yellow!!!");
      //debugmonitor.println("Team Set to Yellow!!!");
    }
    if (incomingData2  ==  10205) {
      Team = 3;
      RGBGREEN = true;
      Serial.println("Team Set to Green!!!");
      //debugmonitor.println("Team Set to Green!!!");
      ANYTEAM = false;
    }
    if (incomingData2  ==  10301) {
      irledinterval = 1000;
      Serial.println(irledinterval);
    }
    if (incomingData2  ==  10302) {
      irledinterval = 2000;
      Serial.println(irledinterval);
    }
    if (incomingData2  ==  10303) {
      irledinterval = 3000;
      Serial.println(irledinterval);
    }
    if (incomingData2  ==  10304) {
      irledinterval = 5000;
      Serial.println(irledinterval);
    }
    if (incomingData2  ==  10305) {
      irledinterval = 10000;
      Serial.println(irledinterval);
    }
    if (incomingData2  ==  10306) {
      irledinterval = 15000;
      Serial.println(irledinterval);
    }
    if (incomingData2  ==  10307) {
      irledinterval = 30000;
      Serial.println(irledinterval);
    }
    if (incomingData2  ==  10308) {
      irledinterval = 60000;
      Serial.println(irledinterval);
    }
    if (incomingData2  ==  10401) {
      TAGACTIVATEDIRCOOLDOWN = false;
    Serial.println("Cool Down Deactivated");  
    }
    if (incomingData2  ==  10402) {
      TAGACTIVATEDIRCOOLDOWN = true;
      CoolDownInterval = 5000;
      Serial.println("Cool Down Activated"); 
      Serial.print("Cool Down Timer Set to: ");
      Serial.println(CoolDownInterval);
    }
    if (incomingData2  ==  10403) {
      TAGACTIVATEDIRCOOLDOWN = true;
      CoolDownInterval = 10000;
      Serial.println("Cool Down Activated"); 
      Serial.print("Cool Down Timer Set to: ");
      Serial.println(CoolDownInterval);
    }
    if (incomingData2  ==  10404) {
      TAGACTIVATEDIRCOOLDOWN = true;
      CoolDownInterval = 15000;
      Serial.println("Cool Down Activated"); 
      Serial.print("Cool Down Timer Set to: ");
      Serial.println(CoolDownInterval);
    }
    if (incomingData2  ==  10405) {
      TAGACTIVATEDIRCOOLDOWN = true;
      CoolDownInterval = 30000;
      Serial.println("Cool Down Activated"); 
      Serial.print("Cool Down Timer Set to: ");
      Serial.println(CoolDownInterval);
    }
    if (incomingData2  ==  10406) {
      TAGACTIVATEDIRCOOLDOWN = true;
      CoolDownInterval = 60000;
      Serial.println("Cool Down Activated"); 
      Serial.print("Cool Down Timer Set to: ");
      Serial.println(CoolDownInterval);
    }
    if (incomingData2  ==  10407) {
      TAGACTIVATEDIRCOOLDOWN = true;
      CoolDownInterval = 120000;
      Serial.println("Cool Down Activated"); 
      Serial.print("Cool Down Timer Set to: ");
      Serial.println(CoolDownInterval);
    }
    if (incomingData2  ==  10408) {
      TAGACTIVATEDIRCOOLDOWN = true;
      CoolDownInterval = 180000;
      Serial.println("Cool Down Activated"); 
      Serial.print("Cool Down Timer Set to: ");
      Serial.println(CoolDownInterval);
    }
    if (incomingData2  ==  10409) {
      TAGACTIVATEDIRCOOLDOWN = true;
      CoolDownInterval = 300000;
      Serial.println("Cool Down Activated"); 
      Serial.print("Cool Down Timer Set to: ");
      Serial.println(CoolDownInterval);
    }
    if (incomingData2  ==  10410) {
      TAGACTIVATEDIRCOOLDOWN = true;
      CoolDownInterval = 600000;
      Serial.println("Cool Down Activated"); 
      Serial.print("Cool Down Timer Set to: ");
      Serial.println(CoolDownInterval);
    }
    if (incomingData2  ==  10411) {
      TAGACTIVATEDIRCOOLDOWN = true;
      CoolDownInterval = 900000;
      Serial.println("Cool Down Activated"); 
      Serial.print("Cool Down Timer Set to: ");
      Serial.println(CoolDownInterval);
    }
    if (incomingData2  ==  10412) {
      TAGACTIVATEDIRCOOLDOWN = true;
      CoolDownInterval = 1200000;
      Serial.println("Cool Down Activated"); 
      Serial.print("Cool Down Timer Set to: ");
      Serial.println(CoolDownInterval);
    }
    if (incomingData2  ==  10413) {
      TAGACTIVATEDIRCOOLDOWN = true;
      CoolDownInterval = 1800000;
      Serial.println("Cool Down Activated"); 
      Serial.print("Cool Down Timer Set to: ");
      Serial.println(CoolDownInterval);
    }
    if (incomingData2  ==  10501) {
      RequiredCapturableEmitterCounter = 1;
      Serial.print("Required Capturable Emitter Counter Set to: ");
      Serial.println(RequiredCapturableEmitterCounter);
    }
    if (incomingData2  ==  10502) {
      RequiredCapturableEmitterCounter = 10;
      Serial.print("Required Capturable Emitter Counter Set to: ");
      Serial.println(RequiredCapturableEmitterCounter);
    }
    if (incomingData2  ==  10503) {
      RequiredCapturableEmitterCounter = 15;
      Serial.print("Required Capturable Emitter Counter Set to: ");
      Serial.println(RequiredCapturableEmitterCounter);
    }
    if (incomingData2  ==  10504) {
      RequiredCapturableEmitterCounter = 30;
      Serial.print("Required Capturable Emitter Counter Set to: ");
      Serial.println(RequiredCapturableEmitterCounter);
    }
    if (incomingData2  ==  10505) {
      RequiredCapturableEmitterCounter = 60;
      Serial.print("Required Capturable Emitter Counter Set to: ");
      Serial.println(RequiredCapturableEmitterCounter);
    }
    if (incomingData2  ==  10506) {
      RequiredCapturableEmitterCounter = 100;
      Serial.print("Required Capturable Emitter Counter Set to: ");
      Serial.println(RequiredCapturableEmitterCounter);
    }
    if (incomingData2  ==  10507) {
      RequiredCapturableEmitterCounter = 150;
      Serial.print("Required Capturable Emitter Counter Set to: ");
      Serial.println(RequiredCapturableEmitterCounter);
    }
    if (incomingData2  ==  10508) {
      RequiredCapturableEmitterCounter = 300;
      Serial.print("Required Capturable Emitter Counter Set to: ");
      Serial.println(RequiredCapturableEmitterCounter);
    }
    if (incomingData2  ==  10509) {
      RequiredCapturableEmitterCounter = 500;
      Serial.print("Required Capturable Emitter Counter Set to: ");
      Serial.println(RequiredCapturableEmitterCounter);
    }
    if (incomingData2  ==  10510) {
      RequiredCapturableEmitterCounter = 1000;
      Serial.print("Required Capturable Emitter Counter Set to: ");
      Serial.println(RequiredCapturableEmitterCounter);
    }
    if (incomingData2  ==  10601) {
      Serial.println("BRX Mode");
      GearMod = 0;
    }
    if (incomingData2  ==  10602) {
      Serial.println("LTTO Mode");
      GearMod = 1;
    }
    if (incomingData2  ==  10603) {
      Serial.println("Recoil Mode");
    }
    if (incomingData2  ==  10700) {
      //ResetAllIRProtocols();
      LTTOOTZ = false;
      LTTORESPAWN = false;
      LTTOHOSTED = false;
      LTTOGG = false;
      Serial.println("Hosted Tag!!!");
      //debugmonitor.println("Control Point Captured Enabled!!!");
    }
    if (incomingData2  ==  10701) {
      //ResetAllIRProtocols();
      LTTOOTZ = false;
      LTTORESPAWN = false;
      LTTOHOSTED = true;
      LTTOGG = false;
      Serial.println("Hosted Tag!!!");
      //debugmonitor.println("Control Point Captured Enabled!!!");
    }
    if (incomingData2  ==  10702) {
      //ResetAllIRProtocols();
      LTTOOTZ = false;
      LTTORESPAWN = false;
      LTTOHOSTED = false;
      LTTOGG = true;
      Serial.println("G&G Tag!!!");
      //debugmonitor.println("Control Point Captured Enabled!!!");
    }
    if (incomingData2  ==  10703) {
      //ResetAllIRProtocols();
      LTTOOTZ = false;
      LTTORESPAWN = true;
      LTTOHOSTED = false;
      LTTOGG = false;
      Serial.println("LTTO Respawns Enabled!!!");
      //debugmonitor.println("Control Point Captured Enabled!!!");
    }
    if (incomingData2  ==  10704) {
      //ResetAllIRProtocols();
      LTTOOTZ = true;
      LTTORESPAWN = false;
      LTTOHOSTED = false;
      LTTOGG = false;
      Serial.println("LTTO Own The Zone!!!");
      //debugmonitor.println("Control Point Captured Enabled!!!");
    }
    if (incomingData2  ==  10801) {
      // game reset
      ResetScores();
      Serial.println("Resetting Scores");
    }
    if (incomingData2  ==  10802) {
      // Game End
      DOMINATIONCLOCK = false;
      UpdateWebApp0();
      UpdateWebApp2();
      Serial.println("Game Over");
    }
    if (incomingData2 < 1000 && incomingData2 > 900) { // Syncing scores
      //int b = incomingData2 - 900;
      //if (b == 2) { // this is an incoming score from a player!
        //AccumulateIncomingScores();
      //}
    }
  }
  digitalWrite(2, LOW);
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
        int LoRaDataVerification = tokenStrings[2].toInt();
        // check the data for a data match
        if(LoRaDataVerification < 1000) {
          // Confirmed the data is coming from a Respawn Station, remaining is the player ID
          if (LoRaDataVerification < 100) {
            // Confirmed this is team red were receiving data about
            int IncomingPlayerID = LoRaDataVerification;
            int IncomingTeamID = 0;
          } else {
            if (LoRaDataVerification < 200) {
              // Confirmed this is team blue were receiving data about
              int IncomingPlayerID = LoRaDataVerification - 100;
              int IncomingTeamID = 1;
            } else {
              if (LoRaDataVerification < 300) {
                // Confirmed this is team yellow were receiving data about
                int IncomingPlayerID = LoRaDataVerification - 200;
                int IncomingTeamID = 2;
              } else {
                  // Confirmed this is team green were receiving data about
                  int IncomingPlayerID = LoRaDataVerification - 300;
                  int IncomingTeamID = 3;
                }
            }
          }
        }
        digitalWrite(LED_BUILTIN, HIGH); // iluminte the onboard led
        receivercounter();
        //TRANSMIT = true; // setting the transmission trigger to enable transmission
      }
    }
}
void transmissioncounter() {
  sendcounter++;
  Serial.println("Added 1 to sendcounter and sent to blynk app");
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
  Serial1.print("AT+SEND=0,5,"+LoRaDataToSend+"\r\n"); // used to send a confirmation to sender
  //  Serial1.print("AT+SEND=0,1,5\r\n"); // used to send data to the specified recipeint
  //  Serial1.print("$PLAY,VA20,4,6,,,,,*");
  //  Serial.println("Sent the '5' to device 0");
  transmissioncounter();
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
  resetRGB();
  if (TeamID == 0) {
    RGBRED = true;
  }
  if (TeamID == 1) {
    RGBBLUE = true;
  }
  if (TeamID == 2) {
    RGBYELLOW = true;
  }
  if (TeamID == 3) {
    RGBGREEN = true;
  }
}
void changergbcolor() {
  int statuschange = 99;
  if(RGBGREEN) {RGBGREEN = false; RGBWHITE = true; statuschange = 0;}
  if(RGBRED) {RGBRED = false; RGBGREEN = true;}
  if(RGBYELLOW) {RGBYELLOW = false; RGBRED = true;}
  if(RGBBLUE) {RGBBLUE = false; RGBYELLOW = true;}
  if(RGBPURPLE) {RGBPURPLE = false; RGBBLUE = true;}
  if(RGBCYAN) {RGBCYAN = false; RGBPURPLE = true;}
  if(RGBWHITE && statuschange == 99) {RGBWHITE = false; RGBCYAN = true;}
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
void rgbblink() {
  if (RGBState == LOW) {
    RGBState = HIGH;
    if(RGBGREEN) {
      rgbgreen();
      Serial.println("Blinking RGB");
      }
    if(RGBRED) {
      rgbred();
      Serial.println("Blinking RGB");
      }
    if(RGBYELLOW) {
      rgbyellow();
      Serial.println("Blinking RGB");
      }
    if(RGBBLUE) {
      rgbblue();
      Serial.println("Blinking RGB");
      }
    if(RGBPURPLE) {
      rgbpurple();
      Serial.println("Blinking RGB");
      }
    if(RGBCYAN) {
      rgbcyan();
      Serial.println("Blinking RGB");
      }
    if(RGBWHITE) {
      rgbwhite();
      //Serial.println("Blinking RGB");
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
void IDplayer() {
      // determining indivudual protocol values for player ID bits
      // Also assign IR values for sending player ID with device originated tags
      int brpid = 0;
      if (TT[0] > 750) {
        brpid = brpid + 1;
      }
      if (PP[5] > 750) {
        PID[0] = 1;
        brpid = brpid + 2;
      } else {
        PID[0] = 0;
      }
      if (PP[4] > 750) {
        PID[1]=2;
        brpid = brpid + 4;
      } else {
        PID[1]=0;
      }
      if (PP[3] > 750) {
        PID[2]=4;
        brpid = brpid + 8;
        } else {
        PID[2]=0;
      }
      if (PP[2] > 750) {
        PID[3]=8;
        brpid = brpid + 16;
      } else {
        PID[3]=0;
      }
      if (PP[1] > 750) {
        PID[4]=16;
        brpid = brpid + 32;
      } else {
        PID[4]=0;
      }
      if (PP[0] > 750) {
        PID[5]=32;
        brpid = brpid + 64;
      } else {
        PID[5]=0;
      }
      // ID Player by summing assigned values above based upon protocol values (1-64)
      PlayerID=PID[0]+PID[1]+PID[2]+PID[3]+PID[4]+PID[5];
      if (GearMod = 2) {
        PlayerID = brpid;
      }
      Serial.print("Player ID = ");
      Serial.println(PlayerID);      
}
void BasicDomination() {
  RGBDEMOMODE = false;
  AlignRGBWithTeam();
  BUZZ = true;
  if (!DOMINATIONCLOCK) {
    DOMINATIONCLOCK = true;
  }
}
void teamID() {
      // check if the IR is from Red team
      if (TT[0] < 750 && TT[1] < 750) {
      // sets the current team as red
      TeamID = 0;
      Serial.print("team = Red = ");
      Serial.println(TeamID);
      }
      // check if the IR is from blue team 
      if (TT[0] < 750 && TT[1] > 750) {
      // sets the current team as blue
      TeamID = 1;
      Serial.print("team = Blue = ");
      Serial.println(TeamID);
      }
      // check if the IR is from green team 
      if (TT[0] > 750 && TT[1] > 750) {
      // sets the current team as green
      TeamID = 3;
      Serial.print("team = Green = ");
      Serial.println(TeamID);
      }
      if (TT[0] > 750 && TT[1] < 750) {
      // sets the current team as red
      TeamID = 2;
      Serial.print("team = Yellow = ");
      Serial.println(TeamID);
      }
      if (GearMod == 2) {
        TeamID = 0;
        if (DD[1] > 750) {
          TeamID = TeamID + 1;
        }
        if (DD[0] > 750) {
          TeamID = TeamID + 2;
        }
        if (TT[1] > 750) {
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
      PP[0] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P1
      PP[1] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P2
      PP[2] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P3
      PP[3] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P4
      PP[4] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P5
      PP[5] = (pulseIn(IR_Sensor_Pin, LOW, 5000)); // P6
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
        if (CAPTURABLEEMITTER) {
          ChangeBaseAlignment();
        }
        if (CAPTURETHEFLAGMODE) {
          if (TeamID != Team) {
            // this is not a friendly tag, flag has been captured
            //sends the flag to player who shot base
            Serial.println("Flag captured");
              datapacket2 = 31599;
              datapacket1 = PlayerID;
              BROADCASTESPNOW = true;
          }
          else {
            if (ShotType == 4) {
              // this is a flag being hung!
              CAPTURETHEFLAGMODE = false;
              // ends the game for everyone
              datapacket2 = 1410 + TeamID;
              datapacket1 = 99;
              BROADCASTESPNOW = true;
              Serial.println("A team has reached the goal, ending game");
            }
          }
        }
        if (BASICDOMINATION) {
          BasicDomination();
        }
        if (TAGACTIVATEDIRCOOLDOWN) {
          if (!TAGACTIVATEDIR) {
            // this means that the base is on cool down, should send alarm or damage or something
          }
        }
        if (TAGACTIVATEDIR) {
          if (TAGACTIVATEDIRCOOLDOWN) {
            TAGACTIVATEDIR = false;
            if (ANYTEAM) {
              Team = TeamID;
            }
            CoolDownStart = millis();
            resetRGB();
            RGBRED = true;
          }
          VerifyCurrentIRTagSelection();
          BUZZ = true;
        }
      } else {
        Serial.println("Protocol not Recognized");
      }
    } else {
      //Serial.println("Incorrect Sync Bits from Protocol, not BRX");
    }
}

//******************************************************************************************************************************************************************************************
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
//******************************************************************************************************************************************************************************************
// This procedure uses the preset IR_Sensor_Pin to determine if an ir received is BRX, if so it records the protocol received
void ReceiveLTTO() {
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
            LTTOTagType = 2; // it is a zone tag.
          }
        }
        if (LTTOTagType < 2) {
          PrintLTTOTag(); // runs a proceedure, see proceedure for details
          IDLTTOTeam(); // check team
          IDLTTODamage(); // check the damageif (CAPTURABLEEMITTER) {
          ChangeBaseAlignment();
          if (BASICDOMINATION) {
            BasicDomination();
          }
          if (TAGACTIVATEDIR) {
            if (TAGACTIVATEDIRCOOLDOWN) {
              TAGACTIVATEDIR = false;
              if (ANYTEAM) {
                Team = TeamID;
              }
              CoolDownStart = millis();
              resetRGB();
              RGBRED = true;
            }
            VerifyCurrentIRTagSelection();
            BUZZ = true;
          }
          if (TAGACTIVATEDIRCOOLDOWN) {
            if (!TAGACTIVATEDIR) {
              // this means that the base is on cool down, should send alarm or damage or something
            }
          }
        }
      }
    }
  }
}
//******************************************************************************************************************************************************************************************

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
void SendLTTOIR() {
  Serial.println("Sending IR signal");
  pulseIR(3000); // sync
  delayMicroseconds(6000); // delay
  pulseIR(SyncLTTO); // sync
  delayMicroseconds(2000); // delay
  pulseIR(LTTOA[0]); // 
  delayMicroseconds(2000); // delay
  pulseIR(LTTOA[1]); // 
  delayMicroseconds(2000); // delay
  pulseIR(LTTOA[2]); //
  delayMicroseconds(2000); // delay
  pulseIR(LTTOA[3]); // 
  delayMicroseconds(2000); // delay
  pulseIR(LTTOA[4]); // 
  delayMicroseconds(2000); // delay
  pulseIR(LTTOA[5]); // 
  delayMicroseconds(2000); // delay
  pulseIR(LTTOA[6]); // 
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
void CustomLTTOTag() {
  if (LTTORESPAWN) {
    SyncLTTO = 6000;
    LTTOA[0] = 1000;
    LTTOA[1] = 1000;
    LTTOA[2] = 1000;
    LTTOA[3] = 2000;
    LTTOA[4] = 2000;
    LTTOA[5] = 0;
    LTTOA[6] = 0;
    SendLTTOIR();
    Serial.println("Sent LTTO Tag - Respawn");
  }
  if (LTTOOTZ) {
    SyncLTTO = 6000;
    LTTOA[0] = 1000;
    LTTOA[1] = 1000;
    LTTOA[2] = 1000;
    LTTOA[3] = 2000;
    LTTOA[4] = 1000;
    LTTOA[5] = 0000;
    LTTOA[6] = 0000;
    SendLTTOIR();
    Serial.println("Sent LTTO Tag - OTZ");
  }
  if (LTTOGG) {
    SyncLTTO = 3000;
    LTTOA[0] = 1000;
    LTTOA[1] = 1000;
    LTTOA[2] = 1000;
    if (Team == 0) {
      LTTOA[3] = 1000;
      LTTOA[4] = 1000;
    }
    if (Team == 1) {
      LTTOA[3] = 1000;
      LTTOA[4] = 2000;
    }
    if (Team == 2) {
      LTTOA[3] = 2000;
      LTTOA[4] = 1000;
    }
    if (Team == 3) {
      LTTOA[3] = 2000;
      LTTOA[4] = 2000;
    }
    if (LTTODamage == 1) {
      LTTOA[5] = 1000;
      LTTOA[6] = 1000;
    }
    if (LTTODamage == 2) {
      LTTOA[5] = 1000;
      LTTOA[6] = 2000;
    }
    if (LTTODamage == 3) {
      LTTOA[5] = 2000;
      LTTOA[6] = 1000;
    }
    if (LTTODamage == 4) {
      LTTOA[5] = 2000;
      LTTOA[6] = 2000;
    }
    SendLTTOIR();
    Serial.println("Sent LTTO Tag - G&G");
  }
  if (LTTOHOSTED) {
    SyncLTTO = 3000;
    LTTOA[0] = 1000;
    LTTOA[1] = 1000;
    LTTOA[2] = 2000;
    if (Team == 0) {
      LTTOA[3] = 1000;
      LTTOA[4] = 1000;
    }
    if (Team == 1) {
      LTTOA[3] = 1000;
      LTTOA[4] = 2000;
    }
    if (Team == 2) {
      LTTOA[3] = 2000;
      LTTOA[4] = 1000;
    }
    if (Team == 3) {
      LTTOA[3] = 2000;
      LTTOA[4] = 2000;
    }
    SendLTTOIR();
    Serial.println("Sent LTTO Tag - Hostile");
  }
}
void Sheilds() { // not set properly yet
  BulletType = 15;
  Player = 63;
  if (Player == PlayerID) {
    Player = random(63);
  }
  Damage = 55;
  Critical = 0;
  Power = 0;
  SetIRProtocol();
}
void ArmorBoost() { // not set properly yet
  BulletType = 15;
  Player = 63;
  if (Player == PlayerID) {
    Player = random(63);
  }
  Damage = 55;
  Critical = 0;
  Power = 0;
  SetIRProtocol();
}
void MedKit() { 
  BulletType = 1;
  Player = 63;
  if (Player == PlayerID) {
    Player = random(63);
  }
  Damage = 40;
  Critical = 0;
  Power = 0;
  SetIRProtocol();
}
void RespawnStation() { // not set properly yet
  BulletType = 15;
  Player = 63;
  Damage = 6;
  Critical = 1;
  Power = 0;
  SetIRProtocol();
}
void WRespawnStation() { // not set properly yet
  // enable espnow based respawn function and IR based Respawn Function
  // Firts IR:
  BulletType = 15;
  Player = 63;
  Damage = 6;
  Critical = 1;
  Power = 0;
  SetIRProtocol();
  // Now ESPNOW:
  datapacket1 = 99;
  datapacket2 = 31110 + Team;
  BROADCASTESPNOW = true;
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
void MediumDamage() { // not set properly yet
  BulletType = 0;
  Player = 63;
  if (Player == PlayerID) {
    Player = random(63);
  }
  Damage = 50;
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
  Damage = 10;
  Critical = 0;
  Power = 0;
  SetIRProtocol();
}
void GasGrenade() { // not set properly yet
  BulletType = 11;
  Player = 63;
  if (Player == PlayerID) {
    Player = random(63);
  }
  Damage = 1;
  Critical = 0;
  Power = 0;
  SetIRProtocol();
}
void CaptureTheFlag() { // not set properly yet
  BulletType = 15;
  Player = 63;
  Damage = 57;
  Critical = 0;
  Power = 0;
  SetIRProtocol();
}
void SwapBRX() {
  BulletType = 1;
  Player = 63;
  if (Player == PlayerID) {
    Player = random(63);
  }
  Damage = 1;
  Critical = 0;
  Power = 1;
  SetIRProtocol();
  datapacket1 = PlayerID;
  datapacket2 = 31000;
  datapacket3 = 99;
  BROADCASTESPNOW = true;
}
void MotionSensor() {
  BulletType = 15;
  Player = 63;
  if (Player == PlayerID) {
    Player = random(63);
  }
  Damage = 10;
  Critical = 0;
  Power = 0;
  SetIRProtocol();
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
void KingOfHill() {
  BulletType = 15;
  Player = 63;
  if (Player == PlayerID) {
    Player = random(63);
  }
  Damage = 55;
  Critical = 1;
  Power = 0;
  SetIRProtocol();
}
void Frag() {
  BulletType = 10;
  Player = 63;
  if (Player == PlayerID) {
    Player = random(63);
  }
  Damage = 200;
  Critical = 0;
  Power = 0;
  SetIRProtocol();
}
void OwnTheZone() {
  BulletType = 3;
  Player = 63;
  if (Player == PlayerID) {
    Player = random(63);
  }
  Team = TeamID;
  Damage = 10;
  Critical = 0;
  Power = 0;
  SetIRProtocol();
  datapacket1 = 99;
  datapacket2 = 31100;
  datapacket3 = Team;
  BROADCASTESPNOW = true;
}
void HitTag() {
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


void VerifyCurrentIRTagSelection() {
  if (CAPTURETHEFLAG) {
    CaptureTheFlag(); // sets ir protocol to motion sensor yellow and sends it
  }
  if (GASGRENADE) {
    GasGrenade(); // sets ir protocol to motion sensor yellow and sends it
  }
  if (LIGHTDAMAGE) {
    LightDamage(); // sets ir protocol to motion sensor yellow and sends it
  }
  if (MEDIUMDAMAGE) {
    MediumDamage(); // sets ir protocol to motion sensor yellow and sends it
  }
  if (HEAVYDAMAGE) {
    HeavyDamage(); // sets ir protocol to motion sensor yellow and sends it
  }
  if (FRAG) {
    Frag(); // sets ir protocol to motion sensor yellow and sends it
  }
  if (RESPAWNSTATION) {
    RespawnStation(); // sets ir protocol to motion sensor yellow and sends it
  }
  if (WRESPAWNSTATION) {
    WRespawnStation(); // sets ir protocol to motion sensor yellow and sends it
  }
  if (MEDKIT) {
    MedKit(); // sets ir protocol to motion sensor yellow and sends it
  }
  if (LOOTBOX) {
    SwapBRX(); // sets ir protocol to motion sensor yellow and sends it
  }
  if (ARMORBOOST) {
    ArmorBoost(); // sets ir protocol to motion sensor yellow and sends it
  }
  if (SHEILDS) {
    Sheilds(); // sets ir protocol to motion sensor yellow and sends it
  }
  if (CONTROLPOINTLOST) {
    ControlPointLost();
  }
  if (CONTROLPOINTCAPTURED) {
    ControlPointCaptured();
  }
  if (CUSTOMLTTOTAG) {
    CustomLTTOTag();
  }
  if (MOTIONSENSOR) {
    MotionSensor();
  }
  if (OWNTHEZONE) {
    OwnTheZone();
  }
  BUZZ = true;
}
void ResetAllIRProtocols() {
  MOTIONSENSOR = false;
  CONTROLPOINTLOST = false;
  CONTROLPOINTCAPTURED = false;
  SWAPBRX = false;
  OWNTHEZONE = false;
  HITTAG = false;
  CAPTURETHEFLAG = false;
  GASGRENADE = false;
  LIGHTDAMAGE = false;
  MEDIUMDAMAGE = false;
  HEAVYDAMAGE = false;
  FRAG = false;
  RESPAWNSTATION = false;
  MEDKIT = false;
  LOOTBOX = false;
  ARMORBOOST = false;
  SHEILDS = false;
  CUSTOMLTTOTAG = false;
  WRESPAWNSTATION = false;
}

//*************************************************************************
//************************** PIEZO BUZZER OBJECTS *************************
//*************************************************************************
void buzz() {
  digitalWrite(buzzer, HIGH); // sounds buzzer for 1 millisecond
  delayMicroseconds(50);
  digitalWrite(buzzer, LOW); // turns off buzzer
  if (buzzerduration > 0) { //checks if buzzer timer is up
    buzzerduration--; 
  } else {
    buzzerduration = fixedbuzzerduration; // setts buzzer duration back to default
    BUZZ = false; // disables the buzzer
    Serial.println("Buzzer Deactivated");
  }
}

//*************************************************************************
//**************************** GAME PLAY OBJECTS **************************
//*************************************************************************


void AddPointToTeamWithPossession() {
  TeamScore[TeamID]++;
  if (FIVEMINUTETUGOWAR) {
    if (TeamID != 0 && TeamScore[0] > 0) {
      TeamScore[0]--;
    }
    if (TeamID != 1 && TeamScore[1] > 0) {
      TeamScore[1]--;
    }
    if (TeamID != 2 && TeamScore[2] > 0) {
      TeamScore[2]--;
    }
    if (TeamID != 3 && TeamScore[3] > 0) {
      TeamScore[3]--;
    }
  }
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
    datapacket2 = 1420 + TeamID;
    datapacket1 = 99;
    BROADCASTESPNOW = true;
    Serial.println("A team has reached the goal, ending game");
  }
}
  
void AddPointToPlayerWithPossession() {
  PlayerScore[PlayerID]++;
}
void ResetScores() {
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
  Serial.println("All scores reset");
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
    vTaskDelay(0);
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
    vTaskDelay(0);
  }
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
      return 0;} 
    else 
    {
      EEPROM.write(0, 1); // set up for upgrade confirmation
      EEPROM.commit(); // store data
      Serial.println(payload);
      Serial.println("New firmware detected");
      
      return 1;
    }
  } 
  return 0;  
}
//******************************************************************************************
// ***********************************  GAME LOOP  ****************************************
// *****************************************************************************************
void loop1(void *pvParameters) {
  Serial.print("GAME loop running on core ");
  Serial.println(xPortGetCoreID());
  while(1) { // starts the forever loop
    // put all the serial activities in here for communication with the brx
    unsigned long currentMillis0 = millis(); // sets the timer for timed operations
    //**************************************************************************************
    // RGB main functions: 
    if (currentMillis0 - rgbpreviousMillis >= rgbinterval) {
      rgbpreviousMillis = currentMillis0;
      rgbblink();
      if (RGBDEMOMODE) {
        changergbcolor();
      }
    }
    //**************************************************************************************
    // IR LED main functions: 
    if (SINGLEIRPULSE) {
      SINGLEIRPULSE = false;
      VerifyCurrentIRTagSelection();  // runs object for identifying set ir protocol and send it
      ResetAllIRProtocols();
      if (CAPTURABLEEMITTER) {
        RESPAWNSTATION = true;
      }
      Serial.println("Executed Single IR Pulse Object");
    }
    if (BASECONTINUOUSIRPULSE) {
      if (currentMillis0 - irledpreviousMillis >= irledinterval) {
        irledpreviousMillis = currentMillis0;
        VerifyCurrentIRTagSelection(); // runs object for identifying set ir protocol and send it
        Serial.println("Executed Base Continuous IR Pulse Object");
      }
    }
    // else {Serial.println("base continuous IR disabled");}
    //**************************************************************************************
    // IR Receiver main functions:
    if (ENABLEIRRECEIVER || TAGACTIVATEDIR) { // default has ir receiver enabled for auto game start
      if (GearMod == 0) {
        receiveBRXir(); // runs the ir receiver, looking for brx ir protocols
      }
      if (GearMod == 1) {
        ReceiveLTTO(); // runs the ir receiver, looking for LTTO ir protocols
      }
      if (GearMod == 2) {
        receiveBRXir(); // runs the ir receiver, looking for BRP protocol
      }
      if (TAGACTIVATEDIRCOOLDOWN) {
        if (!TAGACTIVATEDIR) {
          // Base is in cool down
          if (currentMillis0 - CoolDownStart > CoolDownInterval) {
            // Cool down is over
            TAGACTIVATEDIR = true;
            resetRGB;
            RGBWHITE = true;
            Serial.println("Cool Down Is Over");
          }
        }
      }
    }
    //**********************************************************************
    // Piezo Buzzer main functions:
    if (BUZZ) {
      buzz();
    }
    //**********************************************************************
    // Game Play functions:
    if (BASICDOMINATION) {
      //Serial.println("basic domination toggle confirmed active");
      if (DOMINATIONCLOCK) {
        //Serial.println("dominationclock toggle confirmed active");
        if (currentMillis0 - 1000 > PreviousDominationClock) {
          Serial.println("1 second lapsed under counter object");
          PreviousDominationClock = currentMillis0;
          Serial.println("Run points accumulator for teams");
          AddPointToTeamWithPossession();
          Serial.println("Run points accumulator for Players");
          AddPointToPlayerWithPossession();
          Serial.println("Enable Score Posting");
          if (FIVEMINUTEDOMINATION) {
            if (TeamScore[0] > 300 || TeamScore[1] > 300 || TeamScore[2] > 300 || TeamScore[3] > 300) {
              // one team has 5 minutes
              DOMINATIONCLOCK = false;
              BASICDOMINATION = false;
              // ends the game for everyone
              datapacket2 = 1410 + TeamID;
              datapacket1 = 99;
              BROADCASTESPNOW = true;
              Serial.println("A team has reached the goal, ending game");
            }
            if (TeamScore[0] == 270 || TeamScore[1] == 270 || TeamScore[2] == 270 || TeamScore[3] == 270) {
              // one team has 4.5 minutes
              //DOMINATIONCLOCK = false;
              //BASICDOMINATION = false;
              // ends the game for everyone
              datapacket2 = 1430 + TeamID;
              datapacket1 = 99;
              BROADCASTESPNOW = true;
              Serial.println("A team has reached the goal, ending game");
            }
          }
          if (TENMINUTEDOMINATION) {
            if (TeamScore[0] > 600 || TeamScore[1] > 600 || TeamScore[2] > 600 || TeamScore[3] > 600) {
              // one team has 10 minutes
              DOMINATIONCLOCK = false;
              BASICDOMINATION = false;
              // ends the game for everyone
              datapacket2 = 1410 + TeamID;
              datapacket1 = 99;
              BROADCASTESPNOW = true;
              Serial.println("A team has reached the goal, ending game");
            }
            if (TeamScore[0] == 570 || TeamScore[1] == 570 || TeamScore[2] == 570 || TeamScore[3] == 570) {
              // one team has 9.5 minutes
              //DOMINATIONCLOCK = false;
              //BASICDOMINATION = false;
              // ends the game for everyone
              datapacket2 = 1430 + TeamID;
              datapacket1 = 99;
              BROADCASTESPNOW = true;
              Serial.println("A team has reached the goal, ending game");
            }
          }
        }
      }
    }
    delay(1); // this has to be here or it will just continue to restart the esp32
  }
}
// *****************************************************************************************
// **************************************  Comms LOOP  *************************************
// *****************************************************************************************
void loop2(void *pvParameters) {
  Serial.print("cOMMS loop running on core ");
  Serial.println(xPortGetCoreID());
  // run EEPROM verifications
  incomingData1 = JBOXID; // set incoming data to jbox id for processing of commands 
  int savedsettings = 0;
  // read rest of values for settings 1-11 and apply to device
  savedsettings = EEPROM.read(1);
  if (savedsettings > 0) { // checking if the flash has a saved data set
    // primary function - 9 - 10000
    incomingData2 = EEPROM.read(1);
    incomingData2 = incomingData2 + 10000;
    ProcessIncomingCommands();
    delay(10);
  }
  savedsettings = EEPROM.read(2);
  if (savedsettings > 0) { // checking if the flash has a saved data set
    // ir emitter type - 17 - 10100
    incomingData2 = EEPROM.read(2);
    incomingData2 = incomingData2 + 10100;
    ProcessIncomingCommands();
    delay(10);
  }
  savedsettings = EEPROM.read(3);
  if (savedsettings > 0) { // checking if the flash has a saved data set
    // team alignment - 5 - 10200
    incomingData2 = EEPROM.read(3);
    incomingData2 = incomingData2 + 10200;
    ProcessIncomingCommands();
    delay(10);
  }
  savedsettings = EEPROM.read(4);
  if (savedsettings > 0) { // checking if the flash has a saved data set
    // ir emitter frequency - 8 - 10300
    incomingData2 = EEPROM.read(4);
    incomingData2 = incomingData2 + 10300;
    ProcessIncomingCommands();
    delay(10);
  }
  savedsettings = EEPROM.read(5);
  if (savedsettings > 0) { // checking if the flash has a saved data set
    // ir cool down time - 13 - 10400
    incomingData2 = EEPROM.read(5);
    incomingData2 = incomingData2 + 10400;
    ProcessIncomingCommands();
    delay(10);
  }
  savedsettings = EEPROM.read(6);
  if (savedsettings > 0) { // checking if the flash has a saved data set
    // base tag count for capture - 10 - 10500
    incomingData2 = EEPROM.read(6);
    incomingData2 = incomingData2 + 10500;
    ProcessIncomingCommands();
    delay(10);
  }
  savedsettings = EEPROM.read(7);
  if (savedsettings > 0) { // checking if the flash has a saved data set
    // gear selection - 3 - 10600
    incomingData2 = EEPROM.read(7);
    incomingData2 = incomingData2 + 10600;
    ProcessIncomingCommands();
    delay(10);
  }
  savedsettings = EEPROM.read(8);
  if (savedsettings >= 0) { // checking if the flash has a saved data set
    // ltto tag type - 4 - 10700
    incomingData2 = EEPROM.read(8);
    incomingData2 = incomingData2 + 10700;
    ProcessIncomingCommands();
    delay(10);
  }
  savedsettings = EEPROM.read(9);
  if (savedsettings > 0) { // checking if the flash has a saved data set
    // ltto damage - 4 - 10150
    incomingData2 = EEPROM.read(9);
    incomingData2 = incomingData2 + 10150;
    ProcessIncomingCommands();
    delay(10);
  }
  savedsettings = EEPROM.read(10);
  if (savedsettings > 0) { // checking if the flash has a saved data set
    // jbox id from 100 to 120
    JBOXID = EEPROM.read(10);
    Serial.println("JBOXID = " + String(JBOXID));
    delay(10);
  }
  while (1) { // starts the forever loopws.cleanupClients();
    unsigned long currentMillis1 = millis(); // sets the timer for timed operations
    ws.cleanupClients();
    if (BROADCASTESPNOW) {
      BROADCASTESPNOW = false;
      getReadings();
      BroadcastData(); // sending data via ESPNOW
      Serial.println("Sent Data Via ESPNOW");
      ResetReadings();
    }
    if (BASICDOMINATION) {
      if (currentMillis1 - PreviousMillis > 5000) {
        PreviousMillis = currentMillis1;
        UpdateWebApp0();
        //Serial.println("updated scores on web app");
        if (MULTIBASEDOMINATION) {
          datapacket2 = 903;
          datapacket1 = 99;
          String ScoreData = String(TeamScore[0])+","+String(TeamScore[1])+","+String(TeamScore[2])+","+String(TeamScore[3]);
          Serial.println("Sending the following Score Data to Server");
          Serial.println(ScoreData);
          BROADCASTESPNOW = true;
        }
      }
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
    delay(1); // this has to be here or the esp32 will just keep rebooting
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
  // initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  int bootstatus = EEPROM.read(0);
  Serial.print("boot status = ");
  Serial.println(bootstatus);
  if (bootstatus > 0 && bootstatus < 255) {
    Serial.println("Enabling OTA Update Mode");
    EEPROM.write(0, 0);
    EEPROM.commit();
    OTAMODE = true;
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
  // Connect to Wi-Fi
  Serial.println("Starting AP");
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  if (OTAMODE) {
    Serial.print("Active firmware version:");
    Serial.println(FirmwareVer);
    pinMode(LED_BUILTIN, OUTPUT);
    // Connect to Wi-Fi network with SSID and password
    Serial.print("Setting AP (Access Point)…");
    // Remove the password parameter, if you want the AP (Access Point) to be open
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
    Serial.print("ESP Board MAC Address:  ");
    Serial.println(WiFi.macAddress());
    Serial.println("Starting ESPNOW");
    //IntializeESPNOW();
    //delay(1000);
    //datapacket1 = 9999;
    //getReadings();
    //BroadcastData(); // sending data via ESPNOW
    //Serial.println("Sent Data Via ESPNOW");
    //ResetReadings();
    //while (OTApassword == "dontchangeme") {
      //vTaskDelay(1);
    //}
    //delay(2000);
    Serial.println("wifi credentials");
    Serial.println(OTAssid);
    Serial.println(OTApassword);
    connect_wifi();
  } else {
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
  Serial1.print("AT+ADDRESS=900\r\n");   //needs to be unique
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
  WiFi.softAP(ssid, password);
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
  //server1.on("/scores", HTTP_GET, [](AsyncWebServerRequest *request){
    //request->send_P(200, "text/html", index1_html, processor);
  //});
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
  // Start server
  server.begin();
  //server1.begin();
  //***********************************************************************
  // Start ESP Now
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  Serial.println("Starting ESPNOW");
  IntializeESPNOW();
  //***********************************************************************
  // initialize dual cores and dual loops
  Serial.println(xPortGetCoreID());
  xTaskCreatePinnedToCore(loop1, "loop1", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(loop2, "loop2", 4096, NULL, 1, NULL, 1);
  }
} // End of setup.

void loop() {
  if (OTAMODE) {
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
}
