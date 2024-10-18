
Product {
    name: "TestScript"
    Depends { name: "qtc" }

    builtByDefault: qtc.withAllTests

    Group {
        name: "lua/scripts"
        prefix: "lua/scripts/"
        files: [ "*.lua" ]
        qbs.install: true
        qbs.installDir: qtc.ide_data_path + "/lua/scripts/"
    }
}
