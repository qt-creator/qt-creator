DEFINES += QTSUPPORT_LIBRARY
QT += network

include(../../qtcreatorplugin.pri)

DEFINES += QMAKE_LIBRARY
include(../../shared/proparser/proparser.pri)

HEADERS += \
    codegenerator.h \
    codegensettings.h \
    codegensettingspage.h \
    gettingstartedwelcomepage.h \
    qtprojectimporter.h \
    qtsupportplugin.h \
    qtsupport_global.h \
    qtkitconfigwidget.h \
    qtkitinformation.h \
    qtoutputformatter.h \
    qtversionmanager.h \
    qtversionfactory.h \
    baseqtversion.h \
    qmldumptool.h \
    qtoptionspage.h \
    qtsupportconstants.h \
    profilereader.h \
    qtparser.h \
    exampleslistmodel.h \
    screenshotcropper.h \
    qtconfigwidget.h \
    desktopqtversionfactory.h \
    desktopqtversion.h \
    uicgenerator.h \
    qscxmlcgenerator.h

SOURCES += \
    codegenerator.cpp \
    codegensettings.cpp \
    codegensettingspage.cpp \
    gettingstartedwelcomepage.cpp \
    qtprojectimporter.cpp \
    qtsupportplugin.cpp \
    qtkitconfigwidget.cpp \
    qtkitinformation.cpp \
    qtoutputformatter.cpp \
    qtversionmanager.cpp \
    qtversionfactory.cpp \
    baseqtversion.cpp \
    qmldumptool.cpp \
    qtoptionspage.cpp \
    profilereader.cpp \
    qtparser.cpp \
    exampleslistmodel.cpp \
    screenshotcropper.cpp \
    qtconfigwidget.cpp \
    desktopqtversionfactory.cpp \
    desktopqtversion.cpp \
    uicgenerator.cpp \
    qscxmlcgenerator.cpp

FORMS   +=  \
    codegensettingspagewidget.ui \
    showbuildlog.ui \
    qtversioninfo.ui \
    qtversionmanager.ui \

RESOURCES += \
    qtsupport.qrc
