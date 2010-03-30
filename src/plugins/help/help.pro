TEMPLATE = lib
TARGET = Help
QT += network

include(../../qtcreatorplugin.pri)
include(help_dependencies.pri)

CONFIG += help
DEFINES += QT_CLUCENE_SUPPORT HELP_LIBRARY

HEADERS += \
    centralwidget.h \
    docsettingspage.h \
    filtersettingspage.h \
    generalsettingspage.h \
    help_global.h \
    helpconstants.h \
    helpfindsupport.h \
    helpindexfilter.h \
    helpmanager.h \
    helpmode.h \
    helpplugin.h \
    helpviewer.h \
    helpviewer_p.h \
    openpagesmanager.h \
    openpagesmodel.h \
    openpageswidget.h \
    searchwidget.h \
    xbelsupport.h

SOURCES += \
    centralwidget.cpp \
    docsettingspage.cpp \
    filtersettingspage.cpp \
    generalsettingspage.cpp \
    helpfindsupport.cpp \
    helpindexfilter.cpp \
    helpmanager.cpp \
    helpmode.cpp \
    helpplugin.cpp \
    helpviewer.cpp \
    helpviewer_qtb.cpp \
    helpviewer_qwv.cpp \
    openpagesmanager.cpp \
    openpagesmodel.cpp \
    openpageswidget.cpp \
    searchwidget.cpp \
    xbelsupport.cpp

FORMS += docsettingspage.ui \
    filtersettingspage.ui \
    generalsettingspage.ui

RESOURCES += help.qrc
include(../../shared/help/help.pri)

contains(QT_CONFIG, webkit) {
    QT += webkit
}

OTHER_FILES += Help.pluginspec
