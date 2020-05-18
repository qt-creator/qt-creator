TARGET=cpaster

QTC_LIB_DEPENDS += \
    extensionsystem \
    utils
QTC_PLUGIN_DEPENDS += \
    coreplugin

include(../../../qtcreatortool.pri)

QT += network

HEADERS = ../protocol.h \
    ../cpasterconstants.h \
    ../dpastedotcomprotocol.h \
    ../pastebindotcomprotocol.h \
    ../urlopenprotocol.h \
    argumentscollector.h

SOURCES += ../protocol.cpp \
    ../dpastedotcomprotocol.cpp \
    ../pastebindotcomprotocol.cpp \
    ../urlopenprotocol.cpp \
    argumentscollector.cpp \
    main.cpp
