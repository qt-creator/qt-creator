import qbs.base 1.0
import "../../qbs/defaults.js" as Defaults

DynamicLibrary {
    Depends { name: "cpp" }
    Depends {
        condition: Defaults.testsEnabled(qbs)
        name: "Qt.test"
    }

    targetName: Defaults.qtLibraryName(qbs, name)

    cpp.defines: Defaults.defines(qbs)
    cpp.linkerFlags: {
        if (qbs.buildVariant == "release" && (qbs.toolchain.contains("gcc") || qbs.toolchain.contains("mingw")))
            return ["-Wl,-s"]
    }
    cpp.installNamePrefix: "@rpath/PlugIns/"
    cpp.rpaths: qbs.targetOS.contains("osx")
            ? ["@loader_path/..", "@executable_path/.."]
            : ["$ORIGIN", "$ORIGIN/.."]
    cpp.includePaths: [ ".", ".." ]

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [ "." ]
    }

    Group {
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: project.ide_library_path
    }
}
