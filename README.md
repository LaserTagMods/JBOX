JBOX

BRX compatible utility box for interactive game play

Required Components: 
Wemos D1 Mini ESP32 (1), IR Receiver TSOP4138 or similar (1) and or 
TSAL6400 Emitter (4); one or the other or both as a bare minimum.

Optional Components: 
IR Receiver TSOP4138 (1), TSAL6400 Emitter (4), 
SS8550 Transistors (2), Piezo Buzzer (1), RGB Common Cathode 5mm LED (1), 
RYLR896 LORA module (1), Resistors as indicated on schematic (multiple)

Basic Concepts: 
The JBOX is a game box or accessory for the BRX line of taggers, if desired, 
the IR Protocol could be changed easily to configure the device to any laser 
tag equipment line. The device uses BLE to be configured and set by a mobile 
device using the ITO control platform "BLYNK". A customized Blynk application 
has been developed and is used for configuring and customizing the functions 
of the JBOX device. If all components listed above are integrated, the device 
can provide long range communication (device to device) for interactive game 
play for score keeping or objective completions. Alternatively, A simple IR 
Receiver can be plugged in for basic base capturability for simple games. 
Additionally, Short range base to base communication is built in using the 
ESPNOW wireless communications so that bases within range can transmit data 
back and forth over short wifi limitations. 

This allows for the following communications capabilities, all simultaneously: 
  LORA(device to device only)
  WIFI(device to device only)
  BLE(mobile to device)
  Infrared(device to BRX)
  MultiColor LED(Device to human eye).

*******************************************************************************
Main Functions: 
*******************************************************************************

Basic Domination Game Mode: 

default domination mode that provides both player scoring and team scoring
Scoring reports over BLE to paired mobile device and refreshes every time
the score changes. To deactivate, select this option a second time to pause
the game/score. Recommended as a standalone base use only. Each time base is
shot, the team or player who shot takes posession and the base emitts a tag
that notifies player that the base (control point) was captured.


Cointinuous IR Emitter: 

Continuous IR Emitter Mode, Clears out any existing IR Tag Settings
Then activates a default interval spaced broadcast of a tag of choice.
The tag of choice will need to be selected otherwise motion sensor is broadcasted.
This is good for use as a respawn station that requires no button or trigger
mechanism to activate. The default delay is two seconds but can be modified
by another setting option in the application. Another use is a proximity detector or mine.


Tag Activated IR Emitter Mode:

This mode causes the base to send a selected tag type when a player shoots the base sensor. 
Use for this is if you wanted a headset activated respawn station, med kit activated base, etc. 
this is usefull for players to have an iR based interactive base that in order to activate
You need to be alive. It can also be used to set a base as an armed station that can serve as
A drone to tag other players. It also has a cool down function, this way it can be set to only 
Provide a perk every so often and the cool down period can be adjusted via the app. It will also
Have an activation limit option as well so if you wanted to limit respawns or med kits from the 
Base this can easily be pre configured.

Dual Mode - Continuous IR Emmitter and Tag activated base capture to change team alignment (freindly):

This mode allows you to have a base continuously emit a specific team freindly ir tag/protocol of
Choice, while allowing for other teams to capture the base after a pre-set or customizable number 
Of tags landed on the base. What will this allow you to do? A sentry unit that radiates damage  to 
Opposing teams, based upon the last team that captured the base. A healing station that can be changed
Mid game for who owns it. A respawn station that can be captured/over taken. This last one is the
Purpose in designing this mode. The main game play function is that a number of bases can be scattered
In the play area. At the start of the game teams race to establish bases to their color/alignment and
While some defend, others try to capture others. As the game progresses, teams with unlimited respawns 
Will thin out as they no longer have a base to respawn at, creating quite an ever changing field strategy
For all.
