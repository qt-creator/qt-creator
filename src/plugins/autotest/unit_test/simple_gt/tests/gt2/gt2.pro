include(../gtest_dependency.pri)

TEMPLATE = app
CONFIG += qt console c++11
CONFIG -= app_bundle

HEADERS += \
    queuetest.h

SOURCES += \
    main.cpp

