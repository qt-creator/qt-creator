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
        "testoutputreader.cpp",
        "testoutputreader.h",
        "itestparser.cpp",
        "itestparser.h",
        "itestframework.h",
        "iframeworksettings.h",
        "itestsettingspage.h",
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
