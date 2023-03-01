if (MSVC)
    add_compile_definitions(NOMINMAX UNICODE _UNICODE)
endif()

file(WRITE ${CMAKE_BINARY_DIR}/GenVersion.h.in [=[
const char GenVersion_Version[] = "@VERSION@";
const char GenVersion_Commit[] = "@COMMIT_HASH@";
]=])

file(READ ../VERSION.txt VERSION)
string(REPLACE "\n" "" VERSION "${VERSION}")
configure_file(${CMAKE_BINARY_DIR}/GenVersion.h.in ${CMAKE_BINARY_DIR}/GenVersion.h @ONLY)

set(shared_sources
    shared/AgentMsg.h
    shared/BackgroundDesktop.h
    shared/BackgroundDesktop.cc
    shared/Buffer.h
    shared/Buffer.cc
    shared/DebugClient.h
    shared/DebugClient.cc
    shared/GenRandom.h
    shared/GenRandom.cc
    shared/OsModule.h
    shared/OwnedHandle.h
    shared/OwnedHandle.cc
    shared/StringBuilder.h
    shared/StringUtil.cc
    shared/StringUtil.h
    shared/UnixCtrlChars.h
    shared/WindowsSecurity.cc
    shared/WindowsSecurity.h
    shared/WindowsVersion.h
    shared/WindowsVersion.cc
    shared/WinptyAssert.h
    shared/WinptyAssert.cc
    shared/WinptyException.h
    shared/WinptyException.cc
    shared/WinptyVersion.h
    shared/WinptyVersion.cc
    shared/winpty_snprintf.h
)

#
# winpty-agent
#

add_qtc_executable(winpty-agent
  INCLUDES
    include ${CMAKE_BINARY_DIR}
  DEFINES WINPTY_AGENT_ASSERT
  PROPERTIES QT_COMPILE_OPTIONS_DISABLE_WARNINGS ON
  SOURCES
    agent/Agent.h
    agent/Agent.cc
    agent/AgentCreateDesktop.h
    agent/AgentCreateDesktop.cc
    agent/ConsoleFont.cc
    agent/ConsoleFont.h
    agent/ConsoleInput.cc
    agent/ConsoleInput.h
    agent/ConsoleInputReencoding.cc
    agent/ConsoleInputReencoding.h
    agent/ConsoleLine.cc
    agent/ConsoleLine.h
    agent/Coord.h
    agent/DebugShowInput.h
    agent/DebugShowInput.cc
    agent/DefaultInputMap.h
    agent/DefaultInputMap.cc
    agent/DsrSender.h
    agent/EventLoop.h
    agent/EventLoop.cc
    agent/InputMap.h
    agent/InputMap.cc
    agent/LargeConsoleRead.h
    agent/LargeConsoleRead.cc
    agent/NamedPipe.h
    agent/NamedPipe.cc
    agent/Scraper.h
    agent/Scraper.cc
    agent/SimplePool.h
    agent/SmallRect.h
    agent/Terminal.h
    agent/Terminal.cc
    agent/UnicodeEncoding.h
    agent/Win32Console.cc
    agent/Win32Console.h
    agent/Win32ConsoleBuffer.cc
    agent/Win32ConsoleBuffer.h
    agent/main.cc
    ${shared_sources}
)

#
# libwinpty
#

add_qtc_library(winpty STATIC
  INCLUDES ${CMAKE_BINARY_DIR}
  PUBLIC_DEFINES COMPILING_WINPTY_DLL
  PROPERTIES QT_COMPILE_OPTIONS_DISABLE_WARNINGS ON
  SOURCES
    libwinpty/AgentLocation.cc
    libwinpty/AgentLocation.h
    libwinpty/winpty.cc
    ${shared_sources}
)

target_include_directories(winpty
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
