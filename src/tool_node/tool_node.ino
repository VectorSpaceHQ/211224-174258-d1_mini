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
 * LOLIN(WEMOS) D1 R2 & mini
 *
 * ISR Servo code Copyright (c) 2021 by Khoi Hoang - See below
 *
 */

/****************************************************************************************************************************
  ESP8266_ISR_MultiServos.ino
  For ESP8266 boards
  Written by Khoi Hoang

  Built by Khoi Hoang https://github.com/khoih-prog/ESP8266_ISR_Servo
  Licensed under MIT license

  Loosely based on SimpleTimer - A timer library for Arduino.
  Author: mromani@ottotecnica.com
  Copyright (c) 2010 OTTOTECNICA Italy

  Based on BlynkTimer.h
  Author: Volodymyr Shymanskyy

  Version: 1.2.0

  Version Modified By   Date      Comments
  ------- -----------  ---------- -----------
  1.0.0   K Hoang      04/12/2019 Initial coding
  1.0.1   K Hoang      05/12/2019 Add more features getPosition and getPulseWidth. Optimize.
  1.0.2   K Hoang      20/12/2019 Add more Blynk examples.Change example names to avoid duplication.
  1.1.0   K Hoang      03/01/2021 Fix bug. Add TOC and Version String.
  1.2.0   K Hoang      18/05/2021 Update to match new ESP8266 core v3.0.0
 *****************************************************************************************************************************/

// Ported from Arduino IDE to MS Visual Studio using Platformio extension

#include <Arduino.h>
#include "tool.h"
#include "EspMQTTClient.h"

#define TOOL_NAME "Table Saw 1"

EspMQTTClient espclient(
    "VS-2",
    "fourhundredtwo",
    "10.0.0.218",  // MQTT Broker server ip
    TOOL_NAME,     // Client name that uniquely identify your device
    1883              // The MQTT port, default to 1883. this line can be omitted
    );


// initializations for the ISR_SERVO functions

#ifndef ESP8266
  #error This code is designed to run on ESP8266 platform, not Arduino nor ESP32! Please check your Tools->Board setting.
#endif

#define TIMER_INTERRUPT_DEBUG       1
#define ISR_SERVO_DEBUG             0

#include <ESP8266_ISR_Servo.h>


// Adjusted values for SG46 servos; CLF
#define MIN_MICROS       550  // Min pulswidth 0.55 msec => 0 deg position
#define MAX_MICROS      2450  // Max pulsewidth 2.45 msec => 180 deg position


int servoIndex  = -1;

// Initializations for this application

// Turn-off timer values, in 100 msec "ticks"
#define   GATE_DELAY    1000 //  Test 100 ticks, 10 sec    1200    // 1200 ticks, 120 sec (2 min) delay to close gate after tool turns off
#define   VAC_DELAY     3000 //  Test 300 ticks, 30 sec    3000    // 3000 ticks, 300 sec (5 min) delay to turn off vacuum after tool turns off
#define   CLOSED_GATE     145 //  Closed position of gate is 0 degrees on the servo. 145 when on right side
#define   OPEN_GATE     20 //  Open position of gate is 105 degrees on the servo. 20 when on right side
#define   LED_BLINK       5 //  5 ticks, 0.5 sec to blink the LED

bool toolOn = false;
bool gateOpen = false;
bool vacOn = false;
int gatePosition = 0;
int gateCounter = 0;
int vacCounter = 0;
const int currSensePin = D5;
int currSense = 0;
const int gateCntrlPin = D1;
const int vacCntrlPin = D2;
int vacCntrl = 0;
const int ledPin = D4;
int ledState = 0;
int ledBlinkCounter = 0;
Tool tool;


void blinkLED()
{
  if(ledState==LOW)
  {
    digitalWrite(ledPin,HIGH);
    ledState=HIGH;
  }
  else
  {
    digitalWrite(ledPin,LOW);
    ledState=LOW;
  }
}
 
    
void onConnectionEstablished()
{
    String msg = String(TOOL_NAME) + " Connected";
    espclient.publish("tools/dust_collection", msg);
}


void setup()
{
  tool.name = TOOL_NAME;

  Serial.begin(115200);
  while (!Serial);

  delay(200);

  Serial.print("\nStarting Current Sensor Assembly");
  Serial.print(F("\nStarting ESP8266_ISR_MultiServos on ")); Serial.println(ARDUINO_BOARD);
  Serial.println(ESP8266_ISR_SERVO_VERSION);
  delay(200);
  Serial.println("Starting to setup servo");
  servoIndex = ISR_Servo.setupServo(gateCntrlPin, MIN_MICROS, MAX_MICROS);   // Pin D1 based on schematic as of 11/25/21
  delay(400);
  Serial.println("Returned from setup");
  if (servoIndex != -1)
  {
    Serial.println(F("Setup Servo OK"));
    for(int i=0; i<5; i++)
    {
      digitalWrite(ledPin,HIGH);
      delay(50);
      digitalWrite(ledPin,LOW);
      delay(50);
    }
  }
  else
  {
    Serial.println(F("Setup Servo1 failed"));
    for(int i=0; i<5; i++)
    {
      digitalWrite(ledPin,HIGH);
      delay(200);
      digitalWrite(ledPin,LOW);
      delay(50);
    }
  }

  // Setup Digital I/O pins
  
  pinMode(currSensePin, INPUT_PULLUP);  // Use the internal pull-up, so 3.3V doesn't have to be routed on the board.
  pinMode(vacCntrlPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  // Set gate to closed position

  if ( servoIndex != -1)
  {
    gatePosition = CLOSED_GATE;
    ISR_Servo.setPosition(servoIndex, gatePosition);
    // waits 50 ms for the servo to reach the position
    delay(50);
  }

  // Optional functionalities of EspMQTTClient
  //espclient.enableDebuggingMessages(); // Enable debugging messages sent to serial output
  espclient.enableOTA(); // Enable OTA (Over The Air) updates. Password defaults to MQTTPassword. Port is the default OTA port. Can be overridden with enableOTA("password", port).

} // End of setup



void loop()
{
  espclient.loop();
  
  // Check if Tool is on by checking that digI/O from current sensor
  currSense = digitalRead(currSensePin);
  //Serial.print("Current Sense Input = ");
  //Serial.println(currSense);
  if (currSense == HIGH) // Current sense input is High, indicating current >= 1.5 A
    {
      // Restart the delay counters, since we are opening the gate and turning on the vacuum
      gateCounter = 0;
      vacCounter = 0;
      // Set servo to 105 deg position, fully opening the gate
      gatePosition = OPEN_GATE;
      ISR_Servo.setPosition(servoIndex, gatePosition);
      gateOpen = true;  // Set gate status
      // Turn on Optocoupler to signal vacuum to turn on
      digitalWrite(vacCntrlPin, HIGH);
      vacOn = true; // Set vacuum status

      tool.turn_on(espclient);
      Serial.println("ON");
    } // End if tool is on

  else  // Current sensor input is Lo, indicating the tool has been turned off
    {
      tool.turn_off(espclient);
      if (gateOpen) // Gate is still open from tool previously being on
        {
          if (gateCounter >= GATE_DELAY) // Check to see if the delay before closing the gate has been completed
            {
              gatePosition = CLOSED_GATE;
              gateOpen = false;
            } // End if gate delay counter has timed out

          else
            {
              gateCounter += 1;  // Increment the counter, we'll check it next loop
              //Serial.print("Gate Counter = "); Serial.println(gateCounter);
            }

        } // End if gate is open

      if(vacOn)   // Vacuum is still on from tool previously being on
        {
          if (vacCounter >= VAC_DELAY)  // Check to see if th edelay before turning off the vacuum has been completed
            {
              digitalWrite(vacCntrlPin, LOW);  // Turn off "run" signal to vacuum
              vacOn = false;
            } // End if vacuum delay counter has timed out

          else
            {
              vacCounter += 1;  // Increment the couner, we'll check it next loop
              //Serial.print("Vacuum Counter = "); Serial.println(vacCounter);
            }

        } // End if vacuum is on
      } // End else, tool has been turned off
  ISR_Servo.setPosition(servoIndex, gatePosition);  // Repeat command to servo position

  delay(100); // Delay 0.1 sec "tick"  before repeating the loop

  //  Blink the LED
  ledBlinkCounter += 1;   // Increment the Blink counter
  if (ledBlinkCounter >= LED_BLINK)
  {
    blinkLED();           // Toggle the LED output state
    ledBlinkCounter = 0;  // Reset the counter for the LED
  }

} // End of main loop()


void ask_anyone_open(){
  espclient.publish("tools/dust_collection", "anyone open?");
}

void answer_anyone_open(){
  String msg = String(TOOL_NAME) + " is open";
  espclient.publish("tools/dust_collection", msg);
}
