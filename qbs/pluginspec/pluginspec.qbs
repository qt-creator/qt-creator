import qbs.base 1.0
import qbs.fileinfo as FileInfo

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
            cmd.qtcreator_version = product.moduleProperty("pluginspec", "qtcreator_version");
            cmd.ide_version_major = product.moduleProperty("pluginspec", "ide_version_major");
            cmd.ide_version_minor = product.moduleProperty("pluginspec", "ide_version_minor");
            cmd.ide_version_release = product.moduleProperty("pluginspec", "ide_version_release");

            cmd.pluginspecreplacements = product.moduleProperty("pluginspec", "pluginspecreplacements");
            cmd.plugin_depends = [];
            var deps = product.dependencies;
            for (var d in deps) {
                var depdeps = deps[d].dependencies;
                for (var dd in depdeps) {
                    if (depdeps[dd].name == 'pluginspec') {
                        cmd.plugin_depends.push(deps[d].name);
                        break;
                    }
                }
            }
            cmd.plugin_recommends = product.pluginRecommends

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
                var deplist = ["<dependencyList>"];
                for (i in plugin_depends) {
                    deplist.push("        <dependency name=\"" + plugin_depends[i] + "\" version=\"" + qtcreator_version + "\"/>");
                }
                for (i in plugin_recommends) {
                    deplist.push("        <dependency name=\"" + plugin_recommends[i] + "\" version=\"" + qtcreator_version + "\" type=\"optional\"/>");
                }
                deplist.push("    </dependencyList>");
                vars['dependencyList'] = deplist.join("\n");
                for (i in vars) {
                    all = all.replace(new RegExp('\\\$\\\$' + i + '(?!\w)', 'g'), vars[i]);
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
                var destdir = FileInfo.joinPaths(product.moduleProperty("qt/core", "generatedFilesDir"),
                                                 input.fileName);
                return destdir.replace(/\.[^\.]*$/, '.json');
            }
        }

        prepare: {
            var xslFile = project.path + "/src/pluginjsonmetadata.xsl";
            var xmlPatternsPath = product.moduleProperty("qt/core", "binPath") + "/xmlpatterns";
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

