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
    debuggerprotocol.h \
    debuggerrunner.h \
    debuggerstartparameters.h \
    debuggerstreamops.h \
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
    threadswindow.h \
    watchhandler.h \
    watchutils.h \
    watchwindow.h \
    threaddata.h \
    threadshandler.h \
    watchdelegatewidgets.h \
    debuggerruncontrolfactory.h \
    debuggertooltipmanager.h \
    debuggersourcepathmappingwidget.h \
    memoryview.h \
    localsandexpressionswindow.h \
    imageviewer.h

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
    debuggerprotocol.cpp \
    debuggerrunner.cpp \
    debuggerstreamops.cpp \
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
    imageviewer.cpp

FORMS += \
    localsandexpressionsoptionspage.ui

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
include(namedemangler/namedemangler.pri)

include(shared/shared.pri)

equals(TEST, 1):!isEmpty(copydata) {
    TEST_DIR = tests/manual/debugger/simple
    INPUT_FILE = $$IDE_SOURCE_TREE/$$TEST_DIR/simple.pro
    macx: OUTPUT_DIR = $$IDE_DATA_PATH/$$TEST_DIR
    else: OUTPUT_DIR = $$IDE_BUILD_TREE/$$TEST_DIR
    win32 {
        INPUT_FILE ~= s,/,\\\\,g
        OUTPUT_DIR ~= s,/,\\\\,g
    } else {
        isEmpty(QMAKE_CHK_EXISTS_GLUE):QMAKE_CHK_EXISTS_GLUE  = "|| "
    }
    testfile.target = test_resources
    testfile.commands = ($$QMAKE_CHK_DIR_EXISTS \"$$OUTPUT_DIR\" $$QMAKE_CHK_EXISTS_GLUE $$QMAKE_MKDIR \"$$OUTPUT_DIR\") \
        && $$QMAKE_COPY \"$$INPUT_FILE\" \"$$OUTPUT_DIR\"
    QMAKE_EXTRA_TARGETS += testfile
    PRE_TARGETDEPS += $$testfile.target
}
