QT *= qml quick core widgets
QT += quick-private

VPATH += $$PWD

RESOURCES += assetexporterplugin.qrc

INCLUDEPATH += ./

HEADERS += \
    assetexportdialog.h \
    assetexporter.h \
    assetexporterplugin.h \
    assetexporterview.h \
    assetexportpluginconstants.h \
    componentexporter.h \
    exportnotification.h \
    filepathmodel.h \
    dumpers/assetnodedumper.h \
    dumpers/itemnodedumper.h \
    dumpers/nodedumper.h \
    dumpers/textnodedumper.h

SOURCES += \
    assetexportdialog.cpp \
    assetexporter.cpp \
    assetexporterplugin.cpp \
    assetexporterview.cpp \
    componentexporter.cpp \
    exportnotification.cpp \
    filepathmodel.cpp \
    dumpers/assetnodedumper.cpp \
    dumpers/itemnodedumper.cpp \
    dumpers/nodedumper.cpp \
    dumpers/textnodedumper.cpp

FORMS += \
    assetexportdialog.ui
