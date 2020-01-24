# DEFINES += QT_USE_FAST_OPERATOR_PLUS
# DEFINES += QT_USE_FAST_CONCATENATION
# CONFIG += single

include(../../qtcreatorplugin.pri)

DEFINES += DEBUGGER_LIBRARY

QT += gui \
    network

CONFIG += exceptions

HEADERS += \
    breakhandler.h \
    breakpoint.h \
    commonoptionspage.h \
    debugger_global.h \
    debuggeractions.h \
    debuggerconstants.h \
    debuggercore.h \
    debuggerdialogs.h \
    debuggerengine.h \
    debuggericons.h \
    debuggerinternalconstants.h \
    debuggeritem.h \
    debuggeritemmanager.h \
    debuggerkitinformation.h \
    debuggermainwindow.h \
    debuggerplugin.h \
    debuggerprotocol.h \
    debuggerrunconfigurationaspect.h \
    debuggerruncontrol.h \
    debuggersourcepathmappingwidget.h \
    debuggertooltipmanager.h \
    disassembleragent.h \
    disassemblerlines.h \
    enginemanager.h \
    imageviewer.h \
    loadcoredialog.h \
    localsandexpressionswindow.h \
    logwindow.h \
    memoryagent.h \
    moduleshandler.h \
    outputcollector.h \
    peripheralregisterhandler.h \
    procinterrupt.h \
    registerhandler.h \
    simplifytype.h \
    sourceagent.h \
    sourcefileshandler.h \
    sourceutils.h \
    stackframe.h \
    stackhandler.h \
    stackwindow.h \
    terminal.h \
    threaddata.h \
    threadshandler.h \
    unstartedappwatcherdialog.h \
    watchdelegatewidgets.h \
    watchhandler.h \
    watchutils.h \
    watchwindow.h

SOURCES += \
    breakhandler.cpp \
    breakpoint.cpp \
    commonoptionspage.cpp \
    debuggeractions.cpp \
    debuggerdialogs.cpp \
    debuggerengine.cpp \
    debuggericons.cpp \
    debuggeritem.cpp \
    debuggeritemmanager.cpp \
    debuggerkitinformation.cpp \
    debuggermainwindow.cpp \
    debuggerplugin.cpp \
    debuggerprotocol.cpp \
    debuggerrunconfigurationaspect.cpp \
    debuggerruncontrol.cpp \
    debuggersourcepathmappingwidget.cpp \
    debuggertooltipmanager.cpp \
    disassembleragent.cpp \
    disassemblerlines.cpp \
    enginemanager.cpp \
    imageviewer.cpp \
    loadcoredialog.cpp \
    localsandexpressionswindow.cpp \
    logwindow.cpp \
    memoryagent.cpp \
    moduleshandler.cpp \
    outputcollector.cpp \
    peripheralregisterhandler.cpp \
    procinterrupt.cpp \
    registerhandler.cpp \
    simplifytype.cpp \
    sourceagent.cpp \
    sourcefileshandler.cpp \
    sourceutils.cpp \
    stackframe.cpp \
    stackhandler.cpp \
    stackwindow.cpp \
    terminal.cpp \
    threadshandler.cpp \
    unstartedappwatcherdialog.cpp \
    watchdata.cpp \
    watchdelegatewidgets.cpp \
    watchhandler.cpp \
    watchutils.cpp \
    watchwindow.cpp

RESOURCES += debugger.qrc

false {
    include(../../shared/modeltest/modeltest.pri)
    #DEFINES += USE_WATCH_MODEL_TEST=1
    #DEFINES += USE_BREAK_MODEL_TEST=1
    #DEFINES += USE_REGISTER_MODEL_TEST=1
}
win32 {
include(../../shared/registryaccess/registryaccess.pri)
HEADERS += registerpostmortemaction.h
SOURCES += registerpostmortemaction.cpp
LIBS  *= -lole32 \
    -lshell32
}

equals(TEST, 1) {
    RESOURCES += debuggerunittests.qrc
}

include(cdb/cdb.pri)
include(gdb/gdb.pri)
include(pdb/pdb.pri)
include(lldb/lldb.pri)
include(uvsc/uvsc.pri)
include(qml/qml.pri)
include(namedemangler/namedemangler.pri)
include(console/console.pri)
include(analyzer/analyzer.pri)

include(shared/shared.pri)
