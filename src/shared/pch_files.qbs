import qbs
import qbs.FileInfo

Product {
    name: "precompiled headers"
    condition: qtc.make_dev_package
    Depends { name: "qtc" }
    Group {
        files: [
            "qtcreator_pch.h",
            "qtcreator_gui_pch.h",
        ]
        qbs.install: true
        qbs.installDir: qtc.ide_shared_sources_path
    }
}
