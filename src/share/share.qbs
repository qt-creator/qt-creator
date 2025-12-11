Product {
    name: "shared non-sources"

    Depends { name: "qtc" }

    Group {
        name: "cmake-helper"
        files: "3rdparty/cmake-helper/*"
        qbs.install: true
        qbs.installSourceBase: "3rdparty"
        qbs.installDir: qtc.ide_data_path
    }
}
