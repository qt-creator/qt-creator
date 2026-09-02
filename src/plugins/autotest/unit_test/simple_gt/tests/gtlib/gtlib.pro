include(../gtest_include.pri)

TEMPLATE = lib
CONFIG += staticlib console c++11
CONFIG -= app_bundle
CONFIG += thread
CONFIG -= qt

TARGET = gtlibtests

SOURCES += gtlibtests.cpp

HEADERS += gtlibtests.h
