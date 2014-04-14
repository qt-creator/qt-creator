import qbs 1.0
import QtcFunctions

Application {
    type: "application" // no Mac app bundle
    Depends { name: "cpp" }
    cpp.defines: project.generalDefines
    cpp.cxxFlags: QtcFunctions.commonCxxFlags(qbs)
    cpp.linkerFlags: {
        var flags = QtcFunctions.commonLinkerFlags(qbs);
        if (qbs.buildVariant == "release" && (qbs.toolchain.contains("gcc") || qbs.toolchain.contains("mingw")))
            flags.push("-Wl,-s");
        return flags;
    }

    property string toolInstallDir: project.ide_libexec_path

    cpp.rpaths: qbs.targetOS.contains("osx")
            ? ["@executable_path/../" + project.ide_library_path]
            : ["$ORIGIN/../" + project.ide_library_path]
    cpp.minimumWindowsVersion: "5.1"

    Group {
        fileTagsFilter: product.type
        qbs.install: true
        qbs.installDir: toolInstallDir
    }
}
