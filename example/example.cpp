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
            uint8_t *payload = responseParser.data();
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