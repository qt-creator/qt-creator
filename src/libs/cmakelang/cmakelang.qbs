import qbs
import qbs.Environment
import qbs.File
import qbs.FileInfo
import qbs.TextFile

QtcLibrary {
    name: "CMakeLang"

    property bool autoGenerateParser: Environment.getEnv("QTC_CMAKELANG_AUTOGENERATE_PARSER")

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
        "cmakeindentation.cpp",
        "cmakeindentation.h",
        "cmakelang.h",
        "cmakelexer.cpp",
        "cmakelexer.h",
        "cmakememorypool.h",
        "cmakerewriter.cpp",
        "cmakerewriter.h",
        "cmakesignature.cpp",
        "cmakesignature.h",
    ]

    Group {
        name: "generated parser files"
        condition: !autoGenerateParser
        files: [
            "cmakeparser.cpp",
            "cmakeparser.h",
            "cmakeparsertable.cpp",
            "cmakeparsertable_p.h",
        ]
    }

    Group {
        fileTags: ["qlalrInput"]
        files: [ "cmakelang.g" ]
    }

    // Necessary because qlalr generates its outputs in the working directory,
    // and we want the input file to appear as a relative path in the generated files.
    Rule {
        inputs: ["qlalrInput"]
        condition: product.autoGenerateParser
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
        condition: product.autoGenerateParser
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
