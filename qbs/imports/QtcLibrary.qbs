import qbs 1.0
import QtcFunctions

DynamicLibrary {
    Depends { name: "cpp" }
    Depends {
        condition: project.testsEnabled
        name: "Qt.test"
    }

    targetName: QtcFunctions.qtLibraryName(qbs, name)
    destinationDirectory: project.ide_library_path

    cpp.defines: project.generalDefines
    cpp.cxxFlags: QtcFunctions.commonCxxFlags(qbs)
    cpp.linkerFlags: {
        var flags = QtcFunctions.commonLinkerFlags(qbs);
        if (qbs.buildVariant == "release" && (qbs.toolchain.contains("gcc") || qbs.toolchain.contains("mingw")))
            flags.push("-Wl,-s");
        else if (qbs.buildVariant == "debug" && qbs.toolchain.contains("msvc"))
            flags.push("/INCREMENTAL:NO"); // Speed up startup time when debugging with cdb
        return flags;
    }
    cpp.installNamePrefix: "@rpath/PlugIns/"
    cpp.rpaths: qbs.targetOS.contains("osx")
            ? ["@loader_path/..", "@executable_path/.."]
            : ["$ORIGIN", "$ORIGIN/.."]
    property string libIncludeBase: ".." // #include <lib/header.h>
    cpp.includePaths: [libIncludeBase]
    cpp.minimumWindowsVersion: qbs.architecture === "x86" ? "5.1" : "5.2"

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [libIncludeBase]
    }

    Group {
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: project.ide_library_path
    }
}
