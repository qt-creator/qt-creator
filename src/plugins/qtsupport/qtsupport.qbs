import qbs 1.0

QtcPlugin {
    name: "QtSupport"

    Depends { name: "Qt"; submodules: ["quick", "widgets", "xml"]; }
    Depends { name: "QmlJS" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "CppTools" }

    cpp.includePaths: base.concat([
        project.sharedSourcesDir,
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
        name: "Shared"
        prefix: project.sharedSourcesDir + "/proparser/"
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
            "qmakevfs.cpp",
            "qmakevfs.h",
        ]
    }

    files: [
        "baseqtversion.cpp",
        "baseqtversion.h",
        "codegenerator.cpp",
        "codegenerator.h",
        "codegensettings.cpp",
        "codegensettings.h",
        "codegensettingspage.cpp",
        "codegensettingspage.h",
        "codegensettingspagewidget.ui",
        "qtconfigwidget.cpp",
        "qtconfigwidget.h",
        "qtsupport.qrc",
        "customexecutableconfigurationwidget.cpp",
        "customexecutableconfigurationwidget.h",
        "customexecutablerunconfiguration.cpp",
        "customexecutablerunconfiguration.h",
        "exampleslistmodel.cpp",
        "exampleslistmodel.h",
        "profilereader.cpp",
        "profilereader.h",
        "qmldumptool.cpp",
        "qmldumptool.h",
        "qscxmlcgenerator.cpp",
        "qscxmlcgenerator.h",
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
        "uicgenerator.cpp",
        "uicgenerator.h",
    ]

    Group {
        name: "QtVersion"
        files: [
            "desktopqtversion.cpp", "desktopqtversion.h",
            "desktopqtversionfactory.cpp", "desktopqtversionfactory.h",
            "winceqtversion.cpp", "winceqtversion.h",
            "winceqtversionfactory.cpp", "winceqtversionfactory.h",
        ]
    }

    Group {
        name: "Getting Started Welcome Page"
        files: [
            "gettingstartedwelcomepage.cpp",
            "gettingstartedwelcomepage.h"
        ]
    }


    Export {
        cpp.includePaths: "../../shared"
        cpp.defines: [
            "QMAKE_AS_LIBRARY",
            "PROPARSER_THREAD_SAFE",
            "PROEVALUATOR_CUMULATIVE",
            "PROEVALUATOR_THREAD_SAFE",
            "QMAKE_BUILTIN_PRFS",
            "PROEVALUATOR_SETENV"
        ]
    }
}
