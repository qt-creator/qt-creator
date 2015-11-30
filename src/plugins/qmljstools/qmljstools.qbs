import qbs 1.0

QtcPlugin {
    name: "QmlJSTools"

    Depends { name: "Qt"; submodules: ["script", "widgets"] }
    Depends { name: "Aggregation" }
    Depends { name: "CPlusPlus" }
    Depends { name: "LanguageUtils" }
    Depends { name: "QmlJS" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }
    Depends { name: "QtSupport" }

    files: [
        "qmljsbundleprovider.cpp",
        "qmljsbundleprovider.h",
        "qmljscodestylepreferencesfactory.cpp",
        "qmljscodestylepreferencesfactory.h",
        "qmljscodestylesettingspage.cpp",
        "qmljscodestylesettingspage.h",
        "qmljscodestylesettingspage.ui",
        "qmljsfunctionfilter.cpp",
        "qmljsfunctionfilter.h",
        "qmljsindenter.cpp",
        "qmljsindenter.h",
        "qmljslocatordata.cpp",
        "qmljslocatordata.h",
        "qmljsmodelmanager.cpp",
        "qmljsmodelmanager.h",
        "qmljsqtstylecodeformatter.cpp",
        "qmljsqtstylecodeformatter.h",
        "qmljsrefactoringchanges.cpp",
        "qmljsrefactoringchanges.h",
        "qmljssemanticinfo.cpp",
        "qmljssemanticinfo.h",
        "qmljstools_global.h",
        "qmljstoolsconstants.h",
        "qmljstoolsplugin.cpp",
        "qmljstoolsplugin.h",
        "qmljstoolssettings.cpp",
        "qmljstoolssettings.h",
        "qmlconsolemanager.cpp",
        "qmlconsolemanager.h",
        "qmlconsoleitemmodel.cpp",
        "qmlconsoleitemmodel.h",
        "qmlconsolepane.cpp",
        "qmlconsolepane.h",
        "qmlconsoleview.cpp",
        "qmlconsoleview.h",
        "qmlconsoleitemdelegate.cpp",
        "qmlconsoleitemdelegate.h",
        "qmlconsoleedit.cpp",
        "qmlconsoleedit.h",
        "qmlconsoleproxymodel.cpp",
        "qmlconsoleproxymodel.h",
        "qmljsinterpreter.cpp",
        "qmljsinterpreter.h",
        "qmljstools.qrc"
    ]

    Group {
        name: "Tests"
        condition: project.testsEnabled
        files: ["qmljstools_test.cpp"]
    }

    Export {
        Depends { name: "CppTools" }
        Depends { name: "QmlDebug" }
    }
}
