add_qtc_executable(echo
  DEPENDS Qt5::Core Qt5::Network ClangSupport
  DEFINES CLANGSUPPORT_TESTS DONT_CHECK_MESSAGE_COUNTER
  SOURCES
    echoclangcodemodelserver.cpp echoclangcodemodelserver.h
    echoserverprocessmain.cpp
  SKIP_INSTALL
)
