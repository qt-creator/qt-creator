import qbs.base 1.0
import "../../qbs/defaults.js" as Defaults

DynamicLibrary {
    Depends { name: "cpp" }
    Depends {
        condition: Defaults.testsEnabled(qbs)
        name: "Qt.test"
    }

    cpp.defines: Defaults.defines(qbs)
    cpp.linkerFlags: {
        if (qbs.buildVariant == "release" && (qbs.toolchain == "gcc" || qbs.toolchain == "mingw"))
            return ["-Wl,-s"]
    }
    cpp.includePaths: [ ".", ".." ]

    ProductModule {
        Depends { name: "cpp" }
        cpp.includePaths: [ "." ]
    }

    Group {
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: project.ide_library_path
    }
}
