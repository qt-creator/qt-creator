import qbs
import qbs.File
import qbs.FileInfo

// The unwinder needs libdw.
QtcAutotest {
    name: "QtProfiler dwarf unwinder autotest"

    Probe {
        id: elfutilsProbe
        property string installBase: qbs.getenv("ELFUTILS_INSTALL_DIR")
        property string includeDir: installBase
            ? FileInfo.joinPaths(installBase, "include")
            : "/usr/include"
        property string libDir: installBase ? FileInfo.joinPaths(installBase, "lib") : ""
        property bool found
        configure: {
            found = File.exists(FileInfo.joinPaths(includeDir, "elfutils", "libdwfl.h"));
        }
    }

    condition: elfutilsProbe.found

    Depends { name: "Utils" }
    cpp.defines: base.concat([
        "PROFILER_STATIC_LIBRARY",
        "WITH_LIBDW",
        'FIXTURE_DIR="' + path + '/fixture"',
    ])
    cpp.includePaths: base.concat([
        path + "/../../../../src/plugins/profiler",
        elfutilsProbe.includeDir,
        FileInfo.joinPaths(elfutilsProbe.includeDir, "elfutils"),
    ])
    cpp.libraryPaths: elfutilsProbe.libDir ? base.concat([elfutilsProbe.libDir]) : base
    cpp.dynamicLibraries: base.concat(qbs.targetOS.contains("windows")
                                      ? ["dw", "elf", "eu_compat"] : ["dw", "elf"])
    files: [
        "tst_dwarfunwinder.cpp",
        "../../../../src/plugins/profiler/perfdwarfunwinder.cpp",
        "../../../../src/plugins/profiler/perfdwarfunwinder.h",
        "../../../../src/plugins/profiler/perfelfutils.h",
        "../../../../src/plugins/profiler/perfregisters.h",
    ]
}
