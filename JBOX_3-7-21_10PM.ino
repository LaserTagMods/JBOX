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
 * 
 * 
*/

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

#define BLYNK_USE_DIRECT_CONNECT

//*************************************************************************
//********************* LIBRARY INCLUSIONS - ALL **************************
//*************************************************************************
#include <BlynkSimpleEsp32_BLE.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <HardwareSerial.h> // used for setting up the serial communications on non RX/TX pins
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h> // needed for resetting the mac address

//*************************************************************************
//******************** DEVICE SPECIFIC DEFINITIONS ************************
//*************************************************************************
// These should be modified as applicable for your needs



//*************************************************************************
//********************* GLOBAL DEFINITIONS - ALL **************************
//*************************************************************************
// serial definitions for LoRa
#define SERIAL1_RXPIN 19 // TO LORA TX
#define SERIAL1_TXPIN 23 // TO LORA RX

//*************************************************************************
//********************** BLYNK VARIABLE DELCARATIONS *********************
//*************************************************************************
// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "xiocSUz0buA2fnC4Tp4uReneMb2yg5vt";

// Trigger Variables
bool RUNBLYNK = true;
bool SETUPBLYNK = true;

// Attach virtual serial terminal to Virtual Pin V1
WidgetTerminal terminal(V1);

//*************************************************************************
//********************** ESPNOW VARIABLE DELCARATIONS *********************
//*************************************************************************
// Global copy of slave
// Trigger Variables
bool RUNESPNOW = true;
bool STARTESPNOW = true;
bool BROADCASTESPNOW = false;
bool ESPNOWDATASEND = false;

// for resetting mac address to custom address:
uint8_t newMACAddress[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x09}; 

// REPLACE WITH THE MAC Address of your receiver, this is the address we will be sending to
uint8_t broadcastAddress[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x09};

// Register peer
esp_now_peer_info_t peerInfo;

// Define variables to store and to be sent
String datapacket0;
int datapacket1;
int datapacket2;
int datapacket3;

// Define variables to store incoming readings
String incomingData0;
int incomingData1;
int incomingData2;
int incomingData3;

// Variable to store if sending data was successful
String success;

//Structure example to send data
//Must match the receiver structure
typedef struct struct_message {
  String function;
  int DP1;
  int DP2;
  int DP3;
} struct_message;

// Create a struct_message called DataToBroadcast to hold sensor readings
struct_message DataToBroadcast;

// Create a struct_message to hold incoming sensor readings
struct_message incomingReadings;

// timer settings
unsigned long CurrentMillis = 0;
int interval = 2000;
long PreviousMillis = 0;
int ESPNOWinterval = 20000;
int ESPNOWPreviousMillis = 0;


//*************************************************************************
//*********************** LORA VARIABLE DELCARATIONS **********************
//*************************************************************************
String received; // string used to record data coming in
String tokenStrings[5]; // used to break out the incoming data for checks/balances

bool LORALISTEN = false; // a trigger used to enable the radio in (listening)
bool ENABLELORATRANSMIT = false; // a trigger used to enable transmitting data

int sendcounter = 0;
int receivecounter = 0;

unsigned long lorapreviousMillis = 0;
const long lorainterval = 3000;

//*************************************************************************
//**************** IR RECEIVER VARIABLE DELCARATIONS **********************
//*************************************************************************
// Define Variables used for the game functions
int TeamID=0; // this is for team recognition, team 1 = red, team 2 = blue, team 3 = green
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

//*************************************************************************
//**************** IR LED VARIABLE DELCARATIONS ***************************
//*************************************************************************
int IRledPin =  26;    // LED connected to digital pin 13
int B[4];
int P[6];
int T[2];
int D[8];
int C[1];
int X[2];
int Z[2];

int BulletType = 0;
int Player = 0;
int Team = 0;
int Damage = 0;
int Critical = 0;
int Power = 0;

unsigned long irledpreviousMillis = 0;
const long irledinterval = 5000;
bool BASECONTINUOUSIRPULSE = false; // enables/disables continuous pulsing of ir
bool TAGACTIVATEDIR = true; // enables/disables tag activated onetime IR
bool CONTROLPOINTCAPTURED = false;
bool MOTIONSENSOR = false;
bool SWAPBRX = true;
bool OWNTHEZONE = false;
bool HITTAG = false;


//*************************************************************************
//**************** CORE VARIABLE DELCARATIONS *****************************
//*************************************************************************
unsigned long core0previousMillis = 0;
const long core0interval = 1000;

unsigned long core1previousMillis = 0;
const long core1interval = 3000;

//*************************************************************************
//**************** RGB VARIABLE DELCARATIONS ******************************
//*************************************************************************
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
bool RGBWHITE = false;

unsigned long rgbpreviousMillis = 0;
const long rgbinterval = 1000;

unsigned long changergbpreviousMillis = 0;
const long changergbinterval = 2000;
bool RGBDEMOMODE = false; // enables rgb demo flashing and changing of colors

//*************************************************************************
//**************** PIEZO VARIABLE DELCARATIONS ****************************
//*************************************************************************
const int buzzer = 5; // buzzer pin
bool BUZZ = false; // set to true to send buzzer sound
bool SOUNDBUZZER = false; // set to true to sound buzzer
int buzzerduration = 250; // used for setting cycle length of buzz
const int fixedbuzzerduration = buzzerduration; // assigns a constant buzzer duration

unsigned long buzzerpreviousMillis = 0;
const long buzzerinterval = 5000;

//*************************************************************************
//************************** ESPNOW OBJECTS********************************
//*************************************************************************

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
  Serial.println();
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  Serial.println();
  Serial.println("*************** LORA DATA RECEIVED! *************");
  Serial.print("Bytes received: ");
  Serial.println(len);
  incomingData0 = incomingReadings.function;
  incomingData1 = incomingReadings.DP1;
  incomingData2 = incomingReadings.DP2;
  incomingData3 = incomingReadings.DP3;
  Serial.println("function: " + incomingData0);
  Serial.println("DP1: " + String(incomingData1));
  Serial.println("DP2: " + String(incomingData2));
  Serial.println("DP3: " + String(incomingData3));
  digitalWrite(2, HIGH);
  
}

// object to generate random numbers to send
void getReadings(){
  datapacket0 = "random";
  datapacket1 = random(3200);
  datapacket2 = random(3200);
  datapacket3 = random(3200);

  // Set values to send
  DataToBroadcast.function = datapacket0;
  DataToBroadcast.DP1 = datapacket1;
  DataToBroadcast.DP2 = datapacket2;
  DataToBroadcast.DP3 = datapacket3;
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

void initializeespnow() {
  // Set up the onboard LED
  pinMode(2, OUTPUT);
  
  // run the object for changing the esp default mac address
  ChangeMACaddress();
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  Serial.println("station mode set");
  
  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  } else {
    Serial.println("ESP-NOW initialized");
  }
  
  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent); 
  Serial.println("OnDataSent Registered"); 
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;   // this is the channel being used
  peerInfo.encrypt = false;
  Serial.println("Set Peer data");
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    Serial.println("Try Again");
    vTaskDelay(2000);
    initializeespnow();
    //return;
  } else {
    Serial.println("Peer Added Properly");
  }
  
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("OnDataRecv Registered");
}


// object to used to change esp default mac to custom mac
void ChangeMACaddress() {
  WiFi.mode(WIFI_STA);
  
  Serial.print("[OLD] ESP32 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  
  esp_wifi_set_mac(ESP_IF_WIFI_STA, &newMACAddress[0]);
  
  Serial.print("[NEW] ESP32 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
}
 
//*************************************************************************
//************************** BLYNK OBJECTS ********************************
//*************************************************************************
void initializeblynk() {
  Serial.println("Waiting for connections...");
  Blynk.setDeviceName("BASE2");
  Blynk.begin(auth);
  
  // Clear the terminal content
  terminal.clear();

  // This will print Blynk Software version to the Terminal Widget when
  // your hardware gets connected to Blynk Server
  terminal.println(F("Blynk v" BLYNK_VERSION ": Device started"));
  terminal.println(F("-------------"));
  terminal.println(F("Type 'Marco' and get a reply, or type"));
  terminal.println(F("anything else and get it printed back."));
  terminal.flush();
}
BLYNK_WRITE(V0) {// Test Blynk
int b=param.asInt();
  if (b==1) {
    Serial.println("Blynk Is Working, Value Received!!!");
  }
}
// You can send commands from Terminal to your hardware. Just use
// the same Virtual Pin as your Terminal Widget
BLYNK_WRITE(V1)
{

  // if you type "Marco" into Terminal Widget - it will respond: "Polo:"
  if (String("Marco") == param.asStr()) {
    terminal.println("You said: 'Marco'") ;
    terminal.println("I said: 'Polo'") ;
  } else {

    // Send it back
    terminal.print("You said:");
    terminal.write(param.getBuffer(), param.getLength());
    terminal.println();
  }

  // Ensure everything is sent
  terminal.flush();
}

//*************************************************************************
//*************************** LORA OBJECTS ********************************
//*************************************************************************
void ReceiveTransmission() {
  // this is an object used to listen to the serial inputs from the LoRa Module
  if(Serial1.available()>0){ // checking to see if there is anything incoming from the LoRa module
    Serial.println("Got Data"); // prints a notification that data was received
    received = Serial1.readString(); // stores the incoming data to the pre set string
    Serial.print("Received: "); // printing data received
    Serial.println(received); // printing data received
    if(received.startsWith("+RCV")){ // checking if the data was from another LoRa module
      // convert the received data into a string array for comparing data
      char *ptr = strtok((char*)received.c_str(), ",");
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
        
        // check the data for a data match
        if(tokenStrings[2] == "1"){  //in this case our single received byte would always be at the 11th position
          digitalWrite(LED_BUILTIN, HIGH); // iluminte the onboard led
          receivercounter();
          //TRANSMIT = true; // setting the transmission trigger to enable transmission
        }
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
  // LETS RESPOND:
  delay(5000);
  //String Sender = (String(received[5])+String(received[6])+String(received[7])); // if needed to send a response to the sender, this function builds the needed string
  Serial.println("Transmitting Confirmation"); // printing to serial monitor
  //Serial1.print("AT+SEND="+Sender+",5,GotIt\r\n"); // used to send a confirmation to sender
  Serial1.print("AT+SEND=0,1,5\r\n"); // used to send data to the specified recipeint
  //Serial1.print("$PLAY,VA20,4,6,,,,,*");
  Serial.println("Sent the '5' to device 0");
  transmissioncounter();
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
      if (PP[5] > 750) {
        PID[0] = 1;
      } else {
        PID[0] = 0;
      }
      if (PP[4] > 750) {
        PID[1]=2;
      } else {
        PID[1]=0;
      }
      if (PP[3] > 750) {
        PID[2]=4;
        } else {
        PID[2]=0;
      }
      if (PP[2] > 750) {
        PID[3]=8;
      } else {
        PID[3]=0;
      }
      if (PP[1] > 750) {
        PID[4]=16;
      } else {
        PID[4]=0;
      }
      if (PP[0] > 750) {
        PID[5]=32;
      } else {
        PID[5]=0;
      }
      // ID Player by summing assigned values above based upon protocol values (1-64)
      PlayerID=PID[0]+PID[1]+PID[2]+PID[3]+PID[4]+PID[5];
      Serial.print("Player ID = ");
      Serial.println(PlayerID);      
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
        if (TAGACTIVATEDIR) {
          if (CONTROLPOINTCAPTURED) {
            ControlPointCaptured();
          }
          if (MOTIONSENSOR) {
            MotionSensor();
          }
          if (OWNTHEZONE) {
            OwnTheZone();
          }
          if (SWAPBRX) {
            SwapBRX();
          }
        }
      } else {
        Serial.println("Protocol not Recognized");
      }
    } else {
      //Serial.println("Incorrect Sync Bits from Protocol, not BRX");
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
    delayMicroseconds(10);         // hang out for 10 microseconds, you can also change this to 9 if its not working
    digitalWrite(IRledPin, LOW);   // this also takes about 3 microseconds
    delayMicroseconds(10);         // hang out for 10 microseconds, you can also change this to 9 if its not working
    // so 26 microseconds altogether
    microsecs -= 26;
  }
  sei();  // this turns them back on
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
void SwapBRX() {
  BulletType = 1;
  Player = 63;
  while (Player == PlayerID) {
    Player = random(0,63);
  }
  Team = 0;
  Damage = 1;
  Critical = 0;
  Power = 1;
  SetIRProtocol();
}
void MotionSensor() {
  BulletType = 15;
  Player = 63;
  while (Player == PlayerID) {
    Player = random(0,63);
  }
  Team = 2;
  Damage = 10;
  Critical = 0;
  Power = 0;
  SetIRProtocol();
}
void ControlPointCaptured() {
  BulletType = 15;
  Player = 63;
  while (Player == PlayerID) {
    Player = random(0,63);
  }
  Team = TeamID;
  Damage = 57;
  Critical = 0;
  Power = 0;
  SetIRProtocol();
}
void KingOfHill() {
  BulletType = 15;
  Player = 0;
  Team = 3;
  Damage = 55;
  Critical = 0;
  Power = 0;
}
void Explosion() {
  BulletType = 10;
  Player = 63;
  Team = 2;
  Damage = 200;
  Critical = 0;
  Power = 0;
}
void OwnTheZone() {
  BulletType = 3;
  Player = 63;
  while (Player == PlayerID) {
    Player = random(0,63);
  }
  Team = TeamID;
  Damage = 10;
  Critical = 0;
  Power = 0;
  SetIRProtocol();
}
void HitTag() {
  BulletType = 0;
  Player = 63;
  while (Player == PlayerID) {
    Player = random(0,63);
  }
  Team = 2;
  Damage = 100;
  Critical = 0;
  Power = 0;
  SetIRProtocol();
}
void SetIRProtocol() {
// Set Player ID
if (Player==0) {P[0]=500; P[1]=500; P[2]=500; P[3]=500; P[4]=500; P[5]=500;}
if (Player==1) {P[0]=500; P[1]=500; P[2]=500; P[3]=500; P[4]=500; P[5]=1050;}
if (Player==2) {P[0]=500; P[1]=500; P[2]=500; P[3]=500; P[4]=1050; P[5]=500;}
if (Player==3) {P[0]=500; P[1]=500; P[2]=500; P[3]=500; P[4]=1050; P[5]=1050;}
if (Player==4) {P[0]=500; P[1]=500; P[2]=500; P[3]=1050; P[4]=500; P[5]=500;}
if (Player==5) {P[0]=500; P[1]=500; P[2]=500; P[3]=1050; P[4]=500; P[5]=1050;}
if (Player==6) {P[0]=500; P[1]=500; P[2]=500; P[3]=1050; P[4]=1050; P[5]=500;}
if (Player==7) {P[0]=500; P[1]=500; P[2]=500; P[3]=1050; P[4]=1050; P[5]=1050;}
if (Player==8) {P[0]=500; P[1]=500; P[2]=1050; P[3]=500; P[4]=500; P[5]=500;}
if (Player==9) {P[0]=500; P[1]=500; P[2]=1050; P[3]=500; P[4]=500; P[5]=1050;}
if (Player==10) {P[0]=500; P[1]=500; P[2]=1050; P[3]=500; P[4]=1050; P[5]=500;}
if (Player==11) {P[0]=500; P[1]=500; P[2]=1050; P[3]=500; P[4]=1050; P[5]=1050;}
if (Player==12) {P[0]=500; P[1]=500; P[2]=1050; P[3]=1050; P[4]=500; P[5]=500;}
if (Player==13) {P[0]=500; P[1]=500; P[2]=1050; P[3]=1050; P[4]=500; P[5]=1050;}
if (Player==14) {P[0]=500; P[1]=500; P[2]=1050; P[3]=1050; P[4]=1050; P[5]=500;}
if (Player==15) {P[0]=500; P[1]=500; P[2]=1050; P[3]=1050; P[4]=1050; P[5]=1050;}
if (Player==16) {P[0]=500; P[1]=1050; P[2]=500; P[3]=500; P[4]=500; P[5]=500;}
if (Player==17) {P[0]=500; P[1]=1050; P[2]=500; P[3]=500; P[4]=500; P[5]=1050;}
if (Player==18) {P[0]=500; P[1]=1050; P[2]=500; P[3]=500; P[4]=1050; P[5]=500;}
if (Player==19) {P[0]=500; P[1]=1050; P[2]=500; P[3]=500; P[4]=1050; P[5]=1050;}
if (Player==20) {P[0]=500; P[1]=1050; P[2]=500; P[3]=1050; P[4]=500; P[5]=500;}
if (Player==21) {P[0]=500; P[1]=1050; P[2]=500; P[3]=1050; P[4]=500; P[5]=1050;}
if (Player==22) {P[0]=500; P[1]=1050; P[2]=500; P[3]=1050; P[4]=1050; P[5]=500;}
if (Player==23) {P[0]=500; P[1]=1050; P[2]=500; P[3]=1050; P[4]=1050; P[5]=1050;}
if (Player==24) {P[0]=500; P[1]=1050; P[2]=1050; P[3]=500; P[4]=500; P[5]=500;}
if (Player==25) {P[0]=500; P[1]=1050; P[2]=1050; P[3]=500; P[4]=500; P[5]=1050;}
if (Player==26) {P[0]=500; P[1]=1050; P[2]=1050; P[3]=500; P[4]=1050; P[5]=500;}
if (Player==27) {P[0]=500; P[1]=1050; P[2]=1050; P[3]=500; P[4]=1050; P[5]=1050;}
if (Player==28) {P[0]=500; P[1]=1050; P[2]=1050; P[3]=1050; P[4]=500; P[5]=500;}
if (Player==29) {P[0]=500; P[1]=1050; P[2]=1050; P[3]=1050; P[4]=500; P[5]=1050;}
if (Player==30) {P[0]=500; P[1]=1050; P[2]=1050; P[3]=1050; P[4]=1050; P[5]=500;}
if (Player==31) {P[0]=500; P[1]=1050; P[2]=1050; P[3]=1050; P[4]=1050; P[5]=1050;}
if (Player==32) {P[0]=1050; P[1]=500; P[2]=500; P[3]=500; P[4]=500; P[5]=500;}
if (Player==33) {P[0]=1050; P[1]=500; P[2]=500; P[3]=500; P[4]=500; P[5]=1050;}
if (Player==34) {P[0]=1050; P[1]=500; P[2]=500; P[3]=500; P[4]=1050; P[5]=500;}
if (Player==35) {P[0]=1050; P[1]=500; P[2]=500; P[3]=500; P[4]=1050; P[5]=1050;}
if (Player==36) {P[0]=1050; P[1]=500; P[2]=500; P[3]=1050; P[4]=500; P[5]=500;}
if (Player==37) {P[0]=1050; P[1]=500; P[2]=500; P[3]=1050; P[4]=500; P[5]=1050;}
if (Player==38) {P[0]=1050; P[1]=500; P[2]=500; P[3]=1050; P[4]=1050; P[5]=500;}
if (Player==39) {P[0]=1050; P[1]=500; P[2]=500; P[3]=1050; P[4]=1050; P[5]=1050;}
if (Player==40) {P[0]=1050; P[1]=500; P[2]=1050; P[3]=500; P[4]=500; P[5]=500;}
if (Player==41) {P[0]=1050; P[1]=500; P[2]=1050; P[3]=500; P[4]=500; P[5]=1050;}
if (Player==42) {P[0]=1050; P[1]=500; P[2]=1050; P[3]=500; P[4]=1050; P[5]=500;}
if (Player==43) {P[0]=1050; P[1]=500; P[2]=1050; P[3]=500; P[4]=1050; P[5]=1050;}
if (Player==44) {P[0]=1050; P[1]=500; P[2]=1050; P[3]=1050; P[4]=500; P[5]=500;}
if (Player==45) {P[0]=1050; P[1]=500; P[2]=1050; P[3]=1050; P[4]=500; P[5]=1050;}
if (Player==46) {P[0]=1050; P[1]=500; P[2]=1050; P[3]=1050; P[4]=1050; P[5]=500;}
if (Player==47) {P[0]=1050; P[1]=500; P[2]=1050; P[3]=1050; P[4]=1050; P[5]=1050;}
if (Player==48) {P[0]=1050; P[1]=1050; P[2]=500; P[3]=500; P[4]=500; P[5]=500;}
if (Player==49) {P[0]=1050; P[1]=1050; P[2]=500; P[3]=500; P[4]=500; P[5]=1050;}
if (Player==50) {P[0]=1050; P[1]=1050; P[2]=500; P[3]=500; P[4]=1050; P[5]=500;}
if (Player==51) {P[0]=1050; P[1]=1050; P[2]=500; P[3]=500; P[4]=1050; P[5]=1050;}
if (Player==52) {P[0]=1050; P[1]=1050; P[2]=500; P[3]=1050; P[4]=500; P[5]=500;}
if (Player==53) {P[0]=1050; P[1]=1050; P[2]=500; P[3]=1050; P[4]=500; P[5]=1050;}
if (Player==54) {P[0]=1050; P[1]=1050; P[2]=500; P[3]=1050; P[4]=1050; P[5]=500;}
if (Player==55) {P[0]=1050; P[1]=1050; P[2]=500; P[3]=1050; P[4]=1050; P[5]=1050;}
if (Player==56) {P[0]=1050; P[1]=1050; P[2]=1050; P[3]=500; P[4]=500; P[5]=500;}
if (Player==57) {P[0]=1050; P[1]=1050; P[2]=1050; P[3]=500; P[4]=500; P[5]=1050;}
if (Player==58) {P[0]=1050; P[1]=1050; P[2]=1050; P[3]=500; P[4]=1050; P[5]=500;}
if (Player==59) {P[0]=1050; P[1]=1050; P[2]=1050; P[3]=500; P[4]=1050; P[5]=1050;}
if (Player==60) {P[0]=1050; P[1]=1050; P[2]=1050; P[3]=1050; P[4]=500; P[5]=500;}
if (Player==61) {P[0]=1050; P[1]=1050; P[2]=1050; P[3]=1050; P[4]=500; P[5]=1050;}
if (Player==62) {P[0]=1050; P[1]=1050; P[2]=1050; P[3]=1050; P[4]=1050; P[5]=500;}
if (Player==63) {P[0]=1050; P[1]=1050; P[2]=1050; P[3]=1050; P[4]=1050; P[5]=1050;}
// Set Bullet Type
if (BulletType==0) {B[0]=500; B[1]=500; B[2]=500; B[3]=500;}
if (BulletType==1) {B[0]=500; B[1]=500; B[2]=500; B[3]=1050;}
if (BulletType==2) {B[0]=500; B[1]=500; B[2]=1050; B[3]=500;}
if (BulletType==3) {B[0]=500; B[1]=500; B[2]=1050; B[3]=1050;}
if (BulletType==4) {B[0]=500; B[1]=1050; B[2]=500; B[3]=500;}
if (BulletType==5) {B[0]=500; B[1]=1050; B[2]=500; B[3]=1050;}
if (BulletType==6) {B[0]=500; B[1]=1050; B[2]=1050; B[3]=500;}
if (BulletType==7) {B[0]=500; B[1]=1050; B[2]=1050; B[3]=1050;}
if (BulletType==8) {B[0]=1050; B[1]=500; B[2]=500; B[3]=500;}
if (BulletType==9) {B[0]=1050; B[1]=500; B[2]=500; B[3]=1050;}
if (BulletType==10) {B[0]=1050; B[1]=500; B[2]=1050; B[3]=500;}
if (BulletType==11) {B[0]=1050; B[1]=500; B[2]=1050; B[3]=1050;}
if (BulletType==12) {B[0]=1050; B[1]=1050; B[2]=500; B[3]=500;}
if (BulletType==13) {B[0]=1050; B[1]=1050; B[2]=500; B[3]=1050;}
if (BulletType==14) {B[0]=1050; B[1]=1050; B[2]=1050; B[3]=500;}
if (BulletType==15) {B[0]=1050; B[1]=1050; B[2]=1050; B[3]=1050;}
// SETS PLAYER TEAM
if (Team==0) {T[0]=500; T[1]=500;} // RED
if (Team==1) {T[0]=500; T[1]=1050;} // BLUE
if (Team==2) {T[0]=1050; T[1]=500;} // YELLOW
if (Team==3) {T[0]=1050; T[1]=1050;} // GREEN
// SETS Damage
if (Damage==0) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==1) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==2) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==3) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==4) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==5) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==6) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==7) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==8) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==9) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==10) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==11) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==12) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==13) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==14) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==15) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==16) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==17) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==18) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==19) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==20) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==21) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==22) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==23) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==24) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==25) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==26) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==27) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==28) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==29) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==30) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==31) {D[0] = 500; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==32) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==33) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==34) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==35) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==36) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==37) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==38) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==39) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==40) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==41) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==42) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==43) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==44) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==45) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==46) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==47) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==48) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==49) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==50) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==51) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==52) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==53) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==54) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==55) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==56) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==57) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==58) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==59) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==60) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==61) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==62) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==63) {D[0] = 500; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==64) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==65) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==66) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==67) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==68) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==69) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==70) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==71) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==72) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==73) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==74) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==75) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==76) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==77) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==78) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==79) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==80) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==81) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==82) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==83) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==84) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==85) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==86) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==87) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==88) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==89) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==90) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==91) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==92) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==93) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==94) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==95) {D[0] = 500; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==96) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==97) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==98) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==99) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==100) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==101) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==102) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==103) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==104) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==105) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==106) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==107) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==108) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==109) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==110) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==111) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==112) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==113) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==114) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==115) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==116) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==117) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==118) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==119) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==120) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==121) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==122) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==123) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==124) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==125) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==126) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==127) {D[0] = 500; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==128) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==129) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==130) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==131) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==132) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==133) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==134) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==135) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==136) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==137) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==138) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==139) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==140) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==141) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==142) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==143) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==144) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==145) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==146) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==147) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==148) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==149) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==150) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==151) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==152) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==153) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==154) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==155) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==156) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==157) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==158) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==159) {D[0] = 1050; D[1] = 500; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==160) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==161) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==162) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==163) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==164) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==165) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==166) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==167) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==168) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==169) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==170) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==171) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==172) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==173) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==174) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==175) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==176) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==177) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==178) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==179) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==180) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==181) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==182) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==183) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==184) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==185) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==186) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==187) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==188) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==189) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==190) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==191) {D[0] = 1050; D[1] = 500; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==192) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==193) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==194) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==195) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==196) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==197) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==198) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==199) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==200) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==201) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==202) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==203) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==204) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==205) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==206) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==207) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==208) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==209) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==210) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==211) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==212) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==213) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==214) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==215) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==216) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==217) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==218) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==219) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==220) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==221) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==222) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==223) {D[0] = 1050; D[1] = 1050; D[2] = 500; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==224) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==225) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==226) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==227) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==228) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==229) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==230) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==231) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==232) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==233) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==234) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==235) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==236) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==237) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==238) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==239) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 500; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==240) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==241) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==242) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==243) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==244) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==245) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==246) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==247) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 500; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
if (Damage==248) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 500;}
if (Damage==249) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 500; D[7] = 1050;}
if (Damage==250) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 500;}
if (Damage==251) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 500; D[6] = 1050; D[7] = 1050;}
if (Damage==252) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 500;}
if (Damage==253) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 500; D[7] = 1050;}
if (Damage==254) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 500;}
if (Damage==255) {D[0] = 1050; D[1] = 1050; D[2] = 1050; D[3] = 1050; D[4] = 1050; D[5] = 1050; D[6] = 1050; D[7] = 1050;}
// Sets Critical
if (Critical==1){C[0]=1050;}
  else {C[0]=525;}
// Sets Power
if (Power==0){X[0] = 500; X[1] = 500;}
if (Power==1){X[0] = 500; X[1] = 1050;}
if (Power==2){X[0] = 1050; X[1] = 500;}
if (Power==3){X[0] = 1050; X[1] = 1050;}
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
    Z[0]=1050;
    Z[1]=500;
  }
  else {
    Z[0]=500;
    Z[1]=1050;
  }
  Serial.println("Modified Parity Bits:");
  Serial.println(Z[0]);
  Serial.println(Z[1]);
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
}void changergbcolor() {
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
//
// utility function for digital clock display:
// prints preceding colon and leading 0
//
void printDigits(int digits) {
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
    Serial.print(digits);
}
// print current arduino "uptime" on the serial port
void DigitalClockDisplay() {
  int h,m,s;
  s = millis() / 1000;
  m = s / 60;
  h = s / 3600;
  s = s - m * 60;
  m = m - h * 60;
  Serial.print(h);
  printDigits(m);
  printDigits(s);
  Serial.println();
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
      Serial.println("Blinking RGB");
      }
  } else {
    rgboff();
    RGBState = LOW;
  }
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


//******************************************************************************************
// **********************************  CORE 0 LOOP  ****************************************
// *****************************************************************************************
void loop1(void *pvParameters) {
  Serial.print("loop running on core ");
  Serial.println(xPortGetCoreID());   
  while(1) { // starts the forever loop
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
    if (BASECONTINUOUSIRPULSE) {
      if (currentMillis0 - irledpreviousMillis >= irledinterval) {
        irledpreviousMillis = currentMillis0;
        if (MOTIONSENSOR) {
          MotionSensor(); // sets ir protocol to motion sensor yellow and sends it
        }
        if (CONTROLPOINTCAPTURED) {
          ControlPointCaptured();
        }
        if (OWNTHEZONE) {
          OwnTheZone();
        }
        if (HITTAG) {
          HitTag();
        }
      }
    }
    //**************************************************************************************
    // IR Receiver main functions:
    if (ENABLEIRRECEIVER || TAGACTIVATEDIR) { 
      receiveBRXir(); // runs the ir receiver, looking for brx ir protocols
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
    //**********************************************************************
    // Piezo Buzzer main functions:
    if (BUZZ) {
      buzz();
    }
    delay(1); // this has to be here or it will just continue to restart the esp32
  }
}
// *****************************************************************************************
// **************************************  CORE 1 LOOP *************************************
// *****************************************************************************************
void loop2(void *pvParameters) {
  Serial.print("loop running on core ");
  Serial.println(xPortGetCoreID());
  while (1) { // starts the forever loop
    // place main blynk functions in here, everything wifi related:
    unsigned long currentMillis1 = millis(); // sets the timer for timed operations
    // place main blynk functions in here, everything wifi related:
    if (RUNESPNOW) {
      if (ESPNOWDATASEND) {
        ESPNOWDATASEND = false;
        getReadings();
        BroadcastData();
      }
      if (currentMillis1 - interval > PreviousMillis) {
        PreviousMillis = currentMillis1;
        ESPNOWDATASEND = true;
      }
    }
    if (RUNBLYNK) {
      Blynk.run();
    }
    delay(1); // this has to be here or the esp32 will just keep rebooting
  }
}

//*************************************************************************
//************************** SETUP ROUTINE ********************************
//*************************************************************************
void setup() {
  // initialize serial monitor
  Serial.begin(115200);
  Serial.println("Serial Monitor Started");
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
  Serial1.print("AT+PARAMETER=10,7,1,7\r\n");    //For Less than 3Kms
  //Serial1.print("AT+PARAMETER= 12,4,1,7\r\n");    //For More than 3Kms
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
  // Initialize BLYNK
  if (SETUPBLYNK) {
    initializeblynk();
    SETUPBLYNK = false;
  }
  //***********************************************************************
  // Initialize ESPNOW
  if (STARTESPNOW) {
    STARTESPNOW = false;
    initializeespnow();
  }
  //***********************************************************************
  // initialize dual core and loops
  Serial.println("Initializing Dual Core, Dual Loops");
  Serial.print("setup() running on core ");
  Serial.println(xPortGetCoreID());
  xTaskCreatePinnedToCore(loop1, "loop1", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(loop2, "loop2", 4096, NULL, 1, NULL, 1);
}

//*************************************************************************
//**************************** PRIMARY LOOP *******************************
//*************************************************************************
void loop() {
  // Empty Loop
}
