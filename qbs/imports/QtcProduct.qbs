import qbs 1.0
import QtcFunctions

Product {
    version: project.qtcreator_version
    property bool install: true
    property string installDir

    Depends { name: "cpp" }
    cpp.defines: project.generalDefines
    cpp.cxxLanguageVersion: "c++11"
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
        fileTagsFilter: product.type
        qbs.install: install
        qbs.installDir: installDir
    }
}
