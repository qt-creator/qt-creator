VPATH += $$PWD
INCLUDEPATH += $$PWD
include($$PWD/qtgradienteditor/qtgradienteditor.pri)
SOURCES += propertyeditor.cpp \
    qmlanchorbindingproxy.cpp \
    allpropertiesbox.cpp \
    resetwidget.cpp \
    qlayoutobject.cpp \
    basiclayouts.cpp \
    basicwidgets.cpp \
    colorwidget.cpp \
    behaviordialog.cpp \
    qproxylayoutitem.cpp \
    layoutwidget.cpp \
    filewidget.cpp \
    propertyeditorvalue.cpp \
    fontwidget.cpp
HEADERS += propertyeditor.h \
    qmlanchorbindingproxy.h \
    allpropertiesbox.h \
    resetwidget.h \
    qlayoutobject.h \
    basiclayouts.h \
    basicwidgets.h \
    colorwidget.h \
    behaviordialog.h \
    qproxylayoutitem.h \
    layoutwidget.h \
    filewidget.h \
    propertyeditorvalue.h \
    fontwidget.h
QT += declarative
RESOURCES += propertyeditor.qrc
FORMS += behaviordialog.ui
