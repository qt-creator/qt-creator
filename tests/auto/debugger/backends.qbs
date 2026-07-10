import qbs
import qbs.FileInfo

Project {
    QtcAutotest {
        name: "backends autotest"
        Depends { name: "Debugger" }
        Depends { name: "Utils" }
        Depends { name: "Qt.network" }
        Depends { name: "qmlstack_inferior"; required: false }
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
            if (Qt.quick.present) {
                defines.push('QMLSTACK_INFERIOR_EXECUTABLE="'
                             + FileInfo.joinPaths(destinationDirectory, "qmlstack_inferior") + '"');
                defines.push('QMLMIX_INFERIOR_EXECUTABLE="'
                             + FileInfo.joinPaths(destinationDirectory, "qmlmix_inferior") + '"');
            }
            return defines;
        }
        cpp.includePaths: base.concat([project.debuggerDir])
    }
    references: [
        "qmlstack_inferior/qmlstack_inferior.qbs",
        "qmlmix_inferior.qbs",
    ]
}
