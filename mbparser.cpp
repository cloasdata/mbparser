/*
mbparser.cpp 
Implements concrete ModbusParser classes
ResponseParser
RequestParser
*/
#include "mbparser.h"
#include "Stream.h"
#include <Arduino.h>

// ------- RESPONSE PARSER ------------

ResponseParser::ResponseParser()
{}

ResponseParser::~ResponseParser(){
  free();
}


void ResponseParser::free(){
  _freePayload();
}

void ResponseParser::_handleData()
{
switch (_responseState)
{
case ResponseParserState::byteCount:
  _checkByteCount();
  break;

case ResponseParserState::data:
  _receiveData();
  break;

default:
  break;
}
}

void ResponseParser::_parseException(){
  _errorCode = static_cast<ErrorCode>(_token);
  _currentState = ParserState::error;
}

void ResponseParser::_checkByteCount() {
  if (_token > 0) {
    _payloadSize = _token;
    _bytesUntilComplete += _payloadSize;
    _dataToReceive = _payloadSize;
    _responseState = ResponseParserState::data;
    _allocatePayload();
    _renderCRC();
  } else {
    _currentState = ParserState::error;
    _errorCode = ErrorCode::illegalDataValue;
  }
}

void ResponseParser::_receiveData() {
  if (_reverse){
    _reverseCopyToken();
  } else {
    _copyToken();
  }
  _bytesUntilComplete--; 
  _dataToReceive--;
  if (_dataToReceive == 0) {
    _responseState = ResponseParserState::byteCount;
    _currentState = ParserState::firstCRC;
  }
  _renderCRC();
}

void ResponseParser::_copyToken(){
  *_payloadPtr++ = _token;
}

void ResponseParser::_reverseCopyToken(){
  _swappedBytes--;
  *_payloadPtr-- = _token;
  if (_swappedBytes <= 0){
    _payloadPtr += 2 * _registerSize;
    _swappedBytes = _registerSize;
  }
}

void ResponseParser::_allocatePayload() {
  if (_payloadArray!=nullptr) _freePayload(); // ultimate when user code doesn't do.
  _payloadArray = new uint8_t[_payloadSize];
  if (_reverse){
    _payloadPtr = _payloadArray + _registerSize-1;
    _swappedBytes = _registerSize;
  } else {
    _payloadPtr = _payloadArray;
  }
  }

void ResponseParser::_freePayload() { 
  if (_payloadArray != nullptr) {
    delete[] _payloadArray;
    _payloadArray = nullptr;
  }
  _payloadPtr = nullptr;
  }


// ---- REQUEST PARSER
RequestParser::RequestParser(){};


void RequestParser::_handleData(){
  switch (_requestState )
  {
  case RequestParserState::address:
    _parseOffset();
    break;
  
  case RequestParserState::quantity:
    _parseQuantity();
    break;

  default:
    break;
  }

}

void RequestParser::_parseOffset(){
    if (_lastRequestState == RequestParserState::address){
      _wordAsm.bytes[1] = _token;
      _address = _endianness == BIG_ENDIAN ? (_wordAsm.word_>>8)|(_wordAsm.word_<<8) : _wordAsm.word_ ;
      _requestState = RequestParserState::quantity;
    } else {
      _wordAsm.bytes[0] = _token;
      _lastRequestState = RequestParserState::address;
    }
    _renderCRC();   
}

void RequestParser::_parseQuantity(){
  if (_lastRequestState == RequestParserState::quantity){
    _wordAsm.bytes[1] = _token;
    _quantity = _endianness == BIG_ENDIAN ? (_wordAsm.word_>>8)|(_wordAsm.word_<<8) : _wordAsm.word_;
    _requestState = RequestParserState::address;
    _currentState = ParserState::firstCRC;
  } else {
    _wordAsm.bytes[0] = _token;
    _lastRequestState =  RequestParserState::quantity;
  }
  _renderCRC();

}

