import qbs 1.0
import QtcFunctions

QtcProduct {
    type: "dynamiclibrary"
    installDir: project.ide_library_path
    Depends {
        condition: project.testsEnabled
        name: "Qt.test"
    }

    targetName: QtcFunctions.qtLibraryName(qbs, name)
    destinationDirectory: project.ide_library_path

    cpp.linkerFlags: {
        var flags = base;
        if (qbs.buildVariant == "debug" && qbs.toolchain.contains("msvc"))
            flags.push("/INCREMENTAL:NO"); // Speed up startup time when debugging with cdb
        return flags;
    }
    cpp.installNamePrefix: "@rpath/Frameworks/"
    cpp.rpaths: qbs.targetOS.contains("osx")
            ? ["@loader_path/..", "@executable_path/.."]
            : ["$ORIGIN", "$ORIGIN/.."]
    property string libIncludeBase: ".." // #include <lib/header.h>
    cpp.includePaths: [libIncludeBase]

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [product.libIncludeBase]
    }
}
