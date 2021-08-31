VPATH += $$PWD

SOURCES += navigatorview.cpp \
    navigatortreemodel.cpp \
    navigatorwidget.cpp \
    nameitemdelegate.cpp \
    iconcheckboxitemdelegate.cpp \
    navigatortreeview.cpp \
    choosefrompropertylistdialog.cpp \
    previewtooltip.cpp

HEADERS += navigatorview.h \
    navigatortreemodel.h \
    navigatorwidget.h \
    nameitemdelegate.h \
    iconcheckboxitemdelegate.h \
    navigatortreeview.h \
    navigatormodelinterface.h \
    choosefrompropertylistdialog.h \
    previewtooltip.h

RESOURCES += navigator.qrc

FORMS += choosefrompropertylistdialog.ui \
    previewtooltip.ui
