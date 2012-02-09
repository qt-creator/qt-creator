import qbs.base 1.0
import qbs.fileinfo 1.0 as FileInfo

Module {
    additionalProductFileTags: ["pluginSpec"]
    property int ide_version_major: project.ide_version_major
    property int ide_version_minor: project.ide_version_minor
    property int ide_version_release: project.ide_version_release
    property string qtcreator_version: ide_version_major + '.' + ide_version_minor + '.' + ide_version_release
    property string destdir: "lib/qtcreator/plugins/Nokia/"

    FileTagger {
        pattern: "*.in"
        fileTags: ["pluginSpecIn"]
    }

    Rule {
        inputs: ["pluginSpecIn"]

        Artifact {
            fileTags: ["pluginSpec"]
            fileName: {
                var destdir = input.modules.pluginspec.destdir;
                if (!destdir.match(/\/$/))
                    destdir += "/";
                return destdir + input.fileName.replace(/\.[^\.]*$/,'');
            }
        }

        prepare: {
            var cmd = new JavaScriptCommand();
            cmd.description = "prepare " + FileInfo.fileName(output.fileName);
            cmd.highlight = "codegen";
            cmd.qtcreator_version = product.module.qtcreator_version;
            cmd.ide_version_major = product.module.ide_version_major;
            cmd.ide_version_minor = product.module.ide_version_minor;
            cmd.ide_version_release = product.module.ide_version_release;
            cmd.sourceCode = function() {
                var i;
                var vars = {};
                var inf = new TextFile(input.fileName);
                var all = inf.readAll();
                // replace quoted quotes
                all = all.replace(/\\\"/g, "\"");
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
}

