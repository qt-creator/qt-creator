Product {
    Depends { name: "qtc" }

    property stringList luafiles

    Group {
        prefix: sourceDirectory + '/' + product.name + '/'
        files: luafiles
        qbs.install: true
        qbs.installDir: qtc.ide_plugin_path + '/' + product.name
    }
}
