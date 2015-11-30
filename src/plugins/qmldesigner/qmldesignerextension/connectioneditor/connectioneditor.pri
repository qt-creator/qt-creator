VPATH += $$PWD
INCLUDEPATH += $$PWD

HEADERS += connectionview.h \
    connectionviewwidget.h \
    connectionmodel.h \
    bindingmodel.h \
    dynamicpropertiesmodel.h

SOURCES += connectionview.cpp \
    connectionviewwidget.cpp \
    connectionmodel.cpp \
    bindingmodel.cpp \
    dynamicpropertiesmodel.cpp

FORMS += \
    connectionviewwidget.ui

RESOURCES += connectioneditor.qrc
