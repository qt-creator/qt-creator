TEMPLATE = lib
TARGET = Help

QT += network
greaterThan(QT_MAJOR_VERSION, 4) {
    QT += printsupport help
    !isEmpty(QT.webkitwidgets.name): QT += webkitwidgets webkit
    else: DEFINES += QT_NO_WEBKIT
} else {
    CONFIG += help
    contains(QT_CONFIG, webkit): QT += webkit
}

INCLUDEPATH += $$PWD

include(../../qtcreatorplugin.pri)
include(help_dependencies.pri)

DEFINES += \
    QT_CLUCENE_SUPPORT \
    HELP_LIBRARY

HEADERS += \
    centralwidget.h \
    docsettingspage.h \
    filtersettingspage.h \
    generalsettingspage.h \
    help_global.h \
    helpconstants.h \
    helpfindsupport.h \
    helpindexfilter.h \
    localhelpmanager.h \
    helpmode.h \
    helpplugin.h \
    helpviewer.h \
    helpviewer_p.h \
    openpagesmanager.h \
    openpagesmodel.h \
    openpagesswitcher.h \
    openpageswidget.h \
    remotehelpfilter.h \
    searchwidget.h \
    xbelsupport.h \
    externalhelpwindow.h

SOURCES += \
    centralwidget.cpp \
    docsettingspage.cpp \
    filtersettingspage.cpp \
    generalsettingspage.cpp \
    helpfindsupport.cpp \
    helpindexfilter.cpp \
    localhelpmanager.cpp \
    helpmode.cpp \
    helpplugin.cpp \
    helpviewer.cpp \
    helpviewer_qtb.cpp \
    helpviewer_qwv.cpp \
    openpagesmanager.cpp \
    openpagesmodel.cpp \
    openpagesswitcher.cpp \
    openpageswidget.cpp \
    remotehelpfilter.cpp \
    searchwidget.cpp \
    xbelsupport.cpp \
    externalhelpwindow.cpp

FORMS += docsettingspage.ui \
    filtersettingspage.ui \
    generalsettingspage.ui \
    remotehelpfilter.ui

RESOURCES += help.qrc
include(../../shared/help/help.pri)
