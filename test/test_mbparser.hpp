#include "Arduino.h"
#include "mbparser.h"

// BIG ENDIAN
uint8_t GoodResponse03[] {0x01, 0x03, 0x04, 0x0, 0x6, 0x0, 0x05, 0xDA, 0x31};
uint8_t LongResponse04[] {0x01, 0x04, 0x50, 0x40, 0x6A, 0x9F, 0xBE, 0x40, 0xF5, 0x4F, 0xDF, 0x41, 0x3A, 0xA7, 0xF0, 0x41, 0x7A, 0xA7, 0xF0, 0x41, 0x9D, 0x53, 0xF8, 0x41, 0xBD, 0x53, 0xF8, 0x41, 0xDD, 0x53, 0xF8, 0x41, 0xFD, 0x53, 0xF8, 0x42, 0x0E, 0xA9, 0xFC, 0x42, 0x1E, 0xA9, 0xFC, 0x42, 0x2E, 0xA9, 0xFC, 0x42, 0x3E, 0xA9, 0xFC, 0x42, 0x4E, 0xA9, 0xFC, 0x42, 0x5E, 0xA9, 0xFC, 0x42, 0x6E, 0xA9, 0xFC, 0x42, 0x7E, 0xA9, 0xFC, 0x42, 0x87, 0x54, 0xFE, 0x42, 0x8F, 0x54, 0xFE, 0x42, 0x97, 0x54, 0xFE, 0x42, 0x9F, 0x54, 0xFE, 0x11, 0x94, 0x01, 0x04, 0x54, 0x40, 0x6A, 0x9F, 0xBE, 0x40, 0xF5, 0x4F, 0xDF, 0x41, 0x3A, 0xA7, 0xF0, 0x41, 0x7A, 0xA7, 0xF0, 0x41, 0x9D, 0x53, 0xF8, 0x41, 0xBD, 0x53, 0xF8, 0x41, 0xDD, 0x53, 0xF8, 0x41, 0xFD, 0x53, 0xF8, 0x42, 0x0E, 0xA9, 0xFC, 0x42, 0x1E, 0xA9, 0xFC, 0x42, 0x2E, 0xA9, 0xFC, 0x42, 0x3E, 0xA9, 0xFC, 0x42, 0x4E, 0xA9, 0xFC, 0x42, 0x5E, 0xA9, 0xFC, 0x42, 0x6E, 0xA9, 0xFC, 0x42, 0x7E, 0xA9, 0xFC, 0x42, 0x87, 0x54, 0xFE, 0x42, 0x8F, 0x54, 0xFE, 0x42, 0x97, 0x54, 0xFE, 0x42, 0x9F, 0x54, 0xFE, 0x42, 0xA7, 0x54, 0xFE, 0x0A, 0xE9};
uint8_t Response06[] {0x11, 0x06, 0x00, 0x01, 0x00, 0x03, 0x9A, 0x9B};
uint8_t Response15[] {0x11, 0x10, 0x00, 0x01, 0x00, 0x02, 0x12, 0x98};

uint8_t BadResponseCRC03[] {0x01, 0x03, 0x04, 0x0, 0x6,0x0, 0x05, 0xFF, 0x31};

uint8_t ExceptionResponse [] {0x01, 0x82, 0x02};

uint8_t ReadRequest01[] {0x01, 0x01, 0x00, 0x0A, 0x00, 0x0D, 0xDD, 0xCD};
uint8_t ReadRequest04[] {0x01, 0x04, 0x01, 0x31, 0x0, 0x01E, 0x20, 0x31};

uint8_t WriteRequest05[] {0x01, 0x05, 0x00, 0xAC, 0xFF, 0x00, 0x4C, 0x1B};
uint8_t WriteRequest15[] {0x01, 0x0F, 0x00, 0x13, 0x00, 0x0A, 0x02, 0xCD, 0x01, 0x72, 0xCB};
uint8_t WriteRequest16[] {0x01, 0x10, 0x00, 0x01, 0x00, 0x02, 0x04, 0x00, 0x0A, 0x01, 0x02, 0x92, 0x30};

uint8_t BadCRCRequest04[] {0x01, 0x04, 0x01, 0x31, 0x0, 0x01E, 0x20, 0xFF};

// Basic tests
void GivenGoodResponse_WhenParsed_ReturnComplete(){
    ResponseParser parser{};
    parser.setSlaveAddress(1);

    auto status = parser.parse(GoodResponse03, 9);
    assert(status == ParserState::complete);
}

void GivenBadResponse_WhenParsed_ReturnError(){
    ResponseParser parser{};
    parser.setSlaveAddress(1);
    ParserState status{ParserState::slaveAddress}; 
    while (status != ParserState::error){
        status = parser.parse(BadResponseCRC03, 9);
    }

    assert(status == ParserState::error);
    assert(parser.errorCode() == ErrorCode::CRCError);
}

void GivenGoodResponse_WhenParsed_ReturnProperties(){
    ResponseParser parser{};
    parser.setSlaveAddress(1);

    parser.parse(GoodResponse03, 9);
    
    assert(parser.functionCode() == 0x03);
    assert(parser.byteCount() == 0x04);
    assert(parser.crcBytes() == 0xDA31);
    assert(parser.dataToReceive() == 0);
    assert(parser.data()[0] == 0x00);
}

void GivenGoodRequest_WhenParsed_ReturnComplete(){
    RequestParser parser{};
    parser.setSlaveAddress(1);

    auto status = parser.parse(ReadRequest04, 8);
    assert(status == ParserState::complete);
}

void GivenBadRequest_WhenParsed_ReturnComplete(){
    RequestParser parser{};
    parser.setSlaveAddress(1);

    auto status = parser.parse(BadCRCRequest04, 8);
    
    assert(status == ParserState::error);
    assert(parser.errorCode() == ErrorCode::CRCError);
}


void GivenGoodRequest_WhenParsed_ReturnProperties(){
    RequestParser parser{};
    parser.setSlaveAddress(1);

    parser.parse(ReadRequest04, 8);
    
    assert(parser.functionCode() == 0x04);
    assert(parser.address() == 305);
    assert(parser.quantity() == 30);
    assert(parser.crcBytes() == 0x2031);

}

void GivenGoodRequest_WhenParsed_CallLambda(){
    RequestParser parser{};
    parser.setSlaveAddress(1);
    parser.setOnCompleteCB([](RequestParser *parser){
        assert(parser->functionCode() == 0x04);
        assert(parser->address() == 305);
        assert(parser->quantity() == 30);
        assert(parser->crcBytes() == 0x2031);
        assert(parser->state() == ParserState::complete);
    });

    auto status = parser.parse(ReadRequest04, 8);
    assert(status == ParserState::complete);
}

void GivenBadRequest_WhenParsed_CallLambda(){
    RequestParser parser{};
    parser.setSlaveAddress(1);
    parser.setOnErrorCB([](RequestParser *parser){
        assert(parser->state() == ParserState::error);
        assert(parser->errorCode() == ErrorCode::CRCError);

    });

    auto status = parser.parse(BadCRCRequest04, 8);
    
    assert(status == ParserState::error);
}

void GivenExceptionResponse_WhenParsed_CallError(){
    ResponseParser parser{};
    parser.setSlaveAddress(1);
    parser.setOnErrorCB([](ResponseParser *parser){
        assert(parser->state() == ParserState::error);
        assert(parser->errorCode() == ErrorCode::illegalDataAddress);

    });

    auto status = parser.parse(ExceptionResponse, 3);
    assert(status == ParserState::error);
}

void GivenReadRequest01_WhenParsed_ReturnProperties(){
    RequestParser parser{};
    parser.setSlaveAddress(1);
    parser.setOnCompleteCB([](RequestParser *parser){
        assert(parser->functionCode() == 0x01);
        assert(parser->address() == 0x000A);
        assert(parser->quantity() == 0x000D);
        assert(parser->crcBytes() == 0xDDCD);
        assert(parser->dataToReceive() == 0);
        assert(parser->state() == ParserState::complete);
    });

    auto status = parser.parse(ReadRequest01, 8);
    assert(status == ParserState::complete);
}

void GivenWriteRequest05_WhenParsed_ReturnProperties(){
    RequestParser parser{};
    parser.setSlaveAddress(1);
    parser.setOnCompleteCB([](RequestParser *parser){
        assert(parser->functionCode() == 0x05);
        assert(parser->address() == 0x00AC);
        assert(parser->data());
        assert(parser->crcBytes() == 0x4C1B);
        assert(parser->dataToReceive() == 0);
        assert(parser->state() == ParserState::complete);
    });

    auto status = parser.parse(WriteRequest05, 8);
    assert(status == ParserState::complete);
}

void GivenWriteRequest15_WhenParsed_ReturnProperties(){
    RequestParser parser{};
    parser.setSlaveAddress(1);
    parser.setOnCompleteCB([](RequestParser *parser){
        assert(parser->functionCode() == 0x0F);
        assert(parser->address() == 0x0013);
        assert(parser->data()[0] == 0xCD);
        assert(parser->crcBytes() == 0x72CB);
        assert(parser->byteCount() == 2);
        assert(parser->dataToReceive() == 0);
        assert(parser->state() == ParserState::complete);
    });

    auto status = parser.parse(WriteRequest15, 11);
    assert(status == ParserState::complete);
}

void GivenWriteRequest16_WhenParsed_ReturnProperties(){
    RequestParser parser{};
    parser.setSlaveAddress(1);
    parser.setOnCompleteCB([](RequestParser *parser){
        assert(parser->functionCode() == 0x10);
        assert(parser->address() == 0x0001);
        assert(parser->data()[1] == 0x0A);
        assert(parser->crcBytes() == 0x9230);
        assert(parser->byteCount() == 4);
        assert(parser->dataToReceive() == 0);
        assert(parser->state() == ParserState::complete);
    });

    auto status = parser.parse(WriteRequest16, 13);
    assert(status == ParserState::complete);
}

void GivenWriteRequest16_WhenSlaveAdr0_ReturnProperties(){
    RequestParser parser{};
    parser.setSlaveAddress(0);
    parser.setOnCompleteCB([](RequestParser *parser){
        assert(parser->slaveAddress() == 0x01);
        assert(parser->functionCode() == 0x10);
        assert(parser->address() == 0x0001);
        assert(parser->data()[1] == 0x0A);
        assert(parser->crcBytes() == 0x9230);
        assert(parser->byteCount() == 4);
        assert(parser->dataToReceive() == 0);
        assert(parser->state() == ParserState::complete);
    });

    auto status = parser.parse(WriteRequest16, 13);
    assert(status == ParserState::complete);
}

void GivenResponse06_WhenParsed_ReturnProperties(){
    ResponseParser parser{};
    parser.setSlaveAddress(0);
    parser.setOnCompleteCB([](ResponseParser *parser){
        assert(parser->slaveAddress() == 0x11);
        assert(parser->functionCode() == 0x06);
        assert(parser->address() == 0x0001);
        assert(parser->data()[1] == 0x03);
        assert(parser->crcBytes() == 0x9A9B);
        assert(parser->state() == ParserState::complete);
    });

    auto status = parser.parse(Response06, 8);
    assert(status == ParserState::complete);
}

void GivenResponse15_WhenParsed_ReturnProperties(){
    ResponseParser parser{};
    parser.setSlaveAddress(0);
    parser.setOnCompleteCB([](ResponseParser *parser){
        assert(parser->slaveAddress() == 0x11);
        assert(parser->functionCode() == 0x10);
        assert(parser->address() == 0x0001);
        assert(parser->quantity() == 0x0002);
        assert(parser->crcBytes() == 0x1298);
        assert(parser->state() == ParserState::complete);
    });

    auto status = parser.parse(Response15, 8);
    assert(status == ParserState::complete);
}

// Profile tests
void profile_throughput_small(){
    Serial.print("\n\n");
    ESP.wdtDisable();
    const uint32_t repeats{100000};
    ResponseParser parser{};
    parser.setSlaveAddress(1);
    
    unsigned long time = millis();
    for (unsigned long i = 0; i < repeats; i++){
        parser.parse(GoodResponse03, 9);
    }
    time = millis() - time;
    float tp = repeats / 1000 * 9;
    tp /= 1000;
    tp /= time/1000;
    Serial.printf("Time took: %lu ms\n", time);
    Serial.printf("Throughput: %.2f mb/s\n", tp);
}

void profile_throughput_large(){
    ESP.wdtDisable();
    uint16_t len = sizeof(LongResponse04);
    Serial.print("\n\n");
    const uint32_t repeats{10000};
    ResponseParser parser{};
    parser.setSlaveAddress(1);
    unsigned long time = millis();
    for (unsigned long i = 0; i < repeats; i++){
        parser.parse(LongResponse04, len);
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
}


void test_mbparser(){
    
    printf("\n\n -- TEST STARTING -- \n\n");
    unsigned long runningTime = millis();
    uint32_t heapSize = ESP.getFreeHeap();
    GivenResponse15_WhenParsed_ReturnProperties();
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
    GivenReadRequest01_WhenParsed_ReturnProperties();
    printf(".");
    GivenWriteRequest05_WhenParsed_ReturnProperties();
    printf(".");
    GivenWriteRequest15_WhenParsed_ReturnProperties();
    printf(".");
    GivenWriteRequest16_WhenParsed_ReturnProperties();
    printf(".");
    GivenWriteRequest16_WhenSlaveAdr0_ReturnProperties();
    printf(".");
    GivenResponse06_WhenParsed_ReturnProperties();
    printf(".");
    printf(".");
    heapSize -= ESP.getFreeHeap();
    if (heapSize >0){
        printf("Memory Leak: %d bytes\n", heapSize);
    }
    runningTime = millis() - runningTime;
    printf("\nTime: %lu\n", runningTime);
    printf("TEST DONE.");
    profiling();
    
    ESP.restart();
}