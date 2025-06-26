import qbs 1.0

QtcPlugin {
    name: "Python"

    Depends { name: "Qt.widgets" }

    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "LanguageClient" }
    Depends { name: "LanguageServerProtocol" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "QtSupport" }
    Depends { name: "TextEditor" }

    Group {
        name: "General"
        files: [
            "../../libs/3rdparty/toml11/toml.hpp",
            "pipsupport.cpp",
            "pipsupport.h",
            "pyprojecttoml.cpp",
            "pyprojecttoml.h",
            "pyside.cpp",
            "pyside.h",
            "pythonbuildconfiguration.cpp",
            "pythonbuildconfiguration.h",
            "pysideuicextracompiler.cpp",
            "pysideuicextracompiler.h",
            "pythonbuildsystem.cpp",
            "pythonbuildsystem.h",
            "pythonconstants.h",
            "pythoneditor.cpp",
            "pythoneditor.h",
            "pythonformattoken.h",
            "pythonhighlighter.cpp",
            "pythonhighlighter.h",
            "pythonindenter.cpp",
            "pythonindenter.h",
            "pythonkitaspect.cpp",
            "pythonkitaspect.h",
            "pythonlanguageclient.cpp",
            "pythonlanguageclient.h",
            "pythonplugin.cpp",
            "pythonproject.cpp",
            "pythonproject.h",
            "pythonrunconfiguration.cpp",
            "pythonrunconfiguration.h",
            "pythonscanner.cpp",
            "pythonscanner.h",
            "pythonsettings.cpp",
            "pythonsettings.h",
            "pythontr.h",
            "pythonutils.cpp",
            "pythonutils.h",
            "pythonwizardpage.cpp",
            "pythonwizardpage.h",
        ]
    }

    Group {
        name: "images"
        prefix: "images/"
        fileTags: "qt.core.resource_data"
        files: [
            "qtforpython_neon.png",
            "settingscategory_python.png",
            "settingscategory_python@2x.png",
        ]
    }

    QtcTestFiles {
        name: "tests"
        prefix: "tests/"
        files: [
            "pyprojecttoml_test.cpp",
            "pyprojecttoml_test.h",
        ]
    }
    QtcTestResources {
        Qt.core.resourceSourceBase: product.sourceDirectory + "/tests/testfiles"
        Qt.core.resourcePrefix: "/unittests/Python"
        files: "tests/testfiles/*"
    }
}
