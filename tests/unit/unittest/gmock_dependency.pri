isEmpty(GOOGLETEST_DIR):GOOGLETEST_DIR=$$(GOOGLETEST_DIR)


defineTest(setGoogleTestDirectories) {
    DIRECTORY = $$1
    GTEST_INCLUDE_DIR = $$DIRECTORY/googletest
    GMOCK_INCLUDE_DIR = $$DIRECTORY/googlemock
    GTEST_SRC_DIR = $$GTEST_INCLUDE_DIR
    GMOCK_SRC_DIR = $$GMOCK_INCLUDE_DIR
    export(GTEST_INCLUDE_DIR)
    export(GMOCK_INCLUDE_DIR)
    export(GTEST_SRC_DIR)
    export(GMOCK_SRC_DIR)
}

isEmpty(GOOGLETEST_DIR) {
    exists($$PWD/../../../../googletest) {
        setGoogleTestDirectories($$PWD/../../../../googletest)
    } else: exists($$PWD/../../../../../googletest) {
        setGoogleTestDirectories($$PWD/../../../../../googletest)
    } else: linux {
        GTEST_INCLUDE_DIR = /usr/include/gtest
        GMOCK_INCLUDE_DIR = /usr/include/gmock
        GTEST_SRC_DIR = /usr/src/gtest
        GMOCK_SRC_DIR = /usr/src/gmock
    }
} else {
    setGoogleTestDirectories($$GOOGLETEST_DIR)
}

requires(exists($$GTEST_SRC_DIR):exists($$GMOCK_SRC_DIR))
!exists($$GTEST_SRC_DIR):message("No gmock is found! To enable unit tests set GOOGLETEST_DIR")

DEFINES += \
    GTEST_HAS_STD_INITIALIZER_LIST_ \
    GTEST_LANG_CXX11

INCLUDEPATH *= \
    $$GTEST_INCLUDE_DIR \
    $$GTEST_INCLUDE_DIR/include \
    $$GTEST_SRC_DIR \
    $$GMOCK_INCLUDE_DIR \
    $$GMOCK_INCLUDE_DIR/include \
    $$GMOCK_SRC_DIR

SOURCES += \
    $$GMOCK_SRC_DIR/src/gmock-all.cc \
    $$GTEST_SRC_DIR/src/gtest-all.cc

HEADERS += \
    $$PWD/gtest-qt-printing.h
