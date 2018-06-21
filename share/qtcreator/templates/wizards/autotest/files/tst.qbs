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
@endif

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

@if "%{GTestCXX11}" == "true"
    cpp.cxxLanguageVersion: "c++11"
    cpp.defines: [ "GTEST_LANG_CXX11" ]
@endif
    cpp.dynamicLibraries: {
        if (qbs.hostOS.contains("windows")) {
            return [];
        } else {
            return [ "pthread" ];
        }
    }


    cpp.includePaths: [].concat(googleCommon.getGTestIncludes(qbs, googletestDir))
                        .concat(googleCommon.getGMockIncludes(qbs, googletestDir))

    files: [
        "%{MainCppName}",
        "%{TestCaseFileWithHeaderSuffix}",
    ].concat(googleCommon.getGTestAll(qbs, googletestDir))
     .concat(googleCommon.getGMockAll(qbs, googletestDir))
@endif
@if "%{TestFrameWork}" == "QtQuickTest"
    Depends { name: "cpp" }
    Depends { name: "Qt.core" }
    Depends {
        condition: Qt.core.versionMajor > 4
        name: "Qt.qmltest"
    }

    Group {
        name: "main application"
        files: [ "%{MainCppName}" ]
    }

    Group {
        name: "qml test files"
        files: "%{TestCaseFileWithQmlSuffix}"
    }

    cpp.defines: base.concat("QUICK_TEST_SOURCE_DIR=\\"" + path + "\\"")
@endif
}
