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
    breakpoint.h \
    breakwindow.h \
    consolewindow.h \
    debugger_global.h \
    debuggeractions.h \
    debuggeragents.h \
    debuggerconstants.h \
    debuggerdialogs.h \
    debuggerengine.h \
    debuggermainwindow.h \
    debuggerplugin.h \
    debuggerrunner.h \
    debuggerstringutils.h \
    debuggertooltip.h \
    debuggeruiswitcher.h \
    logwindow.h \
    moduleshandler.h \
    moduleswindow.h \
    name_demangler.h \
    outputcollector.h \
    procinterrupt.h \
    registerhandler.h \
    registerwindow.h \
    sessionengine.h \
    snapshothandler.h \
    snapshotwindow.h \
    sourcefileshandler.h \
    sourcefileswindow.h \
    stackframe.h \
    stackhandler.h \
    stackwindow.h \
    threadswindow.h \
    watchhandler.h \
    watchutils.h \
    watchwindow.h \
    threadshandler.h \
    watchdelegatewidgets.h

SOURCES += breakhandler.cpp \
    breakpoint.cpp \
    breakwindow.cpp \
    consolewindow.cpp \
    debuggeractions.cpp \
    debuggeragents.cpp \
    debuggerdialogs.cpp \
    debuggerengine.cpp \
    debuggermainwindow.cpp \
    debuggerplugin.cpp \
    debuggerrunner.cpp \
    debuggertooltip.cpp \
    debuggeruiswitcher.cpp \
    logwindow.cpp \
    moduleshandler.cpp \
    moduleswindow.cpp \
    name_demangler.cpp \
    outputcollector.cpp \
    procinterrupt.cpp \
    registerhandler.cpp \
    registerwindow.cpp \
    sessionengine.cpp \
    snapshothandler.cpp \
    snapshotwindow.cpp \
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
    watchdelegatewidgets.cpp

FORMS += attachexternaldialog.ui \
    attachcoredialog.ui \
    attachtcfdialog.ui \
    breakbyfunction.ui \
    breakcondition.ui \
    breakpoint.ui \
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

contains(QT_CONFIG, declarative) {
    QT += declarative
    include(qml/qml.pri)
}

include(tcf/tcf.pri)
include(shared/shared.pri)


OTHER_FILES += Debugger.pluginspec
