#ifdef NDEBUG
#undef NDEBUG
#endif
#undef LOGGER_ENABLE_COLORS
#undef LOGGER_PREFIX_LEVEL
#undef LOGGER_PREFIX_FORMAT
#define LOGGER_ENABLE_COLORS 0
#define LOGGER_PREFIX_LEVEL 3
#define LOGGER_PREFIX_FORMAT "MAIN {USER_HEADER} {FILELINE}"
#include "Logger.h"

#include "TranslationUnitPeer.h"

#include <cassert>
#include <stdexcept>

int main(){
  std::ostringstream output;
  LogInfoIf(false) << "";
  auto* supervisor = Logger::getStreamBufferSupervisorPtr();
  supervisor->setStreamBuffer(output.rdbuf());
  Logger::setUserHeaderStr("LOCAL");
  Logger::setMaxLogLevel(Logger::LogLevel::TRACE);

  LogInfo << "local-message" << std::endl;
  emitPeerMessages();
  // Peer prefix, user header and verbosity must not affect the caller.
  LogDebug << "local-debug" << std::endl;
  assert(Logger::getMaxLogLevel() == Logger::LogLevel::TRACE);

  const auto beforeMute = output.str();
  {
    Logger::ScopedMute muteLogger;
    assert(Logger::isMuted());
    assert(isPeerMuted());
    LogInfo << "hidden-local" << std::endl;
    emitPeerMessages();
    emitPeerWithinMutedScope();
    assert(Logger::isMuted());
    assert(isPeerMuted());
  }
  assert(not Logger::isMuted());
  assert(output.str() == beforeMute);

  // A conditional guard must preserve an already active global mute.
  mutePeer(true);
  {
    Logger::ScopedMute muteLogger{false};
    emitPeerWithinMutedScope();
  }
  assert(Logger::isMuted());
  mutePeer(false);
  assert(not Logger::isMuted());

  try{
    Logger::ScopedMute muteLogger;
    emitPeerMessages();
    throw std::runtime_error("scope exit");
  }
  catch(const std::runtime_error&){}
  assert(not isPeerMuted());
  assert(output.str() == beforeMute);

  for(int i = 0; i < 3; ++i){
    LogInfoOnce << "local-once" << std::endl;
    emitPeerOnce();
  }
  // printf formatting and multiline output use the compiled implementation too.
  LogInfo("value=%d, percent=100%%", 42);
  LogInfo << "first-line\nsecond-line" << std::endl;
  Logger::setPrefixFormat(" ");
  LogInfo << "empty-prefix" << std::endl;
  Logger::setPrefixFormat("{FILELINE}");
  const auto expectedLine = __LINE__ + 1;
  LogInfo << "line-marker" << std::endl;
  try{ LogThrow("test-error"); }
  catch(const std::runtime_error& error_){
    assert(std::string(error_.what()).find("TranslationUnits.cpp:") != std::string::npos);
  }

  supervisor->setStreamBuffer(std::cerr.rdbuf());
  const auto logs = output.str();
  assert(logs.find("MAIN LOCAL TranslationUnits.cpp:") != std::string::npos);
  assert(logs.find("PEER_TIME PEER TranslationUnitPeer.cpp:") != std::string::npos);
  assert(logs.find("local-debug") != std::string::npos);
  assert(logs.find("peer-debug-hidden") == std::string::npos);
  for(const std::string marker : {"local-once", "peer-once"}){
    const auto first = logs.find(marker);
    assert(first != std::string::npos);
    assert(logs.find(marker, first + marker.size()) == std::string::npos);
  }
  assert(logs.find("value=42, percent=100%") != std::string::npos);
  assert(logs.find("second-line") != std::string::npos);
  assert(logs.find("TranslationUnits.cpp:" + std::to_string(expectedLine) + ": line-marker") != std::string::npos);
}
