# mbparser
**mbparser** is a simple C++ library to read data from a modbus RTU slave and validate the crc.
This implementation uses a state machine to consume token by token.
Supports all modbus function codes. 

It is typical the core a modbus client implementation, which handles all the hardware and ambient stuff. 

Was original developed to read from a Eastron SDM72D-M Smartmeter. But can be used for any other device to parse its response.

Is natural evolution of my old mbutil library (which was C-Style but C++).

## Disclaimer
Uses C11 on ESP 8266. May not run with other arduino devices (not tested).


## Usage / Example

    //#include <mbparser>
    #include <Arduino.h>
    #include "mbparser.h"
    ModbusParser mbparser{1};

    void doRequest(){
        uint8_t request[8] = {0x01, 0x04, 0x00, 0x00, 0x00, 0x06, 0x70, 0x08};
        for (int i =0; i <8; i++) Serial.write(request[i]);
        Serial.flush();
        delay(100); // until response
    }

    void setup(){
        Serial.begin(9600); // slave
        Serial1.begin(9600); // debug interface
        doRequest();
    }

    void loop(){

        uint16_t status;
        // read one token
        if(Serial.available()){
            status = mbparser.parse(Serial.read());
        }
        if (status == PARSER_COMPLETE){
            uint8_t *payload = mbparser.payload();
            Serial1.print("Payload: ");
            for(int i=0; i<mbparser.byteCount(); i++) Serial1.print(payload[i], HEX);
            Serial1.print("\n");
            mbparser.free(); // important to free payload for next response
            doRequest();
        } else if (status == PARSER_ERROR){
            Serial1.print("ERROR: ");
            Serial1.print(mbparser.errorCode());
            Serial1.print("\n");
            mbparser.free(); // important to free payload for next response
            doRequest();
        }

    }

Instead of polling status of parser one could also set a callback. This is usefully when the overlying client is running async.

## todo
* modbus exception handling?