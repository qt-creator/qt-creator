VPATH += $$PWD

SOURCES += navigatorview.cpp \
    navigatortreemodel.cpp \
    navigatorwidget.cpp \
    nameitemdelegate.cpp \
    iconcheckboxitemdelegate.cpp \
    navigatortreeview.cpp \
    choosetexturepropertydialog.cpp \
    previewtooltip.cpp

HEADERS += navigatorview.h \
    navigatortreemodel.h \
    navigatorwidget.h \
    nameitemdelegate.h \
    iconcheckboxitemdelegate.h \
    navigatortreeview.h \
    navigatormodelinterface.h \
    choosetexturepropertydialog.h \
    previewtooltip.h

RESOURCES += navigator.qrc

FORMS += choosetexturepropertydialog.ui \
    previewtooltip.ui
