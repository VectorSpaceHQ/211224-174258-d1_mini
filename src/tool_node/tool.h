#include "EspMQTTClient.h"

class Tool{
public:
    int PIN;
    String name;
    bool tool_state = false;
    bool old_tool_state = false;
    String message;

    unsigned long currentTime;
    unsigned long cloopTime = millis();

    void set_pin(int pin){
        PIN = pin;
        pinMode(pin, INPUT_PULLUP);
    }

    void turn_on(EspMQTTClient &clt){
        tool_state = true;
        String tool_state_str = "OFF";
        if (tool_state == true){
          tool_state_str = "ON";
        }
        message = name + ", " + tool_state_str;
        if (tool_state != old_tool_state){
            Serial.println("tool state has changed");
            clt.publish("tools/dust_collection", message);
            old_tool_state = tool_state;
        }
    }
    
    void turn_off(EspMQTTClient &clt){
        tool_state = false;
        String tool_state_str = "OFF";
        if (tool_state == true){
          tool_state_str = "ON";
        }
        message = name + ", " + tool_state_str;
        if (tool_state != old_tool_state){
            Serial.println("tool state has changed");
            clt.publish("tools/dust_collection", message);
            old_tool_state = tool_state;
        }
    }

    bool get_state(){
        Serial.println(tool_state);
        return tool_state;
    }
};
