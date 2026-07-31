import qbs
import qbs.FileInfo

Project {
    QtcAutotest {
        name: "backends autotest"
        Depends { name: "Debugger" }
        Depends { name: "Utils" }
        Depends { name: "Qt.network" }
        Depends { name: "qmlstack_inferior"; required: false }
        Depends { name: "qmlserver_inferior"; required: false }
        Depends { name: "qmlmix_inferior"; required: false }
        Group {
            name: "Sources from Debugger plugin"
            prefix: project.debuggerDir
            files: [
                "debuggerprotocol.h", "debuggerprotocol.cpp",
                "disassemblerlines.h", "disassemblerlines.cpp"
            ]
        }
        Group {
            name: "Test sources"
            files: [
                "tst_backends.cpp"
            ]
        }

        cpp.defines: {
            var defines = base.concat([
                'DUMPERDIR="' + path + '/../../../share/qtcreator/debugger"',
                'BACKENDS_TEST_SOURCE_DIR="' + path + '"'
            ]);
            if (qmlstack_inferior.present) {
                defines.push('QMLSTACK_INFERIOR_EXECUTABLE="' + destinationDirectory + '"');
            }
            if (qmlserver_inferior.present) {
                defines.push('QMLSERVER_INFERIOR_EXECUTABLE="' + destinationDirectory + '"');
            }
            if (qmlmix_inferior.present) {
                defines.push('QMLMIX_INFERIOR_EXECUTABLE="' +  destinationDirectory + '"');
            }
            // Mirrors the qtcreatorcdbext product: msvc-only, and the bitness
            // suffix sits on the directory, not on the DLL.
            if (qbs.toolchain.contains("msvc")) {
                var extDir = "qtcreatorcdbext"
                        + (qbs.architecture.contains("x86_64") ? "64" : "32");
                defines.push('CDBEXT_LIBRARY="'
                             + FileInfo.joinPaths(project.buildDirectory, qtc.libDirName,
                                                  extDir, "qtcreatorcdbext.dll") + '"');
            }
            return defines;
        }
        cpp.includePaths: base.concat([project.debuggerDir])
    }
    references: [
        "qmlstack_inferior.qbs",
        "qmlserver_inferior.qbs",
        "qmlmix_inferior.qbs",
    ]
}
