# mbparser
**mbparser** is a simple but modern C++ library to parse from a modbus RTU master or slave.
The Modbus Protocol is handled via finite state machine pattern. This makes it very easy to debug or validate the communication partner. 
A minimal modbus slave can be implemented in less than 5 lines. 

mbParser classes are typically wrapped into a modbus client or server (master or slave) implementation, which handles all the hardware and ambient stuff. 

## Features
* Simple and expressive API.
* Memory Footprint:
  * Response 164 bytes on stack + payload size on heap.
  * Request 152 bytes on stack.
* Supports functions codes: 01, 02, 03, 04, 05, 06, 15, 16
* Maps modbus responses and requests to C++ interfaces
* State machine can be polled or
* Callbacks can be set for on complete and on error events.
* Can change on fly endianness.
* Has less than 1.000 loc.
* Partly test driven development.
* ModbusParser Base Class can be extended for particular user solutions. For exampling including payload handling on byte level within the state machine
* Uses old style C++ memory allocation via new to handle non deterministic payload of response frame
* Header File only library. As most of the code is implemented in a template class, the child classes are also defined in the header. 

## Flags
To run the package with std functional library i.e. lambdas set compiler flag -D STD_FUNCTIONAL
This flag shall not be set when AVR or other non std conforming compilers are used.

## Performance
Profiling on a ESP8266 with 60 MHz gives a parser throughput of 0.5 - 0.6 megabyte per second. That should be far more than typical a modbus network can achieve through RTU (RS485) or even on TCP/IP.
Profiling can be found in test section of the source code. 

## Disclaimer
* C++11 
* Developed on ESP8266 little Endian machine. 
* Uses machine depend unions for byte conversion. 
* May not run with other arduino devices (not tested) or on other machines (not tested).
* Was original developed to read from a Eastron SDM72D-M Smartmeter. But can be used for any other device to parse its response.

## Usage / Example
Belows example parses a response from a modbus slave on slave id 1.
User code should typical transfer (copy) the payload to the desired format/type.
Important to know is that parser.payload() is only valid during when parse is complete until user frees 
or new payload is allocated. 
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

Instead of polling parsers state one could use callbacks to handle the response/request.
Next example demonstrates a simple modbus slave on id 1. On request complete the slave will send a 174 byte long response to the master. 

```C++
    #include <Arduino.h>
    #include "mbparser.h"
    
    RequestParser responseParser{};
    const uint16_t lenResponse = 174;
    const uint8_t LongResponse[lenResponse] {
        0x01, 0x04, 0x50, 0x40, 0x6A, 0x9F, 0xBE, 0x40, 0xF5, 0x4F, 0xDF, 0x41, 0x3A, 0xA7, 0xF0, 0x41, 0x7A, 0xA7, 0xF0, 0x41, 0x9D, 0x53, 0xF8, 0x41, 0xBD, 0x53, 0xF8, 0x41, 0xDD, 0x53, 0xF8, 0x41, 0xFD, 0x53, 0xF8, 0x42, 0x0E, 0xA9, 0xFC, 0x42, 0x1E, 0xA9, 0xFC, 0x42, 0x2E, 0xA9, 0xFC, 0x42, 0x3E, 0xA9, 0xFC, 0x42, 0x4E, 0xA9, 0xFC, 0x42, 0x5E, 0xA9, 0xFC, 0x42, 0x6E, 0xA9, 0xFC, 0x42, 0x7E, 0xA9, 0xFC, 0x42, 0x87, 0x54, 0xFE, 0x42, 0x8F, 0x54, 0xFE, 0x42, 0x97, 0x54, 0xFE, 0x42, 0x9F, 0x54, 0xFE, 0x11, 0x94, 0x01, 0x04, 0x54, 0x40, 0x6A, 0x9F, 0xBE, 0x40, 0xF5, 0x4F, 0xDF, 0x41, 0x3A, 0xA7, 0xF0, 0x41, 0x7A, 0xA7, 0xF0, 0x41, 0x9D, 0x53, 0xF8, 0x41, 0xBD, 0x53, 0xF8, 0x41, 0xDD, 0x53, 0xF8, 0x41, 0xFD, 0x53, 0xF8, 0x42, 0x0E, 0xA9, 0xFC, 0x42, 0x1E, 0xA9, 0xFC, 0x42, 0x2E, 0xA9, 0xFC, 0x42, 0x3E, 0xA9, 0xFC, 0x42, 0x4E, 0xA9, 0xFC, 0x42, 0x5E, 0xA9, 0xFC, 0x42, 0x6E, 0xA9, 0xFC, 0x42, 0x7E, 0xA9, 0xFC, 0x42, 0x87, 0x54, 0xFE, 0x42, 0x8F, 0x54, 0xFE, 0x42, 0x97, 0x54, 0xFE, 0x42, 0x9F, 0x54, 0xFE, 0x42, 0xA7, 0x54, 0xFE, 0x0A, 0xE9
    };

    void send_response(){
        for (uint8_t b : LongResponse){
            Serial.write(b);
        }
    }

    void handleRequest(RequestParser *request){
        if (request->functionCode() == 0x04 && request->quantity()==40){
            send_response();
        } /* else ignore this frame*/
    }

    void setup(){
        Serial.begin(9600); // slave
        Serial1.begin(9600); // debug interface
        responseParser.setSlaveAddress(1);
        responseParser.setOnCompleteCB(handleRequest);
    }

    void loop(){
        while (Serial.available()){
            // parse as many as possible
            responseParser.parse(Serial.read());
        }
    }
```

## Todo
* Test on Big Endian Machine.
* Add more bad responses/requests to tests. 