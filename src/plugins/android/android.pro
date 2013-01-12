TEMPLATE = lib
TARGET = Android

include(../../qtcreatorplugin.pri)
include(android_dependencies.pri)

isEmpty(ANDROID_ENABLE):ANDROID_EXPERIMENTAL_STR="true"
else:ANDROID_EXPERIMENTAL_STR="false"

QT += xml network

HEADERS += \
    androidconstants.h \
    androidconfigurations.h \
    androidmanager.h \
    androidrunconfiguration.h \
    androidruncontrol.h \
    androidrunfactories.h \
    androidsettingspage.h \
    androidsettingswidget.h \
    androidtoolchain.h \
    androidpackageinstallationstep.h \
    androidpackageinstallationfactory.h \
    androidpackagecreationstep.h \
    androidpackagecreationfactory.h \
    androidpackagecreationwidget.h \
    androiddeploystep.h \
    androiddeploystepwidget.h \
    androiddeploystepfactory.h \
    androidglobal.h \
    androidrunner.h \
    androiddebugsupport.h \
    androidqtversionfactory.h \
    androidqtversion.h \
    androiddeployconfiguration.h \
    androidcreatekeystorecertificate.h \
    javaparser.h \
    androidplugin.h \
    androiddevicefactory.h \
    androiddevice.h

SOURCES += \
    androidconfigurations.cpp \
    androidmanager.cpp \
    androidrunconfiguration.cpp \
    androidruncontrol.cpp \
    androidrunfactories.cpp \
    androidsettingspage.cpp \
    androidsettingswidget.cpp \
    androidtoolchain.cpp \
    androidpackageinstallationstep.cpp \
    androidpackageinstallationfactory.cpp \
    androidpackagecreationstep.cpp \
    androidpackagecreationfactory.cpp \
    androidpackagecreationwidget.cpp \
    androiddeploystep.cpp \
    androiddeploystepwidget.cpp \
    androiddeploystepfactory.cpp \
    androidrunner.cpp \
    androiddebugsupport.cpp \
    androidqtversionfactory.cpp \
    androidqtversion.cpp \
    androiddeployconfiguration.cpp \
    androidcreatekeystorecertificate.cpp \
    javaparser.cpp \
    androidplugin.cpp \
    androiddevicefactory.cpp \
    androiddevice.cpp


FORMS += \
    androidsettingswidget.ui \
    androidpackagecreationwidget.ui \
    androiddeploystepwidget.ui \
    addnewavddialog.ui \
    androidcreatekeystorecertificate.ui

RESOURCES = android.qrc
DEFINES += ANDROID_LIBRARY
