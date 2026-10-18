// Different compile-time defaults must survive linking in either mode.
#undef LOGGER_ENABLE_COLORS
#undef LOGGER_PREFIX_LEVEL
#undef LOGGER_PREFIX_FORMAT
#undef LOGGER_TIME_FORMAT
#undef LOGGER_MAX_LOG_LEVEL_PRINTED
#define LOGGER_ENABLE_COLORS 0
#define LOGGER_PREFIX_LEVEL 3
#define LOGGER_PREFIX_FORMAT "{TIME} {USER_HEADER} {FILELINE}"
#define LOGGER_TIME_FORMAT "PEER_TIME"
#define LOGGER_MAX_LOG_LEVEL_PRINTED 4
#include "Logger.h"

#include "TranslationUnitPeer.h"

bool isPeerMuted(){ return Logger::isMuted(); }
void mutePeer(bool mute_){ Logger::setIsMuted(mute_); }

void emitPeerMessages(){
  Logger::setUserHeaderStr("PEER");
  LogInfo << "peer-message" << std::endl;
  LogDebug << "peer-debug-hidden" << std::endl;
}

void emitPeerOnce(){
  LogInfoOnce << "peer-once" << std::endl;
}

void emitPeerWithinMutedScope(){
  Logger::ScopedMute muteLogger;
  emitPeerMessages();
}
