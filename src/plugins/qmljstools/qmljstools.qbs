import qbs.base 1.0

import "../QtcPlugin.qbs" as QtcPlugin
import "../../../qbs/defaults.js" as Defaults

QtcPlugin {
    name: "QmlJSTools"

    Depends { name: "Qt"; submodules: ["script", "widgets"] }
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
    cpp.includePaths: base.concat("../../libs/3rdparty")

    files: [
        "qmljscodestylepreferencesfactory.cpp",
        "qmljscodestylepreferencesfactory.h",
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
        "qmljsmodelmanager.cpp",
        "qmljsmodelmanager.h",
        "qmljsmodelmanager.h",
        "qmljsplugindumper.cpp",
        "qmljsplugindumper.h",
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
        condition: Defaults.testsEnabled(qbs)
        files: ["qmljstools_test.cpp"]
    }

    ProductModule {
        Depends { name: "CppTools" }
        Depends { name: "QmlDebug" }
    }
}
