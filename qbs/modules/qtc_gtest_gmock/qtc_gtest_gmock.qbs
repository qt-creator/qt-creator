import qbs
import qbs.File
import qbs.FileInfo

Module {
    property bool preferExternalLibs: true

    Depends { name: "pkgconfig"; condition: preferExternalLibs; required: false }
    Depends { name: "gtest"; condition: preferExternalLibs; required: false }
    Depends { name: "gmock"; condition: preferExternalLibs; required: false }
    Depends { name: "cpp" }

    Probe {
        id: gtestProbe

        property string repoDir
        property string gtestDir
        property string gmockDir
        property bool hasRepo

        configure: {
            repoDir = FileInfo.cleanPath(path + "/../../../src/libs/3rdparty/googletest");
            gtestDir = FileInfo.joinPaths(repoDir, "googletest");
            gmockDir = FileInfo.joinPaths(repoDir, "googlemock");
            hasRepo = File.exists(gtestDir);
            found = hasRepo;
        }
    }

    property bool hasRepo: gtestProbe.hasRepo
    property bool externalLibsPresent: preferExternalLibs && gtest.present && gmock.present
    property bool useRepo: !externalLibsPresent && hasRepo
    property string gtestDir: gtestProbe.gtestDir
    property string gmockDir: gtestProbe.gmockDir

    Group {
        name: "Files from repository"
        condition: qtc_gtest_gmock.useRepo
        cpp.includePaths: [
            qtc_gtest_gmock.gtestDir,
            qtc_gtest_gmock.gmockDir,
            FileInfo.joinPaths(qtc_gtest_gmock.gtestDir, "include"),
            FileInfo.joinPaths(qtc_gtest_gmock.gmockDir, "include"),
        ]
        files: [
            FileInfo.joinPaths(qtc_gtest_gmock.gtestDir, "src", "gtest-all.cc"),
            FileInfo.joinPaths(qtc_gtest_gmock.gmockDir, "src", "gmock-all.cc"),
        ]
    }

    Properties {
        condition: useRepo
        cpp.includePaths: [
            FileInfo.joinPaths(gtestDir, "include"),
            FileInfo.joinPaths(gmockDir, "include"),
        ]
    }

    validate: {
        if (!externalLibsPresent && !gtestProbe.found) {
            console.warn("No GTest found.");
            throw new Error();
        }
    }
}
