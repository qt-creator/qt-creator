extend_qtc_plugin(McuSupport
    CONDITION WITH_TESTS AND TARGET Googletest
    DEPENDS Googletest
    DEFINES GOOGLE_TEST_IS_FOUND
    SOURCES
      unittest.h unittest.cpp
      packagemock.h
      settingshandlermock.h
)
