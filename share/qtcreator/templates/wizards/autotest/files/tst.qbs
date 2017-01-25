import qbs
@if "%{TestFrameWork}" == "GTest"
import "../googlecommon.js" as googleCommon
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
@if "%{GTestCXX11}" == "true"
    cpp.cxxLanguageVersion: "c++11"
    cpp.defines: [ "GTEST_LANG_CXX11" ]
@endif
    cpp.dynamicLibraries: [ "pthread" ]


    cpp.includePaths: [].concat(googleCommon.getGTestIncludes(project.googletestDir))
                        .concat(googleCommon.getGMockIncludes(project.googletestDir))

    files: [
        "%{MainCppName}",
        "%{TestCaseFileWithHeaderSuffix}",
    ].concat(googleCommon.getGTestAll(project.googletestDir))
     .concat(googleCommon.getGMockAll(project.googletestDir))
@endif
}
