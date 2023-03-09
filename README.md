# mbparser
**mbparser** is a simple but modern C++ library to read data from a modbus RTU master or slave and validate the crc.
The Modbus Protocol is handled via state machine. 

Mbparser classes are typically wrapped into a modbus client or server (master or slave) implementation, which handles all the hardware and ambient stuff. 

Was original developed to read from a Eastron SDM72D-M Smartmeter. But can be used for any other device to parse its response.

## Features
* Supports all functions codes
* Translates response and requests to C++ interfaces
* Can be polled
* Callback on response complete or error
* Can change on fly endianness.
* Has less than 500 lines of code.
* Partly test driven development.

## Performance
Profiling on a ESP8266 with 60 MHz gives a parser throughput of 0.5 megabyte per second. That should be far more than typical a modbus network can achieve through RTU (RS485) or even on TCP/IP.
Profiling can be found in test section of the source code. 

## Disclaimer
* Uses C++11 lambdas for callback. 
* Developed on ESP8266 little Endian machine. Uses machine depend unions for byte conversion. 
* May not run with other arduino devices (not tested).

## Usage / Example
First example explains a very simple usage
```C++
    #include <Arduino.h>
    #include "mbparser.h"
    
    ResponseParser responseParser{};

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

        ParserState status;
        // read one token
        if(Serial.available()){
            status = responseParser.parse(Serial.read());
        }
        if (status == ParserState::complete){
            uint8_t *payload = responseParser.payload();
            Serial1.print("Payload: ");
            for(int i=0; i<responseParser.byteCount(); i++) Serial1.print(payload[i], HEX);
            Serial1.print("\n");
            doRequest();
        } else if (status == ParserState::error){
            Serial1.print("ERROR: ");
            Serial1.print(static_cast<int>(responseParser.errorCode()));
            Serial1.print("\n");
            doRequest();
        }

    }

```

Instead of polling one could use callbacks to handle the response/request

## Todo
* Test on Big Endian Machine.