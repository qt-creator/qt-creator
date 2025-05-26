var Environment = require("qbs.Environment")
var File = require("qbs.File")
var FileInfo = require("qbs.FileInfo")

function libarchiveIncludeDirSearchPaths(hostOs)
{
    var libarchiveRoot = Environment.getEnv("LIBARCHIVE_INSTALL_ROOT");
    if (libarchiveRoot) {
        if (File.exists(libarchiveRoot)) {
            var incDir = FileInfo.joinPaths(libarchiveRoot, "libarchive", "include");
            if (File.exists(incDir))
                return [incDir];
        }
    }
    if (hostOs.contains("macos")) // add homebrew locations (not default)
        return ["/opt/homebrew/libarchive/include", "/usr/local/opt/libarchive/include"];
    return undefined;
}

function libarchiveLibDirSearchPaths(hostOs)
{
    var libarchiveRoot = Environment.getEnv("LIBARCHIVE_INSTALL_ROOT");
    if (libarchiveRoot) {
        if (File.exists(libarchiveRoot)) {
            var libDir = FileInfo.joinPaths(libarchiveRoot, "libarchive", "lib");
            if (File.exists(libDir))
                return [libDir];
        }
    }
    if (hostOs.contains("macos")) // add homebrew locations (not default)
        return ["/opt/homebrew/libarchive/lib", "/usr/local/opt/libarchive/lib"];
    return undefined;
}

function getLibSearchNames(hostOs, toolchain)
{
    if (hostOs.contains("windows"))
        return ["archive_static", "archive"];
    return ["archive"];
}

function getLibNameSuffixes(hostOs, toolchain) // prefer static over dynamic
{
    if (hostOs.contains("windows")) {
        if (toolchain.contains("mingw"))
            return [".a", ".dll.a"]
        else // msvc
            return [".lib", ".dll"];
    }
    if (hostOs.contains("macos"))
        return [".a", ".dylib"];
    return [".a", ".so"];
}

function isStaticLib(libName)
{
    if (!libName)
        return false;
    var dynSuffixes = [".dll", ".dylib", ".so"];
    for (var i = 0; i < dynSuffixes.length; ++i) {
        if (libName.endsWith(dynSuffixes[i]))
            return false;
    }
    return true;
}
