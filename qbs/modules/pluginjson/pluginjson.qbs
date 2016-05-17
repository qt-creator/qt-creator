import qbs 1.0
import qbs.TextFile
import qbs.FileInfo

Module {
    Depends { id: qtcore; name: "Qt.core" }
    Depends { name: "qtc" }

    additionalProductTypes: ["qt_plugin_metadata"]

    Rule {
        inputs: ["pluginJsonIn"]

        Artifact {
            fileTags: ["qt_plugin_metadata"]
            filePath: {
                var destdir = FileInfo.joinPaths(product.moduleProperty("Qt.core",
                                                         "generatedFilesDir"), input.fileName);
                return destdir.replace(/\.[^\.]*$/,'')
            }
        }

        prepare: {
            var cmd = new JavaScriptCommand();
            cmd.description = "prepare " + FileInfo.fileName(output.filePath);
            cmd.highlight = "codegen";
            cmd.pluginJsonReplacements = product.pluginJsonReplacements;
            cmd.plugin_depends = [];
            var deps = product.dependencies;
            for (var d in deps) {
                var depdeps = deps[d].dependencies;
                for (var dd in depdeps) {
                    if (depdeps[dd].name == 'pluginjson') {
                        cmd.plugin_depends.push(deps[d].name);
                        break;
                    }
                }
            }
            cmd.plugin_recommends = product.pluginRecommends
            cmd.plugin_test_depends = product.pluginTestDepends

            cmd.sourceCode = function() {
                var i;
                var vars = pluginJsonReplacements || {};
                var inf = new TextFile(input.filePath);
                var all = inf.readAll();
                // replace quoted quotes
                all = all.replace(/\\\"/g, '"');
                // replace config vars
                var qtcVersion = product.moduleProperty("qtc", "qtcreator_version");
                vars['QTCREATOR_VERSION'] = qtcVersion;
                vars['QTCREATOR_COMPAT_VERSION']
                        = product.moduleProperty("qtc", "qtcreator_compat_version");
                vars['IDE_VERSION_MAJOR'] = product.moduleProperty("qtc", "ide_version_major");
                vars['IDE_VERSION_MINOR'] = product.moduleProperty("qtc", "ide_version_minor");
                vars['IDE_VERSION_RELEASE'] = product.moduleProperty("qtc", "ide_version_release");
                var deplist = [];
                for (i in plugin_depends) {
                    deplist.push("        { \"Name\" : \"" + plugin_depends[i] + "\", \"Version\" : \"" + qtcVersion + "\" }");
                }
                for (i in plugin_recommends) {
                    deplist.push("        { \"Name\" : \"" + plugin_recommends[i] + "\", \"Version\" : \"" + qtcVersion + "\", \"Type\" : \"optional\" }");
                }
                for (i in plugin_test_depends) {
                    deplist.push("        { \"Name\" : \"" + plugin_test_depends[i] + "\", \"Version\" : \"" + qtcVersion + "\", \"Type\" : \"test\" }");
                }
                deplist = deplist.join(",\n")
                vars['dependencyList'] = "\"Dependencies\" : [\n" + deplist + "\n    ]";
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
}

