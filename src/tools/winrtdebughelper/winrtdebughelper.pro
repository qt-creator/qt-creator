CONFIG          += warn_on
CONFIG          -= qt

include(../../qtcreatortool.pri)

SOURCES = winrtdebughelper.cpp

build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}

TARGET = winrtdebughelper
