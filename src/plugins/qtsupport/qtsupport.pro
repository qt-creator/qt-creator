TEMPLATE = lib
TARGET = QtSupport
DEFINES += QT_CREATOR QTSUPPORT_LIBRARY
QT += network
include(../../qtcreatorplugin.pri)
include(qtsupport_dependencies.pri)
DEFINES += PROPARSER_AS_LIBRARY PROPARSER_LIBRARY PROPARSER_THREAD_SAFE PROEVALUATOR_THREAD_SAFE
include(../../shared/proparser/proparser.pri)

HEADERS += \
    qtsupportplugin.h \
    qtsupport_global.h \
    qtoutputformatter.h \
    qtversionmanager.h \
    qtversionfactory.h \
    baseqtversion.h \
    qmldumptool.h \
    qmlobservertool.h \
    qmldebugginglibrary.h \
    qtoptionspage.h \
    debugginghelperbuildtask.h \
    qtsupportconstants.h \
    profilereader.h \

SOURCES += \
    qtsupportplugin.cpp \
    qtoutputformatter.cpp \
    qtversionmanager.cpp \
    qtversionfactory.cpp \
    baseqtversion.cpp \
    qmldumptool.cpp \
    qmlobservertool.cpp \
    qmldebugginglibrary.cpp \
    qtoptionspage.cpp \
    debugginghelperbuildtask.cpp \
    profilereader.cpp \

FORMS   +=  \
    showbuildlog.ui \
    qtversioninfo.ui \
    debugginghelper.ui \
    qtversionmanager.ui \


DEFINES += QT_NO_CAST_TO_ASCII

