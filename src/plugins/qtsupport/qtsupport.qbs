import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "QtSupport"

    Depends { name: "Qt"; submodules: ["widgets", "declarative"] }
    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "QmlJS" }

    Depends { name: "cpp" }
    cpp.includePaths: base.concat([
        "../../shared",
        "../../shared/proparser"
    ])

    cpp.defines: base.concat([
        "QMAKE_AS_LIBRARY",
        "QMAKE_LIBRARY",
        "PROPARSER_THREAD_SAFE",
        "PROEVALUATOR_THREAD_SAFE",
        "PROEVALUATOR_CUMULATIVE",
        "QMAKE_BUILTIN_PRFS",
        "PROEVALUATOR_SETENV"
    ])

    Group {
        prefix: "../../shared/proparser/"
        files: [
            "ioutils.cpp",
            "ioutils.h",
            "profileevaluator.cpp",
            "profileevaluator.h",
            "proitems.cpp",
            "proitems.h",
            "proparser.qrc",
            "prowriter.cpp",
            "prowriter.h",
            "qmake_global.h",
            "qmakebuiltins.cpp",
            "qmakeevaluator.cpp",
            "qmakeevaluator.h",
            "qmakeevaluator_p.h",
            "qmakeglobals.cpp",
            "qmakeglobals.h",
            "qmakeparser.cpp",
            "qmakeparser.h",
        ]
    }

    files: [
        "baseqtversion.cpp",
        "baseqtversion.h",
        "customexecutableconfigurationwidget.cpp",
        "customexecutableconfigurationwidget.h",
        "customexecutablerunconfiguration.cpp",
        "customexecutablerunconfiguration.h",
        "debugginghelper.cpp",
        "debugginghelper.h",
        "debugginghelper.ui",
        "debugginghelperbuildtask.cpp",
        "debugginghelperbuildtask.h",
        "exampleslistmodel.cpp",
        "exampleslistmodel.h",
        "gettingstartedwelcomepage.cpp",
        "gettingstartedwelcomepage.h",
        "profilereader.cpp",
        "profilereader.h",
        "qmldebugginglibrary.cpp",
        "qmldebugginglibrary.h",
        "qmldumptool.cpp",
        "qmldumptool.h",
        "qmlobservertool.cpp",
        "qmlobservertool.h",
        "qtkitconfigwidget.cpp",
        "qtkitconfigwidget.h",
        "qtkitinformation.cpp",
        "qtkitinformation.h",
        "qtoptionspage.cpp",
        "qtoptionspage.h",
        "qtoutputformatter.cpp",
        "qtoutputformatter.h",
        "qtparser.cpp",
        "qtparser.h",
        "qtsupport_global.h",
        "qtsupportconstants.h",
        "qtsupportplugin.cpp",
        "qtsupportplugin.h",
        "qtversionfactory.cpp",
        "qtversionfactory.h",
        "qtversioninfo.ui",
        "qtversionmanager.cpp",
        "qtversionmanager.h",
        "qtversionmanager.ui",
        "screenshotcropper.cpp",
        "screenshotcropper.h",
        "showbuildlog.ui",
    ]

    ProductModule {
        Depends { name: "cpp" }
        cpp.includePaths: "../../shared"
        cpp.defines: [
            "QMAKE_AS_LIBRARY",
            "PROEVALUATOR_THREAD_SAFE",
            "QMAKE_BUILTIN_PRFS",
            "PROEVALUATOR_SETENV"
        ]
    }
}
