include(../../../qtcreator.pri)
include(../../rpath.pri)
include(../../libs/utils/utils.pri)

CONFIG += console
CONFIG -= app_bundle

QT -= gui test

isEmpty(PRECOMPILED_HEADER):PRECOMPILED_HEADER = $$PWD/../../shared/qtcreator_pch.h

SOURCES += \
    main.cpp \
    addkeysoperation.cpp \
    addkitoperation.cpp \
    addqtoperation.cpp \
    addtoolchainoperation.cpp \
    findkeyoperation.cpp \
    findvalueoperation.cpp \
    getoperation.cpp \
    operation.cpp \
    rmkeysoperation.cpp \
    rmkitoperation.cpp \
    rmqtoperation.cpp \
    rmtoolchainoperation.cpp \
    settings.cpp \

HEADERS += \
    addkeysoperation.h \
    addkitoperation.h \
    addqtoperation.h \
    addtoolchainoperation.h \
    findkeyoperation.h \
    findvalueoperation.h \
    getoperation.h \
    operation.h \
    rmkeysoperation.h \
    rmkitoperation.h \
    rmqtoperation.h \
    rmtoolchainoperation.h \
    settings.h \

DESTDIR=$$IDE_LIBEXEC_PATH
macx:DEFINES += "DATA_PATH=\"\\\".\\\"\""
else:DEFINES += "DATA_PATH=\"\\\"../share/qtcreator\\\"\""

target.path  = $$QTC_PREFIX/bin # FIXME: libexec, more or less
INSTALLS += target
