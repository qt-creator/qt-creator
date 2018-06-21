import qbs 1.0
import qbs.Environment
import qbs.FileInfo

Project {
    name: "Qt Creator"
    minimumQbsVersion: "1.8.0"
    property string minimumMacosVersion: "10.8"
    property bool withAutotests: qbs.buildVariant === "debug"
    property path ide_source_tree: path
    property pathList additionalPlugins: []
    property pathList additionalLibs: []
    property pathList additionalTools: []
    property pathList additionalAutotests: []
    property string sharedSourcesDir: path + "/src/shared"
    qbsSearchPaths: "qbs"

    references: [
        "doc/doc.qbs",
        "src/src.qbs",
        "share/share.qbs",
        "share/qtcreator/translations/translations.qbs",
        "tests/tests.qbs"
    ]

    Product {
        name: "qbs_imports_modules"
        Depends { name: "qtc" }
        Group {
            prefix: "qbs/"
            files: ["**/*"]
            qbs.install: qtc.make_dev_package
            qbs.installDir: qtc.ide_qbs_resources_path
            qbs.installSourceBase: "qbs"
        }
    }

    Product {
        name: "qmake project files"
        files: {
            var list = ["**/*.pr[io]"];
            var props = [additionalPlugins, additionalLibs, additionalTools, additionalAutotests];
            for (var i = 0; i < props.length; ++i) {
                for (var j = 0; j < props[i].length; ++j)
                    list.push(props[i][j] + "/**/*.pr[io]");
            }
            return list;
        }
    }

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
