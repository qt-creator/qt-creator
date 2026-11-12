import qbs 1.0
import QtcFunctions

Project {
    // Windows has no rpath to say where the plugins the tool links are. A
    // launcher finds them for it, and the tool is a library next to the launcher.
    QtcLibrary {
        name: "QtProfilerCli"
        condition: Tracing.present && qbs.targetOS.contains("windows")

        Depends { name: "app_version_header" }
        Depends { name: "Utils" }
        Depends { name: "Profiler" }
        Depends { name: "Tracing"; required: false }

        cpp.defines: base.concat(["QTPROFILERCLI_LIBRARY"])
        files: ["qtprofilercli.cpp"]
    }

    QtcTool {
        name: "qtprofiler-cli"

        Depends { name: "codesign" }

        Properties {
            condition: qbs.targetOS.contains("macos")
            // The call-stack sampler needs the debugger entitlement here just as
            // much as in Qt Creator itself (see src/app/app.qbs).
            codesign.enableCodeSigning: true
            codesign.signingType: "ad-hoc"
        }

        Group {
            name: "entitlements"
            condition: qbs.targetOS.contains("macos")
            files: ["../../../dist/installer/mac/Qt Profiler.entitlements"]
        }

        Depends { name: "Tracing"; required: false }
        condition: Tracing.present

        property bool launcher: qbs.targetOS.contains("windows")

        Depends { name: "app_version_header"; condition: !launcher }
        Depends { name: "Utils"; condition: !launcher }
        Depends { name: "Profiler"; condition: !launcher }
        // Built first, but loaded only by the launcher.
        Depends { name: "QtProfilerCli"; condition: launcher; cpp.link: false }

        Properties {
            condition: launcher
            cpp.defines: base.concat(['QTPROFILERCLI_DLL="'
                                      + QtcFunctions.qtLibraryName(qbs, "QtProfilerCli")
                                      + '.dll"'])
        }

        files: launcher ? ["qtprofilercli_win.cpp"] : ["qtprofilercli.cpp"]
    }
}
