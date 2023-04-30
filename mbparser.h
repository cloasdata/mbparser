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

template<typename CB, typename TChild>
class ModbusParser;

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
    data = 3,
    byteCount = 4,
    address = 5,
    quantity = 6,
    firstCRC = 7,
    secondCRC = 8,
    complete = 9,
    modbusException = 10
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
ModbusParser Base class implements the general part of a modbus frame
and provides infrastructure for its child classes, like memory handling.

User classes needs to implement getter function for particular dispatch tables. 
The getter is called when function code is parsed to retrieve the correct state chain.

The architecture uses a mix between switch case state machine and dispatch table. 
The general state is managed via switch-case whereas the particular function code and its state
is managed via a dispatch table (array of states).


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
      while (index < len && _nextState != ParserState::error) {
        _parse(buffer[index]);
        index++;
      }
      return _nextState;
    }
    
    /*
    Parse one token and render state machine.
    Returns the current parser state.
    Parser state and payload is only valid, when parser state is complete
    In all other states class attributes may be inconsistent.
    */
    ParserState parse(uint8_t token){
      _parse(token);
      return _nextState;
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

    /*
    Swaps byte order of data frames
    */
    void setSwap(bool swap){
      _reverse = swap;
    };
    
    /*
    To swap each register the size of register needs to be set
    */
    void setRegisterSize(uint16_t size){
      _registerSize=size;
    };

    
    /*
    Sets modbus slave address.
    When zero is provided, parse will parse all slaves.
    */
    void setSlaveAddress(uint8_t id){
      _mySlaveAddress = id;
    };

    /*
    false: little
    true: big
    */
    void setEndianness(uint16_t v){
      _endianness = v;
    }

    /*
    Sets void pointer to keep reference to third party objects.
    Useful when working within classes and cannot use std::functional 
    */
    void setExtension(void* ptr){
      _extension = ptr;
    }

    /*
    Sets limit for payload to receive.
    If exceeded error is indicated

    The default limit is 96 bytes. 
    */
    void setByteCountLimit(size_t size){
      _byteCountLimit = size;
    }

    // ---GETTERS---

    ParserState state() const {
      return _nextState;
    }

    uint8_t slaveAddress() const {
      return _slaveAddress;
    }

    uint8_t mySlaveAddress() const {
      return _mySlaveAddress;
    }
    
    uint8_t functionCode() const {
      return _functionCode;
    };

    uint16_t address() const {
      return _address;
    }

    uint16_t quantity() const {
      return _quantity;
    }

    uint8_t byteCount() const {
      return _byteCount;
    }

    uint8_t* data() const {
      return _dataArray;
    }
 
    uint16_t crcBytes() const {
      return _endianness == BIG_ENDIAN ? (_crc>>8) | (_crc<<8) : _crc;
    };

    ErrorCode errorCode() const {
      return _errorCode;
    };

    void* getExtension(){
      return _extension;
    }

    uint16_t dataToReceive(){
      return _dataToReceive;
    }

    size_t byteCountLimit() const {
      return _byteCountLimit;
    }

    bool isComplete() const {
      return _nextState == ParserState::complete;
    }

    bool isError() const {
      return _nextState == ParserState::error;
    } 

    /*
    Frees the heap located data array.
    Can be called by user.
    */
    void free(){
      if (_dataArray != nullptr) {
        delete[] _dataArray;
        _dataArray = nullptr;
      }
      _dataPtr = nullptr;
    }

  protected:
    ModbusParser(){};
    virtual const ParserState* dispatch04() = 0;
    virtual const ParserState* dispatch06() = 0;
    virtual const ParserState* dispatch10() = 0;

  private:
    const uint16_t _supportedFunctionCodes[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x0F, 0x10};
    const ParserState* _dispatchFC{nullptr};

    union byteToWord
    {
      uint16_t word_;
      uint8_t bytes[2];
    } assembleWord;
    
    CB _onComplete {nullptr};
    CB _onError {nullptr};
    
    uint8_t _token{};
    
    ParserState _lastState{ParserState::slaveAddress};
    ParserState _nextState{ParserState::slaveAddress};
    
    ErrorCode _errorCode{ErrorCode::noError};
    
    uint8_t _slaveAddress{250}; // invalid
    uint8_t _mySlaveAddress{0};
    uint8_t _functionCode{0}; // invalid
    uint16_t _address{0};
    uint16_t _quantity{0};
    uint8_t _byteCount{0}; 
    uint16_t _dataToReceive{0};
    uint8_t *_dataArray {nullptr};
    uint8_t *_dataPtr {nullptr};
    uint16_t _crc{0xFFFF};

    uint16_t _endianness{BIG_ENDIAN};
    bool _reverse {false};
    uint16_t _registerSize{};
    uint16_t _swappedBytes{};

    void* _extension{nullptr};
    size_t _byteCountLimit{96};

    /*
    Actual implementation of parse.
    Drives the state machine by one token
    */
    void _parse(uint8_t token) {
      _token = token;
      // indeed currentState is laststate until the machine is rendered.
      _lastState = _nextState;
      if (_nextState == ParserState::complete || _nextState == ParserState::error) {
        _reset();
      }
      _renderStateMachine();
      //Serial.printf("T: %2X, S: %d, D: %2d, E: %d, CRC: %X\n", token, static_cast<int>(_lastState),_dataToReceive ,static_cast<int>(errorCode()), _crc);
      _handleCallbacks();
    }
    
    void _renderStateMachine() {
      switch (_nextState) {
      case ParserState::slaveAddress:
        _parseSlaveAddress();
        break;
      case ParserState::functionCode:
        _checkFunctionCode();
        break;
      case ParserState::address:
        _handleAddress();
        break;
      case ParserState::byteCount:
        _handleByteCount();
        break;
      case ParserState::quantity:
        _handleQuantity();
        break;
      case ParserState::data:
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

    void _handleCallbacks(){
      switch (_nextState)
      {
      case ParserState::complete:
        if (_onComplete){
          _onComplete(static_cast<TChild*>(this));
        }
        break;
      case ParserState::error:
        if (_onError) {
          _onError(static_cast<TChild*>(this));
        }
        break;
      default:
        break;
      }
    }

    const ParserState* _getDispatchArray(){
      switch (_functionCode){
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x04:
          return dispatch04();
        case 0x05:
        case 0x06:
          return dispatch06();
        case 0x0F:
        case 0x10:
          return dispatch10();
      default:
        return nullptr;
        break;
      }
    }

    /*Always call before any state change*/
    void _advanceDispatcher(){
      _nextState = *_dispatchFC++;
    }
    
    // --STATES--

    void _parseSlaveAddress() {
      if (_token == _mySlaveAddress || _mySlaveAddress == 0) {
        _slaveAddress = _token;
        _nextState = ParserState::functionCode;
        _renderCRC();
      } else {
        _nextState = ParserState::slaveAddress;
      }
    }

    void _checkFunctionCode() {
      if (_token > 128) {
        _nextState = ParserState::modbusException;
        return;
      }
      if (isFunctionCodeSupported(_token)) {
        _functionCode = _token;
        _dispatchFC = _getDispatchArray();
        _advanceDispatcher();
        _renderCRC();

      } else {
        _nextState = ParserState::error;
        _errorCode = ErrorCode::illegalFunction;
      }
    }

    bool isFunctionCodeSupported(uint8_t fc){
      for (uint16_t idx = 0; idx < 8; idx++) {
        if (fc == _supportedFunctionCodes[idx]) {
          return true;
        }
      }
      return false;
    }

    void _handleAddress(){
      // consumes 2 tokens
      _advanceDispatcher();
      _parseAddress();
      _renderCRC();
    }

    void _parseAddress(){
      // two tokens required
      if (_nextState == ParserState::address){
        assembleWord.bytes[1] = _token;
      } else {
        assembleWord.bytes[0] = _token;
        _address = assembleWord.word_;
      }
    }

    void _handleQuantity(){
      // consumes 2 tokens
      _advanceDispatcher();
      _parseQuantity();
      _renderCRC();
    }

    void _parseQuantity(){
      if (_nextState == ParserState::quantity){
        assembleWord.bytes[1] = _token;
      } else{
        assembleWord.bytes[0] = _token;
        _quantity = assembleWord.word_;
        if (_quantity == 0) {
          _nextState = ParserState::error;
          _errorCode = ErrorCode::illegalDataValue;
        }
      }
    }
    
    void _handleByteCount(){
      _advanceDispatcher();
      
      if (_token > 0){
        _parseByteCount();
        if (_byteCount > _byteCountLimit){
          _nextState = ParserState::error;
          _errorCode = ErrorCode::illegalDataValue;
        }
        _renderCRC();
      } else {
        _nextState = ParserState::error;
        _errorCode = ErrorCode::illegalDataValue;
      }
    }

    void _parseByteCount() {
        _byteCount = _token;
        _dataToReceive = _byteCount;
    }

    void _handleData(){
      _receiveData();
      _renderCRC();
    }

    void _receiveData() {
      if (_dataArray == nullptr){
        _dataToReceive = max(_dataToReceive, uint16_t(2)); // at least 2 bytes
        _allocateData(_dataToReceive);
      }

      if (_reverse){
        _reverseCopyToken();
      } else {
        _copyToken();
      }
      _dataToReceive--;
      if (_dataToReceive == 0){
        _nextState = ParserState::firstCRC;
      }
    }

    void _copyToken(){
      *_dataPtr++ = _token;
    }

    void _reverseCopyToken(){
      _swappedBytes--;
      *_dataPtr-- = _token;
      if (_swappedBytes <= 0){
        _dataPtr += 2 * _registerSize;
        _swappedBytes = _registerSize;
      }
    }

    void _checkFirstCRC() {
      uint8_t crcByte = _endianness == BIG_ENDIAN ? lowByte(_crc) : highByte(_crc);
      if (crcByte == _token) {
        _nextState = ParserState::secondCRC;
      } else {
        _nextState = ParserState::error;
        _errorCode = ErrorCode::CRCError;
      }
    }

    void _checkSecondCRC() {
      uint8_t crcByte = _endianness == BIG_ENDIAN ? highByte(_crc) : lowByte(_crc);
      if (crcByte == _token) {
        // proof of concept.
        if (!_dataToReceive){
          _nextState = ParserState::complete;
        } else {
          // this would mean that implementation is wrong.
          // because all other cases would cause a CRC error.
          _nextState = ParserState::error;
          _errorCode = ErrorCode::illegalDataValue;
        }
      } else {
        _nextState = ParserState::error;
        _errorCode = ErrorCode::CRCError;
      }
    }

    void _parseException(){
      _errorCode = static_cast<ErrorCode>(_token);
      _nextState = ParserState::error;
    }

    void _allocateData(size_t size) { 
      _dataArray = new uint8_t[size];
      if (_reverse){
        _dataPtr = _dataArray + _registerSize-1;
        _swappedBytes = _registerSize;
      } else {
        _dataPtr = _dataArray;
      } 
    }

    void _reset() {
      free();
      _crc = 0xFFFF;
      _errorCode = ErrorCode::noError;
      _nextState = ParserState::slaveAddress;
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


/*
The response Parser is the core of the modbus master/client.
*/
class ResponseParser: public ModbusParser<ResponseCallback, ResponseParser>{
  public:
    ResponseParser(){};
    
    ~ResponseParser(){this->free();};
     
  private:
  	const ParserState _dispatch04[2]{ParserState::byteCount, ParserState::data};
    const ParserState _dispatch06[3]{ParserState::address,ParserState::address, ParserState::data};
  	const ParserState _dispatch10[5]{ParserState::address, ParserState::address, ParserState::quantity, ParserState::quantity, ParserState::firstCRC};
  	const ParserState* dispatch04() override {return _dispatch04;};
  	const ParserState* dispatch06() override {return _dispatch06;};
  	const ParserState* dispatch10() override {return _dispatch10;};
    
};


/*
The request parser is the core of the modbus slave/server.
*/
class RequestParser: public ModbusParser<RequestCallback, RequestParser>{
  public:
    RequestParser(){};
    ~RequestParser(){this->free();};

  private:
    const ParserState _dispatch04[5]{ParserState::address, ParserState::address, ParserState::quantity, ParserState::quantity, ParserState::firstCRC};
    const ParserState _dispatch06[3]{ParserState::address,ParserState::address, ParserState::data};
    const ParserState _dispatch10[6]{ParserState::address, ParserState::address, ParserState::quantity, ParserState::quantity, ParserState::byteCount, ParserState::data};
    const ParserState* dispatch04() override {return _dispatch04;};
  	const ParserState* dispatch06() override {return _dispatch06;};
  	const ParserState* dispatch10() override {return _dispatch10;};
};

#endif