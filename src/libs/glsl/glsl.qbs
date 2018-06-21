import qbs 1.0
import qbs.Environment
import qbs.File
import qbs.FileInfo

QtcLibrary {
    name: "GLSL"

    property bool autoGenerateParser: Environment.getEnv("QTC_GLSL_AUTOGENERATE")

    cpp.defines: base.concat([
        "GLSL_LIBRARY"
    ])
    cpp.includePaths: base.concat([sourceDirectory])

    Depends { name: "Qt.gui" }

    files: [
        "glsl.h",
        "glslast.cpp",
        "glslast.h",
        "glslastdump.cpp",
        "glslastdump.h",
        "glslastvisitor.cpp",
        "glslastvisitor.h",
        "glslengine.cpp",
        "glslengine.h",
        "glslkeywords.cpp",
        "glsllexer.cpp",
        "glsllexer.h",
        "glslmemorypool.cpp",
        "glslmemorypool.h",
        "glslsemantic.cpp",
        "glslsemantic.h",
        "glslsymbol.cpp",
        "glslsymbol.h",
        "glslsymbols.cpp",
        "glslsymbols.h",
        "glsltype.cpp",
        "glsltype.h",
        "glsltypes.cpp",
        "glsltypes.h",
    ]

    Group {
        name: "generated parser files"
        condition: !autoGenerateParser
        files: [
            "glslparser.cpp",
            "glslparser.h",
            "glslparsertable.cpp",
            "glslparsertable_p.h",
        ]
    }

    Group {
        fileTags: ["qlalrInput"]
        files: [ "glsl.g" ]
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
        Artifact { filePath: "glslparsertable_p.h"; fileTags: ["hpp"] }
        Artifact { filePath: "glslparsertable.cpp"; fileTags: ["cpp"] }
        Artifact { filePath: "glslparser.h"; fileTags: ["hpp"] }
        Artifact { filePath: "glslparser.cpp"; fileTags: ["cpp"]}
        prepare: {
            var inputFile = "./" + input.fileName;
            var qlalr = FileInfo.joinPaths(product.Qt.core.binPath, "qlalr")
            var generateCmd = new Command(qlalr, ["--qt", "--no-debug", inputFile]);
            generateCmd.workingDirectory = product.buildDirectory;
            generateCmd.description = "generating glsl parser";

            var copyCmd = new JavaScriptCommand();
            copyCmd.sourceCode = function() {
                var tags = ["hpp", "cpp"];
                for (var i = 0; i < tags.length; ++i) {
                    var artifacts = outputs[tags[i]];
                    for (var j = 0; j < artifacts.length; ++j) {
                        var artifact = artifacts[j];
                        File.copy(artifact.filePath, FileInfo.joinPaths(product.sourceDirectory,
                                    artifact.fileName));
                    }
                }
            };
            copyCmd.silent = true;
            return [generateCmd, copyCmd];
        }
    }
}
