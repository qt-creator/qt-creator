import qbs
import qbs.Probes
import "functions.js" as LibArchiveFunctions

Module {
    Probes.LibraryProbe {
        id: libProbe
        names: LibArchiveFunctions.getLibSearchNames(qbs.hostOS, qbs.toolchain)
        nameSuffixes: LibArchiveFunctions.getLibNameSuffixes(qbs.hostOS, qbs.toolchain)
        searchPaths: LibArchiveFunctions.libarchiveLibDirSearchPaths(qbs.hostOS)
    }

    Probes.IncludeProbe {
        id: incProbe
        names: ["archive.h"]
        searchPaths: LibArchiveFunctions.libarchiveIncludeDirSearchPaths(qbs.hostOS)
    }

    property stringList libarchiveIncludeDir: incProbe.found ? [incProbe.path] : []
    property stringList libarchiveLibDir: libProbe.found ? [libProbe.path] : []
    property stringList libarchiveNames: libProbe.found ? libProbe.names : []
    property bool libarchiveStatic: LibArchiveFunctions.isStaticLib(libProbe.fileName)

    validate: {
        if (!incProbe.found || !libProbe.found) {
            throw new Error("No usable libarchive found.\n"
                         + "Have libarchive and its development headers installed "
                         + "and/or set LIBARCHIVE_INSTALL_ROOT.");
        }
    }
}
