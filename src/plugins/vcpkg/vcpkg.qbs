import qbs 1.0

QtcPlugin {
    name: "Vcpkg"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }
    Depends { name: "Spinner" }

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }

    files: [
        "vcpkgconstants.h",
        "vcpkgmanifesteditor.cpp",
        "vcpkgmanifesteditor.h",
        "vcpkgplugin.cpp",
        "vcpkgsearch.cpp",
        "vcpkgsearch.h",
        "vcpkgsettings.cpp",
        "vcpkgsettings.h",
        "vcpkgtr.h",
    ]

    QtcTestFiles {
        files: [
            "vcpkg_test.h",
            "vcpkg_test.cpp",
        ]
    }

    Group {
        name: "runtime resources"
        fileTags: "qt.core.resource_data"
        files: [
            "images/vcpkgicon.png",
            "images/vcpkgicon@2x.png",
            "wizards/manifest/vcpkg.json.tpl",
            "wizards/manifest/wizard.json",
        ]
    }
}
