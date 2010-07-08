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
    fontwidget.cpp \
    originwidget.cpp \
    siblingcombobox.cpp \
    propertyeditortransaction.cpp \
    propertyeditorcontextobject.cpp \
    declarativewidgetview.cpp \
    contextpanewidget.cpp \
    contextpanetextwidget.cpp \
    fontsizespinbox.cpp

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
    fontwidget.h \
    originwidget.h \
    siblingcombobox.h \
    propertyeditortransaction.h \
    designerpropertymap.h \
    propertyeditorcontextobject.h \
    declarativewidgetview.h \
    contextpanewidget.h \
    contextpanetextwidget.h \
    fontsizespinbox.h
QT += declarative
RESOURCES += propertyeditor.qrc
FORMS += behaviordialog.ui \
         contextpanetext.ui
