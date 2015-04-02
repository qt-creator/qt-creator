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
    breakwindow.h \
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
    debuggeroptionspage.h \
    debuggerplugin.h \
    debuggerprotocol.h \
    debuggerrunconfigurationaspect.h \
    debuggerruncontrol.h \
    debuggerstartparameters.h \
    debuggerstringutils.h \
    debuggerkitconfigwidget.h \
    debuggerkitinformation.h \
    disassembleragent.h \
    disassemblerlines.h \
    loadcoredialog.h \
    logwindow.h \
    memoryagent.h \
    moduleshandler.h \
    moduleswindow.h \
    outputcollector.h \
    procinterrupt.h \
    registerhandler.h \
    registerwindow.h \
    snapshothandler.h \
    snapshotwindow.h \
    sourceagent.h \
    sourcefileshandler.h \
    sourcefileswindow.h \
    sourceutils.h \
    stackframe.h \
    stackhandler.h \
    stackwindow.h \
    terminal.h \
    threadswindow.h \
    watchhandler.h \
    watchutils.h \
    watchwindow.h \
    threaddata.h \
    threadshandler.h \
    watchdelegatewidgets.h \
    debuggertooltipmanager.h \
    debuggersourcepathmappingwidget.h \
    memoryview.h \
    localsandexpressionswindow.h \
    imageviewer.h \
    simplifytype.h \
    unstartedappwatcherdialog.h

SOURCES += \
    breakhandler.cpp \
    breakpoint.cpp \
    breakwindow.cpp \
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
    debuggeroptionspage.cpp \
    debuggerkitconfigwidget.cpp \
    debuggerkitinformation.cpp \
    disassembleragent.cpp \
    disassemblerlines.cpp \
    loadcoredialog.cpp \
    logwindow.cpp \
    memoryagent.cpp \
    moduleshandler.cpp \
    moduleswindow.cpp \
    outputcollector.cpp \
    procinterrupt.cpp \
    registerhandler.cpp \
    registerwindow.cpp \
    snapshothandler.cpp \
    snapshotwindow.cpp \
    sourceagent.cpp \
    sourcefileshandler.cpp \
    sourcefileswindow.cpp \
    sourceutils.cpp \
    stackhandler.cpp \
    stackwindow.cpp \
    threadshandler.cpp \
    threadswindow.cpp \
    terminal.cpp \
    watchdata.cpp \
    watchhandler.cpp \
    watchutils.cpp \
    watchwindow.cpp \
    stackframe.cpp \
    watchdelegatewidgets.cpp \
    debuggertooltipmanager.cpp \
    debuggersourcepathmappingwidget.cpp \
    memoryview.cpp \
    localsandexpressionswindow.cpp \
    imageviewer.cpp \
    simplifytype.cpp \
    unstartedappwatcherdialog.cpp

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
include(cdb/cdb.pri)
include(gdb/gdb.pri)
include(pdb/pdb.pri)
include(lldb/lldb.pri)
include(qml/qml.pri)
include(namedemangler/namedemangler.pri)

include(shared/shared.pri)
