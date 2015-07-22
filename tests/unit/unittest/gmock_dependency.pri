GMOCK_DIR = $$(GMOCK_DIR)
GTEST_DIR = $$GMOCK_DIR/gtest

requires(exists($$GMOCK_DIR))
!exists($$GMOCK_DIR):message("No gmock is found! To enabe unit tests set GMOCK_DIR")

DEFINES += \
    GTEST_HAS_STD_INITIALIZER_LIST_ \
    GTEST_LANG_CXX11

INCLUDEPATH += \
    $$GTEST_DIR \
    $$GTEST_DIR/include \
    $$GMOCK_DIR \
    $$GMOCK_DIR/include

SOURCES += \
    $$GMOCK_DIR/src/gmock-all.cc \
    $$GTEST_DIR/src/gtest-all.cc

HEADERS += \
    $$PWD/gtest-qt-printing.h
