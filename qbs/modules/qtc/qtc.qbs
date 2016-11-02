import qbs
import qbs.Environment
import qbs.FileInfo
import "qtc.js" as HelperFunctions

Module {
    property string ide_version_major: '4'
    property string ide_version_minor: '2'
    property string ide_version_release: '82'
    property string qtcreator_version: ide_version_major + '.' + ide_version_minor + '.'
                                       + ide_version_release

    property string ide_compat_version_major: '4'
    property string ide_compat_version_minor: '2'
    property string ide_compat_version_release: '82'
    property string qtcreator_compat_version: ide_compat_version_major + '.'
            + ide_compat_version_minor + '.' + ide_compat_version_release

    property string libDirName: "lib"
    property string ide_app_path: qbs.targetOS.contains("macos") ? "" : "bin"
    property string ide_app_target: qbs.targetOS.contains("macos") ? "Qt Creator" : "qtcreator"
    property string ide_library_path: {
        if (qbs.targetOS.contains("macos"))
            return ide_app_target + ".app/Contents/Frameworks"
        else if (qbs.targetOS.contains("windows"))
            return ide_app_path
        else
            return libDirName + "/qtcreator"
    }
    property string ide_plugin_path: {
        if (qbs.targetOS.contains("macos"))
            return ide_app_target + ".app/Contents/PlugIns"
        else if (qbs.targetOS.contains("windows"))
            return libDirName + "/qtcreator/plugins"
        else
            return ide_library_path + "/plugins"
    }
    property string ide_data_path: qbs.targetOS.contains("macos")
            ? ide_app_target + ".app/Contents/Resources"
            : "share/qtcreator"
    property string ide_libexec_path: qbs.targetOS.contains("macos")
            ? ide_data_path : qbs.targetOS.contains("windows")
            ? ide_app_path
            : "libexec/qtcreator"
    property string ide_bin_path: qbs.targetOS.contains("macos")
            ? ide_app_target + ".app/Contents/MacOS"
            : ide_app_path
    property string ide_doc_path: qbs.targetOS.contains("macos")
            ? ide_data_path + "/doc"
            : "share/doc/qtcreator"
    property string ide_include_path: "include"
    property string ide_qbs_resources_path: "qbs-resources"
    property string ide_qbs_modules_path: ide_qbs_resources_path + "/modules"
    property string ide_qbs_imports_path: ide_qbs_resources_path + "/imports"
    property string ide_shared_sources_path: "src/shared"

    property bool make_dev_package: false

    // Will be replaced when creating modules from products
    property string export_data_base: project.ide_source_tree + "/share/qtcreator"

    property bool testsEnabled: Environment.getEnv("TEST") || qbs.buildVariant === "debug"
    property stringList generalDefines: [
        "QT_CREATOR",
        'IDE_LIBRARY_BASENAME="' + libDirName + '"',
        'RELATIVE_PLUGIN_PATH="' + FileInfo.relativePath('/' + ide_bin_path,
                                                         '/' + ide_plugin_path) + '"',
        'RELATIVE_LIBEXEC_PATH="' + FileInfo.relativePath('/' + ide_bin_path,
                                                          '/' + ide_libexec_path) + '"',
        'RELATIVE_DATA_PATH="' + FileInfo.relativePath('/' + ide_bin_path,
                                                       '/' + ide_data_path) + '"',
        'RELATIVE_DOC_PATH="' + FileInfo.relativePath('/' + ide_bin_path, '/' + ide_doc_path) + '"',
        "QT_NO_CAST_TO_ASCII",
        "QT_RESTRICTED_CAST_FROM_ASCII",
        "QT_DISABLE_DEPRECATED_BEFORE=0x050600",
    ].concat(testsEnabled ? ["WITH_TESTS"] : [])

    Rule {
        condition: make_dev_package
        inputs: product.type.filter(function f(t) {
            return t === "dynamiclibrary" || t === "staticlibrary" || t === "qtc.dev-headers";
        })
        explicitlyDependsOn: ["qbs"]
        Artifact {
            filePath: product.name + "-module.qbs"
            fileTags: ["qtc.dev-module"]
        }
        prepare: {
            var cmd = new JavaScriptCommand();
            cmd.description = "Creating " + output.fileName;
            cmd.sourceCode = function() {
                var transformedExportBlock = HelperFunctions.transformedExportBlock(product, input,
                                                                                    output);
                HelperFunctions.writeModuleFile(output, transformedExportBlock);

            };
            return [cmd];
        }
    }
}
