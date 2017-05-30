#define PING_BUTTON 4
#define REQUEST_GPS_ONCE_BUTTON 5
#define REQUEST_GPS_CONTINUOUS_BUTTON 6
#define REQUEST_GPS_STOP_BUTTON 7

#define sclk 13 // Don't change
#define mosi 11 // Don't change
#define cs   10
#define dc   A0
#define rst  A1  // you can also connect this to the Arduino reset

#define _SS_MAX_RX_BUFF 128
#define SOFTWARE_SERIAL_RX 3
#define SOFTWARE_SERIAL_TX 2

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_QDTech.h> // Hardware-specific library
#include <SPI.h>
#include <SoftwareSerial.h>

//Include the bitmap of the map. Location of the GPS tracker will be drawn on the map
#include "mapBitmap.h"

SoftwareSerial ss(SOFTWARE_SERIAL_RX, SOFTWARE_SERIAL_TX); //for HC-12 wireless communication module.

Adafruit_QDTech tft = Adafruit_QDTech(cs, dc, rst);

void transmit(String datatype, String data) {
  setCursorLine(5, 13);
  tft.print(datatype + ":" + data);
  ss.print(datatype + ":" + data + "\n");
  delay(1000);
  tft.fillRect(5 * 6, 13 * 8, 17 * 6, 1 * 8, QDTech_BLACK); //clear the transmitted text
}

void blinkLED() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(50);
  digitalWrite(LED_BUILTIN, LOW);
}

bool digitalReadDebounce(int pin) {
  if (digitalRead(pin) == LOW) {
    delay(50);
    bool debounceRunning = true;
    while (debounceRunning) {
      if (digitalRead(pin) == HIGH) {
        debounceRunning = false;
      }
    }
    delay(50);
    return true;
  }
  return false;
}

void setCursorLine(unsigned int lineX, unsigned int lineY) {
  //Screen is 128px x 160px
  //Characters are 6px x 8px
  //(126px used for chars) x 160px  =  21 x 20 characters
  tft.setCursor(lineX * 6, lineY * 8);
}

void clearGPSTFT() {
  tft.fillRect(5 * 6, 15 * 8, 17 * 6, 2 * 8, QDTech_BLACK); //clear lat and long
  tft.fillRect(5 * 6, 17 * 8, 6 * 6, 3 * 8, QDTech_BLACK); //clear bottom left values
  tft.fillRect(16 * 6, 17 * 8, 6 * 6, 3 * 8, QDTech_BLACK); //clear bottom right values
}

void clearReceivedTFT() {
  tft.fillRect(5 * 6, 14 * 8, 17 * 6, 1 * 8, QDTech_BLACK); //clear the received text
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  pinMode(PING_BUTTON, INPUT_PULLUP);
  pinMode(REQUEST_GPS_ONCE_BUTTON, INPUT_PULLUP);
  pinMode(REQUEST_GPS_CONTINUOUS_BUTTON, INPUT_PULLUP);
  pinMode(REQUEST_GPS_STOP_BUTTON, INPUT_PULLUP);

  Serial.begin(1200); //for debugging
  ss.begin(1200); //for HC-12 wireless communication module

  tft.init();
  tft.setRotation(0);  // 0 = Portrait, 1 = Landscape
  tft.fillScreen(QDTech_BLACK);
  tft.setTextWrap(false);
  tft.setCursor(0, 0);
  tft.setTextColor(QDTech_WHITE);
  tft.setTextSize(1);

  //DRAW BITMAP IMAGE AND PRINT VALUE TITLES
  tft.drawXBitmap(0, 0, mapBitmap, 128, 103, QDTech_WHITE); //Draw bitmap image
  tft.drawFastHLine(0, 103, 128, QDTech_WHITE); //Note: draw on line 103, which is the 104th line
  setCursorLine(0, 13); //Print titles for values
  tft.println("Sent:");
  tft.println("Rcvd:");
  tft.println("Lat: ");
  tft.println("Long:");
  tft.print("Sats:");
  setCursorLine(11, 17);
  tft.println("Acc:");
  tft.print("Sped:");
  setCursorLine(11, 18);
  tft.println("Dir:");
  tft.print("Age:");
  setCursorLine(11, 19);
  tft.print("Chck:");

  digitalWrite(LED_BUILTIN, LOW);
}

void loop() {
  if (digitalReadDebounce(PING_BUTTON)) {
    transmit("ping", "handheldsend");
  }
  if (digitalReadDebounce(REQUEST_GPS_ONCE_BUTTON)) {
    transmit("gps", "once");
  }
  if (digitalReadDebounce(REQUEST_GPS_CONTINUOUS_BUTTON)) {
    transmit("gps", "continuous");
  }
  if (digitalReadDebounce(REQUEST_GPS_STOP_BUTTON)) {
    transmit("gps", "stop");
  }

  if (ss.available() > 0) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(2000);
    //// PROCESSING INCOMING SERIAL DATA from "<datatype>:<csv of data>" INTO 'datatype' AND 'datacsvarray' ////
    //Read the incoming byte:
    String receivedSerialString = ss.readStringUntil('\n');
    setCursorLine(5, 14);
    tft.print(String(receivedSerialString));

    Serial.println("received: " + receivedSerialString);
    Serial.println("length: " + String(sizeof(receivedSerialString)));
    if (ss.overflow()) {
      Serial.println("SoftwareSerial overflow!");
    } else {
      Serial.println("no SoftwareSerial overflow!");
    }

    //Split receivedSerialString into datatype and data
    int indexofcolon = receivedSerialString.indexOf(":");
    int lengthofstring = receivedSerialString.length();
    String datatype = "";
    if (indexofcolon == -1) {
      //Transmit("message", "error - no colon in string");
      delay(100); //Do nothing
    } else if (indexofcolon != 0 && indexofcolon != lengthofstring - 1) {
      datatype = receivedSerialString.substring(0, indexofcolon);
      String datacsv = receivedSerialString.substring(indexofcolon + 1);
      int indexofcomma = datacsv.indexOf(",");
      //For every character in datacsv, if it is a comma add 1 to commacount
      //If not a comma then add 0 to commacount (so commacount doesn't increase)
      int i, commacount;
      for (i = 0, commacount = 0; datacsv[i]; i++) {
        commacount += (datacsv[i] == ',');
      }
      //Create an array with 1 more element than the amount of commas
      //e.g. "1,2,3" has 2 commas and 3 pieces of information
      String datacsvarray[commacount + 1];
      //For the amount of commas+1 in datacsv:
      for (int i = 0; i < commacount + 1; i++) {
        //if there's a comma in datacsv
        if (datacsv.indexOf(",") > -1) {
          //i's position in datacsvarray is set to the string between the start of datacsv and the next comma
          datacsvarray[i] = datacsv.substring(0, datacsv.indexOf(","));
          //The string between the start of datacsv up to and including the next comma is removed from the datacsv
          datacsv.remove(0, datacsv.indexOf(",") + 1);
        } else {
          //i's position in datacsvarray is set to datacsv. This happens once at the end of the for loop when all commas have been removed from datacsv
          datacsvarray[i] = datacsv;
          //For consistency, delete the reset of datacsv
          datacsv.remove(0);
        }
      }

      Serial.println("data type: " + datatype);
      Serial.println("Comma count: " + String(commacount));
      Serial.println("sizeof datacsvarray: " + String(sizeof(datacsvarray)));
      Serial.println("==============");
      for (int i = 0; i <= commacount; i++) {
        setCursorLine(5, 14);
        Serial.print("Data " + String(i) + " " + datacsvarray[i]);
        Serial.println(",");
      }
      //// PROCESSING 'datatype' AND 'datacsvarray' ////
      if (datatype == "ping") {
        if (datacsvarray[0] == "trackersend") {
          transmit("message", "handheldreceived");
        } else {
          transmit("message", "ping invalid");
        }
      } else if (datatype == "gps") {
        //data = "latitude,longitude,num of satellites,accuracy,speed,direction,age of data,checksum"
        clearGPSTFT();

        setCursorLine(5, 15);
        tft.print(datacsvarray[0]); //latitude
        setCursorLine(5, 16);
        tft.print(datacsvarray[1]); //longitude
        setCursorLine(5, 17);
        tft.print(datacsvarray[2]); //num of satellites
        setCursorLine(16, 17);
        tft.print(datacsvarray[3]); //accuracy
        setCursorLine(5, 18);
        tft.print(datacsvarray[4]); //speed
        setCursorLine(16, 18);
        tft.print(datacsvarray[5]); //direction
        setCursorLine(5, 19);
        tft.print(datacsvarray[6]); //age of data
        setCursorLine(16, 19);
        tft.print(datacsvarray[7]); //checksum

        if (commacount != 7) { // If incorrect number of commas received, show it in bottom right corner of screen
          setCursorLine(20, 19);
          tft.print(commacount);
        }
      } else if (datatype == "message") {
	    clearReceivedTFT(); //message is written to the same area of the screen as received text, so this is cleared first
        setCursorLine(5, 14);
		tft.print(datacsvarray[0]); //For messages with only one piece of data and therefore one item in datacsvarray[]
		/*for (int i = 0; i <= commacount; i++) { //For messages with multiple items in datacsvarray[] (not used)
          tft.print(datacsvarray[i]);
          tft.print(",");
        }*/
		delay(1000); //Delay for a second to read message
      } else {
        //transmit("message", "message datatype not understood");
        delay(100); //Do nothing
      }
    } else {
      transmit("message", "nothing on one side of colon character");
    }
    delay(200);
	clearReceivedTFT();
    digitalWrite(LED_BUILTIN, LOW);
  }
}