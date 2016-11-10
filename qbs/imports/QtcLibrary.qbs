import qbs 1.0
import QtcFunctions

QtcProduct {
    type: ["dynamiclibrary", "dynamiclibrary_symlink", "qtc.dev-module"]
    installDir: qtc.ide_library_path
    installTags: ["dynamiclibrary", "dynamiclibrary_symlink"]
    useNonGuiPchFile: true
    Depends {
        condition: qtc.testsEnabled
        name: "Qt.testlib"
    }

    targetName: QtcFunctions.qtLibraryName(qbs, name)
    destinationDirectory: qtc.ide_library_path

    cpp.linkerFlags: {
        var flags = base;
        if (qbs.buildVariant == "debug" && qbs.toolchain.contains("msvc"))
            flags.push("/INCREMENTAL:NO"); // Speed up startup time when debugging with cdb
        if (qbs.targetOS.contains("macos"))
            flags.push("-compatibility_version", qtc.qtcreator_compat_version);
        return flags;
    }
    cpp.sonamePrefix: qbs.targetOS.contains("macos")
            ? "@rpath"
            : undefined
    cpp.rpaths: qbs.targetOS.contains("macos")
            ? ["@loader_path/../Frameworks"]
            : ["$ORIGIN", "$ORIGIN/.."]
    property string libIncludeBase: ".." // #include <lib/header.h>
    cpp.includePaths: [libIncludeBase]

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [product.libIncludeBase]
    }
}
