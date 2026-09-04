import qbs
import qbs.File
import qbs.FileInfo
import qbs.TextFile

QtcLibrary {
    name: "CMakeLang"

    cpp.defines: base.concat([
        "CMAKELANG_LIBRARY"
    ])
    cpp.includePaths: base.concat([sourceDirectory])

    files: [
        "cmakeast.cpp",
        "cmakeast.h",
        "cmakeastvisitor.cpp",
        "cmakeastvisitor.h",
        "cmakedocument.cpp",
        "cmakedocument.h",
        "cmakeengine.cpp",
        "cmakeengine.h",
        "cmakelang.h",
        "cmakelexer.cpp",
        "cmakelexer.h",
        "cmakememorypool.h",
    ]

    Group {
        fileTags: ["qlalrInput"]
        files: [ "cmakelang.g" ]
    }

    // Necessary because qlalr generates its outputs in the working directory,
    // and we want the input file to appear as a relative path in the generated files.
    Rule {
        inputs: ["qlalrInput"]
        Artifact { filePath: input.fileName; fileTags: ["qlalrInput.real"] }
        prepare: {
            var cmd = new JavaScriptCommand();
            cmd.sourceCode = function() { File.copy(input.filePath, output.filePath); }
            cmd.silent = true;
            return [cmd];
        }
    }

    Rule {
        inputs: ["qlalrInput.real"]
        Artifact { filePath: "cmakeparsertable_p.h"; fileTags: ["hpp"] }
        Artifact { filePath: "cmakeparsertable.cpp"; fileTags: ["cpp"] }
        Artifact { filePath: "cmakeparser.h"; fileTags: ["hpp"] }
        Artifact { filePath: "cmakeparser.cpp"; fileTags: ["cpp"]}
        prepare: {
            var inputFile = "./" + input.fileName;
            var qlalr = FileInfo.joinPaths(product.Qt.core.libExecPath, "qlalr");
            var generateCmd = new Command(qlalr, ["--qt", "--no-debug", inputFile]);
            generateCmd.workingDirectory = product.buildDirectory;
            generateCmd.description = "generating cmake parser";

            // The copies next to the grammar are refreshed only where they
            // differ, so a build leaves the source tree alone unless the
            // grammar really changed.
            var copyCmd = new JavaScriptCommand();
            copyCmd.sourceCode = function() {
                function contents(filePath) {
                    var file = new TextFile(filePath, TextFile.ReadOnly);
                    try {
                        return file.readAll();
                    } finally {
                        file.close();
                    }
                }
                var tags = ["hpp", "cpp"];
                for (var i = 0; i < tags.length; ++i) {
                    var artifacts = outputs[tags[i]];
                    for (var j = 0; j < artifacts.length; ++j) {
                        var artifact = artifacts[j];
                        var target = FileInfo.joinPaths(product.sourceDirectory,
                                                        artifact.fileName);
                        if (File.exists(target)
                                && contents(target) === contents(artifact.filePath)) {
                            continue;
                        }
                        File.copy(artifact.filePath, target);
                    }
                }
            };
            copyCmd.silent = true;
            return [generateCmd, copyCmd];
        }
    }
}
