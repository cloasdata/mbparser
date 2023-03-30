#include "Arduino.h"
#include "mbparser.h"

// BIG ENDIAN
uint8_t GoodResponse[] {0x01, 0x03, 0x04, 0x0, 0x6, 0x0, 0x05, 0xDA, 0x31};
uint8_t BadResponseCRC[] {0x01, 0x03, 0x04, 0x0, 0x6,0x0, 0x05, 0xFF, 0x31};
uint8_t LongResponse[] {0x01, 0x04, 0x50, 0x40, 0x6A, 0x9F, 0xBE, 0x40, 0xF5, 0x4F, 0xDF, 0x41, 0x3A, 0xA7, 0xF0, 0x41, 0x7A, 0xA7, 0xF0, 0x41, 0x9D, 0x53, 0xF8, 0x41, 0xBD, 0x53, 0xF8, 0x41, 0xDD, 0x53, 0xF8, 0x41, 0xFD, 0x53, 0xF8, 0x42, 0x0E, 0xA9, 0xFC, 0x42, 0x1E, 0xA9, 0xFC, 0x42, 0x2E, 0xA9, 0xFC, 0x42, 0x3E, 0xA9, 0xFC, 0x42, 0x4E, 0xA9, 0xFC, 0x42, 0x5E, 0xA9, 0xFC, 0x42, 0x6E, 0xA9, 0xFC, 0x42, 0x7E, 0xA9, 0xFC, 0x42, 0x87, 0x54, 0xFE, 0x42, 0x8F, 0x54, 0xFE, 0x42, 0x97, 0x54, 0xFE, 0x42, 0x9F, 0x54, 0xFE, 0x11, 0x94, 0x01, 0x04, 0x54, 0x40, 0x6A, 0x9F, 0xBE, 0x40, 0xF5, 0x4F, 0xDF, 0x41, 0x3A, 0xA7, 0xF0, 0x41, 0x7A, 0xA7, 0xF0, 0x41, 0x9D, 0x53, 0xF8, 0x41, 0xBD, 0x53, 0xF8, 0x41, 0xDD, 0x53, 0xF8, 0x41, 0xFD, 0x53, 0xF8, 0x42, 0x0E, 0xA9, 0xFC, 0x42, 0x1E, 0xA9, 0xFC, 0x42, 0x2E, 0xA9, 0xFC, 0x42, 0x3E, 0xA9, 0xFC, 0x42, 0x4E, 0xA9, 0xFC, 0x42, 0x5E, 0xA9, 0xFC, 0x42, 0x6E, 0xA9, 0xFC, 0x42, 0x7E, 0xA9, 0xFC, 0x42, 0x87, 0x54, 0xFE, 0x42, 0x8F, 0x54, 0xFE, 0x42, 0x97, 0x54, 0xFE, 0x42, 0x9F, 0x54, 0xFE, 0x42, 0xA7, 0x54, 0xFE, 0x0A, 0xE9};
uint8_t ExceptionResponse [] {0x01, 0x82, 0x02};

uint8_t GoodRequest[] {0x01, 0x04, 0x01, 0x31, 0x0, 0x01E, 0x20, 0x31};
uint8_t BadRequestCRC[] {0x01, 0x04, 0x01, 0x31, 0x0, 0x01E, 0x20, 0xFF};



// Basic tests
void GivenGoodResponse_WhenParsed_ReturnComplete(){
    ResponseParser parser{};
    parser.setSlaveID(1);

    auto status = parser.parse(GoodResponse, 9);
    
    assert(status == ParserState::complete);
}

void GivenBadResponse_WhenParsed_ReturnError(){
    ResponseParser parser{};
    parser.setSlaveID(1);
    ParserState status{ParserState::slaveAddress}; 
    while (status != ParserState::error){
        status = parser.parse(BadResponseCRC, 9);
    }

    assert(status == ParserState::error);
    assert(parser.errorCode() == ErrorCode::CRCError);
}

void GivenGoodResponse_WhenParsed_ReturnProperties(){
    ResponseParser parser{};
    parser.setSlaveID(1);

    parser.parse(GoodResponse, 9);
    
    assert(parser.functionCode() == 0x03);
    assert(parser.byteCount() == 0x04);
    assert(parser.crcBytes() == 0xDA31);
    assert(parser.payload());
}

void GivenGoodRequest_WhenParsed_ReturnComplete(){
    RequestParser parser{};
    parser.setSlaveID(1);

    auto status = parser.parse(GoodRequest, 8);
    
    assert(status == ParserState::complete);
}

void GivenBadRequest_WhenParsed_ReturnComplete(){
    RequestParser parser{};
    parser.setSlaveID(1);

    auto status = parser.parse(BadRequestCRC, 8);
    
    assert(status == ParserState::error);
    assert(parser.errorCode() == ErrorCode::CRCError);
}


void GivenGoodRequest_WhenParsed_ReturnProperties(){
    RequestParser parser{};
    parser.setSlaveID(1);

    parser.parse(GoodRequest, 8);
    
    assert(parser.functionCode() == 0x04);
    assert(parser.offset() == 305);
    assert(parser.quantity() == 30);
    assert(parser.crcBytes() == 0x2031);

}

void GivenGoodRequest_WhenParsed_CallLambda(){
    RequestParser parser{};
    parser.setSlaveID(1);
    parser.setOnCompleteCB([](RequestParser *parser){
        assert(parser->functionCode() == 0x04);
        assert(parser->offset() == 305);
        assert(parser->quantity() == 30);
        assert(parser->crcBytes() == 0x2031);
        assert(parser->state() == ParserState::complete);
    });

    auto status = parser.parse(GoodRequest, 8);
    assert(status == ParserState::complete);
}

void GivenBadRequest_WhenParsed_CallLambda(){
    RequestParser parser{};
    parser.setSlaveID(1);
    parser.setOnErrorCB([](RequestParser *parser){
        assert(parser->state() == ParserState::error);
        assert(parser->errorCode() == ErrorCode::CRCError);

    });

    auto status = parser.parse(BadRequestCRC, 8);
    
    assert(status == ParserState::error);
}

void GivenExceptionResponse_WhenParsed_CallError(){
    ResponseParser parser{};
    parser.setSlaveID(1);
    parser.setOnErrorCB([](ResponseParser *parser){
        assert(parser->state() == ParserState::error);
        assert(parser->errorCode() == ErrorCode::illegalDataAddress);

    });

    auto status = parser.parse(ExceptionResponse, 3);
    assert(status == ParserState::error);
}

void test_mbparser(){
    
    printf("\n\n -- TEST STARTING -- \n\n");
    unsigned long runningTime = millis();
    GivenGoodResponse_WhenParsed_ReturnComplete();
    printf(".");
    GivenBadResponse_WhenParsed_ReturnError();
    printf(".");
    GivenGoodResponse_WhenParsed_ReturnProperties();
    printf(".");
    GivenGoodRequest_WhenParsed_ReturnComplete();
    printf(".");
    GivenBadRequest_WhenParsed_ReturnComplete();
    printf(".");
    GivenGoodRequest_WhenParsed_ReturnProperties();
    printf(".");
    GivenGoodRequest_WhenParsed_CallLambda();
    printf(".");
    GivenBadRequest_WhenParsed_CallLambda();
    printf(".");
    GivenExceptionResponse_WhenParsed_CallError();
    printf(".");
    runningTime = millis() - runningTime;
    printf("\nTime: %lu\n", runningTime);
    printf("TEST DONE. BYE");
    
    delay(1000);
    ESP.restart();
}


void profile_throughput_small(){
    Serial.print("\n\n");
    ESP.wdtDisable();
    const uint32_t repeats{100000};
    ResponseParser parser{};
    parser.setSlaveID(1);
    
    unsigned long time = millis();
    for (unsigned long i = 0; i < repeats; i++){
        parser.parse(GoodResponse, 9);
    }
    time = millis() - time;
    float tp = repeats / 1000 * 9;
    tp /= time/1000;
    Serial.printf("Time took: %lu ms\n", time);
    Serial.printf("Throughput: %.2f mb/s\n", tp);

}

void profile_throughput_large(){
    ESP.wdtDisable();
    uint16_t len = sizeof(LongResponse);
    Serial.print("\n\n");
    const uint32_t repeats{10000};
    ResponseParser parser{};
    parser.setSlaveID(1);
    unsigned long time = millis();
    for (unsigned long i = 0; i < repeats; i++){
        parser.parse(LongResponse, len);
    }
    time = millis() - time;
    float tp = (repeats / 1000) * len; // kb
    tp /= 1000; // mb
    tp /= time/1000; // s
    Serial.printf("Time took: %lu ms\n", time);
    Serial.printf("Throughput: %.2f mb/s\n", tp);

}

void profiling(){
    profile_throughput_small();
    profile_throughput_large();
    ESP.restart();
}