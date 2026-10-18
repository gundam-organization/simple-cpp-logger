//
// Created by Nadrino on 24/08/2020.
//

#ifndef SIMPLE_CPP_LOGGER_LOGGER_H
#define SIMPLE_CPP_LOGGER_LOGGER_H

#include "implementation/LoggerParameters.h"
#include "implementation/LoggerContext.h"

#include <mutex>
#include <string>
#include <sstream>


// Here is what you want to use
#define LogFatal                (LogFatalImpl( true, false ))
#define LogError                (LogErrorImpl( true, false ))
#define LogAlert                (LogAlertImpl( true, false ))
#define LogWarning              (LogWarningImpl( true, false ))
#define LogInfo                 (LogInfoImpl( true, false ))
#define LogDebug                (LogDebugImpl( true, false ))
#define LogTrace                (LogTraceImpl( true, false ))

// conditional
#define LogFatalIf(isPrint_)    (LogFatalImpl( isPrint_, false ))
#define LogErrorIf(isPrint_)    (LogErrorImpl( isPrint_, false ))
#define LogAlertIf(isPrint_)    (LogAlertImpl( isPrint_, false ))
#define LogWarningIf(isPrint_)  (LogWarningImpl( isPrint_, false ))
#define LogInfoIf(isPrint_)     (LogInfoImpl( isPrint_, false ))
#define LogDebugIf(isPrint_)    (LogDebugImpl( isPrint_, false ))
#define LogTraceIf(isPrint_)    (LogTraceImpl( isPrint_, false ))

// once
#define LogFatalOnce            (LogFatalImpl( true, true ))
#define LogErrorOnce            (LogErrorImpl( true, true ))
#define LogAlertOnce            (LogAlertImpl( true, true ))
#define LogWarningOnce          (LogWarningImpl( true, true ))
#define LogInfoOnce             (LogInfoImpl( true, true ))
#define LogDebugOnce            (LogDebugImpl( true, true ))
#define LogTraceOnce            (LogTraceImpl( true, true ))

// To make assertions
#define LogThrowIf2(isThrowing_, errorMessage_)  if(isThrowing_){(LogError << "(" << __PRETTY_FUNCTION__ << "): "<< errorMessage_ << std::endl).throwError(#isThrowing_ ": " #errorMessage_);}
#define LogThrowIf1(isThrowing_) LogThrowIf2(isThrowing_, #isThrowing_)
#define LogThrowIf(...) GET_OVERLOADED_MACRO2(__VA_ARGS__, LogThrowIf2, LogThrowIf1)(__VA_ARGS__)
#define LogAssert(assertion_, errorMessage_)    LogThrowIf(not (assertion_), errorMessage_)
#define LogThrow(errorMessage_)                 LogThrowIf(true, errorMessage_)

// Assertion using std::exit() instead of throw
#define LogExitIf2(isExit_, errorMessage_)  if(isExit_){(LogError << "(" << __PRETTY_FUNCTION__ << "): "<< errorMessage_ << std::endl).triggerExit(#isExit_ ": " #errorMessage_);}
#define LogExitIf1(isExit_) LogExitIf2(isExit_, #isExit_)
#define LogExitIf(...) GET_OVERLOADED_MACRO2(__VA_ARGS__, LogExitIf2, LogExitIf1)(__VA_ARGS__)
#define LogExit(errorMessage_)                 LogExitIf(true, errorMessage_)

// Within loops
#define LogContinueIf2(isContinue_, continueMessage_)  if(isContinue_){(LogWarning << "(" << __PRETTY_FUNCTION__ << "): "<< continueMessage_ << std::endl); continue; }
#define LogContinueIf1(isContinue_)  LogContinueIf2(isContinue_, #isContinue_)
#define LogContinueIf(...) GET_OVERLOADED_MACRO2(__VA_ARGS__, LogContinueIf2, LogContinueIf1)(__VA_ARGS__)

// Within functions
#define LogReturnIf3(isReturn_, returnMessage_, returnObj_)  if(isReturn_){(LogWarning << "(" << __PRETTY_FUNCTION__ << "): "<< returnMessage_ << std::endl); return returnObj_; }
#define LogReturnIf2(isReturn_, returnMessage_)  LogReturnIf3(isReturn_, returnMessage_, )
#define LogReturnIf1(isReturn_)  LogReturnIf2(isReturn_, (#isReturn_))
#define LogReturnIf(...) GET_OVERLOADED_MACRO3(__VA_ARGS__, LogReturnIf3, LogReturnIf2, LogReturnIf1)(__VA_ARGS__)

#define LogScopeIndent Logger::ScopedIndent MAKE_VARNAME_LINE(scopeIndentTempObj);
#define LogIndent Logger::setIndentStr(Logger::getIndentStr() + "  ")
#define LogUnIndent Logger::setIndentStr(Logger::getIndentStr().substr(0, Logger::getIndentStr().size()-2))

// To set up the logger in a given source file
#define LoggerInit( lambdaInit ) LoggerInitializerImpl( lambdaInit )

// misc
#define DEBUG_VAR(myVar) LogDebug << "DEBUG_VAR: " << GET_VAR_NAME_VALUE(myVar) << std::endl


// The facade and its context have internal linkage: settings remain local to
// each translation unit in both modes. Muting is shared across all contexts.
namespace {

  class Logger {
  public:
    using LogLevel = LoggerDetail::LoggerContext::LogLevel;
    using PrefixLevel = LoggerDetail::LoggerContext::PrefixLevel;
    using ScopedMute = LoggerDetail::ScopedMute;

    static void setIsMuted(bool isMuted_){ _context_.setIsMuted(isMuted_); }
    static void setEnableColors(bool enableColors_){ _context_.setEnableColors(enableColors_); }
    static void setCleanLineBeforePrint(bool cleanLineBeforePrint_){ _context_.setCleanLineBeforePrint(cleanLineBeforePrint_); }
    static void setPropagateColorsOnUserHeader(bool propagateColorsOnUserHeader_){ _context_.setPropagateColorsOnUserHeader(propagateColorsOnUserHeader_); }
    static void setPrefixLevel(const PrefixLevel& prefixLevel_){ _context_.setPrefixLevel(prefixLevel_); }
    static void setUserHeaderStr(const std::string& userHeaderStr_){ _context_.setUserHeaderStr(userHeaderStr_); }
    static void setPrefixFormat(const std::string& prefixFormat_){ _context_.setPrefixFormat(prefixFormat_); }
    static void setIndentStr(const std::string& indentStr_){ _context_.setIndentStr(indentStr_); }
    static std::stringstream& getUserHeader(){ return _context_.getUserHeader(); }
    static std::string indent(){ LogIndent; return {}; }
    static std::string unIndent(){ LogUnIndent; return {}; }
    static void setMaxLogLevel(const Logger&){ _context_.setMaxLogLevel(); }
    static void setMaxLogLevel(){ _context_.setMaxLogLevel(); }
    static void setMaxLogLevel(int maxLogLevel_){ setMaxLogLevel(static_cast<LogLevel>(maxLogLevel_)); }
    static void setMaxLogLevel(const LogLevel& maxLogLevel_){ _context_.setMaxLogLevel(maxLogLevel_); }

    static bool isCleanLineBeforePrint(){ return _context_.isCleanLineBeforePrint(); }
    static bool isMuted(){ return _context_.isMuted(); }
    static int getMaxLogLevelInt(){ return static_cast<int>(getMaxLogLevel()); }
    static const LogLevel& getMaxLogLevel(){ return _context_.getMaxLogLevel(); }
    static const std::string& getIndentStr(){ return _context_.getIndentStr(); }
    static LoggerUtils::StreamBufferSupervisor* getStreamBufferSupervisorPtr(){ return _context_.getStreamBufferSupervisorPtr(); }
    static std::string getPrefixString(){ return _context_.getPrefixString(); }
    static std::string getPrefixString(const Logger&){ return getPrefixString(); }

    static void triggerNewLine(){ _context_.triggerNewLine(); }
    static void quietLineJump(){ _context_.quietLineJump(); }
    static void moveTerminalCursorBack(int nLines_, bool clearLines_ = false){ _context_.moveTerminalCursorBack(nLines_, clearLines_); }
    static void moveTerminalCursorForward(int nLines_, bool clearLines_ = false){ _context_.moveTerminalCursorForward(nLines_, clearLines_); }
    static void clearLine(){ _context_.clearLine(); }
    static void printNewLine(){ _context_.printNewLine(); }
    static void throwError(const std::string& errorStr_ = ""){ _context_.throwError(errorStr_); }
    static void triggerExit(const std::string& errorStr_ = ""){ _context_.triggerExit(errorStr_); }

    Logger(LogLevel logLevel_, const char* fileName_, int lineNumber_, bool once_ = false)
      : _lock_{_context_.getMutex()}{
      _context_.startMessage(logLevel_, fileName_, lineNumber_, once_);
    }

    template<typename... Args> void operator()(const char* fmtStr_, Args&&... args_){
      if( not _context_.isEnabled() ) return;
      _context_.print(LoggerUtils::formatString(fmtStr_, std::forward<Args>(args_)...));
      const auto length = std::strlen(fmtStr_);
      if( length != 0 and fmtStr_[length - 1] != '\n' ) printNewLine();
    }
    template<typename T> Logger& operator<<(const T& data_){
      if( not _context_.isEnabled() ) return *this;
      std::stringstream dataStream;
      dataStream << data_;
      const auto message = dataStream.str();
      if( not message.empty() ) _context_.print(message);
      return *this;
    }
    Logger& operator<<(std::ostream& (*f_)(std::ostream&)){
      _context_.printManipulator(f_);
      return *this;
    }
    Logger& operator<<(Logger&){ return *this; }
    Logger& operator()(bool condition_){ _context_.setCondition(condition_); return *this; }

    struct ScopedIndent {
      ScopedIndent(){ LogIndent; }
      ~ScopedIndent(){ LogUnIndent; }
    };

  private:
    static inline LoggerDetail::LoggerContext _context_{LoggerDetail::LoggerSettings{
      LOGGER_ENABLE_COLORS, LOGGER_ENABLE_COLORS_ON_USER_HEADER, LOGGER_WRITE_OUTFILE,
      LOGGER_MAX_LOG_LEVEL_PRINTED, LOGGER_PREFIX_LEVEL, LOGGER_PREFIX_FORMAT,
      LOGGER_TIME_FORMAT, LOGGER_OUTFILE_FOLDER, LOGGER_OUTFILE_NAME_FORMAT
    }};
    std::scoped_lock<std::mutex> _lock_;
  };

}

#endif // SIMPLE_CPP_LOGGER_LOGGER_H
