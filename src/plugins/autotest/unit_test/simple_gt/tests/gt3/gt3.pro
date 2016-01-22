include(../gtest_dependency.pri)

TEMPLATE = app
CONFIG += qt console c++11
CONFIG -= app_bundle

HEADERS += \
    dummytest.h

SOURCES += \
    main.cpp

