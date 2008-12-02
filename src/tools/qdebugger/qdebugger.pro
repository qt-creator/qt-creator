
QTCREATOR = ../../plugins/

TARGET = ../../../bin/qdebugger

QT += gui \
    network

CONFIG -= release
CONFIG += debug

DEFINES += GDBDEBUGGERLEAN

INCLUDEPATH += $${QTCREATOR}
INCLUDEPATH += $${QTCREATOR}/gdbdebugger

HEADERS += \ 
    lean.h \
    mainwindow.h \
    $${QTCREATOR}/gdbdebugger/attachexternaldialog.h \
    $${QTCREATOR}/gdbdebugger/breakhandler.h \
    $${QTCREATOR}/gdbdebugger/breakwindow.h \
    $${QTCREATOR}/gdbdebugger/disassemblerhandler.h \
    $${QTCREATOR}/gdbdebugger/disassemblerwindow.h \
    $${QTCREATOR}/gdbdebugger/gdbdebuggerconstants.h \
    $${QTCREATOR}/gdbdebugger/gdbdebugger.h \
    $${QTCREATOR}/gdbdebugger/gdbmi.h \
    $${QTCREATOR}/gdbdebugger/imports.h \
    $${QTCREATOR}/gdbdebugger/moduleshandler.h \
    $${QTCREATOR}/gdbdebugger/moduleswindow.h \
    $${QTCREATOR}/gdbdebugger/registerhandler.h \
    $${QTCREATOR}/gdbdebugger/registerwindow.h \
    $${QTCREATOR}/gdbdebugger/gdboutputwindow.h \
    $${QTCREATOR}/gdbdebugger/procinterrupt.h \
    $${QTCREATOR}/gdbdebugger/stackhandler.h \
    $${QTCREATOR}/gdbdebugger/stackwindow.h \
    $${QTCREATOR}/gdbdebugger/startexternaldialog.h \
    $${QTCREATOR}/gdbdebugger/threadswindow.h \
    $${QTCREATOR}/gdbdebugger/watchhandler.h \
    $${QTCREATOR}/gdbdebugger/watchwindow.h \

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    $${QTCREATOR}/gdbdebugger/attachexternaldialog.cpp \
    $${QTCREATOR}/gdbdebugger/breakhandler.cpp \
    $${QTCREATOR}/gdbdebugger/breakwindow.cpp \
    $${QTCREATOR}/gdbdebugger/disassemblerhandler.cpp \
    $${QTCREATOR}/gdbdebugger/disassemblerwindow.cpp \
    $${QTCREATOR}/gdbdebugger/gdbdebugger.cpp \
    $${QTCREATOR}/gdbdebugger/gdbmi.cpp \
    $${QTCREATOR}/gdbdebugger/moduleshandler.cpp \
    $${QTCREATOR}/gdbdebugger/moduleswindow.cpp \
    $${QTCREATOR}/gdbdebugger/registerhandler.cpp \
    $${QTCREATOR}/gdbdebugger/registerwindow.cpp \
    $${QTCREATOR}/gdbdebugger/gdboutputwindow.cpp \
    $${QTCREATOR}/gdbdebugger/procinterrupt.cpp \
    $${QTCREATOR}/gdbdebugger/stackhandler.cpp \
    $${QTCREATOR}/gdbdebugger/stackwindow.cpp \
    $${QTCREATOR}/gdbdebugger/startexternaldialog.cpp \
    $${QTCREATOR}/gdbdebugger/threadswindow.cpp \
    $${QTCREATOR}/gdbdebugger/watchhandler.cpp \
    $${QTCREATOR}/gdbdebugger/watchwindow.cpp

SOURCES *= $${QTCREATOR}/../../bin/gdbmacros/gdbmacros.cpp

FORMS += \
    $${QTCREATOR}/gdbdebugger/attachexternaldialog.ui \
    $${QTCREATOR}/gdbdebugger/gdboptionpage.ui \
    $${QTCREATOR}/gdbdebugger/generaloptionpage.ui \
    $${QTCREATOR}/gdbdebugger/debugmode.ui \
    $${QTCREATOR}/gdbdebugger/startexternaldialog.ui

RESOURCES += \
    $${QTCREATOR}/gdbdebugger/gdbdebugger.qrc

FORMS += \
    $${QTCREATOR}/gdbdebugger/breakbyfunction.ui

# The following chooses between a "really lean" build and a version
# using the BaseTextEditor

true {

DEFINES += USE_BASETEXTEDITOR
DEFINES += TEXTEDITOR_STANDALONE 

INCLUDEPATH += $${QTCREATOR}
INCLUDEPATH += $${QTCREATOR}/texteditor
INCLUDEPATH += $${QTCREATOR}/../libs

HEADERS += \ 
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
    $${QTCREATOR}/../libs/utils/linecolumnlabel.h \

SOURCES += \
    #$${QTCREATOR}/texteditor/basetextmark.cpp \
    $${QTCREATOR}/texteditor/basetextdocument.cpp \
    $${QTCREATOR}/texteditor/basetexteditor.cpp \
    $${QTCREATOR}/texteditor/storagesettings.cpp \
    $${QTCREATOR}/texteditor/tabsettings.cpp \
    $${QTCREATOR}/texteditor/displaysettings.cpp \
    $${QTCREATOR}/../libs/utils/linecolumnlabel.cpp \
}



unix {
    OBJECTS_DIR = .tmp/
    MOC_DIR = .tmp/
    RCC_DIR = .tmp/
    UI_DIR = .tmp/
}

