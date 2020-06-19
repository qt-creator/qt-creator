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
    filepathmodel.h \
    parsers/assetnodeparser.h \
    parsers/modelitemnodeparser.h \
    parsers/modelnodeparser.h \
    parsers/textnodeparser.h

SOURCES += \
    assetexportdialog.cpp \
    assetexporter.cpp \
    assetexporterplugin.cpp \
    assetexporterview.cpp \
    componentexporter.cpp \
    exportnotification.cpp \
    filepathmodel.cpp \
    parsers/assetnodeparser.cpp \
    parsers/modelitemnodeparser.cpp \
    parsers/modelnodeparser.cpp \
    parsers/textnodeparser.cpp

FORMS += \
    assetexportdialog.ui
