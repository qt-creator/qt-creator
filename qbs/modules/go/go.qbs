import qbs.File
import qbs.Process
import qbs.Probes

Module {
    property string platform
    property string architecture
    property string magicPacketMarker: ""

    Probes.BinaryProbe {
        id: goProbe
        names: "go"
    }
    property string goFilePath: goProbe.filePath

    Probes.BinaryProbe {
        id: upxProbe
        names: "upx"
    }
    property string upxFilePath: upxProbe.filePath

    validate: {
        found = goProbe.found
         if (!goProbe.found)
             throw ("The go executable '" + goFilePath + "' does not exist.");
         if (!upxProbe.found)
             console.warn("The upx executable '" + upxFilePath + "' does not exist.");
         if (magicPacketMarker === "")
             console.warn("magicPacketMarker not set.")
         if (!platform)
             throw "go.platform must be set"
         if (!architecture)
             throw "go.architecture must be set"
    }
    FileTagger {
        patterns: [ "*.go", "go.mod", "go.sum" ]
        fileTags: [ "go_src" ]
    }
    Rule {
        multiplex: true
        inputs: "go_src"
        outputFileTags: "application"
        outputArtifacts: {
            var targetName = product.targetName + '-' + product.go.platform + '-'
                    + product.go.architecture;
            if (product.go.platform == "windows")
                targetName = targetName.concat(".exe");
            return [{filePath: targetName, fileTags: "application"}];
        }
        prepare: {
            var commands = [];
            var arch = product.go.architecture;
            var plat = product.go.platform;
            var env = ["GOARCH=" + arch, "GOOS=" + plat, "CGO_ENABLED=0"];
            var args = ['build', '-ldflags',
                        '-s -w -X main.MagicPacketMarker=' + product.go.magicPacketMarker,
                        '-o', output.filePath];
            var cmd = new Command(product.go.goFilePath, args);
            cmd.environment = env;
            cmd.workingDirectory = product.sourceDirectory;
            cmd.description = "building (with go) " + output.fileName;
            cmd.highlight = "compiler";
            cmd.relevantEnvironmentVariables = ["GOARCH", "GOOS", "CGO_ENABLED"];
            commands.push(cmd);
            if ((product.go.upxFilePath !== undefined && File.exists(product.go.upxFilePath))
                    && (plat === 'linux' || (plat === 'windows' && arch === 'amd64'))) {
                var upxCmd = new JavaScriptCommand();
                upxCmd.description = "compressing executable " + output.fileName;
                upxCmd.highlight = "filegen";
                upxCmd.environment = env;
                upxCmd.sourceCode = function() {
                    var upxProc = new Process();
                    upxProc.setWorkingDirectory(product.sourceDirectory);
                    for (var envkey in environment)
                        upxProc.setEnv(envkey, environment[envkey]);

                    if (upxProc.exec(product.go.upxFilePath, ['-9', output.filePath]) == -1
                            || (upxProc.exitCode() != 0 && upxProc.exitCode() != 2)) {
                        var message = "Process '" + product.go.upxFilePath
                                + "' failed with exit code " + upxProc.exitCode() + ".\n";
                        message += "stdout was:\n" + upxProc.readStdOut() + "\n";
                        message += "stderr was:\n" + upxProc.readStdErr() + "\n";
                        throw message;
                    }
                };
                commands.push(upxCmd);
            }
            return commands;
        }
    }
}
