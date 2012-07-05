import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "QtSupport"

    Depends { name: "Qt"; submodules: ["widgets", "quick1"] }
    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "QmlJS" }

    Depends { name: "cpp" }
    cpp.includePaths: [
        ".",
        "../../shared",
        "../../shared/proparser",
        "..",
        "../../libs",
        buildDirectory
    ]
    cpp.defines: {
        return base.concat([
            "QT_NO_CAST_FROM_ASCII",
            "QT_NO_CAST_TO_ASCII",
            "QMAKE_AS_LIBRARY",
            "QMAKE_LIBRARY",
            "PROPARSER_THREAD_SAFE",
            "PROEVALUATOR_THREAD_SAFE",
            "PROEVALUATOR_CUMULATIVE",
            "QMAKE_BUILTIN_PRFS"
        ])
    }

    files: [
        "../../shared/proparser/qmakebuiltins.cpp",
        "../../shared/proparser/qmakeevaluator.cpp",
        "../../shared/proparser/qmakeevaluator.h",
        "../../shared/proparser/qmakeevaluator_p.h",
        "../../shared/proparser/qmakeglobals.cpp",
        "../../shared/proparser/qmakeglobals.h",
        "../../shared/proparser/qmakeparser.cpp",
        "../../shared/proparser/qmakeparser.h",
        "../../shared/proparser/qmake_global.h",
        "../../shared/proparser/profileevaluator.cpp",
        "../../shared/proparser/profileevaluator.h",
        "../../shared/proparser/proitems.cpp",
        "../../shared/proparser/proitems.h",
        "../../shared/proparser/prowriter.cpp",
        "../../shared/proparser/prowriter.h",
        "../../shared/proparser/proparser.qrc",
        "../../shared/proparser/ioutils.h",
        "../../shared/proparser/ioutils.cpp",
        "qtversioninfo.ui",
        "qtversionmanager.ui",
        "baseqtversion.h",
        "debugginghelper.cpp",
        "debugginghelper.h",
        "debugginghelper.ui",
        "debugginghelperbuildtask.h",
        "exampleslistmodel.h",
        "gettingstartedwelcomepage.h",
        "profilereader.cpp",
        "profilereader.h",
        "qmldebugginglibrary.h",
        "qmldumptool.h",
        "qmlobservertool.h",
        "qtoptionspage.h",
        "qtoutputformatter.cpp",
        "qtoutputformatter.h",
        "qtparser.h",
        "qtprofileconfigwidget.cpp",
        "qtprofileconfigwidget.h",
        "qtprofileinformation.cpp",
        "qtprofileinformation.h",
        "qtsupport_global.h",
        "qtsupportconstants.h",
        "qtsupportplugin.cpp",
        "qtsupportplugin.h",
        "qtversionfactory.h",
        "qtversionmanager.h",
        "screenshotcropper.cpp",
        "screenshotcropper.h",
        "showbuildlog.ui",
        "baseqtversion.cpp",
        "customexecutableconfigurationwidget.cpp",
        "customexecutableconfigurationwidget.h",
        "customexecutablerunconfiguration.cpp",
        "customexecutablerunconfiguration.h",
        "debugginghelperbuildtask.cpp",
        "exampleslistmodel.cpp",
        "gettingstartedwelcomepage.cpp",
        "qmldebugginglibrary.cpp",
        "qmldumptool.cpp",
        "qmlobservertool.cpp",
        "qtoptionspage.cpp",
        "qtparser.cpp",
        "qtversionfactory.cpp",
        "qtversionmanager.cpp"
    ]

    ProductModule {
        Depends { name: "cpp" }
        cpp.includePaths: [ "../../shared" ]
        cpp.defines: [
            "QMAKE_AS_LIBRARY",
            "PROEVALUATOR_THREAD_SAFE",
            "QMAKE_BUILTIN_PRFS"
        ]
    }
}
