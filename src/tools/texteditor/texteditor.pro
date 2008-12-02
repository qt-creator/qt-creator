
QTCREATOR = ../../plugins/

TARGET = ../../../bin/texteditor

QT += gui 

CONFIG -= release
CONFIG += debug

DEFINES += TEXTEDITOR_STANDALONE

INCLUDEPATH += $${QTCREATOR}
INCLUDEPATH += $${QTCREATOR}/texteditor

HEADERS += \ 
    mainwindow.h \
    $${QTCREATOR}/texteditor/basetextdocument.h \
    $${QTCREATOR}/texteditor/basetexteditor.h \
    $${QTCREATOR}/texteditor/storagesettings.h \
    $${QTCREATOR}/texteditor/itexteditable.h \
    $${QTCREATOR}/texteditor/itexteditor.h \
    $${QTCREATOR}/texteditor/tabsettings.h \
    $${QTCREATOR}/texteditor/displaysettings.h \
    $${QTCREATOR}/coreplugin/icontext.h \
    $${QTCREATOR}/coreplugin/ifile.h \
    $${QTCREATOR}/coreplugin/editormanager/ieditor.h \

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    $${QTCREATOR}/texteditor/basetextdocument.cpp \
    $${QTCREATOR}/texteditor/basetexteditor.cpp \
    $${QTCREATOR}/texteditor/storagesettings.cpp \
    $${QTCREATOR}/texteditor/tabsettings.cpp \
    $${QTCREATOR}/texteditor/displaysettings.cpp \

#RESOURCES += \
#    $${QTCREATOR}/gdbdebugger/gdbdebugger.qrc

unix {
    OBJECTS_DIR = .tmp/
    MOC_DIR = .tmp/
    RCC_DIR = .tmp/
    UI_DIR = .tmp/
}
