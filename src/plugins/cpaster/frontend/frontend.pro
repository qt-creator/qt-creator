TEMPLATE = app
TARGET=cpaster

QTC_LIB_DEPENDS += \
    extensionsystem \
    utils
QTC_PLUGIN_DEPENDS += \
    coreplugin

include(../../../../qtcreator.pri)
include(../../../rpath.pri)

CONFIG += console
CONFIG -= app_bundle
QT += network

QMAKE_RPATHDIR *= $$IDE_PLUGIN_PATH

DESTDIR=$$IDE_LIBEXEC_PATH

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
