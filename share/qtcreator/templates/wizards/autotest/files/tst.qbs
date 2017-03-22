import qbs
@if "%{TestFrameWork}" == "GTest"
import qbs.Environment
import "googlecommon.js" as googleCommon
@endif

CppApplication {
@if "%{TestFrameWork}" == "QtTest"
    Depends { name: "Qt.testlib" }
@if "%{RequireGUI}" == "false"
    consoleApplication: true
@else
    Depends { name: "Qt.gui" }
@endif
    files: [
       "%{TestCaseFileWithCppSuffix}"
    ]
@else
    consoleApplication: true

@if "%{TestFrameWork}" == "GTest"
    property string googletestDir: {
        if (typeof Environment.getEnv("GOOGLETEST_DIR") === 'undefined') {
            console.warn("Using googletest src dir specified at Qt Creator wizard")
            console.log("set GOOGLETEST_DIR as environment variable or Qbs property to get rid of this message")
            return "%{GTestRepository}"
        } else {
            return Environment.getEnv("GOOGLETEST_DIR")
        }
    }
@endif

@if "%{GTestCXX11}" == "true"
    cpp.cxxLanguageVersion: "c++11"
    cpp.defines: [ "GTEST_LANG_CXX11" ]
@endif
    cpp.dynamicLibraries: [ "pthread" ]


    cpp.includePaths: [].concat(googleCommon.getGTestIncludes(googletestDir))
                        .concat(googleCommon.getGMockIncludes(googletestDir))

    files: [
        "%{MainCppName}",
        "%{TestCaseFileWithHeaderSuffix}",
    ].concat(googleCommon.getGTestAll(googletestDir))
     .concat(googleCommon.getGMockAll(googletestDir))
@endif
}
