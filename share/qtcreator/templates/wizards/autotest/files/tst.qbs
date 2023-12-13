@if "%{TestFrameWork}" == "GTest" || "%{TestFrameWork}" == "GTest_dyn"
import qbs.Environment
import "googlecommon.js" as googleCommon
@endif
@if "%{TestFrameWork}" == "BoostTest"
import qbs.Environment
import qbs.File
@endif
@if "%{TestFrameWork}" == "BoostTest_dyn"
import qbs.Environment
import qbs.File
import qbs.FileInfo
@endif
@if "%{TestFrameWork}" == "Catch2"
import qbs.Environment
import qbs.File
@endif
@if "%{TestFrameWork}" == "Catch2_dyn"
import qbs.Environment
import qbs.File

import "catchCommon.js" as catchCommon
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

@if "%{TestFrameWork}" == "GTest" || "%{TestFrameWork}" == "GTest_dyn"
    property string googletestDir: {
        if (typeof Environment.getEnv("GOOGLETEST_DIR") === 'undefined') {
            if ("%{GTestBaseFolder}" === "" && googleCommon.getGTestDir(qbs, undefined) !== "") {
                console.warn("Using googletest from system")
            } else {
                console.warn("Using googletest src dir specified at Qt Creator wizard")
                console.log("set GOOGLETEST_DIR as environment variable or Qbs property to get rid of this message")
            }
            return "%{GTestBaseFolder}"
        } else {
            return Environment.getEnv("GOOGLETEST_DIR")
        }
    }

    cpp.cxxLanguageVersion: "c++14"
    cpp.dynamicLibraries: {
@if "%{TestFrameWork}" == "GTest"
        var tmp = [];
@else
        var tmp = ["gtest", "gmock"];
@endif
        if (qbs.hostOS.contains("windows")) {
            return tmp;
        } else {
            return tmp.concat([ "pthread" ]);
        }
    }
@endif
@if "%{TestFrameWork}" == "GTest"
    cpp.includePaths: [].concat(googleCommon.getGTestIncludes(qbs, googletestDir))
                        .concat(googleCommon.getGMockIncludes(qbs, googletestDir))

    files: [
        "%{MainCppName}",
        "%{TestCaseFileGTestWithCppSuffix}",
    ].concat(googleCommon.getGTestAll(qbs, googletestDir))
     .concat(googleCommon.getGMockAll(qbs, googletestDir))
@endif
@if "%{TestFrameWork}" == "GTest_dyn"
    cpp.includePaths: [].concat(googleCommon.getChildPath(qbs, googletestDir, "include"));
    cpp.libraryPaths: googleCommon.getChildPath(qbs, googletestDir, "lib")

    files: [
        "%{MainCppName}",
        "%{TestCaseFileGTestWithCppSuffix}",
    ]
@endif
@if "%{TestFrameWork}" == "QtQuickTest"
    Depends { name: "cpp" }
    Depends { name: "Qt.core" }
@if "%{UseSetupCode}" === "true"
    Depends { name: "Qt.qml" }
@endif
    Depends {
        condition: Qt.core.versionMajor > 4
        name: "Qt.qmltest"
    }

    Group {
        name: "main application"
        files: [
            "%{MainCppName}",
@if "%{UseSetupCode}" === "true"
            "setup.cpp",
            "setup.h"
@endif
        ]
    }

    Group {
        name: "qml test files"
        files: "%{TestCaseFileWithQmlSuffix}"
    }

    cpp.defines: base.concat("QUICK_TEST_SOURCE_DIR=\\"" + path + "\\"")
@endif
@if "%{TestFrameWork}" == "BoostTest"
    type: "application"

    property string boostIncDir: {
        if (typeof Environment.getEnv("BOOST_INCLUDE_DIR") !== 'undefined')
            return Environment.getEnv("BOOST_INCLUDE_DIR");
        return "%{BoostIncDir}"; // set by Qt Creator wizard
    }

    Properties {
        condition: boostIncDir && File.exists(boostIncDir)
        cpp.includePaths: [boostIncDir];
    }

    condition: {
        if (!boostIncDir)
            console.log("BOOST_INCLUDE_DIR is not set, assuming Boost can be "
                        + "found automatically in your system");
        return true;
    }

    files: [ "%{MainCppName}" ]

@endif
@if "%{TestFrameWork}" == "BoostTest_dyn"
    type: "application"

    property string boostInstallDir: {
        if (typeof Environment.getEnv("BOOST_INSTALL_DIR") !== 'undefined')
            return Environment.getEnv("BOOST_INSTALL_DIR");
        return "%{BoostInstallDir}"; // set by Qt Creator wizard
    }

    Properties {
        condition: boostInstallDir && File.exists(boostInstallDir)
        cpp.includePaths: base.concat([qbs.hostOS.contains("windows")
                                       ? boostInstallDir
                                       : FileInfo.joinPaths(boostInstallDir, "include")])
        // Windows: adapt to different directory layout, e.g. "lib64-msvc-14.2"
        cpp.libraryPaths: base.concat([FileInfo.joinPaths(boostInstallDir, "lib")])
    }
    cpp.defines: base.concat("BOOST_UNIT_TEST_FRAMEWORK_DYN_LINK")
    // Windows: adapt to name scheme, e.g. "boost_unit_test_framework-vc142-mt-gd-x64-1_80"
    cpp.dynamicLibraries: ["boost_unit_test_framework"]

    condition: {
        if (!boostInstallDir)
            console.log("BOOST_INSTALL_DIR is not set, assuming Boost can be "
                        + "found automatically in your system");
        return true;
    }

    files: [
        "%{MainCppName}",
        "%{TestCaseFileWithCppSuffix}",
    ]

@endif
@if "%{TestFrameWork}" == "Catch2"
    type: "application"

@if "%{Catch2NeedsQt}" == "true"
    Depends { name: "Qt.gui" }
@endif

    property string catchIncDir: {
        if (typeof Environment.getEnv("CATCH_INCLUDE_DIR") !== 'undefined')
            return Environment.getEnv("CATCH_INCLUDE_DIR");
        return "%{CatchIncDir}"; // set by Qt Creator wizard
    }

    Properties {
        condition: catchIncDir && File.exists(catchIncDir)
        cpp.includePaths: [catchIncDir];
    }

    condition: {
        if (!catchIncDir)
            console.log("CATCH_INCLUDE_DIR is not set, assuming Catch2 can be "
                        + "found automatically in your system");
        return true;
    }

    files: [
        "%{MainCppName}",
        "%{TestCaseFileWithCppSuffix}",
    ]
@endif
@if "%{TestFrameWork}" == "Catch2_dyn"
    property string catch2Dir: {
        if (typeof Environment.getEnv("CATCH_INSTALL_DIR") === 'undefined') {
            if ("%{CatchInstallDir}" === "") {
                console.warn("Using Catch2 from system")
            } else {
                console.warn("Using Catch2 install dir specified at Qt Creator wizard")
                console.log("set CATCH_INSTALL_DIR as environment variable or Qbs property to get rid of this message")
                return "%{CatchInstallDir}";
            }
            return "";
        } else {
            return Environment.getEnv("CATCH_INSTALL_DIR");
        }
    }

    Properties {
        condition: catch2Dir !== "" && File.exists(catch2Dir)
        cpp.includePaths: [].concat(catchCommon.getChildPath(qbs, catch2Dir, "include"));
        cpp.libraryPaths: catchCommon.getChildPath(qbs, catch2Dir, "lib")
    }
@if "%{Catch2Main}" == "false"
    cpp.dynamicLibraries: base.concat(["Catch2Main", "Catch2"])
@else
    cpp.dynamicLibraries: base.concat(["Catch2"])
@endif

    files: [
@if "%{Catch2Main}" == "true"
        "%{MainCppName}",
@endif
        "%{TestCaseFileWithCppSuffix}",
    ]

@endif

}
