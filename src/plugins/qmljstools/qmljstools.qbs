import qbs 1.0

QtcPlugin {
    name: "QmlJSTools"

    Depends { name: "Qt"; submodules: ["widgets"] }
    Depends { name: "Aggregation" }
    Depends { name: "CPlusPlus" }
    Depends { name: "LanguageUtils" }
    Depends { name: "QmlJS" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "CppEditor" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "QtSupport" }

    files: [
        "qmlformatsettings.cpp",
        "qmlformatsettings.h",
        "qmljsbundleprovider.cpp",
        "qmljsbundleprovider.h",
        "qmljscodestylesettings.h",
        "qmljscodestylesettings.cpp",
        "qmljsfunctionfilter.cpp",
        "qmljsfunctionfilter.h",
        "qmljsindenter.cpp",
        "qmljsindenter.h",
        "qmljsmodelmanager.cpp",
        "qmljsmodelmanager.h",
        "qmljsqtstylecodeformatter.cpp",
        "qmljsqtstylecodeformatter.h",
        "qmljsrefactoringchanges.cpp",
        "qmljsrefactoringchanges.h",
        "qmljssemanticinfo.cpp",
        "qmljssemanticinfo.h",
        "qmljstools_global.h", "qmljstoolstr.h",
        "qmljstoolsconstants.h",
        "qmljstoolsinternalconstants.h",
        "qmljstoolsplugin.cpp",
        "qmljstools.qrc",
    ]

    QtcTestFiles {
        files: [
            "qmljstools_test.cpp",
            "qmljstools_test.h",
        ]
    }

    Export {
        Depends { name: "CppEditor" }
        Depends { name: "QmlDebug" }
    }
}
