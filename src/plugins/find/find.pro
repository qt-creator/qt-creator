TEMPLATE = lib
TARGET = Find
include(../../qworkbenchplugin.pri)
include(find_dependencies.pri)
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
    searchresulttreeitemdelegate.h \
    searchresulttreeitemroles.h \
    searchresulttreeitems.h \
    searchresulttreemodel.h \
    searchresulttreeview.h \
    searchresultwindow.h
SOURCES += findtoolwindow.cpp \
    currentdocumentfind.cpp \
    basetextfind.cpp \
    findtoolbar.cpp \
    findplugin.cpp \
    searchresulttreeitemdelegate.cpp \
    searchresulttreeitems.cpp \
    searchresulttreemodel.cpp \
    searchresulttreeview.cpp \
    searchresultwindow.cpp
FORMS += findwidget.ui \
    finddialog.ui
RESOURCES += find.qrc
