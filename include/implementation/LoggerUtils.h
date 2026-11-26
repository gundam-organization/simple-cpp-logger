//
// Created by Nadrino on 25/04/2021.
//

#pragma once


#include "LoggerMacros.h"

#include <cstdio>
#include <memory>
#include <stdexcept>
#include <utility>
#include <thread>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <functional>
#ifdef SIMPLE_CPP_LOGGER_COMPILED
#include <atomic>
#include <deque>
#include <mutex>
#endif


// Header
namespace LoggerUtils{

#ifdef SIMPLE_CPP_LOGGER_COMPILED
  // Mirror the C++ console streams in a shared capture.
  class StreamCapture {
  public:
    SIMPLE_CPP_LOGGER_INLINE StreamCapture();
    SIMPLE_CPP_LOGGER_INLINE ~StreamCapture();
    SIMPLE_CPP_LOGGER_INLINE std::string finish();
    SIMPLE_CPP_LOGGER_INLINE std::vector<std::string> drain(std::size_t keepLines_, std::size_t batchSize_);
    SIMPLE_CPP_LOGGER_INLINE std::vector<std::string> finishLines();
    SIMPLE_CPP_LOGGER_INLINE std::vector<std::string> snapshotLines();

  private:
    friend class StreamBufferSupervisor;

    class CaptureBuffer : public std::streambuf {
    public:
      std::string str() const;
      std::size_t size() const { return _lines_.size(); }
      std::vector<std::string> drain(std::size_t keepLines_, std::size_t batchSize_);
      std::vector<std::string> finishLines();
      std::vector<std::string> snapshotLines() const;

    protected:
      int_type overflow(int_type value_) override;
      std::streamsize xsputn(const char* data_, std::streamsize size_) override;

    private:
      enum class State { Text, Escape, ControlSequence };
      void applyControlSequence(char command_);

      std::deque<std::string> _lines_{1};
      std::size_t _line_{0};
      bool _replaceLine_{false};
      State _state_{State::Text};
      std::string _parameters_;
    };

    class ConsoleBuffer : public std::streambuf {
    public:
      void attach(std::ostream& stream_);
      void detach(std::ostream& stream_);

    protected:
      int_type overflow(int_type value_) override;
      std::streamsize xsputn(const char* data_, std::streamsize size_) override;
      int sync() override;

    private:
      static std::streambuf** downstreamLink(std::streambuf* buffer_);
      std::streambuf* _downstream_{nullptr};
    };
    struct CaptureRegistry {
      std::mutex mutex;
      std::vector<StreamCapture*> captures;
      ConsoleBuffer output;
      ConsoleBuffer error;
    };
    static SIMPLE_CPP_LOGGER_INLINE CaptureRegistry& registry();
    SIMPLE_CPP_LOGGER_INLINE void detach();

    CaptureBuffer _buffer_;
    std::atomic<std::size_t> _lineCount_{1};
  };
#endif

  class StreamBufferSupervisor : public std::streambuf {
    // An external class is need to keep track of the last char printed.
    // It can't be handled by the Logger since each time a logger is called, it is deleted after the ";"
  public:
    StreamBufferSupervisor(){
      _streamBufferPtr_ = _outputStream_->rdbuf();   // back up cout's streambuf
      _outputStream_->flush();
      setp(nullptr, nullptr);
//      _streamBufferSupervisorPtr_->setStreamBuffer(cbuf);
      _outputStream_->rdbuf(this);          // reassign your streambuf to cout
    }
    ~StreamBufferSupervisor() override {
      if( _outFileStream_.is_open() ){
        _outFileStream_.close();
      }
      _outputStream_->rdbuf(_streamBufferPtr_);
    }

    void setStreamBuffer(std::streambuf* buf_){
      _streamBufferPtr_ = buf_;
      // Forward writes directly to the downstream buffer.
      setp(nullptr, nullptr);
    }
    const char& getLastChar() const { return _lastChar_; }
    int_type overflow(int_type c) override {
      if( _streamBufferPtr_ != nullptr ) _streamBufferPtr_->sputc(char(c));
      _lastChar_ = char(c);
      return c;
    }
    std::streamsize xsputn(const char* data_, std::streamsize size_) override {
      if( size_ <= 0 ) return 0;
      const auto written = _streamBufferPtr_ == nullptr ? size_ : _streamBufferPtr_->sputn(data_, size_);
      if( written > 0 ) _lastChar_ = data_[written - 1];
      return written;
    }

    void openOutFileStream(const std::string& outFilePath_){
      _outFileStream_.open(outFilePath_);
    }
    template<typename T> StreamBufferSupervisor& operator<<(const T& something){
      (*_outputStream_) << something;
      if( _outFileStream_.is_open() ) _outFileStream_ << something;
      return *this;
    }
    StreamBufferSupervisor &operator<<(std::ostream &(*f)(std::ostream &)){
      (*_outputStream_) << f;
      if( _outFileStream_.is_open() ) _outFileStream_ << f;
      return *this;
    }
    StreamBufferSupervisor &flush(){
         if(*_outputStream_) _outputStream_->flush();
         if(_outFileStream_.is_open()) _outFileStream_.flush();
         return *this;
    }

  private:
#ifdef SIMPLE_CPP_LOGGER_COMPILED
    friend class StreamCapture::ConsoleBuffer;
#endif
    std::streambuf* _streamBufferPtr_{nullptr};
    std::ofstream _outFileStream_;
    std::ostream* _outputStream_ = &std::cout;
    char _lastChar_{static_cast<char>(traits_type::eof())};
  };

  //! String Utils
  SIMPLE_CPP_LOGGER_INLINE bool doesStringContainsSubstring(const std::string &string_, const std::string &substring_, bool ignoreCase_ = false);
  SIMPLE_CPP_LOGGER_INLINE std::string padString(const std::string& inputStr_, const unsigned int &padSize_, const char& padChar = ' ');
  SIMPLE_CPP_LOGGER_INLINE std::string toLowerCase(const std::string &inputStr_);
  SIMPLE_CPP_LOGGER_INLINE std::string stripStringUnicode(const std::string &inputStr_);
  SIMPLE_CPP_LOGGER_INLINE std::string repeatString(const std::string &inputStr_, int amount_);
  SIMPLE_CPP_LOGGER_INLINE std::vector<std::string> splitString(const std::string& input_string_, const std::string& delimiter_);
  SIMPLE_CPP_LOGGER_INLINE std::string formatString( const std::string& strToFormat_ ); // 0 args overrider
  template<typename ... Args> inline std::string formatString( const std::string& strToFormat_, const Args& ... args ) {
    size_t size = snprintf(nullptr, 0, strToFormat_.c_str(), args ...) + 1; // Extra space for '\0'
    if (size <= 0) { throw std::runtime_error("Error during formatting."); }
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, strToFormat_.c_str(), args ...);
    return {buf.get(), buf.get() + size - 1}; // We don't want the '\0' inside
  }

  SIMPLE_CPP_LOGGER_INLINE void removeRepeatedCharInsideInputStr(std::string &inputStr_, const std::string &doubledChar_);
  SIMPLE_CPP_LOGGER_INLINE void replaceSubstringInsideInputString(std::string &input_str_, const std::string &substr_to_look_for_, const std::string &substr_to_replace_);

  // Hardware Utils
  SIMPLE_CPP_LOGGER_INLINE int getTerminalWidth();
  SIMPLE_CPP_LOGGER_INLINE std::string getExecutableName();

  // hash Utils
  template <class T> inline void hashCombine(std::size_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
  }

}

#ifndef SIMPLE_CPP_LOGGER_COMPILED
#include "LoggerUtils.impl.h"
#endif
