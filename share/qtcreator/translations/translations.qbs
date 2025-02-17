import qbs.Utilities

Project {
    Product {
        name: "Translations"
        builtByDefault: false

        property string pythonExecutable: "python"

        Depends { name: "Qt.core" }
        Depends { name: "qtc" }

        Group {
            name: "translations to use"
            files: [
                "qtcreator_cs.ts",
                "qtcreator_da.ts",
                "qtcreator_de.ts",
                "qtcreator_en.ts",
                "qtcreator_fr.ts",
                "qtcreator_hr.ts",
                "qtcreator_ja.ts",
                "qtcreator_pl.ts",
                "qtcreator_ru.ts",
                "qtcreator_sl.ts",
                "qtcreator_uk.ts",
                "qtcreator_zh_CN.ts",
                "qtcreator_zh_TW.ts",
            ]
        }

        Group {
            name: "translations not to use"
            files: [
                "qtcreator_hu.ts",
                "qtcreator_es.ts",
                "qtcreator_it.ts",
            ]
            fileTags: []
        }

        Group {
            name: "extractor scripts"
            files: [
                "extract-customwizards.py",
                "extract-externaltools.py",
                "extract-jsonwizards.py",
                "extract-snippets.py",
            ]
            fileTags: "ts_extractor"
        }

        Group {
            fileTagsFilter: product.type
            qbs.install: true
            qbs.installDir: qtc.ide_data_path + "/translations"
        }

        Rule {
            inputs: "ts_extractor"
            Artifact { filePath: input.baseName + ".cpp"; fileTags: "cpp" }
            prepare: {
                var inputDir = project.ide_source_tree + '/';
                if (input.fileName.includes("wizards"))
                    inputDir += "share/qtcreator/templates/wizards";
                else if (input.fileName.contains("externaltools"))
                    inputDir += "src/share/qtcreator/externaltools";
                else
                    inputDir += "share/qtcreator/snippets";
                var cmd = new Command(product.pythonExecutable,
                                      [input.filePath, inputDir, output.filePath]);
                cmd.description = "creating " + output.fileName;
                cmd.highlight = "filegen";
                return cmd;
            }
        }
    }

    Project {
        condition: Utilities.versionCompare(qbs.version, "2.6") >= 0
        references: "lupdaterunner.qbs"
    }
}
