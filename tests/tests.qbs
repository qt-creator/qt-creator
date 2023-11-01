import qbs.FileInfo

Project {
    name: "Tests"
    references: [
        "auto/auto.qbs",
        "manual/manual.qbs",
        "tools/qml-ast2dot/qml-ast2dot.qbs",
        "unit/unit.qbs",
    ]

    AutotestRunner {
        Depends { name: "Qt.core" }
        Depends { name: "qtc" }
        environment: {
            var env = base;
            if (!qbs.hostOS.contains("windows") || !qbs.targetOS.contains("windows"))
                return env;
            var path = "";
            for (var i = 0; i < env.length; ++i) {
                if (env[i].startsWith("PATH=")) {
                    path = env[i].substring(5);
                    break;
                }
            }
            var fullQtcInstallDir = FileInfo.joinPaths(qbs.installRoot, qbs.installPrefix);
            var fullLibInstallDir = FileInfo.joinPaths(fullQtcInstallDir, qtc.ide_library_path);
            var fullPluginInstallDir = FileInfo.joinPaths(fullQtcInstallDir, qtc.ide_plugin_path);
            path = Qt.core.binPath + ";" + fullLibInstallDir + ";" + fullPluginInstallDir
                    + ";" + path;
            var arrayElem = "PATH=" + path;
            if (i < env.length)
                env[i] = arrayElem;
            else
                env.push(arrayElem);
            return env;
        }
    }
}
