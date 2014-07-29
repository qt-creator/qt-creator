DEFINES += QTSUPPORT_LIBRARY
QT += network

include(../../qtcreatorplugin.pri)

DEFINES += QMAKE_LIBRARY
include(../../shared/proparser/proparser.pri)

HEADERS += \
    codegensettings.h \
    codegensettingspage.h \
    qtsupportplugin.h \
    qtsupport_global.h \
    qtkitconfigwidget.h \
    qtkitinformation.h \
    qtoutputformatter.h \
    qtversionmanager.h \
    qtversionfactory.h \
    uicodemodelsupport.h \
    baseqtversion.h \
    qmldumptool.h \
    qtoptionspage.h \
    customexecutablerunconfiguration.h \
    customexecutableconfigurationwidget.h \
    debugginghelperbuildtask.h \
    qtsupportconstants.h \
    profilereader.h \
    qtparser.h \
    exampleslistmodel.h \
    screenshotcropper.h \
    qtconfigwidget.h \
    desktopqtversionfactory.h \
    simulatorqtversionfactory.h \
    desktopqtversion.h \
    simulatorqtversion.h \
    winceqtversionfactory.h \
    winceqtversion.h

SOURCES += \
    codegensettings.cpp \
    codegensettingspage.cpp \
    qtsupportplugin.cpp \
    qtkitconfigwidget.cpp \
    qtkitinformation.cpp \
    qtoutputformatter.cpp \
    qtversionmanager.cpp \
    qtversionfactory.cpp \
    uicodemodelsupport.cpp \
    baseqtversion.cpp \
    qmldumptool.cpp \
    qtoptionspage.cpp \
    customexecutablerunconfiguration.cpp \
    customexecutableconfigurationwidget.cpp \
    debugginghelperbuildtask.cpp \
    profilereader.cpp \
    qtparser.cpp \
    exampleslistmodel.cpp \
    screenshotcropper.cpp \
    qtconfigwidget.cpp \
    desktopqtversionfactory.cpp \
    simulatorqtversionfactory.cpp \
    desktopqtversion.cpp \
    simulatorqtversion.cpp \
    winceqtversionfactory.cpp \
    winceqtversion.cpp

greaterThan(QT_MAJOR_VERSION, 4) {
    QT += quick
    HEADERS += gettingstartedwelcomepage.h
    SOURCES += gettingstartedwelcomepage.cpp
}

FORMS   +=  \
    codegensettingspagewidget.ui \
    showbuildlog.ui \
    qtversioninfo.ui \
    debugginghelper.ui \
    qtversionmanager.ui \

RESOURCES += \
    qtsupport.qrc
