import qbs 1.0
import qbs.FileInfo
import QtcFunctions

Product {
    name: project.name
    version: qtc.qtcreator_version
    property bool install: true
    property string installDir
    property stringList installTags: type
    property string fileName: FileInfo.fileName(sourceDirectory) + ".qbs"

    Depends { name: "cpp" }
    Depends { name: "qtc" }
    Depends { name: product.name + " dev headers"; required: false }

    cpp.cxxLanguageVersion: "c++11"
    cpp.defines: qtc.generalDefines
    cpp.linkerFlags: {
        var flags = [];
        if (qbs.buildVariant == "release" && (qbs.toolchain.contains("gcc") || qbs.toolchain.contains("mingw")))
            flags.push("-Wl,-s");
        return flags;
    }
    cpp.minimumOsxVersion: "10.7"
    cpp.minimumWindowsVersion: qbs.architecture === "x86" ? "5.1" : "5.2"
    cpp.visibility: "minimal"

    Depends { name: "Qt.core" }

    Group {
        fileTagsFilter: installTags
        qbs.install: install
        qbs.installDir: installDir
    }

    Group {
        fileTagsFilter: ["qtc.dev-module"]
        qbs.install: true
        qbs.installDir: qtc.ide_qbs_modules_path + '/' + product.name
    }
}
