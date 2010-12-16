VPATH += $$PWD
INCLUDEPATH += $$PWD
SOURCES += propertyeditor.cpp \
    qmlanchorbindingproxy.cpp \
    allpropertiesbox.cpp \
    resetwidget.cpp \
    qlayoutobject.cpp \
    basiclayouts.cpp \
    basicwidgets.cpp \
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
    gradientlineqmladaptor.cpp

HEADERS += propertyeditor.h \
    qmlanchorbindingproxy.h \
    allpropertiesbox.h \
    resetwidget.h \
    qlayoutobject.h \
    basiclayouts.h \
    basicwidgets.h \
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
    gradientlineqmladaptor.h
    
QT += declarative
RESOURCES += propertyeditor.qrc
FORMS += behaviordialog.ui \
         contextpanetext.ui \
         contextpanewidgetrectangle.ui \
         contextpanewidgetimage.ui \
         contextpanewidgetborderimage.ui
