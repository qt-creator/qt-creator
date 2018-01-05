import qbs 1.0
import qbs.FileInfo

QtcLibrary {
    name: "GLSL"

    cpp.defines: base.concat([
        "GLSL_LIBRARY"
    ])

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
        fileTags: ["qlalrInput"]
        files: [ "glsl.g" ]
    }

    Rule {
        inputs: ["qlalrInput"]
        Artifact {
            filePath: FileInfo.joinPaths(product.sourceDirectory, "glslparsertable_p.h")
            fileTags: ["hpp"]
        }
        Artifact {
            filePath: FileInfo.joinPaths(product.sourceDirectory, "glslparsertable.cpp")
            fileTags: ["cpp"]
        }
        Artifact {
            filePath: FileInfo.joinPaths(product.sourceDirectory, "glslparser.h")
            fileTags: ["hpp"]
        }
        Artifact {
            filePath: FileInfo.joinPaths(product.sourceDirectory, "glslparser.cpp")
            fileTags: ["cpp"]
        }
        prepare: {
            // Keep the input file path relative, so it's consistent with
            // make-parser.sh and no absolute paths end up in e.g. glslparser.cpp.
            var inputFile = "./" + inputs["qlalrInput"][0].fileName
            var qlalr = FileInfo.joinPaths(product.Qt.core.binPath, "qlalr")
            var generateCmd = new Command(qlalr, ["--qt", "--no-debug", inputFile]);
            generateCmd.workingDirectory = product.sourceDirectory;
            generateCmd.description = "generating glsl parser";
            return generateCmd;
        }
    }
}
