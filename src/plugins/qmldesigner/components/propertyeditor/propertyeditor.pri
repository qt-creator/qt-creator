VPATH += $$PWD

SOURCES += propertyeditorview.cpp \
    qmlanchorbindingproxy.cpp \
    propertyeditorvalue.cpp \
    propertyeditortransaction.cpp \
    designerpropertymap.cpp \
    propertyeditorcontextobject.cpp \
    quick2propertyeditorview.cpp \
    propertyeditorqmlbackend.cpp \
    propertyeditorwidget.cpp \
    fileresourcesmodel.cpp \
    components/propertyeditor/gradientmodel.cpp

HEADERS += propertyeditorview.h \
    qmlanchorbindingproxy.h \
    propertyeditorvalue.h \
    propertyeditortransaction.h \
    designerpropertymap.h \
    propertyeditorcontextobject.h \
    quick2propertyeditorview.h \
    propertyeditorqmlbackend.h \
    propertyeditorwidget.h \
    fileresourcesmodel.h \
    components/propertyeditor/gradientmodel.h
    
QT += qml quick

RESOURCES += propertyeditor.qrc
