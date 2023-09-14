import qbs.FileInfo
import QtcFunctions

QtcProduct {
    type: ["dynamiclibrary", "dynamiclibrary_symlink"]
    destinationDirectory: FileInfo.joinPaths(project.buildDirectory, qtc.ide_library_path)
    targetName: QtcFunctions.qtLibraryName(qbs, name)

    installDir: qtc.ide_library_path
    installTags: type.concat("debuginfo_dll")
    useNonGuiPchFile: true

    cpp.linkerFlags: {
        var flags = base;
        if (qbs.buildVariant == "debug" && qbs.toolchain.contains("msvc"))
            flags.push("/INCREMENTAL:NO"); // Speed up startup time when debugging with cdb
        if (qbs.targetOS.contains("macos"))
            flags.push("-compatibility_version", qtc.qtcreator_compat_version);
        return flags;
    }
    cpp.sonamePrefix: qbs.targetOS.contains("macos") ? "@rpath" : undefined
    cpp.rpaths: qbs.targetOS.contains("macos")
            ? ["@loader_path/../Frameworks"]
            : ["$ORIGIN", "$ORIGIN/.."]

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: project.ide_source_tree + "/src/libs"
    }
}
