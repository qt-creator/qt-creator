import qbs
import qbs.File
import "../common/functions.js" as googleCommon

StaticLibrary {
    name: "googletestlib"

    Depends { name: "cpp" }

    property string gtestDir: googleCommon.getGTestDir(qbs, project.googletestDir)
    property string gmockDir: googleCommon.getGMockDir(qbs, project.googletestDir)

    condition: {
        if (File.exists(gtestDir) && File.exists(gmockDir))
            return true;

        console.error("Cannot find Google Test - specify environment variable GOOGLETEST_DIR "
                      + "or qbs property " + project.name + ".googletestDir" );
        return false;
    }

    cpp.includePaths: [].concat(googleCommon.getGTestIncludes(qbs, project.googletestDir))
                        .concat(googleCommon.getGMockIncludes(qbs, project.googletestDir))

    cpp.cxxLanguageVersion: "c++11"
    cpp.defines: ["GTEST_LANG_CXX11"]

    files: [
        "gtlibtests.cpp",
        "gtlibtests.h",
    ]

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [exportingProduct.sourceDirectory]
    }
}
