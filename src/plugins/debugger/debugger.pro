TEMPLATE = lib
TARGET = Debugger

# DEFINES += QT_USE_FAST_OPERATOR_PLUS
# DEFINES += QT_USE_FAST_CONCATENATION
# CONFIG += single

include(../../qtcreatorplugin.pri)
include(debugger_dependencies.pri)

DEFINES += DEBUGGER_LIBRARY

INCLUDEPATH += $$PWD/../../libs/utils

QT += gui \
    network \
    script

CONFIG += exceptions

HEADERS += \
    basewindow.h \
    breakhandler.h \
    breakpoint.h \
    breakpointmarker.h \
    breakwindow.h \
    commonoptionspage.h \
    debugger_global.h \
    debuggeractions.h \
    debuggercore.h \
    debuggerconstants.h \
    debuggerinternalconstants.h \
    debuggerdialogs.h \
    debuggerengine.h \
    debuggermainwindow.h \
    debuggerplugin.h \
    debuggerrunner.h \
    debuggerstartparameters.h \
    debuggerstreamops.h \
    debuggerstringutils.h \
    debuggerprofileconfigwidget.h \
    debuggerprofileinformation.h \
    disassembleragent.h \
    disassemblerlines.h \
    loadremotecoredialog.h \
    logwindow.h \
    memoryagent.h \
    moduleshandler.h \
    moduleswindow.h \
    name_demangler.h \
    outputcollector.h \
    procinterrupt.h \
    registerhandler.h \
    registerwindow.h \
    snapshothandler.h \
    snapshotwindow.h \
    sourceagent.h \
    sourcefileshandler.h \
    sourcefileswindow.h \
    stackframe.h \
    stackhandler.h \
    stackwindow.h \
    threadswindow.h \
    watchhandler.h \
    watchutils.h \
    watchwindow.h \
    threaddata.h \
    threadshandler.h \
    watchdelegatewidgets.h \
    debuggerruncontrolfactory.h \
    debuggertooltipmanager.h \
    debuggertoolchaincombobox.h \
    debuggersourcepathmappingwidget.h \
    memoryview.h \
    qtmessagelogwindow.h \
    qtmessagelogeditor.h \
    qtmessagelogview.h \
    qtmessagelogproxymodel.h \
    qtmessagelogitemdelegate.h \
    qtmessageloghandler.h \
    localsandexpressionswindow.h

SOURCES += \
    basewindow.cpp \
    breakhandler.cpp \
    breakpoint.cpp \
    breakpointmarker.cpp \
    breakwindow.cpp \
    commonoptionspage.cpp \
    debuggeractions.cpp \
    debuggerdialogs.cpp \
    debuggerengine.cpp \
    debuggermainwindow.cpp \
    debuggerplugin.cpp \
    debuggerrunner.cpp \
    debuggerstreamops.cpp \
    debuggerprofileconfigwidget.cpp \
    debuggerprofileinformation.cpp \
    disassembleragent.cpp \
    disassemblerlines.cpp \
    loadremotecoredialog.cpp \
    logwindow.cpp \
    memoryagent.cpp \
    moduleshandler.cpp \
    moduleswindow.cpp \
    name_demangler.cpp \
    outputcollector.cpp \
    procinterrupt.cpp \
    registerhandler.cpp \
    registerwindow.cpp \
    snapshothandler.cpp \
    snapshotwindow.cpp \
    sourceagent.cpp \
    sourcefileshandler.cpp \
    sourcefileswindow.cpp \
    stackhandler.cpp \
    stackwindow.cpp \
    threadshandler.cpp \
    threadswindow.cpp \
    watchdata.cpp \
    watchhandler.cpp \
    watchutils.cpp \
    watchwindow.cpp \
    stackframe.cpp \
    watchdelegatewidgets.cpp \
    debuggertooltipmanager.cpp \
    debuggertoolchaincombobox.cpp \
    debuggersourcepathmappingwidget.cpp \
    memoryview.cpp \
    qtmessagelogwindow.cpp \
    qtmessagelogproxymodel.cpp \
    qtmessagelogview.cpp \
    qtmessagelogitemdelegate.cpp \
    qtmessageloghandler.cpp \
    qtmessagelogeditor.cpp \
    localsandexpressionswindow.cpp

FORMS += attachexternaldialog.ui \
    attachcoredialog.ui \
    breakcondition.ui \
    breakpoint.ui \
    localsandexpressionsoptionspage.ui \
    commonoptionspage.ui \
    startexternaldialog.ui \
    startremotedialog.ui \
    startremoteenginedialog.ui

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
include(script/script.pri)
include(pdb/pdb.pri)
include(lldb/lldbhost.pri)
include(qml/qml.pri)

include(shared/shared.pri)
