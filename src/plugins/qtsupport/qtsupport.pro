TEMPLATE = lib
TARGET = QtSupport
DEFINES += QT_CREATOR QTSUPPORT_LIBRARY
QT += network
greaterThan(QT_MAJOR_VERSION, 4) {
    QT += quick1
} else {
    QT += declarative
}

include(../../qtcreatorplugin.pri)
include(qtsupport_dependencies.pri)
DEFINES += \
    QMAKE_AS_LIBRARY QMAKE_LIBRARY \
    PROPARSER_THREAD_SAFE PROEVALUATOR_THREAD_SAFE PROEVALUATOR_CUMULATIVE
include(../../shared/proparser/proparser.pri)

HEADERS += \
    qtsupportplugin.h \
    qtsupport_global.h \
    qtprofileconfigwidget.h \
    qtprofileinformation.h \
    qtoutputformatter.h \
    qtversionmanager.h \
    qtversionfactory.h \
    baseqtversion.h \
    qmldumptool.h \
    qmlobservertool.h \
    qmldebugginglibrary.h \
    qtoptionspage.h \
    customexecutablerunconfiguration.h \
    customexecutableconfigurationwidget.h \
    debugginghelper.h \
    debugginghelperbuildtask.h \
    qtsupportconstants.h \
    profilereader.h \
    qtparser.h \
    gettingstartedwelcomepage.h \
    exampleslistmodel.h \
    screenshotcropper.h

SOURCES += \
    qtsupportplugin.cpp \
    qtprofileconfigwidget.cpp \
    qtprofileinformation.cpp \
    qtoutputformatter.cpp \
    qtversionmanager.cpp \
    qtversionfactory.cpp \
    baseqtversion.cpp \
    qmldumptool.cpp \
    qmlobservertool.cpp \
    qmldebugginglibrary.cpp \
    qtoptionspage.cpp \
    customexecutablerunconfiguration.cpp \
    customexecutableconfigurationwidget.cpp \
    debugginghelper.cpp \
    debugginghelperbuildtask.cpp \
    profilereader.cpp \
    qtparser.cpp \
    gettingstartedwelcomepage.cpp \
    exampleslistmodel.cpp \
    screenshotcropper.cpp

FORMS   +=  \
    showbuildlog.ui \
    qtversioninfo.ui \
    debugginghelper.ui \
    qtversionmanager.ui \


DEFINES += QT_NO_CAST_TO_ASCII

