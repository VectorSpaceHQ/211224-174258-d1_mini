/**************************************************************************************************************************
 * Current Sense Assembly - Vector Space Wood Shop
 *
 * Built by Chaz Fisher for the Vector Space Makerspace, <address>, Lynchburg, VA
 *
 * This code is part of an automated dust collection system, intended to run on an ESP8266. A current sensor measures
 * current being drawn by the stationary tool (table saw, etc) and provides a digital signal to the ESP when the tool
 * is turned on. The ESP provides an output to an RC servo to open the dust collector gate, and a signal via an optocoupler
 * to turn on the central vacuum. When the current falls to zero, dealys are counted down before closing the gate and
 * turning off vacuum.
 *
 * Future work will add an MQTT client, that will communicate when tools are in use, and a separate unit on the central
 * vacuum and air filter to turn them on and off without needing wires.
 *
 * ISR Servo code Copyright (c) 2021 by Khoi Hoang - See below
 *
 */

#include <Arduino.h>
#include "EspMQTTClient.h"

EspMQTTClient espclient(
    "VS-2",
    "fourhundredtwo",
    "10.0.0.218",  // MQTT Broker server ip
    "Dust Collector",     // Client name that uniquely identify your device
    1883              // The MQTT port, default to 1883. this line can be omitted
    );

#ifndef ESP8266
  #error This code is designed to run on ESP8266 platform, not Arduino nor ESP32! Please check your Tools->Board setting.
#endif

// Turn-off timer values, in 100 msec "ticks"
#define   VAC_DELAY     600000 //  600,000 millis = 10 min. 6 Starts per hour.
#define SAFETY_TIMER    1800000 // 1,800,000 millis = 30 min

bool vacOn = false;
unsigned int vacCounter = millis();
const int vacCntrlPin = D2;
const int ledPin = D4;
const int switchPin = D5;
int tool_on_counter = 0;
long lastbeat = millis();
bool safetyBlock = true;
long timeOn;
bool switchState = false;


void onMessageReceived(const String& message) {
  delay(100); // debounce
    if (message.indexOf(", ON") != -1){
        Serial.println("ON command received");

        espclient.publish("tools/dust_collection", "** Dust collector is turning ON **");
        digitalWrite(vacCntrlPin, HIGH);
        digitalWrite(ledPin, LOW); //turns on led
        vacOn = true;
        tool_on_counter += 1;
        timeOn = millis();
        Serial.println(tool_on_counter);
        vacCounter = millis() + SAFETY_TIMER;

    }
    else if (message.indexOf(", OFF") != -1){
        Serial.println("OFF command received");
        tool_on_counter = tool_on_counter - 1;
        tool_on_counter = max(0, tool_on_counter);
        Serial.println(tool_on_counter);
    }

    if (tool_on_counter == 0){
        Serial.println("start coundown");
        vacCounter = millis(); // no tools on, start countdown
    }
}


void onConnectionEstablished()
{
    espclient.publish("tools/dust_collection", "Dust Collector connected");
    espclient.subscribe("tools/dust_collection", onMessageReceived);
}

void heartbeat()
{
  if (millis() - lastbeat > 1000){
    if (vacOn == false){
      digitalWrite(ledPin, LOW);
      delay(500);
      digitalWrite(ledPin, HIGH);
      lastbeat = millis();
      Serial.println("heartbeat off");
    }
    else{
      digitalWrite(ledPin, HIGH);
      delay(500);
      digitalWrite(ledPin, LOW);
      lastbeat = millis();
      Serial.print("heartbeat on, ");
      Serial.print("tool count = ");
      Serial.println(tool_on_counter);
    }
  }
}


void setup()
{
  Serial.begin(115200);
  while (!Serial);

  // Optional functionalities of EspMQTTClient
  espclient.enableDebuggingMessages(); // Enable debugging messages sent to serial output
  espclient.enableHTTPWebUpdater(); // Enable the web updater. User and password default to values of MQTTUsername and MQTTPassword. These can be overridded with enableHTTPWebUpdater("user", "password").
  espclient.enableOTA(); // Enable OTA (Over The Air) updates. Password defaults to MQTTPassword. Port is the default OTA port. Can be overridden with enableOTA("password", port).
  espclient.enableLastWillMessage("TestClient/lastwill", "I am going offline");  // You can activate the retain flag by setting the third parameter to true

  pinMode(vacCntrlPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(switchPin, INPUT_PULLUP);
  digitalWrite(ledPin, HIGH); //initialize led off

  heartbeat();

  // startup delay to wait for wifi
  delay(3000);

  safetyBlock = false;
}


void loop()
{
  espclient.loop();
  heartbeat();


  // ************** manual operation ****************
  if(digitalRead(switchPin) == HIGH and switchState == false){
    delay(100); // debounce
    if(digitalRead(switchPin) == HIGH and switchState == false and safetyBlock == false){
      switchState = true;
      espclient.publish("tools/dust_collection", "manual switch, ON");
    }
  }
  else if (switchState == true and digitalRead(switchPin) == LOW){
    delay(100); // debounce
    if(digitalRead(switchPin) == LOW){
      safetyBlock = false;
      switchState = false;
      espclient.publish("tools/dust_collection", "manual switch, OFF");
    }
  }
  //  ***********************************

  if(vacOn == true and millis() > (vacCounter + VAC_DELAY)){ // turn off after delay
      vacOn = false;
      digitalWrite(vacCntrlPin, LOW);
      digitalWrite(ledPin, HIGH);
      espclient.publish("tools/dust_collection", "** Dust collector is turning OFF **");
  }


  // Safety turn off
  if(vacOn == true and millis() - timeOn > SAFETY_TIMER){ // turn off after 20 minutes as fail-safe
      vacOn = false;
      digitalWrite(vacCntrlPin, LOW);
      digitalWrite(ledPin, HIGH);
      espclient.publish("tools/dust_collection", "** Dust collector is turnign OFF due to safety timer **");
      Serial.println("Safety Timer reached. Shutting down dust collector.");
      delay(1000);
      safetyBlock = true; // require manual switch to be turned off before turning on again
  }

  //delay(1000);
}
