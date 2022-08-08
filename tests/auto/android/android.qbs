import qbs

QtcAutotest {
    name: "Android AVD Manager autotest"
    Depends { name: "Utils" }
    Depends { name: "Qt.network" }

    property string androidDir: project.ide_source_tree + "/src/plugins/android/"

    Group {
        name: "Sources from Android plugin"
        prefix: androidDir
        files: [
                    "androiddeviceinfo.cpp", "androiddeviceinfo.h",
                    "avdmanageroutputparser.cpp", "avdmanageroutputparser.h",
               ]
    }
    Group {
        name: "Test sources"
        files: [
            "tst_avdmanageroutputparser.cpp",
        ]
    }

    Group {
        name: "Resource files"
        Qt.core.resourcePrefix: "/"
        Qt.core.resourceSourceBase: "."
        fileTags: "qt.core.resource_data"
        files: ["Test.avd/config.ini", "TestTablet.avd/config.ini"]
    }

    cpp.includePaths: base.concat([androidDir, project.ide_source_tree + "/src/plugins/"])
}
