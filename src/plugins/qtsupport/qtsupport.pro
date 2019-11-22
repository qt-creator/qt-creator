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
    qtbuildaspects.h \
    qtcppkitinfo.h \
    qtprojectimporter.h \
    qtsupportplugin.h \
    qtsupport_global.h \
    qtkitinformation.h \
    qtoutputformatter.h \
    qttestparser.h \
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
    qtversions.h \
    uicgenerator.h \
    qscxmlcgenerator.h \
    translationwizardpage.h

SOURCES += \
    codegenerator.cpp \
    codegensettings.cpp \
    codegensettingspage.cpp \
    gettingstartedwelcomepage.cpp \
    qtbuildaspects.cpp \
    qtcppkitinfo.cpp \
    qtprojectimporter.cpp \
    qtsupportplugin.cpp \
    qtkitinformation.cpp \
    qtoutputformatter.cpp \
    qttestparser.cpp \
    qtversionmanager.cpp \
    baseqtversion.cpp \
    qmldumptool.cpp \
    qtoptionspage.cpp \
    profilereader.cpp \
    qtparser.cpp \
    exampleslistmodel.cpp \
    screenshotcropper.cpp \
    qtconfigwidget.cpp \
    qtversions.cpp \
    uicgenerator.cpp \
    qscxmlcgenerator.cpp \
    translationwizardpage.cpp

FORMS   +=  \
    codegensettingspagewidget.ui \
    showbuildlog.ui \
    qtversioninfo.ui \
    qtversionmanager.ui \

RESOURCES += \
    qtsupport.qrc
