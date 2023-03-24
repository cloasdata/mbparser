/*
mbparser.h by Simon Bauer 2023

Contains:
Declaration and Definition of ModbusParser Base Class. 
Definition of Derivate ResponseParser and RequestParser.
Type safe enums for state and error codes.


Remarks:
The parser is default implemented as big endian. 
This is relevant for 16 bit containers like address or CRC.
However, it was original implemented for little endian machines. 
*/
#ifndef mbparser_h
#define  mbparser_h
#include <Arduino.h>


// Endian Literals
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 1234
#endif
#ifndef BIG_ENDIAN
#define BIG_ENDIAN 4321
#endif

// Protos
class ResponseParser;
class RequestParser;

// Function Pointers
#ifdef STD_FUNCTIONAL
  #include <functional>
  typedef std::function<void(ResponseParser *parser)> ResponseCallback;
  typedef std::function<void(RequestParser *parser)> RequestCallback;
#else
  typedef void(*ResponseCallback)(ResponseParser *parser);
  typedef void(*RequestCallback)(ResponseParser *parser);
#endif

// General used enums

enum class ParserState{
    error = 0,
    slaveAddress = 1,
    functionCode = 2,
    payload = 3,
    firstCRC = 4,
    secondCRC = 5,
    complete= 6,
    modbusException = 7
};

enum class ErrorCode{
    noError = 0,
    // Modbus Exception
    illegalFunction = 1,
    illegalDataAddress =2,
    illegalDataValue = 3,
    slaveDeviceFailure = 4,
    acknowledge = 5,
    slaveDeviceBusy = 6,
    memoryParityError = 8,
    // mbParser Exception
    CRCError = 21
};

/*
ModbusParser Base class implements the general part of a modbus frame.
User classes needs to implement the _handleData method. 
The method is called when function code was retrieved.

The parser implements BIG ENDIAN format modbus frames.
User can swap to LITTLE ENDIAN. This makes it possible to parse
LITTLE_ENDIAN and BIG_ENDIAN subscribers.
*/
template<typename CB, typename TChild>
class ModbusParser{
  public:
    virtual ~ModbusParser() = default;
    ModbusParser(const ModbusParser&) = delete;
    ModbusParser& operator= (const ModbusParser&) = delete;
   
    /*
    Parses the buffer until len.
    Returns the current parser state.
    If error occurs returns immediately.
    Parser state and payload is only valid, when parser state is complete
    In all other states class attributes may be inconsistent.
    */
    ParserState parse(uint8_t *buffer, uint16_t len) {
      uint16_t index = 0;

      // consume all provided tokens
      while (index < len && _currentState != ParserState::error) {
        _parse(buffer[index]);
        index++;
      }
      return _currentState;
    }
    
    /*
    Parse one token and render state machine.
    Returns the current parser state.
    Parser state and payload is only valid, when parser state is complete
    In all other states class attributes may be inconsistent.
    */
    ParserState parse(uint8_t token){
      _parse(token);
      return _currentState;
    };

    /*
    Sets on complete callback.
    Is called when parser has finished one complete response frame.
    */
    void setOnCompleteCB(CB cb){
      _onComplete=cb;
    };

    /*
    Sets on error callback.
    Is called when parser detects any error.
    */
    void setOnErrorCB(CB cb){
      _onError=cb;
    };

    void setSlaveID(uint8_t id){
      _slaveID = id;
      };

    /*
    false: little
    true: big
    */
    void setEndianness(uint16_t v){
      _endianness = v;
    }

    //properties
    ParserState state() const {
      return _currentState;
    }
    
    uint8_t functionCode() const {
      return _functionCode;
    };
    
    uint16_t crcBytes() const {
      return _endianness == BIG_ENDIAN ? (_crc>>8) | (_crc<<8) : _crc;
    };

    ErrorCode errorCode() const {
      return _errorCode;
    };

  protected:
    ModbusParser(){};
    const uint16_t _supportedFunctionCodes[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x15, 0x16};
    CB _onComplete = nullptr;
    CB _onError = nullptr;
    
    uint16_t _bytesUntilComplete{4}; // at least

    uint8_t _token{};
    
    ParserState lastState{ParserState::slaveAddress};
    ParserState _currentState{ParserState::slaveAddress};
    ErrorCode _errorCode{ErrorCode::noError};
    
    uint8_t _slaveID{250}; // invalid
    uint8_t _functionCode{0}; // invalid
    uint16_t _endianness{BIG_ENDIAN};

    uint16_t _crc{0xFFFF};
    virtual void _handleData() = 0;



    /*
    Actual implementation of parse.
    */
    void _parse(uint8_t token) {
      _token = token;
      // indeed currentState is laststate until the machine is rendered.
      lastState = _currentState;
      if (lastState == ParserState::complete || lastState == ParserState::error) {
        _reset();
      }
      _renderStateMachine();
      //Serial.printf("T: %X, S: %d E: %d CRC: %X\n", token, static_cast<int>(_currentState), static_cast<int>(errorCode()), _crc);
      _handleCallbacks();
    }

    /*
    Calls registered callbacks. 
    */
    void _handleCallbacks(){
      if (_currentState == ParserState::complete){
        if (_onComplete) _onComplete(static_cast<TChild*>(this));
      } else if (_currentState == ParserState::error) {
        if (_onError) _onError(static_cast<TChild*>(this));
      }
    }


    /*
    State machine
    */
    void _renderStateMachine() {
      switch (_currentState) {
      case ParserState::slaveAddress:
        _checkSlaveAddress();
        break;

      case ParserState::functionCode:
        _checkFunctionCode();
        break;

      case ParserState::payload:
        _handleData();
        break;

      case ParserState::modbusException:
        _parseException();
        break;

      case ParserState::firstCRC:
        _checkFirstCRC();
        break;

      case ParserState::secondCRC:
        _checkSecondCRC();
        break;

      default:
        break;
      }
    }

    // STATES

    /*
    Consumes tokens until slave address is found
    */
    void _checkSlaveAddress() {
      if (_token == _slaveID) {
        _currentState = ParserState::functionCode;
        _bytesUntilComplete--;
        _renderCRC();
      } else {
        _currentState = ParserState::slaveAddress;
      }
    }

    void _checkFunctionCode() {
      if (_token > 128) {
        _currentState = ParserState::modbusException;
        return;
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
        _currentState = ParserState::payload;
        _bytesUntilComplete--;
        _renderCRC();
      } else {
        _currentState = ParserState::error;
        _errorCode = ErrorCode::illegalFunction;
      }
    }

    virtual void _parseException() = 0;

    void _checkFirstCRC() {
      uint8_t crcByte = _endianness == BIG_ENDIAN ? lowByte(_crc) : highByte(_crc);
      if (crcByte == _token) {
        _currentState = ParserState::secondCRC;
        _bytesUntilComplete--;
      } else {
        _currentState = ParserState::error;
        _errorCode = ErrorCode::CRCError;
      }
    }

    void _checkSecondCRC() {
      uint8_t crcByte = _endianness == BIG_ENDIAN ? highByte(_crc) : lowByte(_crc);
      if (crcByte == _token) {
        _bytesUntilComplete--;
        
        // proof of concept.
        if (!_bytesUntilComplete){
          _currentState = ParserState::complete;
        } else {
          // this would mean that implementation is wrong.
          // because all other cases would cause a CRC error.
          _currentState = ParserState::error;
          _errorCode = ErrorCode::illegalDataValue;
        }
      } else {
        _currentState = ParserState::error;
        _errorCode = ErrorCode::CRCError;
      }
    }

    void _reset() {
      _crc = 0xFFFF;
      _bytesUntilComplete = 4;
      _errorCode = ErrorCode::noError;
      _currentState = ParserState::slaveAddress;
    }

    void _renderCRC() {
      _crc ^= (uint16_t)_token;  // XOR byte into least sig. byte of crc
      for (int i = 8; i != 0; i--) { // Loop over each bit
        if ((_crc & 0x0001) != 0) {  // If the LSB is set
          _crc >>= 1;                // Shift right and XOR 0xA001
          _crc ^= 0xA001;
        } else        // Else LSB is not set
          _crc >>= 1; // Just shift right
      }
    }

};

class ResponseParser: public ModbusParser<ResponseCallback, ResponseParser>{
  enum class ResponseParserState{
    byteCount,
    data,
  };


  public:
    ResponseParser();
    ~ResponseParser();
    
    uint16_t byteCount(){return _payloadSize;};
    uint8_t *payload(){return _payloadArray;};
    uint16_t untilComplete(){return _bytesUntilComplete;};
    
    void free();
    void setSwap(bool swap){_reverse = true;};
    void setRegisterSize(uint16_t size){_registerSize=size;};
    

  private:
    ResponseParserState _responseState{ResponseParserState::byteCount};
    uint16_t _payloadSize;
    uint16_t _dataToReceive;
    uint8_t *_payloadArray = nullptr;
    uint8_t *_payloadPtr = nullptr;
  	
    bool _reverse = false;
    uint16_t _registerSize;
    uint16_t _swappedBytes;

    void _handleData();
    void _parseException();
    void _checkByteCount();
    
    void _receiveData();
    
    void _copyToken();
    void _reverseCopyToken();
    void _allocatePayload();
    void _freePayload();
};

class RequestParser: public ModbusParser<RequestCallback, RequestParser>{
  enum class RequestParserState{
    offset,
    quantity
  };

  union byteToWord
    {
      uint16_t word_;
      uint8_t bytes[2];
    } ;

  public:
    RequestParser();

    uint16_t offset() const {return _offset;};
    uint16_t quantity() const {return _quantity;};

  private:


    RequestParserState _requestState{RequestParserState::offset};
    RequestParserState _lastRequestState{RequestParserState::quantity};

    uint16_t _offset{0}; // should be invalid
    uint16_t _quantity{0}; // invalid
    byteToWord _wordAsm{};

    void _handleData();
    void _parseException(){};
    void _parseOffset();
    void _parseQuantity();

};

#endif