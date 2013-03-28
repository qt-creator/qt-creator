include(../../qtcreatorplugin.pri)
DEFINES += FIND_LIBRARY
HEADERS += findtoolwindow.h \
    textfindconstants.h \
    ifindsupport.h \
    ifindfilter.h \
    currentdocumentfind.h \
    basetextfind.h \
    find_global.h \
    findtoolbar.h \
    findplugin.h \
    searchresultcolor.h \
    searchresulttreeitemdelegate.h \
    searchresulttreeitemroles.h \
    searchresulttreeitems.h \
    searchresulttreemodel.h \
    searchresulttreeview.h \
    searchresultwindow.h \
    searchresultwidget.h \
    treeviewfind.h
SOURCES += findtoolwindow.cpp \
    currentdocumentfind.cpp \
    basetextfind.cpp \
    findtoolbar.cpp \
    findplugin.cpp \
    searchresulttreeitemdelegate.cpp \
    searchresulttreeitems.cpp \
    searchresulttreemodel.cpp \
    searchresulttreeview.cpp \
    searchresultwindow.cpp \
    ifindfilter.cpp \
    ifindsupport.cpp \
    searchresultwidget.cpp \
    treeviewfind.cpp
FORMS += findwidget.ui \
    finddialog.ui
RESOURCES += find.qrc

