import qbs.FileInfo
import qbs.Utilities

Product {
    version: qtc.qtcreator_version

    property bool install: true
    property string installDir
    property string installSourceBase: destinationDirectory
    property stringList installTags: type
    property bool useNonGuiPchFile: false
    property bool useGuiPchFile: false
    property bool useQt: true
    property string pathToSharedSources: FileInfo.joinPaths(path,
            FileInfo.relativePath(FileInfo.joinPaths('/', qtc.ide_qbs_imports_path),
                                  FileInfo.joinPaths('/', qtc.ide_shared_sources_path)))
    property bool sanitizable: true

    Depends { name: "cpp" }
    Depends {
        name: "Qt"
        condition: useQt
        submodules: ["core", "core5compat"]
        versionAtLeast: "6.2.0"
    }

    Depends { name: "qtc" }

    cpp.cxxFlags: {
        var flags = [];
        if (qbs.toolchain.contains("gcc")) {
            if (qbs.toolchain.contains("clang")
                    && !qbs.hostOS.contains("darwin")
                    && Utilities.versionCompare(cpp.compilerVersion, "10") >= 0) {
                // Triggers a lot in Qt.
                flags.push("-Wno-deprecated-copy", "-Wno-constant-logical-operand");
            }
            if (!qbs.toolchain.contains("clang")) {
                flags.push("-Wno-missing-field-initializers", "-Wno-noexcept-type");
                if (Utilities.versionCompare(cpp.compilerVersion, "9") >= 0)
                    flags.push("-Wno-deprecated-copy", "-Wno-init-list-lifetime");
            }
            if (qtc.enableAddressSanitizer)
                flags.push("-fno-omit-frame-pointer");
        } else if (qbs.toolchain.contains("msvc")) {
            flags.push("/w44996");
        }
        return flags;
    }
    cpp.cxxLanguageVersion: "c++17"
    cpp.defines: qtc.generalDefines
    Properties {
        condition: sanitizable && qbs.toolchain.contains("gcc")
        cpp.driverFlags: {
            var flags = [];
            if (qtc.enableAddressSanitizer)
                flags.push("-fsanitize=address");
            if (qtc.enableUbSanitizer)
                flags.push("-fsanitize=undefined");
            if (qtc.enableThreadSanitizer)
                flags.push("-fsanitize=thread");
            return flags;
        }
    }
    cpp.useCxxPrecompiledHeader: useQt && (useNonGuiPchFile || useGuiPchFile)
    cpp.visibility: "minimal"

    Group {
        fileTagsFilter: installTags
        qbs.install: install
        qbs.installDir: installDir
        qbs.installSourceBase: installSourceBase
    }

    Group {
        name: "standard pch file (non-gui)"
        condition: useNonGuiPchFile
        prefix: pathToSharedSources + '/'
        files: "qtcreator_pch.h"
        fileTags: "cpp_pch_src"
    }

    Group {
        name: "standard pch file (gui)"
        condition: useGuiPchFile
        prefix: pathToSharedSources + '/'
        files: "qtcreator_gui_pch.h"
        fileTags: "cpp_pch_src"
    }
}
