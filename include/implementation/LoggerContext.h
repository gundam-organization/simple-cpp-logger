#pragma once

#include "LoggerUtils.h"

#include <atomic>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_set>

namespace LoggerDetail {

  // Values come from the including translation unit, even in compiled mode.
  // Keep macro-dependent defaults out of types with external linkage.
  struct LoggerSettings {
    bool enableColors;
    bool enableColorsOnUserHeader;
    bool cleanLineBeforePrint;
    int maxLogLevel;
    int prefixLevel;
    std::string prefixFormat;
    std::string timeFormat;
    std::string outFileFolder;
    std::string outFileNameFormat;
  };

  class LoggerContext {
  public:
    enum class LogLevel {
      FATAL   = 0,
      ERROR   = 1,
      ALERT   = 2,
      WARNING = 3,
      INFO    = 4,
      DEBUG   = 5,
      TRACE   = 6,
      INVALID = 7
    };
    enum class PrefixLevel {
      NONE        = 0,
      MINIMAL     = 1,
      PRODUCTION  = 2,
      DEBUG       = 3,
      FULL        = 4
    };

    SIMPLE_CPP_LOGGER_INLINE explicit LoggerContext(LoggerSettings settings_);

    SIMPLE_CPP_LOGGER_INLINE static void setIsMuted(bool isMuted_);
    void setEnableColors(bool enableColors_){ _enableColors_ = enableColors_; }
    void setCleanLineBeforePrint(bool cleanLineBeforePrint_){ _cleanLineBeforePrint_ = cleanLineBeforePrint_; }
    void setPropagateColorsOnUserHeader(bool propagateColorsOnUserHeader_){ _propagateColorsOnUserHeader_ = propagateColorsOnUserHeader_; }
    void setPrefixLevel(const PrefixLevel& prefixLevel_){ _prefixLevel_ = prefixLevel_; }
    void setUserHeaderStr(const std::string& userHeaderStr_){ _userHeaderSs_.str(userHeaderStr_); }
    void setPrefixFormat(const std::string& prefixFormat_){ _prefixFormat_ = prefixFormat_; }
    void setIndentStr(const std::string& indentStr_){ _indentStr_ = indentStr_; }
    void setMaxLogLevel(const LogLevel& maxLogLevel_){ _maxLogLevel_ = maxLogLevel_; }
    void setMaxLogLevel(){ _maxLogLevel_ = _currentLogLevel_; }

    bool isCleanLineBeforePrint() const { return _cleanLineBeforePrint_; }
    SIMPLE_CPP_LOGGER_INLINE static bool isMuted();
    bool isEnabled() const { return not isMuted() and _currentLogLevel_ <= _maxLogLevel_; }
    const LogLevel& getMaxLogLevel() const { return _maxLogLevel_; }
    const std::string& getIndentStr() const { return _indentStr_; }
    std::stringstream& getUserHeader(){ return _userHeaderSs_; }
    std::mutex& getMutex(){ return _loggerMutex_; }
    LoggerUtils::StreamBufferSupervisor* getStreamBufferSupervisorPtr(){ return _streamBufferSupervisorPtr_; }
    void triggerNewLine(){ _isNewLine_ = true; }
    void setCondition(bool condition_){ if( not condition_ ) _currentLogLevel_ = LogLevel::INVALID; }

    SIMPLE_CPP_LOGGER_INLINE void startMessage(LogLevel logLevel_, const char* fileName_, int lineNumber_, bool once_);
    SIMPLE_CPP_LOGGER_INLINE std::string getPrefixString();
    SIMPLE_CPP_LOGGER_INLINE void quietLineJump();
    SIMPLE_CPP_LOGGER_INLINE void moveTerminalCursorBack(int nLines_, bool clearLines_ = false);
    SIMPLE_CPP_LOGGER_INLINE void moveTerminalCursorForward(int nLines_, bool clearLines_ = false);
    SIMPLE_CPP_LOGGER_INLINE void clearLine();
    SIMPLE_CPP_LOGGER_INLINE void printNewLine();
    SIMPLE_CPP_LOGGER_INLINE void print(const std::string& formattedString);
    SIMPLE_CPP_LOGGER_INLINE void printManipulator(std::ostream& (*f)(std::ostream&));
    SIMPLE_CPP_LOGGER_INLINE void throwError(const std::string& errorStr_);
    SIMPLE_CPP_LOGGER_INLINE void triggerExit(const std::string& errorStr_);

  private:
    SIMPLE_CPP_LOGGER_INLINE void buildCurrentPrefix();
    SIMPLE_CPP_LOGGER_INLINE void generateUserHeader(std::string& strBuffer_);
    std::string generateUserHeader(){ std::string out; generateUserHeader(out); return out; }
    SIMPLE_CPP_LOGGER_INLINE std::string getLogLevelColorStr(const LogLevel& selectedLogLevel_);
    SIMPLE_CPP_LOGGER_INLINE std::string getLogLevelStr(const LogLevel& selectedLogLevel_);
    SIMPLE_CPP_LOGGER_INLINE void setupStreamBufferSupervisor();
    SIMPLE_CPP_LOGGER_INLINE void setupOutputFile();

    SIMPLE_CPP_LOGGER_INLINE static std::atomic<bool>& getMuteState();

    const LoggerSettings _settings_;
    // parameters
    bool _enableColors_{_settings_.enableColors};
    bool _propagateColorsOnUserHeader_{_settings_.enableColorsOnUserHeader};
    bool _cleanLineBeforePrint_{_settings_.cleanLineBeforePrint};
    bool _writeInOutputFile_{false};
    std::string _prefixFormat_{};
    std::string _indentStr_{};
    std::stringstream _userHeaderSs_{};
    LogLevel _maxLogLevel_{static_cast<LoggerContext::LogLevel>(_settings_.maxLogLevel)};
    PrefixLevel _prefixLevel_{static_cast<LoggerContext::PrefixLevel>(_settings_.prefixLevel)};

    // internal
    bool _isNewLine_{true};
    int _currentLineNumber_{-1};
    std::string _currentFileName_{};
    std::string _currentPrefix_{};
    std::string _outputFileName_{};
    std::mutex _loggerMutex_{};
    std::unordered_set<size_t> _onceLogList_{};
    LogLevel _currentLogLevel_{LoggerContext::LogLevel::TRACE};
    LoggerUtils::StreamBufferSupervisor* _streamBufferSupervisorPtr_{nullptr};

  };

  // Muting is shared by all translation units, including in header-only mode.
  class ScopedMute {
  public:
    explicit ScopedMute(bool mute_ = true) : _wasMuted_{LoggerContext::isMuted()}{
      if( mute_ ) LoggerContext::setIsMuted(true);
    }
    ~ScopedMute(){ LoggerContext::setIsMuted(_wasMuted_); }
    ScopedMute(const ScopedMute&) = delete;
    ScopedMute& operator=(const ScopedMute&) = delete;
    ScopedMute(ScopedMute&&) = delete;
    ScopedMute& operator=(ScopedMute&&) = delete;

  private:
    bool _wasMuted_;
  };

}

#ifndef SIMPLE_CPP_LOGGER_COMPILED
#include "Logger.impl.h"
#endif
