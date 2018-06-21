import qbs
import qbs.Environment
import qbs.File
import qbs.FileInfo
import qbs.Process
import qbs.Utilities

QtcLibrary {
    condition: qbs.toolchain.contains("msvc") && cdbPath
    name: "qtcreatorcdbext"
    targetName: name
    property string cdbPath: {
        var paths = [
            Environment.getEnv("CDB_PATH"),
            Environment.getEnv("ProgramFiles") + "/Debugging Tools For Windows/sdk",
            Environment.getEnv("ProgramFiles") + "/Debugging Tools For Windows (x86)/sdk",
            Environment.getEnv("ProgramFiles") + "/Debugging Tools For Windows (x64)/sdk",
            Environment.getEnv("ProgramFiles") + "/Debugging Tools For Windows 64-bit/sdk",
            Environment.getEnv("ProgramW6432") + "/Debugging Tools For Windows (x86)/sdk",
            Environment.getEnv("ProgramW6432") + "/Debugging Tools For Windows (x64)/sdk",
            Environment.getEnv("ProgramW6432") + "/Debugging Tools For Windows 64-bit/sdk",
            Environment.getEnv("ProgramFiles") + "/Windows Kits/8.0/Debuggers",
            Environment.getEnv("ProgramFiles") + "/Windows Kits/8.1/Debuggers",
            Environment.getEnv("ProgramFiles") + "/Windows Kits/10/Debuggers",
            Environment.getEnv("ProgramFiles(x86)") + "/Windows Kits/8.0/Debuggers/inc",
            Environment.getEnv("ProgramFiles(x86)") + "/Windows Kits/8.1/Debuggers/inc",
            Environment.getEnv("ProgramFiles(x86)") + "/Windows Kits/10/Debuggers/inc"
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

    property string pythonInstallDir: Environment.getEnv("PYTHON_INSTALL_DIR")

    Probe {
        id: pythonDllProbe
        condition: product.condition
        property string pythonDir: pythonInstallDir // Input
        property string buildVariant: qbs.buildVariant // Input
        property string fileNamePrefix // Output
        configure: {
            function printWarning(msg) {
                console.warn(msg + " The python dumpers for cdb will not be available.");
            }

            if (!pythonDir) {
                printWarning("PYTHON_INSTALL_DIR not set.");
                return;
            }
            if (!File.exists(pythonDir)) {
                printWarning("The provided python installation directory '" + pythonDir
                             + "' does not exist.");
                return;
            }
            var p = new Process();
            try {
                var pythonFilePath = FileInfo.joinPaths(pythonDir, "python.exe");
                p.exec(pythonFilePath, ["--version"], true);
                var output = p.readStdOut().trim();
                var magicPrefix = "Python ";
                if (!output.startsWith(magicPrefix)) {
                    printWarning("Unexpected python output when checking for version: '"
                                 + output + "'");
                    return;
                }
                var versionNumberString = output.slice(magicPrefix.length);
                var versionNumbers = versionNumberString.split('.');
                if (versionNumbers.length < 2) {
                    printWarning("Unexpected python output when checking for version: '"
                            + output + "'");
                    return;
                }
                if (Utilities.versionCompare(versionNumberString, "3.5") < 0) {
                    printWarning("The python installation at '" + pythonDir
                                 + "' has version " + versionNumberString + ", but 3.5 or higher "
                                 + "is required.");
                    return;
                }
                found = true;
                fileNamePrefix = "python" + versionNumbers[0] + versionNumbers[1];
                if (buildVariant === "debug")
                    fileNamePrefix += "_d"
            } finally {
                p.close();
            }

        }
    }

    Group {
        name: "pythonDumper"
        condition: pythonDllProbe.found
        files: [
            "pycdbextmodule.cpp",
            "pycdbextmodule.h",
            "pyfield.cpp",
            "pyfield.h",
            "pystdoutredirect.cpp",
            "pystdoutredirect.h",
            "pytype.cpp",
            "pytype.h",
            "pyvalue.cpp",
            "pyvalue.h",
        ]
    }

    Properties {
        condition: pythonDllProbe.found
        cpp.defines: ["WITH_PYTHON=1"]
    }
    cpp.includePaths: {
        var paths = [FileInfo.joinPaths(cdbPath, "inc")];
        if (pythonDllProbe.found)
            paths.push(FileInfo.joinPaths(pythonInstallDir, "include"));
        return paths;
    }
    cpp.dynamicLibraries: {
        var libs = [ "user32.lib", FileInfo.joinPaths(cdbLibPath, "dbgeng.lib") ];
        if (pythonDllProbe.found)
            libs.push(FileInfo.joinPaths(pythonInstallDir, "libs",
                                         pythonDllProbe.fileNamePrefix + ".lib"));
        return libs;
    }
    cpp.linkerFlags: ["/DEF:" + FileInfo.toWindowsSeparators(
                                    FileInfo.joinPaths(product.sourceDirectory,
                                                       "qtcreatorcdbext.def"))]
    installDir: {
        var dirName = name;
        if (qbs.architecture.contains("x86_64"))
            dirName += "64";
        else
            dirName += "32";
        return FileInfo.joinPaths(qtc.libDirName, dirName);
    }

    Group {
        condition: pythonDllProbe.found
        files: [FileInfo.joinPaths(pythonInstallDir, pythonDllProbe.fileNamePrefix + ".dll")]
        qbs.install: true
        qbs.installDir: installDir
    }

    useNonGuiPchFile: false

    files: [
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

