import qbs.File
import qbs.FileInfo
import qbs.TextFile
import qbs.Utilities

Product {
    Probe {
        id: qtMinVersion
        readonly property string filePath: path + "/../../cmake/QtCreatorAPI.cmake"
        readonly property var lastModified: File.lastModified(filePath)
        property string result
        configure: {
            var f = new TextFile(filePath);
            var content = f.readAll();
            f.close();
            result = content.match(/set\(IDE_QT_VERSION_MIN "([^"]+)"\)/)[1];
            found = true;
        }
    }

    version: qtc.qtcreator_version

    property bool install: true
    property string installDir
    property string installSourceBase: destinationDirectory
    property stringList installTags: type
    property bool useNonGuiPchFile: false
    property bool useGuiPchFile: false
    property bool useQt: true
    property bool hasCMakeProjectFile: true
    property string pathToSharedSources: FileInfo.joinPaths(path,
            FileInfo.relativePath(FileInfo.joinPaths('/', qtc.ide_qbs_imports_path),
                                  FileInfo.joinPaths('/', qtc.ide_shared_sources_path)))
    property bool sanitizable: true
    property bool enforceInternalLinkage: false
    // What Windows shows as "Description". Empty adds no version resource.
    property string windowsFileDescription
    // Directory holding qtcreator.ico. Empty leaves the binary without an icon.
    property string windowsIconPath
    // A resource file of one's own that includes qtcreator_versioninfo.rc.
    // Only one per product works: the icons of a second one collide.
    property string windowsResourceFile

    Depends { name: "cpp" }
    Depends {
        name: "Qt"
        condition: useQt
        submodules: ["core"]
        versionAtLeast: qtMinVersion.result
    }

    Depends { name: "qtc" }

    Properties {
        condition: qbs.toolchain.includes("clang")
        cpp.commonCompilerFlags: "-Wno-parentheses-equality"
    }

    cpp.cxxFlags: {
        var flags = [];
        if (qbs.toolchain.contains("gcc")) {
            flags.push("-Wno-missing-field-initializers");
            if (qbs.toolchain.contains("clang")) {
                if (enforceInternalLinkage)
                    flags.push("-Wmissing-prototypes");
                if (!qbs.hostOS.contains("darwin")
                        && Utilities.versionCompare(cpp.compilerVersion, "10") >= 0) {
                // Triggers a lot in Qt.
                flags.push("-Wno-deprecated-copy", "-Wno-constant-logical-operand");
                }
            } else {
                if (enforceInternalLinkage)
                    flags.push("-Wmissing-declarations");
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
    cpp.cxxLanguageVersion: "c++20"
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
        name: "windows version resource"
        condition: qbs.targetOS.contains("windows") && !!windowsFileDescription
        files: windowsResourceFile
               ? windowsResourceFile
               : pathToSharedSources + "/qtcreator_versioninfo.rc"
        // We need the version in two separate formats for the .rc file
        //  RC_VERSION=4,3,82,0 (quadruple)
        //  RC_VERSION_STRING="4.4.0-beta1" (free text)
        // Also, we need to replace space with \x20 to be able to work with both rc and windres
        cpp.defines: {
            var defines = outer.concat([
                "RC_VERSION=" + qtc.qtcreator_version.replace(/\./g, ",") + ",0",
                "RC_VERSION_STRING=" + qtc.qtcreator_display_version,
                "RC_DESCRIPTION=" + windowsFileDescription.replace(/ /g, "\\x20"),
                "RC_APPLICATION_NAME=" + qtc.ide_display_name.replace(/ /g, "\\x20"),
                "RC_PUBLISHER=" + qtc.ide_publisher.replace(/ /g, "\\x20"),
                "RC_COPYRIGHT=" + qtc.ide_copyright_string.replace(/ /g, "\\x20")]);
            if (windowsIconPath)
                defines.push("RC_ICON_PATH=" + windowsIconPath);
            return defines;
        }
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

    Group {
        name: "CMake project file"
        condition: hasCMakeProjectFile
        prefix: sourceDirectory + '/'
        files: "CMakeLists.txt"
    }
}
