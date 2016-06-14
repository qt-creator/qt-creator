import qbs

QtcDevHeaders {
    name: "ProParser"
    condition: true
    type: ["qtc.dev-headers", "qtc.dev-module"]
    productName: name
    property string fileName: "proparser.qbs"

    Group {
        fileTagsFilter: ["qtc.dev-module"]
        qbs.install: true
        qbs.installDir: qtc.ide_qbs_modules_path + '/' + product.name
    }

    // TODO: Remove when qbs 1.6 is out.
    FileTagger {
        patterns: ["*.h"]
        fileTags: ["qtc.dev-headers-input"]
    }

    Rule {
        inputs: ["qtc.dev-headers-input"]
        multiplex: true
        Artifact {
            filePath: "dummy"
            fileTags: ["qtc.dev-headers"]
        }
        prepare: {
            var cmd = new JavaScriptCommand();
            cmd.silent = true;
            cmd.sourceCode = function() { };
            return [cmd];
        }
    }

    Export {
        Depends { name: "cpp" }
        cpp.defines: base.concat([
            "QMAKE_AS_LIBRARY",
            "PROPARSER_THREAD_SAFE",
            "PROEVALUATOR_THREAD_SAFE",
            "PROEVALUATOR_CUMULATIVE",
            "PROEVALUATOR_SETENV",
        ])
        cpp.includePaths: base.concat([product.sourceDirectory + "/.."])
    }
}
