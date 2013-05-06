VPATH += $$PWD

SOURCES += propertyeditor.cpp \
    qmlanchorbindingproxy.cpp \
    resetwidget.cpp \
    qlayoutobject.cpp \
    basiclayouts.cpp \
    basicwidgets.cpp \
    behaviordialog.cpp \
    layoutwidget.cpp \
    filewidget.cpp \
    propertyeditorvalue.cpp \
    fontwidget.cpp \
    originwidget.cpp \
    siblingcombobox.cpp \
    propertyeditortransaction.cpp \
    propertyeditorcontextobject.cpp \
    declarativewidgetview.cpp \
    gradientlineqmladaptor.cpp

HEADERS += propertyeditor.h \
    qmlanchorbindingproxy.h \
    resetwidget.h \
    qlayoutobject.h \
    basiclayouts.h \
    basicwidgets.h \
    behaviordialog.h \
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
    gradientlineqmladaptor.h
    
QT += declarative

RESOURCES += propertyeditor.qrc
FORMS += behaviordialog.ui
