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


// initializations for the ISR_SERVO functions

#ifndef ESP8266
  #error This code is designed to run on ESP8266 platform, not Arduino nor ESP32! Please check your Tools->Board setting.
#endif


// Turn-off timer values, in 100 msec "ticks"
#define   VAC_DELAY     2000 //  Test 300 ticks, 30 sec    3000    // 3000 ticks, 300 sec (5 min) delay to turn off vacuum after tool turns off
#define   LED_BLINK       5 //  5 ticks, 0.5 sec to blink the LED


bool vacOn = false;
int vacCounter = 0;
const int vacCntrlPin = D2;
int vacCntrl = 0;
const int ledPin = D4;
int ledState = 0;
int ledBlinkCounter = 0;
int tool_on_counter = 0;


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


void onMessageReceived(const String& message) {
    Serial.println("message received from test/mytopic: " + message);

    if (message.indexOf("ON")){
        digitalWrite(vacCntrlPin, HIGH);
        vacOn = true;
        tool_on_counter += 1;
    }
    else{
        vacOn = false;
        tool_on_counter -= 1;
    }

    if (tool_on_counter == 0){
        vacCounter = millis(); // no tools on, start countdown
    }
}


void onConnectionEstablished()
{
    espclient.publish("tools/dust_collection", "Dust Collector connected");

    espclient.subscribe("tools/dust_collection", onMessageReceived);
}


void setup()
{
  pinMode(D3, INPUT_PULLUP); // for testing Adam:

  Serial.begin(115200);
  while (!Serial);

  pinMode(vacCntrlPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
}


void loop()
{
  espclient.loop();

  if(millis() > vacCounter + VAC_DELAY){
      digitalWrite(vacCntrlPin, LOW);
  }

  if((millis() - vacCounter) > 1,000,000){
      digitalWrite(vacCntrlPin, LOW); // turn off every 20 minutes as fail-safe
  }
}
