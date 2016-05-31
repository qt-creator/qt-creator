VPATH += $$PWD
INCLUDEPATH += $$PWD

HEADERS += delegates.h \
    connectionview.h \
    connectionviewwidget.h \
    connectionmodel.h \
    bindingmodel.h \
    dynamicpropertiesmodel.h

SOURCES += delegates.cpp \
    connectionview.cpp \
    connectionviewwidget.cpp \
    connectionmodel.cpp \
    bindingmodel.cpp \
    dynamicpropertiesmodel.cpp

FORMS += \
    connectionviewwidget.ui

RESOURCES += connectioneditor.qrc
