cmake_minimum_required(VERSION 3.10 FATAL_ERROR)

if(POLICY CMP0077)
  cmake_policy(SET CMP0077 NEW) # Honor settings supplied by the parent project.
endif()
project(simple-cpp-logger LANGUAGES CXX)

set(SIMPLE_CPP_LOGGER_STANDALONE OFF)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(SIMPLE_CPP_LOGGER_STANDALONE ON)
endif()

option(SIMPLE_CPP_LOGGER_COMPILED "Compile the logger implementation into a library" OFF)
option(SIMPLE_CPP_LOGGER_BUILD_SHARED "Build the compiled logger as a shared library" "${BUILD_SHARED_LIBS}")
option(SIMPLE_CPP_LOGGER_BUILD_EXAMPLES "Build the logger example" ${SIMPLE_CPP_LOGGER_STANDALONE})
option(SIMPLE_CPP_LOGGER_BUILD_TESTS "Build the logger tests" ${SIMPLE_CPP_LOGGER_STANDALONE})

if(SIMPLE_CPP_LOGGER_COMPILED)
  if(SIMPLE_CPP_LOGGER_BUILD_SHARED)
    add_library(SimpleCppLogger SHARED src/Logger.cpp)
  else()
    add_library(SimpleCppLogger STATIC src/Logger.cpp)
  endif()
  target_compile_definitions(SimpleCppLogger PUBLIC SIMPLE_CPP_LOGGER_COMPILED)
  set_target_properties(SimpleCppLogger PROPERTIES POSITION_INDEPENDENT_CODE ON WINDOWS_EXPORT_ALL_SYMBOLS ON)
  set(LOGGER_USAGE_SCOPE PUBLIC)
  install(TARGETS SimpleCppLogger ARCHIVE DESTINATION lib LIBRARY DESTINATION lib RUNTIME DESTINATION bin)
else()
  add_library(SimpleCppLogger INTERFACE)
  set(LOGGER_USAGE_SCOPE INTERFACE)
endif()

target_include_directories(SimpleCppLogger ${LOGGER_USAGE_SCOPE} $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_compile_features(SimpleCppLogger ${LOGGER_USAGE_SCOPE} cxx_std_17)

if(SIMPLE_CPP_LOGGER_BUILD_EXAMPLES)
  add_executable(LoggerExample example/main.cpp example/ClassExample.cpp)
  target_link_libraries(LoggerExample PRIVATE SimpleCppLogger)
endif()

if(SIMPLE_CPP_LOGGER_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
