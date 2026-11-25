// Exercise the same logging API in header-only and compiled C++17 modes.
#ifdef NDEBUG
#undef NDEBUG
#endif
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

#ifdef SIMPLE_CPP_LOGGER_COMPILED
  supervisor->setStreamBuffer(output.rdbuf());
  LoggerUtils::StreamCapture capture;
  *supervisor << "heading\nprogress 10%\rprogress 100%\r\n";
  *supervisor << "old first row\nold second row\n";
  *supervisor << '\033' << "[" << 2 << "F" << "\033[2K";
  *supervisor << "\033[32mnew first row\033[0m\nnew second row\n";
  *supervisor << "done" << std::endl;
  std::cout << "stdout\n";
  std::cerr << "stderr\n";
  assert(capture.drain(5, 4).empty());
  const std::vector<std::string> expectedBatch{"heading", "progress 100%", "new first row"};
  assert(capture.drain(5, 3) == expectedBatch);
  std::cout << "\033[2F" << "updated stdout\n\033[1E";
  const std::vector<std::string> expectedTail{"new second row", "done", "updated stdout", "stderr"};
  assert(capture.finishLines() == expectedTail);
  supervisor->setStreamBuffer(std::cerr.rdbuf());
#endif
}
