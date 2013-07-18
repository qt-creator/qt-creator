import qbs
import qbs.TextFile

Product {
    name: "app_version_header"
    type: "hpp"
    files: "app_version.h.in"
    property string ide_version_major: project.ide_version_major
    property string ide_version_minor: project.ide_version_minor
    property string ide_version_release: project.ide_version_release
    property string qtcreator_version: project.qtcreator_version

    Transformer {
        inputs: ["app_version.h.in"]
        Artifact {
            fileName: "app/app_version.h"
            fileTags: "hpp"
        }
        prepare: {
            var cmd = new JavaScriptCommand();
            cmd.description = "generating app_version.h";
            cmd.highlight = "codegen";
            cmd.qtcreator_version = product.qtcreator_version;
            cmd.ide_version_major = product.ide_version_major;
            cmd.ide_version_minor = product.ide_version_minor;
            cmd.ide_version_release = product.ide_version_release;
            cmd.onWindows = (product.moduleProperty("qbs", "targetOS").contains("windows"));
            cmd.sourceCode = function() {
                var file = new TextFile(input.fileName);
                var content = file.readAll();
                // replace quoted quotes
                content = content.replace(/\\\"/g, '"');
                // replace Windows line endings
                if (onWindows)
                    content = content.replace(/\r\n/g, "\n");
                // replace the magic qmake incantations
                content = content.replace(/(\n#define IDE_VERSION) .+\n/, "$1 " + qtcreator_version + "\n");
                content = content.replace(/(\n#define IDE_VERSION_MAJOR) .+\n/, "$1 " + ide_version_major + "\n")
                content = content.replace(/(\n#define IDE_VERSION_MINOR) .+\n/, "$1 " + ide_version_minor + "\n")
                content = content.replace(/(\n#define IDE_VERSION_RELEASE) .+\n/, "$1 " + ide_version_release + "\n")
                file = new TextFile(output.fileName, TextFile.WriteOnly);
                file.truncate();
                file.write(content);
                file.close();
            }
            return cmd;
        }
    }

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: product.buildDirectory
    }
}
