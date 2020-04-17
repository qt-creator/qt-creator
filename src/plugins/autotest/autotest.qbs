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
    Depends { name: "Debugger" }
    Depends { name: "TextEditor" }

    pluginTestDepends: [
        "QbsProjectManager",
        "QmakeProjectManager"
    ]

    Depends {
        name: "QtSupport"
        condition: qtc.testsEnabled
    }

    Depends {
        name: "Qt.testlib"
        condition: qtc.testsEnabled
    }

    Depends { name: "Qt.widgets" }

    files: [
        "autotest.qrc",
        "autotesticons.h",
        "autotest_global.h",
        "autotestconstants.h",
        "autotestplugin.cpp",
        "autotestplugin.h",
        "projectsettingswidget.cpp",
        "projectsettingswidget.h",
        "testcodeparser.cpp",
        "testcodeparser.h",
        "testconfiguration.cpp",
        "testconfiguration.h",
        "testeditormark.cpp",
        "testeditormark.h",
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
        "testoutputreader.cpp",
        "testoutputreader.h",
        "testprojectsettings.cpp",
        "testprojectsettings.h",
        "itestparser.cpp",
        "itestparser.h",
        "itestframework.cpp",
        "itestframework.h",
        "iframeworksettings.h",
        "testframeworkmanager.cpp",
        "testframeworkmanager.h",
        "testrunconfiguration.h"
    ]

    Group {
        name: "QtTest framework files"
        files: [
            "qtest/*"
        ]
    }

    Group {
        name: "Quick Test framework files"
        files: [
            "quick/*"
        ]
    }

    Group {
        name: "Google Test framework files"
        files: [
            "gtest/*"
        ]
    }

    Group {
        name: "Boost Test framework files"
        files: [
            "boost/*"
        ]
    }

    Group {
        name: "Catch framework files"
        files: [
            "catch/*"
        ]
    }

    Group {
        name: "Test sources"
        condition: qtc.testsEnabled
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
        qbs.installDir: qtc.ide_data_path + "/templates/wizards/autotest"
    }
}
