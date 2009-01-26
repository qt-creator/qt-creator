TEMPLATE = lib
TARGET = Help
include(../../qworkbenchplugin.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/find/find.pri)
include(../../plugins/quickopen/quickopen.pri)
QT += network
CONFIG += help
DEFINES += QT_CLUCENE_SUPPORT \
    HELP_LIBRARY
HEADERS += helpplugin.h \
    docsettingspage.h \
    filtersettingspage.h \
    helpmode.h \
    centralwidget.h \
    searchwidget.h \
    helpfindsupport.h \
    help_global.h \
    helpindexfilter.h

SOURCES += helpplugin.cpp \
    docsettingspage.cpp \
    filtersettingspage.cpp \
    helpmode.cpp \
    centralwidget.cpp \
    searchwidget.cpp \
    helpfindsupport.cpp \
    helpindexfilter.cpp

FORMS += docsettingspage.ui \
    filtersettingspage.ui
RESOURCES += help.qrc
include(../../shared/help/help.pri)

contains(QT_CONFIG, webkit) {
    QT += webkit
}
