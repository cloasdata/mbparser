/*
Declaration of ModbusParser class. 
Properties are defined here

Uses to enums to control state and describe errors.
*/
#ifndef mbparser_h
#define  mbparser_h
#include <Arduino.h>

enum _State{
    error,
    slaveAddress,
    functionCode,
    byteCount,
    dataReceive,
    crcLow,
    crcHigh,
    complete
};

enum _ErrorCode{
    noError = 0,
    illegalFunction = 1,
    illegalDataValue = 3,
    deviceFailure = 4,
    CRCError = 21,
    exception = 128
};

#define PARSER_COMPLETE _State::complete
#define PARSER_ERROR _State::error

class ModbusParser;

typedef std::function<void(ModbusParser *parser)> parserCallback;

class ModbusParser{
  public:
    ModbusParser(uint8_t slaveID);
    uint16_t parse(uint8_t *buffer, uint16_t len);
    uint16_t parse(uint8_t token);
    void free();

    //properties
    uint8_t functionCode(){return _functionCode;};
    uint16_t byteCount(){return _payloadSize;};
    uint16_t crcBytes(){return _crc;};
    uint16_t errorCode(){return _errorCode;};
    uint8_t *payload(){return _payloadArray;};
    uint16_t untilComplete(){return _bytesUntilComplete;};
    void setOnCompleteCB(parserCallback cb);
    void setOnErrorCB(parserCallback cb);
    void setSwap(bool swap){_reverse = true;};
    void setRegisterSize(uint16_t size){_registerSize=size;};
    void setSlaveID(uint8_t id){_slaveID = id;};

  private:
    const uint16_t _supportedFunctionCodes[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x15, 0x16};
    parserCallback _onComplete = nullptr;
    parserCallback _onError = nullptr;
    
    

    uint8_t _token;
    
    _State lastState = _State::slaveAddress;
    _State currentState = _State::slaveAddress;
    _ErrorCode _errorCode = _ErrorCode::noError;
    
    uint8_t _slaveID;
    uint8_t _functionCode;
    uint16_t _payloadSize;
    uint16_t _dataToReceive;
    uint8_t *_payloadArray = nullptr;
    uint8_t *_payloadPtr = nullptr;
    uint16_t _crc = 0xFFFF;
    uint16_t _bytesUntilComplete = 4; // at least
  	
    bool _reverse = false;
    uint16_t _registerSize;
    uint16_t _swappedBytes;

    void _parse(uint8_t token);

    void _renderStateMachine();
    void _checkSlaveAddress();
    void _checkFunctionCode();
    void _checkByteCount();
    void _receiveData();
    void _copyToken();
    void _reverseCopyToken();
    void _checkLowCRC();
    void _checkHighCRC();
  
    void _reset();
    void _renderCRC();
    void _allocatePayload();
    void _freePayload();
};

#endif