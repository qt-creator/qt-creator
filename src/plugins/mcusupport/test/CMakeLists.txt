extend_qtc_plugin(McuSupport
    CONDITION WITH_TESTS AND TARGET Googletest
    DEPENDS Googletest
    SOURCES
      unittest.h unittest.cpp
      packagemock.h
      settingshandlermock.h
)
