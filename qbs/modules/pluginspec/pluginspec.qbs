import qbs 1.0
import qbs.TextFile
import qbs.FileInfo

Module {
    Depends { id: qtcore; name: "Qt.core" }

    additionalProductTypes: qtcore.versionMajor < 5 ? ["pluginSpec"] : ["qt_plugin_metadata"]

    Rule {
        inputs: ["pluginSpecIn"]

        Artifact {
            fileTags: ["pluginSpec"]
            filePath: input.fileName.replace(/\.[^\.]*$/,'')
        }

        prepare: {
            var cmd = new JavaScriptCommand();
            cmd.description = "prepare " + FileInfo.fileName(output.filePath);
            cmd.highlight = "codegen";
            cmd.pluginspecreplacements = product.pluginspecreplacements;
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
                var inf = new TextFile(input.filePath);
                var all = inf.readAll();
                // replace quoted quotes
                all = all.replace(/\\\"/g, '"');
                // replace config vars
                vars['QTCREATOR_VERSION'] = project.qtcreator_version;
                vars['QTCREATOR_COMPAT_VERSION'] = project.qtcreator_compat_version;
                vars['IDE_VERSION_MAJOR'] = project.ide_version_major;
                vars['IDE_VERSION_MINOR'] = project.ide_version_minor;
                vars['IDE_VERSION_RELEASE'] = project.ide_version_release;
                var deplist = ["<dependencyList>"];
                for (i in plugin_depends) {
                    deplist.push("        <dependency name=\"" + plugin_depends[i] + "\" version=\"" + project.qtcreator_version + "\"/>");
                }
                for (i in plugin_recommends) {
                    deplist.push("        <dependency name=\"" + plugin_recommends[i] + "\" version=\"" + project.qtcreator_version + "\" type=\"optional\"/>");
                }
                deplist.push("    </dependencyList>");
                vars['dependencyList'] = deplist.join("\n");
                for (i in vars) {
                    all = all.replace(new RegExp('\\\$\\\$' + i + '(?!\w)', 'g'), vars[i]);
                }
                var file = new TextFile(output.filePath, TextFile.WriteOnly);
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
            filePath: {
                var destdir = FileInfo.joinPaths(product.moduleProperty("Qt.core", "generatedFilesDir"),
                                                 input.fileName);
                return destdir.replace(/\.[^\.]*$/, '.json');
            }
        }

        prepare: {
            var xslFile = project.path + "/../qtcreatorplugin2json.xsl"; // project is "Plugins"
            var xmlPatternsPath = product.moduleProperty("Qt.core", "binPath") + "/xmlpatterns";
            var args = [
                "-no-format",
                "-output",
                output.filePath,
                xslFile,
                input.filePath
            ];
            var cmd = new Command(xmlPatternsPath, args);
            cmd.description = "generating " + FileInfo.fileName(output.filePath);
            cmd.highlight = "codegen";
            return cmd;
        }
    }
}

