import qbs
import qbs.Environment
import qbs.File
import qbs.FileInfo
import qbs.TextFile

QtcLibrary {
    name: "RstLang"

    property bool autoGenerateParser: Environment.getEnv("QTC_RSTLANG_AUTOGENERATE_PARSER")

    cpp.defines: base.concat([
        "RSTLANG_LIBRARY"
    ])
    cpp.includePaths: base.concat([sourceDirectory])

    Depends { name: "Parsing" }

    files: [
        "rstast.cpp",
        "rstast.h",
        "rstastvisitor.cpp",
        "rstastvisitor.h",
        "rstdocument.cpp",
        "rstdocument.h",
        "rstlang.h",
        "rstlexer.cpp",
        "rstlexer.h",
        "rstmarkdown.cpp",
        "rstmarkdown.h",
    ]

    Group {
        name: "generated parser files"
        condition: !autoGenerateParser
        files: [
            "rstparser.cpp",
            "rstparser.h",
            "rstparsertable.cpp",
            "rstparsertable_p.h",
        ]
    }

    Group {
        fileTags: ["qlalrInput"]
        files: [ "rstlang.g" ]
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
        Artifact { filePath: "rstparsertable_p.h"; fileTags: ["hpp"] }
        Artifact { filePath: "rstparsertable.cpp"; fileTags: ["cpp"] }
        Artifact { filePath: "rstparser.h"; fileTags: ["hpp"] }
        Artifact { filePath: "rstparser.cpp"; fileTags: ["cpp"]}
        prepare: {
            var inputFile = "./" + input.fileName;
            var qlalr = FileInfo.joinPaths(product.Qt.core.libExecPath, "qlalr");
            var generateCmd = new Command(qlalr, ["--qt", "--no-debug", inputFile]);
            generateCmd.workingDirectory = product.buildDirectory;
            generateCmd.description = "generating reStructuredText parser";

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

    Export {
        Depends { name: "Parsing" }
    }
}
