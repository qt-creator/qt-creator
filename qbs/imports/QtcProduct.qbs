import qbs 1.0
import qbs.FileInfo
import qbs.Utilities
import QtcFunctions

Product {
    name: project.name
    version: qtc.qtcreator_version
    property bool install: true
    property string installDir
    property string installSourceBase: destinationDirectory
    property stringList installTags: type
    property string fileName: FileInfo.fileName(sourceDirectory) + ".qbs"
    property bool useNonGuiPchFile: false
    property bool useGuiPchFile: false
    property string pathToSharedSources: FileInfo.joinPaths(path,
            FileInfo.relativePath(FileInfo.joinPaths('/', qtc.ide_qbs_imports_path),
                                  FileInfo.joinPaths('/', qtc.ide_shared_sources_path)))
    property bool sanitizable: true

    Depends { name: "cpp" }
    Depends { name: "qtc" }
    Depends {
        name: product.name + " dev headers";
        required: false
        Properties {
            condition: Utilities.versionCompare(qbs.version, "1.13") >= 0
            enableFallback: false
        }
    }
    Depends { name: "Qt.core"; versionAtLeast: "5.14.0" }

    // TODO: Should fall back to what came from Qt.core for Qt < 5.7, but we cannot express that
    //       atm. Conditionally pulling in a module that sets the property is also not possible,
    //       because conflicting scalar values would be reported (QBS-1225 would fix that).
    cpp.minimumMacosVersion: project.minimumMacosVersion

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
                flags.push("-Wno-noexcept-type");
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

    cpp.cxxLanguageVersion: "c++17"
    cpp.defines: qtc.generalDefines
    cpp.minimumWindowsVersion: "6.1"
    cpp.useCxxPrecompiledHeader: useNonGuiPchFile || useGuiPchFile
    cpp.visibility: "minimal"

    Group {
        fileTagsFilter: installTags
        qbs.install: install
        qbs.installDir: installDir
        qbs.installSourceBase: installSourceBase
    }

    Group {
        fileTagsFilter: ["qtc.dev-module"]
        qbs.install: true
        qbs.installDir: qtc.ide_qbs_modules_path + '/' + product.name
    }

    Group {
        name: "standard pch file (non-gui)"
        condition: useNonGuiPchFile
        prefix: pathToSharedSources + '/'
        files: ["qtcreator_pch.h"]
        fileTags: ["cpp_pch_src"]
    }

    Group {
        name: "standard pch file (gui)"
        condition: useGuiPchFile
        prefix: pathToSharedSources + '/'
        files: ["qtcreator_gui_pch.h"]
        fileTags: ["cpp_pch_src"]
    }
}
