import qbs
import qbs.File
import "../common/functions.js" as googleCommon

CppApplication {
    type: "application"
    name: "googletest4"

    Depends { name: "googletestlib" }

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
    cpp.dynamicLibraries: [ "pthread" ]

    files: [
        "main.cpp",
    ].concat(googleCommon.getGTestAll(qbs, project.googletestDir))
     .concat(googleCommon.getGMockAll(qbs, project.googletestDir))
}
