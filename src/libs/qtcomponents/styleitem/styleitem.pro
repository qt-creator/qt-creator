TEMPLATE = lib
CONFIG += qt plugin
QT += script
greaterThan(QT_MAJOR_VERSION, 4) {
    QT += widgets quick1
} else {
    QT += declarative
}

TARGET  = styleplugin
include(../../../../qtcreator.pri)
DESTDIR = $$IDE_LIBRARY_PATH/qtcomponents/plugin
OBJECTS_DIR = tmp
MOC_DIR = tmp

HEADERS += qtmenu.h \
           qtmenubar.h \
           qtmenuitem.h \
           qrangemodel_p.h \
           qrangemodel.h \
           qstyleplugin.h \
           qdeclarativefolderlistmodel.h \
           qstyleitem.h \
           qwheelarea.h

SOURCES += qtmenu.cpp \
           qtmenubar.cpp \
           qtmenuitem.cpp \
           qrangemodel.cpp \
           qstyleplugin.cpp \
           qdeclarativefolderlistmodel.cpp \
           qstyleitem.cpp \
           qwheelarea.cpp

!macx {
    target.path = /$${IDE_LIBRARY_BASENAME}/qtcreator/qtcomponents/plugin
    INSTALLS += target
}
