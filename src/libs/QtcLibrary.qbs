import qbs.base 1.0
import "../../qbs/defaults.js" as Defaults

DynamicLibrary {
    Depends { name: "cpp" }
    Depends {
        condition: project.testsEnabled
        name: "Qt.test"
    }

    targetName: Defaults.qtLibraryName(qbs, name)
    destinationDirectory: project.ide_library_path

    cpp.defines: project.generalDefines
    cpp.linkerFlags: {
        if (qbs.buildVariant == "release" && (qbs.toolchain.contains("gcc") || qbs.toolchain.contains("mingw")))
            return ["-Wl,-s"]
    }
    cpp.installNamePrefix: "@rpath/PlugIns/"
    cpp.rpaths: qbs.targetOS.contains("osx")
            ? ["@loader_path/..", "@executable_path/.."]
            : ["$ORIGIN", "$ORIGIN/.."]
    cpp.includePaths: [path]

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [path]
    }

    Group {
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: project.ide_library_path
    }
}
