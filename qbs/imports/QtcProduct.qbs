import qbs 1.0
import qbs.FileInfo
import qbs.Utilities
import QtcFunctions

Product {
    name: project.name
    version: qtc.qtcreator_version
    property bool install: true
    property string installDir
    property string installSourceBase
    property stringList installTags: type
    property string fileName: FileInfo.fileName(sourceDirectory) + ".qbs"
    property bool useNonGuiPchFile: false
    property bool useGuiPchFile: false
    property string pathToSharedSources: FileInfo.joinPaths(path,
            FileInfo.relativePath(FileInfo.joinPaths('/', qtc.ide_qbs_imports_path),
                                  FileInfo.joinPaths('/', qtc.ide_shared_sources_path)))

    Depends { name: "cpp" }
    Depends { name: "qtc" }
    Depends { name: product.name + " dev headers"; required: false }

    Properties {
        condition: Utilities.versionCompare(Qt.core.version, "5.7") < 0
        cpp.minimumMacosVersion: project.minimumMacosVersion
    }

    cpp.cxxLanguageVersion: "c++14"
    cpp.defines: qtc.generalDefines
    cpp.minimumWindowsVersion: qbs.architecture === "x86" ? "5.1" : "5.2"
    cpp.useCxxPrecompiledHeader: useNonGuiPchFile || useGuiPchFile
    cpp.visibility: "minimal"

    Depends { name: "Qt.core" }

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
