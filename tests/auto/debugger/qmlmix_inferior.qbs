import qbs.FileInfo
import qbs.TextFile
import qbs.Utilities

QtApplication {
    name: "qmlmix_inferior"
    condition: Qt.quick.present

    Depends { name: "Qt.quick"; required: false }
    Depends { name: "bundle" }

    Qt.qml.importName: "MixTest"
    Qt.qml.importVersion: "1.0"
    Qt.core.resourcePrefix: "/qt/qml/" + Qt.qml.importName

    cpp.defines: ["QT_QML_DEBUG"]
    cpp.includePaths: base.concat(path)

    bundle.isBundle: false

    install: false
    destinationDirectory: project.buildDirectory + '/'
                          + FileInfo.relativePath(project.ide_source_tree, sourceDirectory)

    files: [
        "qmlmix_inferior.h",
        "qmlmix_inferior.cpp",
    ]

    Group {
        name: "qmlmix QML sources"
        Qt.core.resourceSourceBase: path
        files: ["Main.qml"]
        fileTags: ["qt.qml.qml", "qt.core.resource_data"]
    }

    // qbs < 3.4 registers the C++ types, but does not generate a qmldir
    Rule {
        condition: Utilities.versionCompare(qbs.version, "3.4") < 0
        multiplex: true
        inputs: ["qt.qml.qml"]
        Artifact {
            filePath: "qmldir"
            fileTags: ["qt.core.resource_data"]
        }
        prepare: {
            var cmd = new JavaScriptCommand();
            cmd.description = "generating qmldir";
            cmd.moduleName = product.Qt.qml.importName;
            cmd.moduleVersion = product.Qt.qml.importVersion;
            cmd.qmlFileNames = inputs["qt.qml.qml"].map(function(artifact) {
                return artifact.fileName;
            });
            cmd.sourceCode = function() {
                var file = new TextFile(output.filePath, TextFile.WriteOnly);
                try {
                    file.writeLine("module " + moduleName);
                    for (var i = 0; i < qmlFileNames.length; ++i) {
                        file.writeLine(FileInfo.baseName(qmlFileNames[i]) + " " + moduleVersion
                                       + " " + qmlFileNames[i]);
                    }
                } finally {
                    file.close();
                }
            };
            return [cmd];
        }
    }
}
