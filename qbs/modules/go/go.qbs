import qbs.File
import qbs.Probes

Module {
    property stringList architectures: []
    property stringList platforms: []
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
         if (architectures.length == 0)
             console.warn("No architectures given.");
         if (platforms.length == 0)
             console.warn("No platforms given.");
         if (magicPacketMarker === "")
             console.warn("magicPacketMarker not set.")
    }
    FileTagger {
        patterns: [ "*.go", "go.mod", "go.sum" ]
        fileTags: [ "go_src" ]
    }
    Rule {
        multiplex: true
        outputArtifacts: {
            var result = [];
            for (var i = 0; i < product.go.architectures.length; ++i) {
                var arch = product.go.architectures[i];
                for (var j = 0; j < product.go.platforms.length; ++j) {
                    var plat = product.go.platforms[j];
                    var artifact = {
                        filePath: product.targetName + '-' + plat + '-' + arch,
                        fileTags: [ "application", plat, arch ]
                    };
                    result.push(artifact);
                }
            }
            return result;
        }

        inputs: [ "go_src" ]
        outputFileTags: [ "application" ].concat(architectures, platforms)
        prepare: {
            var commands = [];
            var appOutputs = outputs.application || [];
            for (var i = 0; i < appOutputs.length; ++i) {
                var out = appOutputs[i];
                for (var j = 0; j < product.go.architectures.length; ++j) {
                    var arch = product.go.architectures[j];
                    if (!out.fileTags.contains(arch))
                        continue;
                    for (var k = 0; k < product.go.platforms.length; ++k) {
                        var plat = product.go.platforms[k];
                        if (!out.fileTags.contains(plat))
                            continue;

                        var env = ["GOARCH=" + arch, "GOOS=" + plat];
                        var workDir = product.sourceDirectory;
                        var args = ['build', '-ldflags',
                                    '-s -w -X main.MagicPacketMarker=' + product.go.magicPacketMarker,
                                    '-o', out.filePath];
                        var cmd = new Command(product.go.goFilePath, args);
                        cmd.environment = env;
                        cmd.workingDirectory = workDir;
                        cmd.description = "building (with go) " + out.fileName;
                        cmd.highlight = "compiler";
                        commands.push(cmd);
                        if ((product.go.upxFilePath !== undefined
                             && File.exists(product.go.upxFilePath))
                                && (plat === 'linux' || (plat === 'windows' && arch === 'amd64'))) {
                            var upxCmd = new Command(product.go.upxFilePath, ['-9', out.filePath]);
                            upxCmd.environment = env;
                            upxCmd.workingDirectory = workDir;
                            upxCmd.description = "packaging executable " + out.fileName;
                            cmd.highlight = "filegen";
                            commands.push(upxCmd);
                        }
                    }
                }
            }
            return commands;
        }
    }
}
