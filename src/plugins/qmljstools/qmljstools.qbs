import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin

QtcPlugin {
    name: "QmlJSTools"

    Depends { name: "Qt.widgets" }
    Depends { name: "Core" }
    Depends { name: "LanguageUtils" }
    Depends { name: "CppTools" }
    Depends { name: "QmlJS" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "Locator" }
    Depends { name: "QmlDebug" }
    Depends { name: "QtSupport" }

    Depends { name: "cpp" }
    cpp.defines: base.concat(["QT_NO_CAST_TO_ASCII"])
    cpp.includePaths: [
        "..",
        "../../libs",
        "../../libs/3rdparty",
        buildDirectory
    ]

    files: [
        "qmljsmodelmanager.cpp",
        "qmljsmodelmanager.h",
        "qmljscodestylesettingspage.cpp",
        "qmljscodestylesettingspage.h",
        "qmljscodestylesettingspage.ui",
        "qmljsfindexportedcpptypes.cpp",
        "qmljsfindexportedcpptypes.h",
        "qmljsfunctionfilter.cpp",
        "qmljsfunctionfilter.h",
        "qmljsindenter.cpp",
        "qmljsindenter.h",
        "qmljslocatordata.cpp",
        "qmljslocatordata.h",
        "qmljsmodelmanager.cpp",
        "qmljsmodelmanager.h",
        "qmljsplugindumper.cpp",
        "qmljsplugindumper.h",
        "qmljsqtstylecodeformatter.cpp",
        "qmljsqtstylecodeformatter.h",
        "qmljsrefactoringchanges.cpp",
        "qmljsrefactoringchanges.h",
        "qmljstools_global.h",
        "qmljstoolsconstants.h",
        "qmljstoolsplugin.cpp",
        "qmljstoolsplugin.h",
        "qmljstoolssettings.cpp",
        "qmljstoolssettings.h",
        "qmljscodestylepreferencesfactory.cpp",
        "qmljscodestylepreferencesfactory.h",
        "qmljssemanticinfo.cpp",
        "qmljssemanticinfo.h"
    ]

    ProductModule {
        Depends { name: "CppTools" }
        Depends { name: "QmlDebug" }
    }
}

