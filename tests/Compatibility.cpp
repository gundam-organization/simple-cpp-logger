// Build and run with -std=c++14 and -std=c++17, -Iinclude and -pthread.
#include "Logger.h"

#include <cassert>
#include <sstream>

int main(){
  std::ostringstream output;
  LogInfoIf(false) << "";
  auto* supervisor = Logger::getStreamBufferSupervisorPtr();
  supervisor->setStreamBuffer(output.rdbuf());
  Logger::setEnableColors(false);
  Logger::setPrefixFormat("");
  for( int i = 0 ; i < 3 ; ++i ){
    LogInfoOnce << "once-marker" << std::endl;
    LogInfo << "regular-marker" << std::endl;
  }
  LogInfoOnce << "other-marker" << std::endl;
  // Keep the supervisor attached to a stream buffer that outlives this capture.
  supervisor->setStreamBuffer(std::cerr.rdbuf());

  const auto logs = output.str();
  const auto once = logs.find("once-marker");
  assert(once != std::string::npos);
  assert(logs.find("once-marker", once + 1) == std::string::npos);
  assert(logs.find("other-marker") != std::string::npos);
  size_t begin{0};
  int regularCount{0};
  while( (begin = logs.find("regular-marker", begin)) != std::string::npos ){
    ++regularCount;
    ++begin;
  }
  assert(regularCount == 3);
}
