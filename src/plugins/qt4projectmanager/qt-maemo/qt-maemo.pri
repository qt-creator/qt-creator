INCLUDEPATH += $$PWD/../../../libs/3rdparty/botan/build
INCLUDEPATH += $$PWD/../../../libs/3rdparty/net7ssh/src
LIBS += -l$$qtLibraryTarget(Net7ssh) -l$$qtLibraryTarget(Botan)
HEADERS += $$PWD/maemorunconfiguration.h \
    $$PWD/maemomanager.h \
    $$PWD/maemotoolchain.h \
    $$PWD/maemodeviceconfigurations.h \
    $$PWD/maemosettingspage.h \
    $$PWD/maemosettingswidget.h \
    $$PWD/maemosshconnection.h \
    $$PWD/maemosshthread.h \
    $$PWD/maemoruncontrol.h \
    $$PWD/maemorunconfigurationwidget.h \
    $$PWD/maemorunfactories.h \
    $$PWD/maemoconstants.h \
    $$PWD/maemoconfigtestdialog.h

SOURCES += $$PWD/maemorunconfiguration.cpp \
    $$PWD/maemomanager.cpp \
    $$PWD/maemotoolchain.cpp \
    $$PWD/maemodeviceconfigurations.cpp \
    $$PWD/maemosettingspage.cpp \
    $$PWD/maemosettingswidget.cpp \
    $$PWD/maemosshconnection.cpp \
    $$PWD/maemosshthread.cpp \
    $$PWD/maemoruncontrol.cpp \
    $$PWD/maemorunconfigurationwidget.cpp \
    $$PWD/maemorunfactories.cpp \
    $$PWD/maemoconfigtestdialog.cpp

FORMS += $$PWD/maemosettingswidget.ui \
    $$PWD/maemoconfigtestdialog.ui
RESOURCES += $$PWD/qt-maemo.qrc
