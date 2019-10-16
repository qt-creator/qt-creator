QT += help network printsupport sql

INCLUDEPATH += $$PWD

include(../../qtcreatorplugin.pri)

minQtVersion(5, 15, 0) {
DEFINES += HELP_NEW_FILTER_ENGINE
}

DEFINES += \
    QT_CLUCENE_SUPPORT \
    HELP_LIBRARY

HEADERS += \
    docsettingspage.h \
    filtersettingspage.h \
    generalsettingspage.h \
    helpconstants.h \
    helpfindsupport.h \
    helpindexfilter.h \
    localhelpmanager.h \
    helpmanager.h \
    helpmode.h \
    helpplugin.h \
    helpviewer.h \
    openpagesmanager.h \
    openpagesswitcher.h \
    openpageswidget.h \
    searchwidget.h \
    xbelsupport.h \
    searchtaskhandler.h \
    textbrowserhelpviewer.h \
    helpwidget.h

SOURCES += \
    docsettingspage.cpp \
    filtersettingspage.cpp \
    generalsettingspage.cpp \
    helpfindsupport.cpp \
    helpindexfilter.cpp \
    localhelpmanager.cpp \
    helpmanager.cpp \
    helpmode.cpp \
    helpplugin.cpp \
    helpviewer.cpp \
    openpagesmanager.cpp \
    openpagesswitcher.cpp \
    openpageswidget.cpp \
    searchwidget.cpp \
    xbelsupport.cpp \
    searchtaskhandler.cpp \
    textbrowserhelpviewer.cpp \
    helpwidget.cpp

FORMS += docsettingspage.ui \
    filtersettingspage.ui \
    generalsettingspage.ui

!isEmpty(QT.webenginewidgets.name) {
    QT += webenginewidgets
    HEADERS += webenginehelpviewer.h
    SOURCES += webenginehelpviewer.cpp
    DEFINES += QTC_WEBENGINE_HELPVIEWER
}

osx {
    DEFINES += QTC_MAC_NATIVE_HELPVIEWER
    HEADERS += macwebkithelpviewer.h
    OBJECTIVE_SOURCES += macwebkithelpviewer.mm
    LIBS += -framework WebKit -framework AppKit

    !isEmpty(USE_QUICK_WIDGET) {
        DEFINES += QTC_MAC_NATIVE_HELPVIEWER_DEFAULT
    }
}

exists($$PWD/qlitehtml/litehtml/CMakeLists.txt)|!isEmpty(LITEHTML_INSTALL_DIR) {
    include(qlitehtml/qlitehtml.pri)
    HEADERS += litehtmlhelpviewer.h
    SOURCES += litehtmlhelpviewer.cpp
    DEFINES += QTC_LITEHTML_HELPVIEWER
}

RESOURCES += help.qrc
include(../../shared/help/help.pri)
