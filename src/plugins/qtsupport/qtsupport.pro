DEFINES += QTSUPPORT_LIBRARY
QT += network quick

include(../../qtcreatorplugin.pri)

DEFINES += QMAKE_LIBRARY
include(../../shared/proparser/proparser.pri)

HEADERS += \
    codegenerator.h \
    codegensettings.h \
    codegensettingspage.h \
    gettingstartedwelcomepage.h \
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
    desktopqtversion.h \
    winceqtversionfactory.h \
    winceqtversion.h

SOURCES += \
    codegenerator.cpp \
    codegensettings.cpp \
    codegensettingspage.cpp \
    gettingstartedwelcomepage.cpp \
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
    desktopqtversion.cpp \
    winceqtversionfactory.cpp \
    winceqtversion.cpp

FORMS   +=  \
    codegensettingspagewidget.ui \
    showbuildlog.ui \
    qtversioninfo.ui \
    debugginghelper.ui \
    qtversionmanager.ui \

RESOURCES += \
    qtsupport.qrc
