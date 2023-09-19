import qbs.File
import qbs.FileInfo

CppApplication {
    name: "Manual Test Simple Application"
    targetName: "simple_test_app"

    Depends { name: "Qt.core" }
    Depends { name: "Qt.core-private"; required: false; condition: Qt.core.versionMajor > 4 }
    Depends { name: "Qt.core5compat"; condition: Qt.core.versionMajor > 5 }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.xml"; condition: Qt.core.versionMajor < 6 }
    Depends { name: "Qt.widgets"; condition: Qt.core.versionMajor > 4 }
    Depends { name: "Qt.network" }
    Depends { name: "Qt.script"; required: false }
    Depends { name: "Qt.webkit"; required: false }
    Depends { name: "Qt.webkitwidgets"; required: false; condition: Qt.core.versionMajor > 4 }

    cpp.cxxLanguageVersion: "c++11"

    cpp.defines: {
        var additional = ["USE_GUILIB"];

        if (File.exists("/usr/include/boost/optional.hpp"))
            additional = additional.concat(["HAS_BOOST"]);

        if (File.exists("/usr/include/eigen2/Eigen/Core") || File.exists("/usr/local/include/eigen2/Eigen/Core"))
            additional = additional.concat(["HAS_EIGEN2"]);
        if (File.exists("/usr/include/eigen3/Eigen/Core") || File.exists("/usr/local/include/eigen3/Eigen/Core"))
            additional = additional.concat(["HAS_EIGEN3"]);

        if (Qt.core.versionMajor > 4)
            additional = additional.concat(["HAS_PRIVATE"]);

        if (Qt.script.present)
            additional = additional.concat(["HAS_SCRIPT"]);

        if (qbs.toolchain.contains("msvc"))
            additional = additional.concat(["_CRT_SECURE_NO_WARNINGS"]);

        /* use following for semi-automated testing */
        /* additional.concat(["USE_AUTORUN=1"]); */

        return additional;
    }

    cpp.includePaths: {
        var additional = [];

        if (File.exists("/usr/include/eigen2/Eigen/Core"))
            additional = additional.concat(["/usr/include/eigen2"]);
        if (File.exists("/usr/include/eigen3/Eigen/Core"))
            additional = additional.concat(["/usr/include/eigen3"]);
        if (File.exists("/usr/local/include/eigen2/Eigen/Core"))
            additional.concat(["/usr/local/include/eigen2"]);
        if (File.exists("/usr/local/include/eigen3/Eigen/Core"))
            additional = additional.concat(["/usr/include/eigen3"]);

        return additional;
    }

    files: [
        "simple_test_app.cpp"
    ]

    destinationDirectory: FileInfo.joinPaths(
                              FileInfo.path(project.buildDirectory + '/'
                                            + FileInfo.relativePath(project.ide_source_tree,
                                                                    sourceDirectory)),
                              "simple")

    install: false
}
