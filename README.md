![](resources/logo/simple-cpp-logger-logo_flat.png)

[![CC-0 license](https://img.shields.io/badge/License-CC--0-blue.svg)](https://creativecommons.org/licenses/by-nd/4.0)
[![GitHub version](https://badge.fury.io/gh/nadrino%2Fsimple-cpp-logger.svg)](https://github.com/nadrino/simple-cpp-logger/releases/)
[![Github all releases](https://img.shields.io/github/downloads/nadrino/simple-cpp-logger/total.svg)](https://GitHub.com/nadrino/simple-cpp-logger/releases/)


## Description 

This simple header-only library written in C++ provides logging features with handy options.
Because it's written as a header-only library, you can easily add it to your current project. 
An optional compiled mode builds the implementation once in `SimpleCppLogger`, while keeping the same logging API.


## Features

- 7 different levels of logging severity: Fatal, Error, Alert, Warning, Info, Debug and Trace.
- Both "std::cout <<" and "printf()" call-style are available with the same names.
- Ajustable prefix can display the **Time**, **Severity**, **Current Filename**, **Current Line Number**, **Current Thread ID** and even a custom header string set by the user.
- Ability to mute prints based on their severity. 
- Option to disable colors on the prefix.
- Static members (user options) of this header class are globally settable with preprocessor macros (see cmake's add_definitions calls).
- In-string "\n" char are recognised and the prefix is automatically reprinted.


## Showcase

Here is a screenshot of the first output lines you get while running the example.

![](resources/screenshot/example_showcase.png)

If you want to see all the features embedded in the example, feel free to build and run it yourself: 

```bash
git clone https://github.com/nadrino/simple-cpp-logger.git
mkdir simple-cpp-logger/build
cd simple-cpp-logger/build
cmake ../
make
./LoggerExample
```


## Requirements

- **C++17 or higher is required to use this logger.**


## Getting Setup

You can add simple-cpp-logger to your existing project as a submodule for example.
In this guide we assume that your project is using CMake.
In your CMakeLists.txt, add the following lines:

```cmake
include_directories(path/to/simple-cpp-logger/include)
```

Alternatively, use the CMake target. Header-only mode is the default:

```cmake
add_subdirectory(path/to/simple-cpp-logger)
target_link_libraries(MyApplication PRIVATE SimpleCppLogger)
```

To compile the implementation into a library:

```cmake
set(SIMPLE_CPP_LOGGER_COMPILED ON)
add_subdirectory(path/to/simple-cpp-logger)
target_link_libraries(MyApplication PRIVATE SimpleCppLogger)
```

`SIMPLE_CPP_LOGGER_BUILD_SHARED` selects a shared library; it defaults to
`BUILD_SHARED_LIBS`. Otherwise the compiled library is static and can also be
linked into shared libraries. The target propagates the include directory,
C++17 requirement and `SIMPLE_CPP_LOGGER_COMPILED` definition to consumers.
Without CMake, compile `src/Logger.cpp`, define `SIMPLE_CPP_LOGGER_COMPILED` for
all consumers and link the resulting library. Use the same mode throughout
one program.

In compiled mode, non-template context and utility implementations are emitted
only in `src/Logger.cpp`. The header retains the logging macros, templates and
small facade. File names and line numbers still refer to the logging call site.
Compile-time configuration is passed to each file's context, so local defaults
continue to work in both modes.

Examples and tests are built by default for standalone builds and disabled by
default when included as a subdirectory. Enable them with
`SIMPLE_CPP_LOGGER_BUILD_EXAMPLES` and `SIMPLE_CPP_LOGGER_BUILD_TESTS`.
The tests exercise both modes:

```bash
cmake -S . -B build -DSIMPLE_CPP_LOGGER_COMPILED=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Preprocessor variables are defined in `Logger.h`, and can be changed by adding the following lines to CMakeLists.txt:

```cmake
add_definitions( -D LOGGER_MAX_LOG_LEVEL_PRINTED=6 )
add_definitions( -D LOGGER_PREFIX_LEVEL=2 )
add_definitions( -D LOGGER_ENABLE_COLORS=1 )
add_definitions( -D LOGGER_ENABLE_COLORS_ON_USER_HEADER=0 )
```

Now you can include the header in your source file:

```cpp
#include <Logger.h>
```

Then you're all set!


## Implementation Guidelines

Here are simple example lines of implementation. 
**If you want to know more about the available features, you can refer to "example/main.cpp".**

Let's print a simple line:

```cpp
LogInfo << "My very important message" << std::endl;
```

You have just printed a line on screen with the tag "Info". 
`LogInfo` is actually a preprocessor macros which automatically passes the **filename** and the **line number** from where it's called.

It exists different variants of this same call: `LogFatal`, `LogError`, `LogAlert`, `LogWarning`, `LogInfo`, `LogDebug` and `LogTrace`.
Each of these will display their proper verbosity tag.

These calls can as well be made in the style of the C function `printf()`:

```cpp
LogInfo("The variable is: %i", my_int);
```

Few user parameters can be access from the Logger class.
For example let's change the highest printed log level:

```cpp
Logger::setMaxLogLevel(Logger::LogLevel::INFO);
```

From this moment, all following Log calls will be printed if their associated log level is lower than `INFO`.
This means that `LogFatal`, `LogError`, `LogAlert`, `LogWarning` and `LogInfo` will be printed, but `LogDebug` and `LogTrace` will be muted. 

Verbosity and prefix settings apply only to the current source file. Muting is shared across source files (see below).
For a global effect, you need to set the associated preprocessor variables accordingly (cf. CMakeLists.txt).


### Muting a scope

Muting is shared across all source files in **both modes**, including dependencies
that use this logger. Prefixes, user headers and verbosity remain local to each
source file.

```cpp
{
  Logger::ScopedMute muteLogger;
  callDependencies(); // Their logs are muted too.
} // Restore the previous mute state, including on exception.
```

Nested guards preserve an outer mute. `Logger::ScopedMute muteLogger{quiet};`
only enables muting when `quiet` is true; it preserves an existing mute either
way. `Logger::setIsMuted(true/false)` and `Logger::isMuted()` also use the shared
state. A mute affects all threads; scoped guards restore the state they observed
on entry and should be nested rather than overlap independently across threads.
