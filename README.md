# JBOX
BRX compatible utility box for interactive game play

Required Components: Wemos D1 Mini ESP32 (1), IR Receiver TSOP4138 or similar (1).

Optional Components: TSAL6400 Emitter (4), SS8550 Transistors (2), Piezo Buzzer (1), RGB Common Cathode 5mm LED (1), RYLR896 LORA module (1)

Basic Concepts: 
The JBOX is a game box or accessory for the BRX line of taggers, if desired, the IR Protocol could be changed easily to configure the device to any laser tag equipment line. The device uses BLE to be configured and set by a mobile device using the ITO control platform "BLYNK". A customized Blynk application has been developed and is used for configuring and customizing the functions of the JBOX device. If all components listed above are integrated, the device can provide long range communication (device to device) for interactive game play for score keeping or objective completions. Alternatively, A simple IR Receiver can be plugged in for basic base capturability for simple games. Additionally, Short range base to base communication is built in using the ESPNOW wireless communications so that bases within range can transmit data back and forth over short wifi limitations. 
This allows for the following communications capabilities, all simultaneously: LORA(device to device only), WIFI(device to device only), BLE(mobile to device), Infrared(device to BRX), MultiColor LED(Device to human eye).

