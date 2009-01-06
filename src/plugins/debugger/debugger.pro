TEMPLATE = lib
TARGET = Debugger

# CONFIG += single
include(../../qworkbenchplugin.pri)
include(../../plugins/projectexplorer/projectexplorer.pri)
include(../../plugins/find/find.pri)
include(../../plugins/coreplugin/coreplugin.pri)
include(../../plugins/texteditor/texteditor.pri)
include(../../plugins/cpptools/cpptools.pri)
include(../../libs/cplusplus/cplusplus.pri)

# DEFINES += QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII
QT += gui network script

HEADERS += attachexternaldialog.h \
    attachremotedialog.h \
    breakhandler.h \
    breakwindow.h \
    debuggerconstants.h \
    debuggermanager.h \
    debuggeroutputwindow.h \
    debuggerplugin.h \
    debuggerrunner.h \
    mode.h \
    disassemblerhandler.h \
    disassemblerwindow.h \
    gdbengine.h \
    gdbmi.h \
    gdboptionpage.h \
    idebuggerengine.h \
    imports.h \
    moduleshandler.h \
    moduleswindow.h \
    procinterrupt.h \
    registerhandler.h \
    registerwindow.h \
    scriptengine.h \
    stackhandler.h \
    stackwindow.h \
    startexternaldialog.h \
    threadswindow.h \
    watchhandler.h \
    watchwindow.h
    
SOURCES += attachexternaldialog.cpp \
    attachremotedialog.cpp \
    breakhandler.cpp \
    breakwindow.cpp \
    breakwindow.h \
    debuggermanager.cpp \
    debuggeroutputwindow.cpp \
    debuggerplugin.cpp \
    debuggerrunner.cpp \
    mode.cpp \
    disassemblerhandler.cpp \
    disassemblerwindow.cpp \
    gdbengine.cpp \
    gdbmi.cpp \
    gdboptionpage.cpp \
    gdbengine.h \
    moduleshandler.cpp \
    moduleswindow.cpp \
    procinterrupt.cpp \
    registerhandler.cpp \
    registerwindow.cpp \
    scriptengine.cpp \
    stackhandler.cpp \
    stackwindow.cpp \
    startexternaldialog.cpp \
    threadswindow.cpp \
    watchhandler.cpp \
    watchwindow.cpp

FORMS += attachexternaldialog.ui \
    attachremotedialog.ui \
    breakbyfunction.ui \
    breakcondition.ui \
    gdboptionpage.ui \
    startexternaldialog.ui \

RESOURCES += debugger.qrc

false {
SOURCES += $$PWD/modeltest.cpp
HEADERS += $$PWD/modeltest.h
DEFINES += USE_MODEL_TEST=1
}
