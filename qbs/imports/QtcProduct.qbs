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
    Depends { name: "Qt.core"; versionAtLeast: "5.9.0" }

    // TODO: Should fall back to what came from Qt.core for Qt < 5.7, but we cannot express that
    //       atm. Conditionally pulling in a module that sets the property is also not possible,
    //       because conflicting scalar values would be reported (QBS-1225 would fix that).
    cpp.minimumMacosVersion: project.minimumMacosVersion

    Properties {
        condition: qbs.toolchain.contains("gcc") && !qbs.toolchain.contains("clang")
        cpp.cxxFlags: base.concat(["-Wno-noexcept-type"])
    }
    Properties {
        condition: qbs.toolchain.contains("msvc")
        cpp.cxxFlags: base.concat(["/w44996"])
    }
    cpp.cxxLanguageVersion: "c++14"
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
