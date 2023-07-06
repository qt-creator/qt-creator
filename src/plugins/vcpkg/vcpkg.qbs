import qbs 1.0

QtcPlugin {
    name: "Vcpkg"

    Depends { name: "Qt.widgets" }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "TextEditor" }

    files: [
        "vcpkg.qrc",
        "vcpkgconstants.h",
        "vcpkgmanifesteditor.cpp",
        "vcpkgmanifesteditor.h",
        "vcpkgplugin.cpp",
        "vcpkgsearch.cpp",
        "vcpkgsearch.h",
        "vcpkgsettings.cpp",
        "vcpkgsettings.h",
    ]

    QtcTestFiles {
        files: [
            "vcpkg_test.h",
            "vcpkg_test.cpp",
        ]
    }
}
