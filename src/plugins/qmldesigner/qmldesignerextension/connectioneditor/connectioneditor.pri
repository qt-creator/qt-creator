VPATH += $$PWD
INCLUDEPATH += $$PWD

HEADERS += delegates.h \
    connectionview.h \
    connectionviewwidget.h \
    connectionmodel.h \
    bindingmodel.h \
    dynamicpropertiesmodel.h \
    backendmodel.h \
    $$PWD/addnewbackenddialog.h

SOURCES += delegates.cpp \
    connectionview.cpp \
    connectionviewwidget.cpp \
    connectionmodel.cpp \
    bindingmodel.cpp \
    dynamicpropertiesmodel.cpp \
    backendmodel.cpp \
    $$PWD/addnewbackenddialog.cpp

FORMS += \
    connectionviewwidget.ui \
    $$PWD/addnewbackenddialog.ui

RESOURCES += connectioneditor.qrc
