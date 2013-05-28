DEFINES += QTSUPPORT_LIBRARY
QT += network declarative

include(../../qtcreatorplugin.pri)

DEFINES += QMAKE_LIBRARY
include(../../shared/proparser/proparser.pri)

HEADERS += \
    qtsupportplugin.h \
    qtsupport_global.h \
    qtkitconfigwidget.h \
    qtkitinformation.h \
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
    screenshotcropper.h \
    qtconfigwidget.h \
    qtfeatureprovider.h

SOURCES += \
    qtsupportplugin.cpp \
    qtkitconfigwidget.cpp \
    qtkitinformation.cpp \
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
    screenshotcropper.cpp \
    qtconfigwidget.cpp

FORMS   +=  \
    showbuildlog.ui \
    qtversioninfo.ui \
    debugginghelper.ui \
    qtversionmanager.ui \

RESOURCES += \
    qtsupport.qrc
