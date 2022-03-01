import qbs 1.0

QtcPlugin {
    name: "McuSupport"

    Depends { name: "Qt.core" }
    Depends { name: "Qt.widgets" }
    Depends { name: "Qt.testlib"; condition: qtc.testsEnabled }
    Depends { name: "Utils" }

    Depends { name: "Core" }
    Depends { name: "BareMetal" }
    Depends { name: "ProjectExplorer" }
    Depends { name: "Debugger" }
    Depends { name: "CMakeProjectManager" }
    Depends { name: "QtSupport" }

    Depends { name: "qtc_gtest_gmock"; condition: qtc.testsEnabled; required: false }

    files: [
        "mcuabstractpackage.h",
        "mcupackage.cpp",
        "mcupackage.h",
        "mcutarget.cpp",
        "mcutarget.h",
        "mcusupport.qrc",
        "mcusupport_global.h",
        "mcusupportconstants.h",
        "mcusupportdevice.cpp",
        "mcusupportdevice.h",
        "mcusupportoptions.cpp",
        "mcusupportoptions.h",
        "mcukitmanager.cpp",
        "mcukitmanager.h",
        "mcusupportoptionspage.cpp",
        "mcusupportoptionspage.h",
        "mcusupportplugin.cpp",
        "mcusupportplugin.h",
        "mcusupportsdk.cpp",
        "mcusupportsdk.h",
        "mcusupportrunconfiguration.cpp",
        "mcusupportrunconfiguration.h",
        "mcusupportversiondetection.cpp",
        "mcusupportversiondetection.h",
        "mcusupportcmakemapper.h",
        "mcusupportcmakemapper.cpp",
        "mcutargetdescription.h",
        "mcukitinformation.cpp",
        "mcukitinformation.h"
    ]

    Group {
        name: "McuSupport test files"
        condition: qtc.testsEnabled && (qtc_gtest_gmock.hasRepo || qtc_gtest_gmock.externalLibsPresent)
        prefix: "test/"
        files: [
            "packagemock.h",
            "unittest.cpp", "unittest.h"
        ]
    }

    Properties {
        condition: qtc.testsEnabled && (qtc_gtest_gmock.hasRepo || qtc_gtest_gmock.externalLibsPresent)
        cpp.defines: base.concat(["GOOGLE_TEST_IS_FOUND"])
        cpp.includePaths: base.concat([ "." ])
    }
}
