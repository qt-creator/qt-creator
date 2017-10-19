import qbs 1.0

Project {
    name: "Debugger"

    QtcDevHeaders { }

    QtcPlugin {
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
        Depends { name: "app_version_header" }

        pluginTestDepends: [
            "QmakeProjectManager"
        ]

        cpp.includePaths: base.concat([project.sharedSourcesDir + "/registryaccess"])
        cpp.enableExceptions: true

        pluginRecommends: [
            "CppEditor",
            "BinEditor"
        ]

        Group {
            name: "General"
            files: [
                "breakhandler.cpp", "breakhandler.h",
                "breakpoint.cpp", "breakpoint.h",
                "commonoptionspage.cpp", "commonoptionspage.h",
                "debugger.qrc",
                "debugger_global.h",
                "debuggeractions.cpp", "debuggeractions.h",
                "debuggerconstants.h",
                "debuggericons.h", "debuggericons.cpp",
                "debuggercore.h",
                "debuggerdialogs.cpp", "debuggerdialogs.h",
                "debuggerengine.cpp", "debuggerengine.h",
                "debuggerinternalconstants.h",
                "debuggeritem.cpp", "debuggeritem.h",
                "debuggeritemmanager.cpp", "debuggeritemmanager.h",
                "debuggerkitconfigwidget.cpp", "debuggerkitconfigwidget.h",
                "debuggerkitinformation.cpp", "debuggerkitinformation.h",
                "debuggermainwindow.cpp", "debuggermainwindow.h",
                "debuggerplugin.cpp", "debuggerplugin.h",
                "debuggerprotocol.cpp", "debuggerprotocol.h",
                "debuggerrunconfigurationaspect.cpp", "debuggerrunconfigurationaspect.h",
                "debuggerruncontrol.cpp", "debuggerruncontrol.h",
                "debuggersourcepathmappingwidget.cpp", "debuggersourcepathmappingwidget.h",
                "debuggertooltipmanager.cpp", "debuggertooltipmanager.h",
                "disassembleragent.cpp", "disassembleragent.h",
                "disassemblerlines.cpp", "disassemblerlines.h",
                "imageviewer.cpp", "imageviewer.h",
                "loadcoredialog.cpp", "loadcoredialog.h",
                "localsandexpressionswindow.cpp", "localsandexpressionswindow.h",
                "logwindow.cpp", "logwindow.h",
                "memoryagent.cpp", "memoryagent.h",
                "moduleshandler.cpp", "moduleshandler.h",
                "outputcollector.cpp", "outputcollector.h",
                "procinterrupt.cpp", "procinterrupt.h",
                "registerhandler.cpp", "registerhandler.h",
                "snapshothandler.cpp", "snapshothandler.h",
                "snapshotwindow.cpp", "snapshotwindow.h",
                "sourceagent.cpp", "sourceagent.h",
                "sourcefileshandler.cpp", "sourcefileshandler.h",
                "sourceutils.cpp", "sourceutils.h",
                "stackframe.cpp", "stackframe.h",
                "stackhandler.cpp", "stackhandler.h",
                "stackwindow.cpp", "stackwindow.h",
                "terminal.cpp", "terminal.h",
                "threaddata.h",
                "threadshandler.cpp", "threadshandler.h",
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
                "cdbengine.cpp", "cdbengine.h",
                "cdboptionspage.cpp", "cdboptionspage.h",
                "cdboptionspagewidget.ui",
                "cdbparsehelpers.cpp", "cdbparsehelpers.h",
                "stringinputstream.cpp", "stringinputstream.h",
            ]
        }

        Group {
            name: "gdb"
            prefix: "gdb/"
            files: [
                "gdbengine.cpp", "gdbengine.h",
                "gdboptionspage.cpp",
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
            prefix: "analyzer/"
            files: [
                "analyzerbase.qrc",
                "analyzerconstants.h",
                "analyzermanager.h",
                "analyzerrunconfigwidget.cpp",
                "analyzerrunconfigwidget.h",
                "analyzerutils.cpp",
                "analyzerutils.h",
                "detailederrorview.cpp",
                "detailederrorview.h",
                "diagnosticlocation.cpp",
                "diagnosticlocation.h",
                "startremotedialog.cpp",
                "startremotedialog.h",
            ]
        }

        Group {
            name: "Unit tests"
            condition: qtc.testsEnabled
            files: [
                "debuggerunittests.qrc",
            ]
        }

        Group {
            name: "Unit test resources"
            prefix: "unit-tests/"
            fileTags: []
            files: ["**/*"]
        }

        Export {
            Depends { name: "QtcSsh" }
            Depends { name: "CPlusPlus" }
        }
    }
}
