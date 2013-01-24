import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin
import "../../../qbs/defaults.js" as Defaults

QtcPlugin {
    name: "Debugger"

    Depends { name: "Qt"; submodules: ["widgets", "network", "script"] }
    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "QmlJSTools" }
    Depends { name: "Find" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "CPlusPlus" }
    Depends { name: "QmlJS" }
    Depends { name: "QmlDebug" }
    Depends { name: "QtcSsh" }

    Depends { name: "cpp" }
    cpp.includePaths: base.concat([
        "shared",
        "lldb",
        "../../shared/json",
        "../../shared/registryaccess"
    ])

    Group {
        condition: Defaults.testsEnabled(qbs)
        qbs.install: true
        qbs.installDir: "tests/manual/debugger/simple/"
        files: ["../../../tests/manual/debugger/simple/simple.pro"]
    }

    files: [
        "basewindow.cpp",
        "basewindow.h",
        "breakhandler.cpp",
        "breakhandler.h",
        "breakpoint.cpp",
        "breakpoint.h",
        "breakpointmarker.cpp",
        "breakpointmarker.h",
        "breakwindow.cpp",
        "breakwindow.h",
        "commonoptionspage.cpp",
        "commonoptionspage.h",
        "debugger.qrc",
        "debugger_global.h",
        "debuggeractions.cpp",
        "debuggeractions.h",
        "debuggerconstants.h",
        "debuggercore.h",
        "debuggerdialogs.cpp",
        "debuggerdialogs.h",
        "debuggerengine.cpp",
        "debuggerengine.h",
        "debuggerinternalconstants.h",
        "debuggerkitconfigwidget.cpp",
        "debuggerkitconfigwidget.h",
        "debuggerkitinformation.cpp",
        "debuggerkitinformation.h",
        "debuggermainwindow.cpp",
        "debuggermainwindow.h",
        "debuggerplugin.cpp",
        "debuggerplugin.h",
        "debuggerprotocol.cpp",
        "debuggerprotocol.h",
        "debuggerruncontrolfactory.h",
        "debuggerrunner.cpp",
        "debuggerrunner.h",
        "debuggersourcepathmappingwidget.cpp",
        "debuggersourcepathmappingwidget.h",
        "debuggerstartparameters.h",
        "debuggerstreamops.cpp",
        "debuggerstreamops.h",
        "debuggerstringutils.h",
        "debuggertooltipmanager.cpp",
        "debuggertooltipmanager.h",
        "disassembleragent.cpp",
        "disassembleragent.h",
        "disassemblerlines.cpp",
        "disassemblerlines.h",
        "imageviewer.cpp",
        "imageviewer.h",
        "loadcoredialog.cpp",
        "loadcoredialog.h",
        "localsandexpressionsoptionspage.ui",
        "localsandexpressionswindow.cpp",
        "localsandexpressionswindow.h",
        "logwindow.cpp",
        "logwindow.h",
        "memoryagent.cpp",
        "memoryagent.h",
        "memoryview.cpp",
        "memoryview.h",
        "moduleshandler.cpp",
        "moduleshandler.h",
        "moduleswindow.cpp",
        "moduleswindow.h",
        "outputcollector.cpp",
        "outputcollector.h",
        "procinterrupt.cpp",
        "procinterrupt.h",
        "registerhandler.cpp",
        "registerhandler.h",
        "registerwindow.cpp",
        "registerwindow.h",
        "snapshothandler.cpp",
        "snapshothandler.h",
        "snapshotwindow.cpp",
        "snapshotwindow.h",
        "sourceagent.cpp",
        "sourceagent.h",
        "sourcefileshandler.cpp",
        "sourcefileshandler.h",
        "sourcefileswindow.cpp",
        "sourcefileswindow.h",
        "sourceutils.cpp",
        "sourceutils.h",
        "stackframe.cpp",
        "stackframe.h",
        "stackhandler.cpp",
        "stackhandler.h",
        "stackwindow.cpp",
        "stackwindow.h",
        "threaddata.h",
        "threadshandler.cpp",
        "threadshandler.h",
        "threadswindow.cpp",
        "threadswindow.h",
        "watchdata.cpp",
        "watchdata.h",
        "watchdelegatewidgets.cpp",
        "watchdelegatewidgets.h",
        "watchhandler.cpp",
        "watchhandler.h",
        "watchutils.cpp",
        "watchutils.h",
        "watchwindow.cpp",
        "watchwindow.h",
        "cdb/bytearrayinputstream.cpp",
        "cdb/bytearrayinputstream.h",
        "cdb/cdbengine.cpp",
        "cdb/cdbengine.h",
        "cdb/cdboptions.cpp",
        "cdb/cdboptions.h",
        "cdb/cdboptionspage.cpp",
        "cdb/cdboptionspage.h",
        "cdb/cdboptionspagewidget.ui",
        "cdb/cdbparsehelpers.cpp",
        "cdb/cdbparsehelpers.h",
        "gdb/abstractgdbprocess.cpp",
        "gdb/abstractgdbprocess.h",
        "gdb/abstractplaingdbadapter.cpp",
        "gdb/abstractplaingdbadapter.h",
        "gdb/attachgdbadapter.cpp",
        "gdb/attachgdbadapter.h",
        "gdb/classicgdbengine.cpp",
        "gdb/coregdbadapter.cpp",
        "gdb/coregdbadapter.h",
        "gdb/gdb.qrc",
        "gdb/gdbengine.cpp",
        "gdb/gdbengine.h",
        "gdb/gdboptionspage.cpp",
        "gdb/gdboptionspage.h",
        "gdb/localgdbprocess.cpp",
        "gdb/localgdbprocess.h",
        "gdb/localplaingdbadapter.cpp",
        "gdb/localplaingdbadapter.h",
        "gdb/pythongdbengine.cpp",
        "gdb/remotegdbprocess.cpp",
        "gdb/remotegdbprocess.h",
        "gdb/remotegdbserveradapter.cpp",
        "gdb/remotegdbserveradapter.h",
        "gdb/remoteplaingdbadapter.cpp",
        "gdb/remoteplaingdbadapter.h",
        "gdb/startgdbserverdialog.cpp",
        "gdb/startgdbserverdialog.h",
        "gdb/termgdbadapter.cpp",
        "gdb/termgdbadapter.h",
        "images/breakpoint_16.png",
        "images/breakpoint_24.png",
        "images/breakpoint_disabled_16.png",
        "images/breakpoint_disabled_24.png",
        "images/breakpoint_disabled_32.png",
        "images/breakpoint_pending_16.png",
        "images/breakpoint_pending_24.png",
        "images/debugger_breakpoints.png",
        "images/debugger_continue.png",
        "images/debugger_continue_32.png",
        "images/debugger_continue_small.png",
        "images/debugger_empty_14.png",
        "images/debugger_interrupt.png",
        "images/debugger_interrupt_32.png",
        "images/debugger_interrupt_small.png",
        "images/debugger_reversemode_16.png",
        "images/debugger_singleinstructionmode.png",
        "images/debugger_snapshot_small.png",
        "images/debugger_start.png",
        "images/debugger_start_small.png",
        "images/debugger_stepinto_small.png",
        "images/debugger_steponeproc_small.png",
        "images/debugger_stepout_small.png",
        "images/debugger_stepover_small.png",
        "images/debugger_stepoverproc_small.png",
        "images/debugger_stop.png",
        "images/debugger_stop_32.png",
        "images/debugger_stop_small.png",
        "images/location_16.png",
        "images/location_24.png",
        "images/tracepoint.png",
        "images/watchpoint.png",
        "lldb/ipcenginehost.cpp",
        "lldb/ipcenginehost.h",
        "lldb/lldbenginehost.cpp",
        "lldb/lldbenginehost.h",
        "namedemangler/demanglerexceptions.h",
        "namedemangler/globalparsestate.cpp",
        "namedemangler/globalparsestate.h",
        "namedemangler/namedemangler.cpp",
        "namedemangler/namedemangler.h",
        "namedemangler/parsetreenodes.cpp",
        "namedemangler/parsetreenodes.h",
        "pdb/pdbengine.cpp",
        "pdb/pdbengine.h",
        "qml/baseqmldebuggerclient.cpp",
        "qml/baseqmldebuggerclient.h",
        "qml/interactiveinterpreter.cpp",
        "qml/interactiveinterpreter.h",
        "qml/qmladapter.cpp",
        "qml/qmladapter.h",
        "qml/qmlcppengine.cpp",
        "qml/qmlcppengine.h",
        "qml/qmlengine.cpp",
        "qml/qmlengine.h",
        "qml/qmlinspectoradapter.cpp",
        "qml/qmlinspectoradapter.h",
        "qml/qmlinspectoragent.cpp",
        "qml/qmlinspectoragent.h",
        "qml/qmllivetextpreview.cpp",
        "qml/qmllivetextpreview.h",
        "qml/qmlv8debuggerclient.cpp",
        "qml/qmlv8debuggerclient.h",
        "qml/qmlv8debuggerclientconstants.h",
        "qml/qscriptdebuggerclient.cpp",
        "qml/qscriptdebuggerclient.h",
        "script/scriptengine.cpp",
        "script/scriptengine.h",
        "shared/backtrace.cpp",
        "shared/backtrace.h",
        "shared/cdbsymbolpathlisteditor.cpp",
        "shared/cdbsymbolpathlisteditor.h",
        "shared/hostutils.cpp",
        "shared/hostutils.h",
    ]

    Group {
        condition: qbs.targetOS == "windows"
        prefix: "../../shared/registryaccess/"
        files: [
            "registryaccess.cpp",
            "registryaccess.h",
        ]
    }

    Group {
        condition: qbs.targetOS == "windows"
        files: [
            "registerpostmortemaction.cpp",
            "registerpostmortemaction.h",
            "shared/peutils.cpp",
            "shared/peutils.h",
        ]
    }

    Group {
        condition: qbs.targetOS == "mac"
        files: [
            "lldb/lldboptionspage.cpp",
            "lldb/lldboptionspage.h",
            "lldb/lldboptionspagewidget.ui",
        ]
    }

    Properties {
        condition: qbs.targetOS == "windows"
        cpp.dynamicLibraries: [
            "advapi32",
            "ole32",
            "shell32"
        ]
    }

    ProductModule {
        Depends { name: "cpp" }
        Depends { name: "QtcSsh" }
        cpp.includePaths: ["."]
    }
}
