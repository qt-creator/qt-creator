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
    ../pastebindotcomprotocol.h \
    ../pastecodedotxyzprotocol.h \
    ../urlopenprotocol.h \
    argumentscollector.h

SOURCES += ../protocol.cpp \
    ../pastebindotcomprotocol.cpp \
    ../pastecodedotxyzprotocol.cpp \
    ../urlopenprotocol.cpp \
    argumentscollector.cpp \
    main.cpp
