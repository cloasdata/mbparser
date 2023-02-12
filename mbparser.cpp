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
uint16_t ModbusParser::parse(uint8_t *buffer, uint16_t len) {
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
uint16_t ModbusParser::parse(uint8_t token){
  _parse(token);
  if (currentState == _State::complete){
    if (_onComplete) _onComplete(this);
  } else if (currentState == _State::error) {
    if (_onError) _onError(this);
  }
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
Actual implementation of parse
*/
void ModbusParser::_parse(uint8_t token) {
  _token = token;
  // indeed currentState is laststate until the machine is rendered.
  lastState = currentState;
  if (lastState == _State::complete || lastState == _State::error) {
    _reset();
  }
  _renderStateMachine();

}

/*
State machine
*/
void ModbusParser::_renderStateMachine() {
  switch (currentState) {
  case _State::slaveAddress:
    _checkSlaveAddress();
    break;

  case _State::functionCode:
    _checkFunctionCode();
    break;

  case _State::byteCount:
    _checkByteCount();
    break;

  case _State::dataReceive:
    _receiveData();
    break;

  case _State::crcLow:
    _checkLowCRC();
    break;

  case _State::crcHigh:
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
    currentState = _State::functionCode;
    _bytesUntilComplete--;
    _renderCRC();
  } else {
    currentState = _State::slaveAddress;
  }
}

void ModbusParser::_checkFunctionCode() {
  if (_token > 128) {
    currentState = _State::error;
    _errorCode = _ErrorCode::exception;
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
    currentState = _State::byteCount;
    _bytesUntilComplete--;
    _renderCRC();
  } else {
    currentState = _State::error;
    _errorCode = _ErrorCode::illegalFunction;
  }
}

void ModbusParser::_checkByteCount() {
  if (_token > 0) {
    _payloadSize = _token;
    _bytesUntilComplete += _payloadSize;
    _dataToReceive = _payloadSize;
    currentState = _State::dataReceive;
    _allocatePayload();
    _renderCRC();
  } else {
    currentState = _State::error;
    _errorCode = _ErrorCode::illegalDataValue;
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
    currentState = _State::crcLow;
  }
  _renderCRC();
}

void ModbusParser::_copyToken(){
  *_payloadPtr++ = _token;
}

void ModbusParser::_reverseCopyToken(){
  Serial1.printf("pl idx: %d\n", _payloadPtr - _payloadArray);
  _swappedBytes--;
  *_payloadPtr-- = _token;
  if (_swappedBytes <= 0){
    _payloadPtr += 2 * _registerSize;
    _swappedBytes = _registerSize;
    Serial1.printf("ptr: %p\n", _payloadPtr);
  }
  
  
}

void ModbusParser::_checkLowCRC() {
  if (lowByte(_crc) == _token) {
    currentState = _State::crcHigh;
    _bytesUntilComplete--;
  } else {
    currentState = _State::error;
    _errorCode = _ErrorCode::CRCError;
  }
}

void ModbusParser::_checkHighCRC() {
  if (highByte(_crc) == _token) {
    _bytesUntilComplete--;
    
    // proof of concept.
    if (!_bytesUntilComplete){
      currentState = _State::complete;
    } else {
      // this would mean that implementation is wrong.
      // because all other cases would cause a CRC error.
      currentState = _State::error;
      _errorCode = _ErrorCode::illegalDataValue;
    }
  } else {
    currentState = _State::error;
    _errorCode = _ErrorCode::CRCError;
  }
}

void ModbusParser::_reset() {
  _crc = 0xFFFF;
  _payloadSize = 0;
  _bytesUntilComplete = 4;
  _errorCode = _ErrorCode::noError;
  currentState = _State::slaveAddress;
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

