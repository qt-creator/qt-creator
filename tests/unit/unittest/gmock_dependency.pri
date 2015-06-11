GMOCK_DIR = $$(GMOCK_DIR)
!isEmpty(GMOCK_DIR):GTEST_DIR = $$GMOCK_DIR/gtest
linux-* {
    isEmpty(GMOCK_DIR):GMOCK_DIR = /usr/include/gmock
    !exists($$GTEST_DIR):GTEST_DIR = /usr/include/gtest
}

requires(exists($$GMOCK_DIR))
!exists($$GMOCK_DIR):message("No gmock is found! To enabe unit tests set GMOCK_DIR")

GTEST_SRC_DIR = $$GTEST_DIR
GMOCK_SRC_DIR = $$GMOCK_DIR
linux-* {
    !exists($$GTEST_SRC_DIR/src/gtest-all.cc):GTEST_SRC_DIR = /usr/src/gtest
    !exists($$GMOCK_SRC_DIR/src/gmock-all.cc):GMOCK_SRC_DIR = /usr/src/gmock
}

DEFINES += \
    GTEST_HAS_STD_INITIALIZER_LIST_ \
    GTEST_LANG_CXX11

INCLUDEPATH *= \
    $$GTEST_DIR \
    $$GTEST_DIR/include \
    $$GMOCK_DIR \
    $$GMOCK_DIR/include \
    $$GTEST_SRC_DIR \
    $$GMOCK_SRC_DIR

SOURCES += \
    $$GMOCK_SRC_DIR/src/gmock-all.cc \
    $$GTEST_SRC_DIR/src/gtest-all.cc

HEADERS += \
    $$PWD/gtest-qt-printing.h
