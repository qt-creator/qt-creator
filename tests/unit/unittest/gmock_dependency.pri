GOOGLETEST_DIR = $$(GOOGLETEST_DIR)

isEmpty(GOOGLETEST_DIR):linux-* {
    GTEST_SRC_DIR = /usr/include/gmock
    GMOCK_SRC_DIR = /usr/include/gtest
} else {
    GTEST_SRC_DIR = $$GOOGLETEST_DIR/googletest
    GMOCK_SRC_DIR = $$GOOGLETEST_DIR/googlemock
}

requires(exists($$GTEST_SRC_DIR):exists($$GMOCK_SRC_DIR))
!exists($$GOOGLETEST_DIR):message("No gmock is found! To enabe unit tests set GOOGLETEST_DIR")

DEFINES += \
    GTEST_HAS_STD_INITIALIZER_LIST_ \
    GTEST_LANG_CXX11

INCLUDEPATH *= \
    $$GTEST_SRC_DIR \
    $$GTEST_SRC_DIR/include \
    $$GMOCK_SRC_DIR \
    $$GMOCK_SRC_DIR/include \

SOURCES += \
    $$GMOCK_SRC_DIR/src/gmock-all.cc \
    $$GTEST_SRC_DIR/src/gtest-all.cc

HEADERS += \
    $$PWD/gtest-qt-printing.h
