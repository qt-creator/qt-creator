import qbs
import qbs.TextFile

Product {
    name: "app_version_header"
    type: "hpp"

    Group {
        files: ["app_version.h.in"]
        fileTags: ["hpp.in"]
    }

    Depends { name: "qtc" }

    Rule {
        inputs: ["hpp.in"]
        Artifact {
            filePath: "app/app_version.h"
            fileTags: "hpp"
        }
        prepare: {
            var cmd = new JavaScriptCommand();
            cmd.description = "generating app_version.h";
            cmd.highlight = "codegen";
            cmd.onWindows = (product.moduleProperty("qbs", "targetOS").contains("windows"));
            cmd.sourceCode = function() {
                var file = new TextFile(input.filePath);
                var content = file.readAll();
                // replace quoted quotes
                content = content.replace(/\\\"/g, '"');
                // replace Windows line endings
                if (onWindows)
                    content = content.replace(/\r\n/g, "\n");
                // replace the magic qmake incantations
                content = content.replace(/(\n#define IDE_VERSION_DISPLAY_DEF) .+\n/, "$1 "
                        + product.moduleProperty("qtc", "qtcreator_display_version") + "\n");
                content = content.replace(/(\n#define IDE_VERSION) .+\n/, "$1 "
                        + product.moduleProperty("qtc", "qtcreator_version") + "\n");
                content = content.replace(/(\n#define IDE_VERSION_MAJOR) .+\n/, "$1 "
                        + product.moduleProperty("qtc", "ide_version_major") + "\n");
                content = content.replace(/(\n#define IDE_VERSION_MINOR) .+\n/, "$1 "
                        + product.moduleProperty("qtc", "ide_version_minor") + "\n");
                content = content.replace(/(\n#define IDE_VERSION_RELEASE) .+\n/, "$1 "
                        + product.moduleProperty("qtc", "ide_version_release") + "\n");
                file = new TextFile(output.filePath, TextFile.WriteOnly);
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
