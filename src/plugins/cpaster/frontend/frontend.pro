TEMPLATE = app
TARGET=cpaster

include(../../../../qtcreator.pri)
include(../../../rpath.pri)
include(../../../plugins/coreplugin/coreplugin.pri)

CONFIG += console
CONFIG -= app_bundle
QT += network

LIBS *= -L$$IDE_PLUGIN_PATH/QtProject
QMAKE_RPATHDIR *= $$IDE_PLUGIN_PATH/QtProject

DESTDIR=$$IDE_APP_PATH

HEADERS = ../protocol.h \
    ../cpasterconstants.h \
    ../pastebindotcomprotocol.h \
    ../pastebindotcaprotocol.h \
    ../kdepasteprotocol.h \
    ../urlopenprotocol.h \
    argumentscollector.h

SOURCES += ../protocol.cpp \
    ../pastebindotcomprotocol.cpp \
    ../pastebindotcaprotocol.cpp \
    ../kdepasteprotocol.cpp \
    ../urlopenprotocol.cpp \
    argumentscollector.cpp \
    main.cpp
