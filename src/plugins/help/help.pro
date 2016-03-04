QT += help network printsupport sql
!isEmpty(QT.htmlviewwidgets.name): QT += htmlviewwidgets htmlview
else:!isEmpty(QT.webkitwidgets.name): QT += webkitwidgets webkit
else: DEFINES += QT_NO_WEBKIT

INCLUDEPATH += $$PWD

include(../../qtcreatorplugin.pri)

DEFINES += \
    QT_CLUCENE_SUPPORT \
    HELP_LIBRARY

HEADERS += \
    centralwidget.h \
    docsettingspage.h \
    filtersettingspage.h \
    generalsettingspage.h \
    helpconstants.h \
    helpfindsupport.h \
    helpindexfilter.h \
    localhelpmanager.h \
    helpmode.h \
    helpplugin.h \
    helpviewer.h \
    openpagesmanager.h \
    openpagesmodel.h \
    openpagesswitcher.h \
    openpageswidget.h \
    remotehelpfilter.h \
    searchwidget.h \
    xbelsupport.h \
    searchtaskhandler.h \
    qtwebkithelpviewer.h \
    textbrowserhelpviewer.h \
    helpwidget.h

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
    openpagesmanager.cpp \
    openpagesmodel.cpp \
    openpagesswitcher.cpp \
    openpageswidget.cpp \
    remotehelpfilter.cpp \
    searchwidget.cpp \
    xbelsupport.cpp \
    searchtaskhandler.cpp \
    qtwebkithelpviewer.cpp \
    textbrowserhelpviewer.cpp \
    helpwidget.cpp

FORMS += docsettingspage.ui \
    filtersettingspage.ui \
    generalsettingspage.ui \
    remotehelpfilter.ui

!isEmpty(QT.webenginewidgets.name):minQtVersion(5, 6, 0) {
    QT += webenginewidgets
    HEADERS += webenginehelpviewer.h
    SOURCES += webenginehelpviewer.cpp
    DEFINES += QTC_WEBENGINE_HELPVIEWER
}

osx {
    DEFINES += QTC_MAC_NATIVE_HELPVIEWER
    QT += macextras
    HEADERS += macwebkithelpviewer.h
    OBJECTIVE_SOURCES += macwebkithelpviewer.mm
    LIBS += -framework WebKit -framework AppKit

    !isEmpty(USE_QUICK_WIDGET) {
        DEFINES += QTC_MAC_NATIVE_HELPVIEWER_DEFAULT
    }
}


RESOURCES += help.qrc
include(../../shared/help/help.pri)
