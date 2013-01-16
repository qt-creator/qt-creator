import qbs.base 1.0
import qbs.fileinfo 1.0 as FileInfo

Module {
    Depends { id: qtcore; name: "Qt.core" }

    additionalProductFileTags: qtcore.versionMajor < 5 ? ["pluginSpec"] : ["qt_plugin_metadata"]
    property int ide_version_major: project.ide_version_major
    property int ide_version_minor: project.ide_version_minor
    property int ide_version_release: project.ide_version_release
    property string qtcreator_version: ide_version_major + '.' + ide_version_minor + '.' + ide_version_release
    property var pluginspecreplacements: product.pluginspecreplacements

    Rule {
        inputs: ["pluginSpecIn"]

        Artifact {
            fileTags: ["pluginSpec"]
            fileName: input.fileName.replace(/\.[^\.]*$/,'')
        }

        prepare: {
            var cmd = new JavaScriptCommand();
            cmd.description = "prepare " + FileInfo.fileName(output.fileName);
            cmd.highlight = "codegen";
            cmd.qtcreator_version = product.module.qtcreator_version;
            cmd.ide_version_major = product.module.ide_version_major;
            cmd.ide_version_minor = product.module.ide_version_minor;
            cmd.ide_version_release = product.module.ide_version_release;

            cmd.pluginspecreplacements = product.module.pluginspecreplacements;

            cmd.sourceCode = function() {
                var i;
                var vars = pluginspecreplacements || {};
                var inf = new TextFile(input.fileName);
                var all = inf.readAll();
                // replace quoted quotes
                all = all.replace(/\\\"/g, '"');
                // replace config vars
                vars['QTCREATOR_VERSION'] = qtcreator_version;
                vars['IDE_VERSION_MAJOR'] = ide_version_major;
                vars['IDE_VERSION_MINOR'] = ide_version_minor;
                vars['IDE_VERSION_RELEASE'] = ide_version_release;
                for (i in vars) {
                    all = all.replace(new RegExp('\\\$\\\$' + i.toUpperCase() + '(?!\w)', 'g'), vars[i]);
                }
                var file = new TextFile(output.fileName, TextFile.WriteOnly);
                file.truncate();
                file.write(all);
                file.close();
            }
            return cmd;
        }
    }

    Rule {
        inputs: ["pluginSpec"]

        Artifact {
            fileTags: ["qt_plugin_metadata"]
            fileName: {
                var destdir = FileInfo.joinPaths(product.modules["qt/core"].generatedFilesDir,
                                                 input.fileName);
                return destdir.replace(/\.[^\.]*$/, '.json');
            }
        }

        prepare: {
            var xslFile = project.path + "/src/pluginjsonmetadata.xsl";
            var xmlPatternsPath = product.modules["qt/core"].binPath + "/xmlpatterns";
            var args = [
                "-no-format",
                "-output",
                output.fileName,
                xslFile,
                input.fileName
            ];
            var cmd = new Command(xmlPatternsPath, args);
            cmd.description = "generating " + FileInfo.fileName(output.fileName);
            cmd.highlight = "codegen";
            return cmd;
        }
    }
}

