import qbs
import qbs.File
import qbs.FileInfo

Module {
    property path targetDirectory
    additionalProductTypes: "copied_resource"
    Rule {
        inputs: ["copyable_resource"]
        outputFileTags: ["copied_resource"]
        outputArtifacts: {
            var destinationDir = input.moduleProperty("copyable_resource", "targetDirectory");
            if (!destinationDir) {
                // If the destination directory has not been explicitly set, replicate the
                // structure from the source directory in the build directory.
                destinationDir = project.buildDirectory + '/'
                        + FileInfo.relativePath(project.sourceDirectory, input.filePath);
            }
            return [{
                filePath: destinationDir + '/' + input.fileName,
                fileTags: ["copied_resource"]
            }];
        }
        prepare: {
            var cmd = new JavaScriptCommand();
            cmd.description = "Copying " + FileInfo.fileName(input.fileName);
            cmd.highlight = "codegen";
            cmd.sourceCode = function() { File.copy(input.filePath, output.filePath); };
            return cmd;
        }
    }
}
