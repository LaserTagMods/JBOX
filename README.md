*******************************************************************************
JBOX INTRODUCTION
=================
*******************************************************************************
Description:
------------
BRX compatible utility box for interactive game play

Required Components:
--------------------
Wemos D1 Mini ESP32 (1), IR Receiver TSOP4138 or similar (1) and or 
TSAL6400 Emitter (4); one or the other or both as a bare minimum.

Optional Components:
--------------------
IR Receiver TSOP4138 (1), TSAL6400 Emitter (4), 
SS8550 Transistors (2) or 2N2222 Transistors (alternate design), Piezo Buzzer (1), RGB Common Cathode 5mm LED (1), 
RYLR896 LORA module (1), Resistors as indicated on schematic (multiple)

Basic Concepts:
---------------
The JBOX is a game box or accessory for the BRX line of taggers, if desired, 
the IR Protocol could be changed easily to configure the device to any laser 
tag equipment line. I hope to, in the future, make it compatible with all my 
favorite laser tag gear. The device uses BLE to be configured and set by a mobile 
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
  WIFI (ESPNOW) (device to device only)
  BLE(mobile to device)
  Infrared(device to BRX)
  MultiColor LED(Device to human eye).

*******************************************************************************
Main Functions:
================
*******************************************************************************
Basic Domination Game Mode:
---------------------------
default domination mode that provides both player scoring and team scoring
Scoring reports over BLE to paired mobile device and refreshes every time
the score changes. To deactivate, select this option a second time to pause
the game/score. Recommended as a standalone base use only. Each time base is
shot, the team or player who shot takes posession and the base emitts a tag
that notifies player that the base (control point) was captured.

Cointinuous IR Emitter:
-----------------------
Continuous IR Emitter Mode, Clears out any existing IR Tag Settings
Then activates a default interval spaced broadcast of a tag of choice.
The tag of choice will need to be selected otherwise motion sensor is broadcasted.
This is good for use as a respawn station that requires no button or trigger
mechanism to activate. The default delay is five seconds but can be modified
by another setting option in the application. Another use is a proximity detector 
or mine.

Tag Activated IR Emitter Mode:
------------------------------
This mode causes the base to send a selected tag type when a player shoots the base sensor. 
Use for this is if you wanted a headset activated respawn station, med kit activated base, etc. 
this is usefull for players to have an iR based interactive base that in order to activate
You need to be alive. It can also be used to set a base as an armed station that can serve as
A drone to tag other players. It also has a cool down function, this way it can be set to only 
Provide a perk every so often and the cool down period can be adjusted via the app. It will also
Have an activation limit option as well so if you wanted to limit respawns or med kits from the 
Base this can easily be pre configured.

If this mode is engaged and the respawn station tag is selected, the base will send a Lora signal
Out each time the respawn is requested. Also it only recognizes respawn request signal from the 
Headset. 

Capturable Continuous IR Emmitter:
----------------------------------
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

Capture the Flag:
-----------------
This mode allows you to have two bases that communicate with each other (battle lines) or interact with 
JEDGE modded taggers. The two modes are described below but general play/set up is the same. JEDGE is a more 
concise capture the flag game play. LoRa is used for communication between bases. If you do not have LoRa 
Modules installed (see optional components) the game will not work.

General setup/play:
Two bases are used and set up as capture the flag, two bases are set up as respawn stations. Each team will 
have a respawn station and a capture the flag base aligned with their team (friendly). It is recommended that 
each teams respawn station and flag base are located in different locations requiring defending players, that 
are tagged out, to retreat to their respawn station and have to return quickly to continue to defend. Team 
mates will need to capture the other teams flag by shooting it. The same player will now need to quickly return 
to their flag, without being tagged out, and “hang the flag” by shooting their own (friendly) base. 

When an enemy flag is “captured” by a player, the flag sends a rf notification to the players base/flag, informing 
the player’s flag that the player has now captured a flag and has one in his possession. When the same player 
returns to his own flag, tags his flag, his friendly flag will check to see if he carries a flag based upon previously 
receiving the notification from the enemies flag. If this is the case, the game is over, the base exits capture the flag 
mode and flashes the color that the team has won. It also sends a signal to the enemy flag to do the same. Both bases 
also emit an IR tag that sounds an alarm on the players taggers that are near by to ensure players know the game has ended.

If a player is tagged out after capturing a flag, he will need to respawn at his teams respawn station. When he respawns at
the respawn station, he will need to use his respawn request tag from his headset to respawn (tag activated respawn station). 
When the player activated the respawn station, the respawn station will send a notification (via LoRa) to the freindly flag, 
to now disregard that players tag so that he can not “hang a flag.” 

There is potential for players to cheat this mode by respawning from another players respawn request and then be able to go 
to their friendly base and “hang the flag”. This is why JEDGE is more accurate...

JEDGE DIFFERENCE: if using JEDGE modified BRXs, the enemy flag will enable the tagger to be in flag carrier mode until it is 
tagged out. Flag carrier mode provides the player with a special weapon that will enable the activation of a flag hang. Planers 
exit flag carrier mode when they are tagged out an need to respawn. This is a much more effective method and avoids potential 
cheating. 

*******************************************************************************
How To Use The JBOX: 
====================
*******************************************************************************
Notes:
If your not familiar with it, is pretty complicated to get set up and one
needs to have a pretty decent grasp of basic electronics and networking in order
to get it up and running. My goal for JBOX is to make it user freindly so there
is little customization needed for the accessory, therefore, I'm putting together
a PCB circuit board for the build. If someone wants to customize their own build
of a cool looking 3d printed base, cool! Make me one? :) and that is why I have
a schematic for my build that allows for anyone to get as creative as they want
to with their JBOX, or alternatively, just get a JBOX PCB board. Make your own
and have it printed, or possibly my son will build them and sell them and I'll
QC it so it is a good learning opportunity for him as well as life lesson on work
and pay. So if you want one built, just let me know, otherwise, go wild and make
your own. It's open source so have at it.The PCB that we are building will fit
easily in a clear box with a power bank and allow for the ir to pass in and out.

Steps to Make it work!!!
-------------------------
Step 1: Get the stuff!, list of supplies are listed above, any version of the ESP32
        will work and you can find the supplies from amazon, digikey, etc. I'll see if I
        can get the links on here soon enough. (Alternatively, Order a JBOX from me for my
        son to build and help him learn work/reward. Youth today need that.)

Step 2: Download and install Arduino IDE on your computer, install the libraries
        needed, ESP32 board manager, etc... yeah, this may take you a bit to get working
        if youve never done this before... so here are some tips/links:
  2a) Download and install Arduino: https://www.arduino.cc/en/software
  2b) Install Board Managers for ESP32: https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/
  2c) Install the Blynk Library: http://help.blynk.cc/en/articles/512105-how-to-install-blynk-library-for-arduino-ide
  2d) Hopefully thats all, If you cant upload the ino file to your esp32, you may have to search to find a driver for your computer
      for your specific board your using and also maybe see if there is a library that didnt get installed that the code is using, i
      think that they are covered though from the link above
  2X) forget it and just get one from my son if all this is too much of see if someone in the community will make one for you

Step 3: Download and install the Blynk App to your bluetooth enabled mobile device: 
  3a) Google Play: https://play.google.com/store/apps/details?id=cc.blynk
  3b) Apple Store: https://apps.apple.com/us/app/blynk-iot-for-arduino-esp32/id808760481

Step 4: Install a clone of my Blynk Application
  4a) follow this link and scroll to the very last two steps: https://github.com/blynkkk/blynkkk.github.io/blob/master/Sharing.md
  4b) use the qr image in this github to access to my Blynk Project for your own cloned copy

Step 5: Open the Ino file from this repository on github.com

Step 6: Youll need to change the Blynk Token in your ino file... 
  6a) Search the ino file by pressing ctrl+f to find the following: "char auth[] ="
  6b) Replace the "xxdlksuhneo84u498" craxy number/letter combination with what is found in your cloned blynk app, see link:
  https://learn.sparkfun.com/tutorials/blynk-board-arduino-development-guide/get-a-blynk-auth-token#:~:text=To%20find%20an%20existing%20Blynk,and%20a%20couple%20handy%20buttons.

Step 7: Upload the ino (downloadable from this repository) to your esp32: https://www.dummies.com/computers/arduino/how-to-upload-a-sketch-to-an-arduino/
  7a) be sure to set the "partition scheme" under to tools menu to "large apps (no OTA)"

Step 8: follow the App Use Video: (video coming soon)

How To Use The App!!
--------------------
!!! IMPORTANT !!!
Any time you change the main function in the App, you reset team alignment for 
the JBOX. Be sure to modify the sub-settings only AFTER you change the main 
function for game play/interaction so that the JBOX does exactly what you want.
Most settings are not saved but reset when going between main base functions.

NOTE: The JBOX boots up in "Basic Domination Mode" so it will start out of the box
running a domination game as soon as the first team tags it to "capture it".

Step 1: Select a main function for the JBOX to execute

Step 2: Select the applicable functions that would apply to the main function selected:
  2a) Here are the applicable sub-settings for each function:
        
        Basic Domination Mode: 
          - No Sub Options
          - Reset - Resets Score and stops the timer
          - Selecting the main function "basic domination mode" stops the clock on the game
        
        Continuous IR Emitter:
          - IR Emitter Type - Changes the tag type sent by base (motion sensor / alarm is default)
          - Team Friendly Selection - Changes the team protocol of the tag being sent by base (default is yellow)
          - Adjust Continuous Emitter Frequency - changes the time between tags sent by base
        
        Tag Activated IR Emitter:
          - IR Emitter Type - Changes the tag type sent by base (motion sensor / alarm is default)
          - Team Friendly Selection - Changes the team protocol of the tag being sent by base (default is yellow)
          - Adjust/Set Tag Activated Cool Down - Base will have a delay between its ability to be activated (Default is off)
        
        Capturable Continuous IR Emitter:
          - IR Emitter Type - Changes the tag type sent by base (motion sensor / alarm is default)
          - Team Friendly Selection - Changes the team protocol of the tag being sent by base (default is yellow)
          - Adjust Continuous Emitter Frequency - changes the time between tags sent by base
          - Capturable IR Base Tag Count - Sets the minimum number of tags needed to be received by a team in order
            to capture the base (default 10)

        Capture The Flag:
          - Team Freindly Selection - changes team alignment  
          - Capturable IR Base Tag Count - Sets how many tags are needed to capture the enemy flag
          - 



