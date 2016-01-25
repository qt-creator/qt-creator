import qbs

QtcPlugin {
    name: "AutoTest"

    Depends { name: "Core" }
    Depends { name: "CppTools" }
    Depends { name: "CPlusPlus" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QmlJS" }
    Depends { name: "QmlJSTools" }
    Depends { name: "Utils" }

    pluginTestDepends: [
        "QbsProjectManager",
        "QmakeProjectManager"
    ]

    Depends {
        name: "QtSupport"
        condition: project.testsEnabled
    }

    Depends {
        name: "Qt.test"
        condition: project.testsEnabled
    }

    Depends { name: "Qt.widgets" }

    files: [
        "autotest.qrc",
        "autotesticons.h",
        "autotest_global.h",
        "autotest_utils.h",
        "autotestconstants.h",
        "autotestplugin.cpp",
        "autotestplugin.h",
        "testcodeparser.cpp",
        "testcodeparser.h",
        "testconfiguration.cpp",
        "testconfiguration.h",
        "testnavigationwidget.cpp",
        "testnavigationwidget.h",
        "testresult.cpp",
        "testresult.h",
        "testresultdelegate.cpp",
        "testresultdelegate.h",
        "testresultmodel.cpp",
        "testresultmodel.h",
        "testresultspane.cpp",
        "testresultspane.h",
        "testrunner.cpp",
        "testrunner.h",
        "testsettings.cpp",
        "testsettings.h",
        "testsettingspage.cpp",
        "testsettingspage.h",
        "testsettingspage.ui",
        "testtreeitem.cpp",
        "testtreeitem.h",
        "testtreeitemdelegate.cpp",
        "testtreeitemdelegate.h",
        "testtreemodel.cpp",
        "testtreemodel.h",
        "testtreeview.cpp",
        "testtreeview.h",
        "testvisitor.cpp",
        "testvisitor.h",
        "testoutputreader.cpp",
        "testoutputreader.h",
    ]

    Group {
        name: "Test sources"
        condition: project.testsEnabled
        files: [
            "autotestunittests.cpp",
            "autotestunittests.h",
            "autotestunittests.qrc",
        ]
    }

    Group {
        name: "Auto Test Wizard"
        prefix: "../../shared/autotest/"
        files: [
            "*"
        ]
        fileTags: []
        qbs.install: true
        qbs.installDir: project.ide_data_path + "/templates/wizards/autotest"
    }
}
