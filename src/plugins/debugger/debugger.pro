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
HEADERS += breakhandler.h \
    breakwindow.h \
    debuggeragents.h \
    debuggeractions.h \
    debuggerconstants.h \
    debuggerdialogs.h \
    debuggermanager.h \
    debugger_global.h \
    debuggeroutputwindow.h \
    debuggerplugin.h \
    debuggerrunner.h \
    debuggertooltip.h \
    debuggerstringutils.h \
    watchutils.h \
    idebuggerengine.h \
    moduleshandler.h \
    moduleswindow.h \
    outputcollector.h \
    procinterrupt.h \
    registerhandler.h \
    registerwindow.h \
    stackhandler.h \
    stackwindow.h \
    snapshothandler.h \
    snapshotwindow.h \
    sourcefileswindow.h \
    threadswindow.h \
    watchhandler.h \
    watchwindow.h \
    name_demangler.h
SOURCES += breakhandler.cpp \
    breakwindow.cpp \
    breakwindow.h \
    debuggeragents.cpp \
    debuggeractions.cpp \
    debuggerdialogs.cpp \
    debuggermanager.cpp \
    debuggeroutputwindow.cpp \
    debuggerplugin.cpp \
    debuggerrunner.cpp \
    debuggertooltip.cpp \
    watchutils.cpp \
    moduleshandler.cpp \
    moduleswindow.cpp \
    outputcollector.cpp \
    procinterrupt.cpp \
    registerhandler.cpp \
    registerwindow.cpp \
    snapshothandler.cpp \
    snapshotwindow.cpp \
    stackhandler.cpp \
    stackwindow.cpp \
    sourcefileswindow.cpp \
    threadswindow.cpp \
    watchhandler.cpp \
    watchwindow.cpp \
    name_demangler.cpp
FORMS += attachexternaldialog.ui \
    attachcoredialog.ui \
    breakbyfunction.ui \
    breakcondition.ui \
    dumperoptionpage.ui \
    commonoptionspage.ui \
    startexternaldialog.ui \
    startremotedialog.ui
RESOURCES += debugger.qrc
false {
    SOURCES += $$PWD/modeltest.cpp
    HEADERS += $$PWD/modeltest.h
    DEFINES += USE_MODEL_TEST=1
}
include(cdb/cdb.pri)
include(gdb/gdb.pri)
include(script/script.pri)
include(shared/shared.pri)

OTHER_FILES += Debugger.pluginspec
