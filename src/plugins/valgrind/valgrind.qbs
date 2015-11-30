import qbs 1.0

QtcPlugin {
    name: "Valgrind"

    Depends { name: "Qt"; submodules: ["widgets", "network"] }
    Depends { name: "CPlusPlus"}
    Depends { name: "QtcSsh" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "AnalyzerBase" }
    Depends { name: "Debugger" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "RemoteLinux" }

    Group {
        name: "General"
        files: [
            "callgrindcostdelegate.cpp", "callgrindcostdelegate.h",
            "callgrindcostview.cpp", "callgrindcostview.h",
            "callgrindengine.cpp", "callgrindengine.h",
            "callgrindhelper.cpp", "callgrindhelper.h",
            "callgrindnamedelegate.cpp", "callgrindnamedelegate.h",
            "callgrindtextmark.cpp", "callgrindtextmark.h",
            "callgrindtool.cpp", "callgrindtool.h",
            "callgrindvisualisation.cpp", "callgrindvisualisation.h",
            "memcheckengine.cpp", "memcheckengine.h",
            "memcheckerrorview.cpp", "memcheckerrorview.h",
            "memchecktool.cpp", "memchecktool.h",
            "suppressiondialog.cpp", "suppressiondialog.h",
            "valgrindconfigwidget.cpp", "valgrindconfigwidget.h", "valgrindconfigwidget.ui",
            "valgrindengine.cpp", "valgrindengine.h",
            "valgrindplugin.cpp", "valgrindplugin.h",
            "valgrindprocess.cpp", "valgrindprocess.h",
            "valgrindruncontrolfactory.cpp", "valgrindruncontrolfactory.h",
            "valgrindrunner.cpp", "valgrindrunner.h",
            "valgrindsettings.cpp", "valgrindsettings.h",
            "workarounds.cpp", "workarounds.h",
        ]
    }

    Group {
        name: "Callgrind"
        prefix: "callgrind/"
        files: [
            "callgrindabstractmodel.h",
            "callgrindcallmodel.cpp", "callgrindcallmodel.h",
            "callgrindcontroller.cpp", "callgrindcontroller.h",
            "callgrindcostitem.cpp", "callgrindcostitem.h",
            "callgrindcycledetection.cpp", "callgrindcycledetection.h",
            "callgrinddatamodel.cpp", "callgrinddatamodel.h",
            "callgrindfunction.cpp", "callgrindfunction.h", "callgrindfunction_p.h",
            "callgrindfunctioncall.cpp", "callgrindfunctioncall.h",
            "callgrindfunctioncycle.cpp", "callgrindfunctioncycle.h",
            "callgrindparsedata.cpp", "callgrindparsedata.h",
            "callgrindparser.cpp", "callgrindparser.h",
            "callgrindproxymodel.cpp", "callgrindproxymodel.h",
            "callgrindrunner.cpp", "callgrindrunner.h",
            "callgrindstackbrowser.cpp", "callgrindstackbrowser.h"
        ]
    }

    Group {
        name: "Memcheck"
        prefix: "memcheck/"
        files: [
            "memcheckrunner.cpp",
            "memcheckrunner.h"
        ]
    }

    Group {
        name: "XML Protocol"
        prefix: "xmlprotocol/"
        files: [
            "announcethread.cpp", "announcethread.h",
            "error.cpp", "error.h",
            "errorlistmodel.cpp", "errorlistmodel.h",
            "frame.cpp", "frame.h",
            "modelhelpers.cpp", "modelhelpers.h",
            "parser.cpp", "parser.h",
            "stack.cpp", "stack.h",
            "stackmodel.cpp", "stackmodel.h",
            "status.cpp", "status.h",
            "suppression.cpp", "suppression.h",
            "threadedparser.cpp", "threadedparser.h",
        ]
    }
}
