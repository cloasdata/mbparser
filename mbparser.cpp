// Implementation of ModbusParser class.
#include "mbparser.h"
#include "Stream.h"
#include <Arduino.h>


ModbusParser::ModbusParser(uint8_t slaveID) : _slaveID(slaveID) {}

/*
Parses the buffer until len.
Returns the current parser state.
Parser state and payload is only valid, when parser state is complete
In all other states class attributes may be inconsistent.
*/
ParserState ModbusParser::parse(uint8_t *buffer, uint16_t len) {
  uint16_t index = 0;

  // consume all provided tokens
  while (index < len) {
    _parse(buffer[index]);
    index++;
  }
  return currentState;
}

/*
Parse one token and render state machine.
Returns the current parser state.
Parser state and payload is only valid, when parser state is complete
In all other states class attributes may be inconsistent.
*/
ParserState ModbusParser::parse(uint8_t token){
  _parse(token);
  return currentState;
};


/*
Frees payload from memory.
*/
void ModbusParser::free() { _freePayload(); };

// Properties

/*
Sets on complete callback.
Is called when parser has finished one complete response frame.
*/
void ModbusParser::setOnCompleteCB(parserCallback cb){
  _onComplete=cb;
};

/*
Sets on error callback.
Is called when parser detects any error.
*/
void ModbusParser::setOnErrorCB(parserCallback cb){
  _onError=cb;
};


/*
Actual implementation of parse.
*/
void ModbusParser::_parse(uint8_t token) {
  _token = token;
  // indeed currentState is laststate until the machine is rendered.
  lastState = currentState;
  if (lastState == ParserState::complete || lastState == ParserState::error) {
    _reset();
  }
  _renderStateMachine();
  _handleCallbacks();
}

/*
Calls registered callbacks. 
*/
void ModbusParser::_handleCallbacks(){
  if (currentState == ParserState::complete){
    if (_onComplete) _onComplete(this);
  } else if (currentState == ParserState::error) {
    if (_onError) _onError(this);
  }
}

/*
State machine
*/
void ModbusParser::_renderStateMachine() {
  switch (currentState) {
  case ParserState::slaveAddress:
    _checkSlaveAddress();
    break;

  case ParserState::functionCode:
    _checkFunctionCode();
    break;

  case ParserState::byteCount:
    _checkByteCount();
    break;

  case ParserState::dataReceive:
    _receiveData();
    break;

  case ParserState::crcLow:
    _checkLowCRC();
    break;

  case ParserState::crcHigh:
    _checkHighCRC();
    break;

  default:
    break;
  }
}

// STATES

/*
Consumes tokens until slave address is found
*/
void ModbusParser::_checkSlaveAddress() {
  if (_token == _slaveID) {
    currentState = ParserState::functionCode;
    _bytesUntilComplete--;
    _renderCRC();
  } else {
    currentState = ParserState::slaveAddress;
  }
}

void ModbusParser::_checkFunctionCode() {
  if (_token > 128) {
    currentState = ParserState::error;
    _errorCode = ErrorCode::exception;
  }
  bool fcOkay = false;
  for (uint16_t idx = 0; idx < sizeof(_supportedFunctionCodes); idx++) {
    if (_token == _supportedFunctionCodes[idx]) {
      fcOkay = true;
      break;
    }
  }

  if (fcOkay) {
    _functionCode = _token;
    currentState = ParserState::byteCount;
    _bytesUntilComplete--;
    _renderCRC();
  } else {
    currentState = ParserState::error;
    _errorCode = ErrorCode::illegalFunction;
  }
}

void ModbusParser::_checkByteCount() {
  if (_token > 0) {
    _payloadSize = _token;
    _bytesUntilComplete += _payloadSize;
    _dataToReceive = _payloadSize;
    currentState = ParserState::dataReceive;
    _allocatePayload();
    _renderCRC();
  } else {
    currentState = ParserState::error;
    _errorCode = ErrorCode::illegalDataValue;
  }
}

void ModbusParser::_receiveData() {
  if (_reverse){
    _reverseCopyToken();
  } else {
    _copyToken();
  }
  _bytesUntilComplete--; 
  _dataToReceive--;
  if (_dataToReceive == 0) {
    currentState = ParserState::crcLow;
  }
  _renderCRC();
}

void ModbusParser::_copyToken(){
  *_payloadPtr++ = _token;
}

void ModbusParser::_reverseCopyToken(){
  _swappedBytes--;
  *_payloadPtr-- = _token;
  if (_swappedBytes <= 0){
    _payloadPtr += 2 * _registerSize;
    _swappedBytes = _registerSize;
  }
  
  
}

void ModbusParser::_checkLowCRC() {
  if (lowByte(_crc) == _token) {
    currentState = ParserState::crcHigh;
    _bytesUntilComplete--;
  } else {
    currentState = ParserState::error;
    _errorCode = ErrorCode::CRCError;
  }
}

void ModbusParser::_checkHighCRC() {
  if (highByte(_crc) == _token) {
    _bytesUntilComplete--;
    
    // proof of concept.
    if (!_bytesUntilComplete){
      currentState = ParserState::complete;
    } else {
      // this would mean that implementation is wrong.
      // because all other cases would cause a CRC error.
      currentState = ParserState::error;
      _errorCode = ErrorCode::illegalDataValue;
    }
  } else {
    currentState = ParserState::error;
    _errorCode = ErrorCode::CRCError;
  }
}

void ModbusParser::_reset() {
  _crc = 0xFFFF;
  _payloadSize = 0;
  _bytesUntilComplete = 4;
  _errorCode = ErrorCode::noError;
  currentState = ParserState::slaveAddress;
}

void ModbusParser::_renderCRC() {
  _crc ^= (uint16_t)_token;                // XOR byte into least sig. byte of crc
  for (int i = 8; i != 0; i--) { // Loop over each bit
    if ((_crc & 0x0001) != 0) {  // If the LSB is set
      _crc >>= 1;                // Shift right and XOR 0xA001
      _crc ^= 0xA001;
    } else        // Else LSB is not set
      _crc >>= 1; // Just shift right
  }
}

void ModbusParser::_allocatePayload() {
  if (_payloadArray!=nullptr) _freePayload(); // ultimate when user code doesn't do.
  _payloadArray = new uint8_t[_payloadSize];
  if (_reverse){
    _payloadPtr = _payloadArray + _registerSize-1;
    _swappedBytes = _registerSize;
  } else {
    _payloadPtr = _payloadArray;
  }
  }

void ModbusParser::_freePayload() { 
  if (_payloadArray != nullptr) {
    delete[] _payloadArray;
    _payloadArray = nullptr;
  }
  _payloadPtr = nullptr;
  }

