import qbs
import qbs.Environment
import "qtc.js" as HelperFunctions

Module {
    property string ide_version_major: '4'
    property string ide_version_minor: '0'
    property string ide_version_release: '82'
    property string qtcreator_version: ide_version_major + '.' + ide_version_minor + '.'
                                       + ide_version_release

    property string ide_compat_version_major: '4'
    property string ide_compat_version_minor: '0'
    property string ide_compat_version_release: '82'
    property string qtcreator_compat_version: ide_compat_version_major + '.'
            + ide_compat_version_minor + '.' + ide_compat_version_release

    property string libDirName: "lib"
    property string ide_app_path: qbs.targetOS.contains("osx") ? "" : "bin"
    property string ide_app_target: qbs.targetOS.contains("osx") ? "Qt Creator" : "qtcreator"
    property string ide_library_path: {
        if (qbs.targetOS.contains("osx"))
            return ide_app_target + ".app/Contents/Frameworks"
        else if (qbs.targetOS.contains("windows"))
            return ide_app_path
        else
            return libDirName + "/qtcreator"
    }
    property string ide_plugin_path: {
        if (qbs.targetOS.contains("osx"))
            return ide_app_target + ".app/Contents/PlugIns"
        else if (qbs.targetOS.contains("windows"))
            return libDirName + "/qtcreator/plugins"
        else
            return ide_library_path + "/plugins"
    }
    property string ide_data_path: qbs.targetOS.contains("osx")
            ? ide_app_target + ".app/Contents/Resources"
            : "share/qtcreator"
    property string ide_libexec_path: qbs.targetOS.contains("osx")
            ? ide_data_path : qbs.targetOS.contains("windows")
            ? ide_app_path
            : "libexec/qtcreator"
    property string ide_bin_path: qbs.targetOS.contains("osx")
            ? ide_app_target + ".app/Contents/MacOS"
            : ide_app_path
    property string ide_doc_path: qbs.targetOS.contains("osx")
            ? ide_data_path + "/doc"
            : "share/doc/qtcreator"
    property string ide_include_path: "include"
    property string ide_qbs_resources_path: "qbs-resources"
    property string ide_qbs_modules_path: ide_qbs_resources_path + "/modules"
    property string ide_qbs_imports_path: ide_qbs_resources_path + "/imports"

    property bool make_dev_package: false

    // Will be replaced when creating modules from products
    property string export_data_base: project.ide_source_tree + "/share/qtcreator"

    property bool testsEnabled: Environment.getEnv("TEST") || qbs.buildVariant === "debug"
    property stringList generalDefines: [
        "QT_CREATOR",
        'IDE_LIBRARY_BASENAME="' + libDirName + '"',
        "QT_NO_CAST_TO_ASCII",
        "QT_RESTRICTED_CAST_FROM_ASCII"
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
