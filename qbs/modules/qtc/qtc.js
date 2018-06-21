var File = require("qbs.File");
var FileInfo = require("qbs.FileInfo");
var TextFile = require("qbs.TextFile");

function getExportBlock(productFile)
{
    var exportBlock = "";
    var exportIndex = -1;
    while (!productFile.atEof()) {
        var line = productFile.readLine();
        if (exportIndex === -1) {
            exportIndex = line.indexOf("Export {");
            continue;
        }
        if (line.indexOf('}') === exportIndex)
            break;
        exportBlock += "    " + line.trim() + '\n';
    }
    return exportBlock;
}

function insertOrAddToProperty(product, content, propertyName, value)
{
    var valueAsList = '[' + value + ']';
    var propertyNameIndex = content.indexOf(propertyName + ':');
    if (propertyNameIndex !== -1) {
        var endListIndex = content.indexOf(']', propertyNameIndex);
        if (endListIndex === -1)
            throw "Failed to parse Export item of product '" + product.name + "'";
        var contentStart = content.slice(0, endListIndex + 1);
        var contentEnd = content.slice(endListIndex + 1);
        return contentStart + ".concat(" + valueAsList + ')' + contentEnd;
    }
    return content + '\n' + propertyName + ": " + valueAsList;
}

function transformedExportBlock(product, input, output)
{
    var productFilePath = FileInfo.joinPaths(product.sourceDirectory, product.fileName);
    var productFile = new TextFile(productFilePath, TextFile.ReadOnly);
    try {
        var exportBlock = getExportBlock(productFile);
        exportBlock = "    Depends { name: 'cpp' }\n" + exportBlock;
        var modulePath = FileInfo.joinPaths("/",
                product.moduleProperty("qtc", "ide_qbs_modules_path"), product.name);
        var includePath = FileInfo.joinPaths("/",
                                             product.moduleProperty("qtc", "ide_include_path"));
        var relPathToIncludes = FileInfo.relativePath(modulePath, includePath);
        var absPathToIncludes = "path + '/" + relPathToIncludes + "'";
        exportBlock = exportBlock.replace(/product.sourceDirectory/g, absPathToIncludes + " + '/"
                                          + FileInfo.fileName(product.sourceDirectory) + "'");
        var dataPath = FileInfo.joinPaths("/", product.moduleProperty("qtc", "ide_data_path"));
        var relPathToData = FileInfo.relativePath(modulePath, dataPath);
        exportBlock = exportBlock.replace(/qtc.export_data_base/g, "path + '/"
                                          + relPathToData + "'");
        exportBlock = insertOrAddToProperty(product, exportBlock, "cpp.includePaths",
                                            absPathToIncludes);
        if (input.fileTags.contains("dynamiclibrary") || input.fileTags.contains("staticlibrary")) {
            var libInstallPath = FileInfo.joinPaths("/", input.moduleProperty("qbs", "installDir"));
            var relPathToLibrary = FileInfo.relativePath(modulePath, libInstallPath);
            var fullPathToLibrary = "path + '/" + relPathToLibrary + "/" + input.fileName + "'";
            var libType = input.fileTags.contains("dynamiclibrary") ? "dynamic" : "static";
            exportBlock = insertOrAddToProperty(product, exportBlock, "cpp." + libType
                                                + "Libraries", fullPathToLibrary);
        }
        return exportBlock;
    } finally {
        productFile.close();
    }
}

function writeModuleFile(output, content)
{
    var moduleFile = new TextFile(output.filePath, TextFile.WriteOnly);
    try {
        moduleFile.writeLine("import qbs");
        moduleFile.writeLine("");
        moduleFile.writeLine("Module {")
        moduleFile.writeLine(content);
        moduleFile.writeLine("}");
    } finally {
        moduleFile.close();
    }
}
