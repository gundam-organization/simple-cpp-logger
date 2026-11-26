//
// Created by Nadrino on 25/04/2021.
//

#ifndef SIMPLE_CPP_LOGGER_LOGGERUTILS_IMPL_H
#define SIMPLE_CPP_LOGGER_LOGGERUTILS_IMPL_H

#include "LoggerUtils.h"

#include <cctype>
#include <cmath>
#include <vector>
#include <string>
#include <memory>
#include <cstring> // strrchr
#include <fstream>
#include <iostream>
#include <algorithm>

#ifdef SIMPLE_CPP_LOGGER_COMPILED
#include <charconv>
#include <iterator>
#include <string_view>

namespace LoggerUtils {
  std::vector<std::string> StreamCapture::CaptureBuffer::drain(std::size_t keepLines_, std::size_t batchSize_){
    const auto count = std::min(_line_, _lines_.size() - std::min(keepLines_, _lines_.size()));
    if( count < batchSize_ ) return {};
    std::vector<std::string> lines;
    lines.reserve(count);
    for( std::size_t i = 0; i < count; ++i ){
      lines.emplace_back(std::move(_lines_.front()));
      _lines_.pop_front();
    }
    _line_ -= count;
    return lines;
  }

  std::vector<std::string> StreamCapture::CaptureBuffer::finishLines(){
    if( _lines_.back().empty() ) _lines_.pop_back();
    return {std::make_move_iterator(_lines_.begin()), std::make_move_iterator(_lines_.end())};
  }
  std::vector<std::string> StreamCapture::CaptureBuffer::snapshotLines() const{
    const auto end = _lines_.back().empty() ? std::prev(_lines_.end()) : _lines_.end();
    return {_lines_.begin(), end};
  }

  std::string StreamCapture::CaptureBuffer::str() const{
    std::string result;
    for( const auto& line : _lines_ ){
      if( &line != &_lines_.front() ) result += '\n';
      result += line;
    }
    return result;
  }

  StreamCapture::CaptureBuffer::int_type StreamCapture::CaptureBuffer::overflow(int_type value_){
    if( traits_type::eq_int_type(value_, traits_type::eof()) ) return traits_type::not_eof(value_);
    const auto c = traits_type::to_char_type(value_);

    if( _state_ == State::Escape ){
      _state_ = c == '[' ? State::ControlSequence : State::Text;
      if( _state_ == State::ControlSequence ) return value_;
    }
    if( _state_ == State::ControlSequence ){
      if( c >= '@' and c <= '~' ){
        applyControlSequence(c);
        _parameters_.clear();
        _state_ = State::Text;
      }
      else if( _parameters_.size() < 32 ){
        _parameters_ += c;
      }
      return value_;
    }

    if( c == '\033' ) _state_ = State::Escape;
    else if( c == '\r' ) _replaceLine_ = true;
    else if( c == '\n' ){
      if( ++_line_ == _lines_.size() ) _lines_.emplace_back();
      _replaceLine_ = true;
    }
    else{
      auto& line = _lines_[_line_];
      if( _replaceLine_ ){
        line.clear();
        _replaceLine_ = false;
      }
      line += c;
    }
    return value_;
  }

  std::streamsize StreamCapture::CaptureBuffer::xsputn(const char* data_, std::streamsize size_){
    if( size_ <= 0 ) return 0;
    std::string_view text(data_, static_cast<std::size_t>(size_));
    while( not text.empty() ){
      if( _state_ != State::Text or text.front() == '\r' or text.front() == '\n' or text.front() == '\033' ){
        overflow(traits_type::to_int_type(text.front()));
        text.remove_prefix(1);
        continue;
      }
      const auto count = std::min(text.find_first_of("\r\n\033"), text.size());
      auto& line = _lines_[_line_];
      if( _replaceLine_ ){
        line.clear();
        _replaceLine_ = false;
      }
      line.append(text.data(), count);
      text.remove_prefix(count);
    }
    return size_;
  }

  void StreamCapture::CaptureBuffer::applyControlSequence(char command_){
    // Logger and Toolbox use CSI F/E to move between rows and CSI K to erase.
    if( command_ != 'F' and command_ != 'E' and command_ != 'K' ) return;
    std::size_t count{0};
    std::from_chars(_parameters_.data(), _parameters_.data() + _parameters_.size(), count);
    if( command_ == 'K' ){
      if( count == 2 ) _lines_[_line_].clear();
      return;
    }
    count = std::max(count, std::size_t{1});
    if( command_ == 'F' ){
      _line_ -= std::min(count, _line_);
      _replaceLine_ = true;
    }
    else{
      _line_ += std::min(count, _lines_.size() - 1 - _line_);
      _replaceLine_ = true;
    }
  }

  SIMPLE_CPP_LOGGER_INLINE StreamCapture::CaptureRegistry& StreamCapture::registry(){
    static CaptureRegistry active;
    return active;
  }
  std::streambuf** StreamCapture::ConsoleBuffer::downstreamLink(std::streambuf* buffer_){
    auto* supervisor = dynamic_cast<StreamBufferSupervisor*>(buffer_);
    return supervisor == nullptr ? nullptr : &supervisor->_streamBufferPtr_;
  }
  void StreamCapture::ConsoleBuffer::attach(std::ostream& stream_){
    auto* link = downstreamLink(stream_.rdbuf());
    if( link == nullptr ){
      _downstream_ = stream_.rdbuf(this);
      return;
    }
    while( auto* next = downstreamLink(*link) ) link = next;
    _downstream_ = *link;
    *link = this;
  }
  void StreamCapture::ConsoleBuffer::detach(std::ostream& stream_){
    if( stream_.rdbuf() == this ){
      stream_.rdbuf(_downstream_);
      return;
    }
    for( auto* link = downstreamLink(stream_.rdbuf()); link != nullptr; link = downstreamLink(*link) ){
      if( *link == this ){
        *link = _downstream_;
        return;
      }
    }
  }
  StreamCapture::ConsoleBuffer::int_type StreamCapture::ConsoleBuffer::overflow(int_type value_){
    if( traits_type::eq_int_type(value_, traits_type::eof()) ) return traits_type::not_eof(value_);
    const auto c = traits_type::to_char_type(value_);
    return xsputn(&c, 1) == 1 ? value_ : traits_type::eof();
  }
  std::streamsize StreamCapture::ConsoleBuffer::xsputn(const char* data_, std::streamsize size_){
    auto& active = registry();
    std::lock_guard<std::mutex> lock(active.mutex);
    const auto written = _downstream_ == nullptr ? size_ : _downstream_->sputn(data_, size_);
    for( auto* capture : active.captures ){
      const auto previousSize = capture->_buffer_.size();
      capture->_buffer_.sputn(data_, written);
      if( capture->_buffer_.size() != previousSize ){
        capture->_lineCount_.store(capture->_buffer_.size(), std::memory_order_relaxed);
      }
    }
    return written;
  }
  int StreamCapture::ConsoleBuffer::sync(){
    std::lock_guard<std::mutex> lock(registry().mutex);
    return _downstream_ == nullptr ? 0 : _downstream_->pubsync();
  }
  SIMPLE_CPP_LOGGER_INLINE StreamCapture::StreamCapture(){
    auto& active = registry();
    std::lock_guard<std::mutex> lock(active.mutex);
    active.captures.push_back(this);
    if( active.captures.size() == 1 ){
      active.output.attach(std::cout);
      active.error.attach(std::cerr);
    }
  }
  SIMPLE_CPP_LOGGER_INLINE StreamCapture::~StreamCapture(){
    detach();
  }
  SIMPLE_CPP_LOGGER_INLINE std::string StreamCapture::finish(){
    detach();
    return _buffer_.str();
  }
  std::vector<std::string> StreamCapture::drain(std::size_t keepLines_, std::size_t batchSize_){
    const auto count = _lineCount_.load(std::memory_order_relaxed);
    if( count < keepLines_ or count - keepLines_ < batchSize_ ) return {};
    auto& active = registry();
    std::lock_guard<std::mutex> lock(active.mutex);
    auto lines = _buffer_.drain(keepLines_, batchSize_);
    _lineCount_.store(_buffer_.size(), std::memory_order_relaxed);
    return lines;
  }
  std::vector<std::string> StreamCapture::finishLines(){
    detach();
    return _buffer_.finishLines();
  }
  std::vector<std::string> StreamCapture::snapshotLines(){
    std::lock_guard<std::mutex> lock(registry().mutex);
    return _buffer_.snapshotLines();
  }
  SIMPLE_CPP_LOGGER_INLINE void StreamCapture::detach(){
    auto& active = registry();
    std::lock_guard<std::mutex> lock(active.mutex);
    auto& captures = active.captures;
    const auto entry = std::find(captures.begin(), captures.end(), this);
    if( entry == captures.end() ) return;
    captures.erase(entry);
    if( captures.empty() ){
      active.output.detach(std::cout);
      active.error.detach(std::cerr);
    }
  }
}
#endif

#if defined(_WIN32)
// Windows
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
// Only console APIs are needed; wingdi.h defines ERROR, which conflicts
// with LoggerContext::LogLevel::ERROR.
#ifndef NOGDI
#define NOGDI
#endif
#include <Windows.h>
#include <psapi.h>
#elif defined(__APPLE__) && defined(__MACH__)
// MacOS
#include <unistd.h>
#include <sys/resource.h>
#include <mach/mach.h>
#include <sys/ioctl.h>

#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
// Linux
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdio.h>
#include <sys/ioctl.h>

#elif (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__)))
// AIX and Solaris
#include <unistd.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <procfs.h>
#include <sys/ioctl.h>

#else
// Unsupported
#endif

#include <cstdio>
#include <cstdlib>
extern char* __progname;


// Implementation:
namespace LoggerUtils{

// String Utils
  SIMPLE_CPP_LOGGER_INLINE bool doesStringContainsSubstring(const std::string &string_, const std::string &substring_, bool ignoreCase_){
    if(substring_.size() > string_.size()) return false;
    if(ignoreCase_){
      if(toLowerCase(string_).find(toLowerCase(substring_)) != std::string::npos){
        return true;
      }
    }
    else if(string_.find(substring_) != std::string::npos){
      return true;
    }
    return false;
  }
  SIMPLE_CPP_LOGGER_INLINE std::string padString(const std::string& inputStr_, const unsigned int &padSize_, const char& padChar){
    std::string outputString;
    int padDelta = int(inputStr_.size()) - int(padSize_);
    while( padDelta < 0 ){
      // add extra chars if needed
      outputString += padChar;
      padDelta++;
    }
    outputString += inputStr_;
    return outputString.substr(0, outputString.size() - padDelta);
  }
  SIMPLE_CPP_LOGGER_INLINE std::string toLowerCase(const std::string &inputStr_){
    std::string output_str(inputStr_);
    std::transform(output_str.begin(), output_str.end(), output_str.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return output_str;
  }
  SIMPLE_CPP_LOGGER_INLINE std::string stripStringUnicode(const std::string &inputStr_){
    std::string outputStr(inputStr_);

    if(LoggerUtils::doesStringContainsSubstring(outputStr, "\033")){
      // remove color
      std::string tempStr;
      auto splitOutputStr = LoggerUtils::splitString(outputStr, "\033");
      for(const auto& sliceStr : splitOutputStr){
        if(sliceStr.empty()) continue;
        if(tempStr.empty()){
          tempStr = sliceStr;
          continue;
        }
        // look for a 'm' char that determines the end of the color code
        bool mCharHasBeenFound = false;
        for(const char& c : sliceStr){
          if(not mCharHasBeenFound){
            if(c == 'm'){
              mCharHasBeenFound = true;
            }
          }
          else{
            tempStr += c;
          }
        }
      }
      outputStr = tempStr;
    }

    outputStr.erase(
        remove_if(
            outputStr.begin(), outputStr.end(),
            [](const char& c){return !isprint( static_cast<unsigned char>( c ) );}
        ),
        outputStr.end()
    );

    return outputStr;
  }
  SIMPLE_CPP_LOGGER_INLINE std::string repeatString(const std::string &inputStr_, int amount_){
    std::string outputStr;
    if(amount_ <= 0) return outputStr;
    for(int i_count = 0 ; i_count < amount_ ; i_count++){
      outputStr += inputStr_;
    }
    return outputStr;
  }
  SIMPLE_CPP_LOGGER_INLINE void removeRepeatedCharInsideInputStr(std::string &inputStr_, const std::string &doubledChar_){
    std::string doubledCharStr = doubledChar_+doubledChar_;
    std::string lastStr;
    do{
      lastStr = inputStr_;
      LoggerUtils::replaceSubstringInsideInputString(inputStr_, doubledCharStr, doubledChar_);
    } while( lastStr != inputStr_ );
  }
  SIMPLE_CPP_LOGGER_INLINE void replaceSubstringInsideInputString(std::string &input_str_, const std::string &substr_to_look_for_, const std::string &substr_to_replace_){
    size_t index = 0;
    while ((index = input_str_.find(substr_to_look_for_, index)) != std::string::npos) {
      input_str_.replace(index, substr_to_look_for_.length(), substr_to_replace_);
      index += substr_to_replace_.length();
    }
  }
  SIMPLE_CPP_LOGGER_INLINE std::vector<std::string> splitString(const std::string& input_string_, const std::string& delimiter_) {

    std::vector<std::string> output_split_string;

    const char *src = input_string_.c_str();
    const char *next;

    std::string out_string_piece;

    while ((next = std::strstr(src, delimiter_.c_str())) != nullptr) {
      out_string_piece = "";
      while (src != next) {
        out_string_piece += *src++;
      }
      output_split_string.emplace_back(out_string_piece);
      /* Skip the delimiter_ */
      src += delimiter_.size();
    }

    /* Handle the last token */
    out_string_piece = "";
    while (*src != '\0')
      out_string_piece += *src++;

    output_split_string.emplace_back(out_string_piece);

    return output_split_string;

  }
  SIMPLE_CPP_LOGGER_INLINE std::string formatString( const std::string& strToFormat_ ){
    return strToFormat_;
  }
  // Hardware related tools
  SIMPLE_CPP_LOGGER_INLINE int getTerminalWidth(){
    int outWith;
#if defined(_WIN32)
    CONSOLE_SCREEN_BUFFER_INFO csbi;
      GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
      outWith = (int)(csbi.dwSize.X);
  //    outWith = (int)(csbi.dwSize.Y);
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__) \
    || (defined(_AIX) || defined(__TOS__AIX__)) || (defined(__sun__) || defined(__sun) || defined(sun) && (defined(__SVR4) || defined(__svr4__))) \
    || (defined(__APPLE__) && defined(__MACH__))
    struct winsize w{};
    ioctl(fileno(stdout), TIOCGWINSZ, &w);
    outWith = (int)(w.ws_col);
    //    outWith = (int)(w.ws_row);
#endif // Windows/Linux
    return outWith;
  }
  SIMPLE_CPP_LOGGER_INLINE std::string getExecutableName(){
    std::string outStr;
#if defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__) //check defines for your setup
    std::ifstream("/proc/self/comm") >> outStr;
#elif defined(_WIN32)
    char buf[MAX_PATH];
    GetModuleFileNameA(nullptr, buf, MAX_PATH);
    outStr = buf;
#elif defined(__SWITCH__)
    outStr = "UNDEFINED";
#else
    outStr = __progname;
#endif
    return outStr;
  }

}

#endif //SIMPLE_CPP_LOGGER_LOGGERUTILS_IMPL_H
