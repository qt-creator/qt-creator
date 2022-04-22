find_package(Googletest MODULE)

if(TARGET Googletest)
    message("Googletest target is present")
    extend_qtc_plugin(McuSupport
        CONDITION WITH_TESTS
        DEPENDS Googletest
        SOURCES
            unittest.h unittest.cpp packagemock.h settingshandlermock.h
        )
else()
    message("Googletest target is missing")
endif()
