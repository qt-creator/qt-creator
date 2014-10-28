import qbs
import qbs.File
import qbs.FileInfo

QtcLibrary {
    condition: qbs.toolchain.contains("msvc") && cdbPath
    name: "qtcreatorcdbext"
    targetName: name
    property string cdbPath: {
        var paths = [
            qbs.getEnv("CDB_PATH"),
            qbs.getEnv("ProgramFiles") + "/Debugging Tools For Windows/sdk",
            qbs.getEnv("ProgramFiles") + "/Debugging Tools For Windows (x86)/sdk",
            qbs.getEnv("ProgramFiles") + "/Debugging Tools For Windows (x64)/sdk",
            qbs.getEnv("ProgramFiles") + "/Debugging Tools For Windows 64-bit/sdk",
            qbs.getEnv("ProgramW6432") + "/Debugging Tools For Windows (x86)/sdk",
            qbs.getEnv("ProgramW6432") + "/Debugging Tools For Windows (x64)/sdk",
            qbs.getEnv("ProgramW6432") + "/Debugging Tools For Windows 64-bit/sdk",
            qbs.getEnv("ProgramFiles") + "/Windows Kits/8.0/Debuggers",
            qbs.getEnv("ProgramFiles") + "/Windows Kits/8.1/Debuggers",
            qbs.getEnv("ProgramFiles(x86)") + "/Windows Kits/8.0/Debuggers/inc",
            qbs.getEnv("ProgramFiles(x86)") + "/Windows Kits/8.1/Debuggers/inc"
        ];
        var c = paths.length;
        for (var i = 0; i < c; ++i) {
            if (File.exists(paths[i])) {
                // The inc subdir is just used for detection. See qtcreatorcdbext.pro.
                return paths[i].endsWith("/inc") ? paths[i].substr(0, paths[i].length - 4)
                                                 : paths[i];
            }
        }
        return undefined;
    }
    property string cdbLibPath: {
        var paths = qbs.architecture.contains("x86_64") ? ["x64", "amd64"] : ["x86", "i386"];
        var c = paths.length;
        for (var i = 0; i < c; ++i) {
            var libPath = FileInfo.joinPaths(cdbPath, "lib", paths[i]);
            if (File.exists(libPath)) {
                return libPath;
            }
        }
        return undefined;
    }
    cpp.includePaths: [FileInfo.joinPaths(cdbPath, "inc")]
    cpp.dynamicLibraries: [
        "user32.lib",
        FileInfo.joinPaths(cdbLibPath, "dbgeng.lib")
    ]
    cpp.linkerFlags: ["/DEF:" + FileInfo.toWindowsSeparators(
                                    FileInfo.joinPaths(product.sourceDirectory,
                                                       "qtcreatorcdbext.def"))]
    installDir: {
        var dirName = name;
        if (qbs.architecture.contains("x86_64"))
            dirName += "64";
        else
            dirName += "32";
        return FileInfo.joinPaths(project.libDirName, dirName);
    }
    files: [
        "base64.cpp",
        "base64.h",
        "common.cpp",
        "common.h",
        "containers.cpp",
        "containers.h",
        "eventcallback.cpp",
        "eventcallback.h",
        "extensioncontext.cpp",
        "extensioncontext.h",
        "gdbmihelpers.cpp",
        "gdbmihelpers.h",
        "iinterfacepointer.h",
        "knowntype.h",
        "outputcallback.cpp",
        "outputcallback.h",
        "qtcreatorcdbextension.cpp",
        "stringutils.cpp",
        "stringutils.h",
        "symbolgroup.cpp",
        "symbolgroup.h",
        "symbolgroupnode.cpp",
        "symbolgroupnode.h",
        "symbolgroupvalue.cpp",
        "symbolgroupvalue.h",
    ]
}

