import qbs 1.0

QtcPlugin {
    name: "Debugger"

    Depends { name: "Qt"; submodules: ["widgets", "network"] }
    Depends { name: "Aggregation" }
    Depends { name: "CPlusPlus" }
    Depends { name: "QtcSsh" }
    Depends { name: "QmlDebug" }
    Depends { name: "LanguageUtils" }
    Depends { name: "QmlJS" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }
    Depends { name: "TextEditor" }

    cpp.includePaths: base.concat([project.sharedSourcesDir + "/registryaccess"])

    pluginRecommends: [
        "CppEditor"
    ]

    Group {
        name: "General"
        files: [
            "breakhandler.cpp", "breakhandler.h",
            "breakpoint.cpp", "breakpoint.h",
            "breakwindow.cpp", "breakwindow.h",
            "commonoptionspage.cpp", "commonoptionspage.h",
            "debugger.qrc",
            "debugger_global.h",
            "debuggeractions.cpp", "debuggeractions.h",
            "debuggerconstants.h",
            "debuggericons.h",
            "debuggercore.h",
            "debuggerdialogs.cpp", "debuggerdialogs.h",
            "debuggerengine.cpp", "debuggerengine.h",
            "debuggerinternalconstants.h",
            "debuggeritem.cpp", "debuggeritem.h",
            "debuggeritemmanager.cpp", "debuggeritemmanager.h",
            "debuggerkitconfigwidget.cpp", "debuggerkitconfigwidget.h",
            "debuggerkitinformation.cpp", "debuggerkitinformation.h",
            "debuggermainwindow.cpp", "debuggermainwindow.h",
            "debuggeroptionspage.cpp", "debuggeroptionspage.h",
            "debuggerplugin.cpp", "debuggerplugin.h",
            "debuggerprotocol.cpp", "debuggerprotocol.h",
            "debuggerrunconfigurationaspect.cpp", "debuggerrunconfigurationaspect.h",
            "debuggerruncontrol.cpp", "debuggerruncontrol.h",
            "debuggersourcepathmappingwidget.cpp", "debuggersourcepathmappingwidget.h",
            "debuggerstartparameters.h",
            "debuggerstringutils.h",
            "debuggertooltipmanager.cpp", "debuggertooltipmanager.h",
            "disassembleragent.cpp", "disassembleragent.h",
            "disassemblerlines.cpp", "disassemblerlines.h",
            "imageviewer.cpp", "imageviewer.h",
            "loadcoredialog.cpp", "loadcoredialog.h",
            "localsandexpressionswindow.cpp", "localsandexpressionswindow.h",
            "logwindow.cpp", "logwindow.h",
            "memoryagent.cpp", "memoryagent.h",
            "memoryview.cpp", "memoryview.h",
            "moduleshandler.cpp", "moduleshandler.h",
            "moduleswindow.cpp", "moduleswindow.h",
            "outputcollector.cpp", "outputcollector.h",
            "procinterrupt.cpp", "procinterrupt.h",
            "registerhandler.cpp", "registerhandler.h",
            "registerwindow.cpp", "registerwindow.h",
            "snapshothandler.cpp", "snapshothandler.h",
            "snapshotwindow.cpp", "snapshotwindow.h",
            "sourceagent.cpp", "sourceagent.h",
            "sourcefileshandler.cpp", "sourcefileshandler.h",
            "sourcefileswindow.cpp", "sourcefileswindow.h",
            "sourceutils.cpp", "sourceutils.h",
            "stackframe.cpp", "stackframe.h",
            "stackhandler.cpp", "stackhandler.h",
            "stackwindow.cpp", "stackwindow.h",
            "terminal.cpp", "terminal.h",
            "threaddata.h",
            "threadshandler.cpp", "threadshandler.h",
            "threadswindow.cpp", "threadswindow.h",
            "watchdata.cpp", "watchdata.h",
            "watchdelegatewidgets.cpp", "watchdelegatewidgets.h",
            "watchhandler.cpp", "watchhandler.h",
            "watchutils.cpp", "watchutils.h",
            "watchwindow.cpp", "watchwindow.h",
            "simplifytype.cpp", "simplifytype.h",
            "unstartedappwatcherdialog.cpp", "unstartedappwatcherdialog.h"
        ]
    }

    Group {
        name: "cdb"
        prefix: "cdb/"
        files: [
            "bytearrayinputstream.cpp", "bytearrayinputstream.h",
            "cdbengine.cpp", "cdbengine.h",
            "cdboptionspage.cpp", "cdboptionspage.h",
            "cdboptionspagewidget.ui",
            "cdbparsehelpers.cpp", "cdbparsehelpers.h"
        ]
    }

    Group {
        name: "gdb"
        prefix: "gdb/"
        files: [
            "attachgdbadapter.cpp", "attachgdbadapter.h",
            "coregdbadapter.cpp", "coregdbadapter.h",
            "gdbengine.cpp", "gdbengine.h",
            "gdboptionspage.cpp",
            "gdbplainengine.cpp", "gdbplainengine.h",
            "remotegdbserveradapter.cpp", "remotegdbserveradapter.h",
            "startgdbserverdialog.cpp", "startgdbserverdialog.h",
            "termgdbadapter.cpp", "termgdbadapter.h"
        ]
    }

    Group {
        name: "lldb"
        prefix: "lldb/"
        files: [
            "lldbengine.cpp", "lldbengine.h"
        ]
    }

    Group {
        name: "pdb"
        prefix: "pdb/"
        files: ["pdbengine.cpp", "pdbengine.h"]
    }

    Group {
        name: "Name Demangler"
        prefix: "namedemangler/"
        files: [
            "demanglerexceptions.h",
            "globalparsestate.cpp", "globalparsestate.h",
            "namedemangler.cpp", "namedemangler.h",
            "parsetreenodes.cpp", "parsetreenodes.h",
        ]
    }

    Group {
        name: "QML Debugger"
        prefix: "qml/"
        files: [
            "interactiveinterpreter.cpp", "interactiveinterpreter.h",
            "qmlcppengine.cpp", "qmlcppengine.h",
            "qmlengine.cpp", "qmlengine.h",
            "qmlengineutils.cpp", "qmlengineutils.h",
            "qmlinspectoragent.cpp", "qmlinspectoragent.h",
            "qmlv8debuggerclientconstants.h"
        ]
    }

     Group {
        name: "Debugger Console"
        prefix: "console/"
        files: [
            "consoleitem.cpp", "consoleitem.h",
            "consoleedit.cpp", "consoleedit.h",
            "consoleitemdelegate.cpp", "consoleitemdelegate.h",
            "consoleitemmodel.cpp", "consoleitemmodel.h",
            "console.cpp", "console.h",
            "consoleproxymodel.cpp", "consoleproxymodel.h",
            "consoleview.cpp", "consoleview.h"
        ]
    }

    Group {
        name: "shared"
        prefix: "shared/"
        files: [
            "backtrace.cpp", "backtrace.h",
            "cdbsymbolpathlisteditor.cpp",
            "cdbsymbolpathlisteditor.h",
            "hostutils.cpp", "hostutils.h",
            "peutils.cpp", "peutils.h",
            "symbolpathsdialog.ui", "symbolpathsdialog.cpp", "symbolpathsdialog.h"
        ]
    }

    Group {
        name: "Images"
        prefix: "images/"
        files: ["*.png", "*.xpm"]
    }

    Group {
        name: "Images/qml"
        prefix: "images/qml/"
        files: ["*.png"]
    }

    Group {
        name: "Images/analyzer"
        prefix: "analyzer/images/"
        files: ["*.png"]
    }

    Group {
        name: "RegistryAccess"
        condition: qbs.targetOS.contains("windows")
        prefix: project.sharedSourcesDir + "/registryaccess/"
        files: [
            "registryaccess.cpp",
            "registryaccess.h",
        ]
    }

    Group {
        name: "RegisterPostMortem"
        condition: qbs.targetOS.contains("windows")
        files: [
            "registerpostmortemaction.cpp",
            "registerpostmortemaction.h",
        ]
    }

    Properties {
        condition: qbs.targetOS.contains("windows")
        cpp.dynamicLibraries: [
            "advapi32",
            "ole32",
            "shell32"
        ]
    }

    Group {
        name: "Analyzer"

        files: [
            "analyzer/analyzerbase.qrc",
            "analyzer/analyzerbase_global.h",
            "analyzer/analyzerconstants.h",
            "analyzer/analyzericons.h",
            "analyzer/analyzermanager.cpp",
            "analyzer/analyzermanager.h",
            "analyzer/analyzerrunconfigwidget.cpp",
            "analyzer/analyzerrunconfigwidget.h",
            "analyzer/analyzerruncontrol.cpp",
            "analyzer/analyzerruncontrol.h",
            "analyzer/analyzerstartparameters.h",
            "analyzer/analyzerutils.cpp",
            "analyzer/analyzerutils.h",
            "analyzer/detailederrorview.cpp",
            "analyzer/detailederrorview.h",
            "analyzer/diagnosticlocation.cpp",
            "analyzer/diagnosticlocation.h",
            "analyzer/startremotedialog.cpp",
            "analyzer/startremotedialog.h",
        ]
    }

    Export {
        Depends { name: "QtcSsh" }
        Depends { name: "CPlusPlus" }
    }
}
