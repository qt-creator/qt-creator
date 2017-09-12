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
    debuggercore.h \
    debuggerconstants.h \
    debuggerinternalconstants.h \
    debuggeritem.h \
    debuggeritemmanager.h \
    debuggerdialogs.h \
    debuggerengine.h \
    debuggermainwindow.h \
    debuggerplugin.h \
    debuggerprotocol.h \
    debuggerrunconfigurationaspect.h \
    debuggerruncontrol.h \
    debuggerkitconfigwidget.h \
    debuggerkitinformation.h \
    disassembleragent.h \
    disassemblerlines.h \
    loadcoredialog.h \
    logwindow.h \
    memoryagent.h \
    moduleshandler.h \
    outputcollector.h \
    procinterrupt.h \
    registerhandler.h \
    snapshothandler.h \
    snapshotwindow.h \
    sourceagent.h \
    sourcefileshandler.h \
    sourceutils.h \
    stackframe.h \
    stackhandler.h \
    stackwindow.h \
    terminal.h \
    watchhandler.h \
    watchutils.h \
    watchwindow.h \
    threaddata.h \
    threadshandler.h \
    watchdelegatewidgets.h \
    debuggertooltipmanager.h \
    debuggersourcepathmappingwidget.h \
    localsandexpressionswindow.h \
    imageviewer.h \
    simplifytype.h \
    unstartedappwatcherdialog.h \
    debuggericons.h

SOURCES += \
    breakhandler.cpp \
    breakpoint.cpp \
    commonoptionspage.cpp \
    debuggeractions.cpp \
    debuggerdialogs.cpp \
    debuggerengine.cpp \
    debuggeritem.cpp \
    debuggeritemmanager.cpp \
    debuggermainwindow.cpp \
    debuggerplugin.cpp \
    debuggerprotocol.cpp \
    debuggerrunconfigurationaspect.cpp \
    debuggerruncontrol.cpp \
    debuggerkitconfigwidget.cpp \
    debuggerkitinformation.cpp \
    disassembleragent.cpp \
    disassemblerlines.cpp \
    loadcoredialog.cpp \
    logwindow.cpp \
    memoryagent.cpp \
    moduleshandler.cpp \
    outputcollector.cpp \
    procinterrupt.cpp \
    registerhandler.cpp \
    snapshothandler.cpp \
    snapshotwindow.cpp \
    sourceagent.cpp \
    sourcefileshandler.cpp \
    sourceutils.cpp \
    stackhandler.cpp \
    stackwindow.cpp \
    threadshandler.cpp \
    terminal.cpp \
    watchdata.cpp \
    watchhandler.cpp \
    watchutils.cpp \
    watchwindow.cpp \
    stackframe.cpp \
    watchdelegatewidgets.cpp \
    debuggertooltipmanager.cpp \
    debuggersourcepathmappingwidget.cpp \
    localsandexpressionswindow.cpp \
    imageviewer.cpp \
    simplifytype.cpp \
    unstartedappwatcherdialog.cpp \
    debuggericons.cpp

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
include(qml/qml.pri)
include(namedemangler/namedemangler.pri)
include(console/console.pri)
include(analyzer/analyzer.pri)

include(shared/shared.pri)
