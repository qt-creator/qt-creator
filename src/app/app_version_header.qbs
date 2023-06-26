import qbs
import qbs.TextFile

Product {
    name: "app_version_header"
    type: "hpp"

    Group {
        files: ["app_version.h.cmakein"]
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
                // replace Windows line endings
                if (onWindows)
                    content = content.replace(/\r\n/g, "\n");
                // replace the magic qmake incantations
                content = content.replace("${IDE_VERSION_DISPLAY}",
                        product.moduleProperty("qtc", "qtcreator_display_version"));
                content = content.replace("${IDE_VERSION_COMPAT}",
                        product.moduleProperty("qtc", "qtcreator_compat_version"));
                content = content.replace("${PROJECT_VERSION}",
                        product.moduleProperty("qtc", "qtcreator_version"));
                content = content.replace("${PROJECT_VERSION_MAJOR}",
                        product.moduleProperty("qtc", "ide_version_major"));
                content = content.replace("${PROJECT_VERSION_MINOR}",
                        product.moduleProperty("qtc", "ide_version_minor"));
                content = content.replace("${PROJECT_VERSION_PATCH}",
                        product.moduleProperty("qtc", "ide_version_release"));
                content = content.replace("${IDE_COPYRIGHT_YEAR}",
                        product.moduleProperty("qtc", "qtcreator_copyright_year"));
                content = content.replace("${IDE_DISPLAY_NAME}",
                        product.moduleProperty("qtc", "ide_display_name"));
                content = content.replace("${IDE_ID}",
                        product.moduleProperty("qtc", "ide_id"));
                content = content.replace("${IDE_CASED_ID}",
                        product.moduleProperty("qtc", "ide_cased_id"));
                content = content.replace(/\n#cmakedefine IDE_REVISION\n/, "");
                content = content.replace("${IDE_REVISION_STR}", "");
                content = content.replace("${IDE_REVISION_URL}", "");
                content = content.replace("${PROJECT_USER_FILE_EXTENSION}",
                        product.moduleProperty("qtc", "ide_user_file_extension"));
                content = content.replace("${IDE_SETTINGSVARIANT}", "QtProject");
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
        cpp.includePaths: exportingProduct.buildDirectory
    }
}
