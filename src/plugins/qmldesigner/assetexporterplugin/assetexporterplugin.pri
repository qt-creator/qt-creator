QT *= qml quick core widgets

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
    parsers/modelitemnodeparser.h \
    parsers/modelnodeparser.h

SOURCES += \
    assetexportdialog.cpp \
    assetexporter.cpp \
    assetexporterplugin.cpp \
    assetexporterview.cpp \
    componentexporter.cpp \
    exportnotification.cpp \
    parsers/modelitemnodeparser.cpp \
    parsers/modelnodeparser.cpp

FORMS += \
    assetexportdialog.ui
