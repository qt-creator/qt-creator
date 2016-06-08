var File = loadExtension("qbs.File");
var FileInfo = loadExtension("qbs.FileInfo");
var TextFile = loadExtension("qbs.TextFile");

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
        exportBlock += line + '\n';
    }
    return exportBlock;
}

function getDependsItemsFromExportBlock(exportBlock)
{
    var lines = exportBlock.split('\n');
    var dependsItems = ["Depends { name: 'cpp' }"];
    var dependsIndex = -1;
    var currentDependsItem;
    for (var i = 0; i < lines.length; ++i) {
        var line = lines[i];
        if (dependsIndex !== -1) {
            currentDependsItem += line;
            if (line.indexOf('}') === dependsIndex) {
                dependsItems.push(currentDependsItem);
                dependsIndex = -1;
            }
            continue;
        }
        dependsIndex = line.indexOf("Depends {");
        if (dependsIndex === -1)
            continue;
        if (line.contains('}')) {
            if (!dependsItems.contains("cpp"))
                dependsItems.push(line);
            dependsIndex = -1;
        } else {
            currentDependsItem = line;
        }
    }
    return dependsItems;
}

function getDependsItems(product)
{
    var productFilePath = FileInfo.joinPaths(product.sourceDirectory,
                                             FileInfo.fileName(product.sourceDirectory) + ".qbs");
    var productFile = new TextFile(productFilePath, TextFile.ReadOnly);
    try {
        var exportBlock = getExportBlock(productFile);
        return getDependsItemsFromExportBlock(exportBlock);
    } finally {
        productFile.close();
    }
}

function writeModuleFile(product, input, output, dependsItems)
{
    var moduleFile = new TextFile(output.filePath, TextFile.WriteOnly);
    try {
        moduleFile.writeLine("import qbs");
        moduleFile.writeLine("");
        moduleFile.writeLine("Module {")
        for (var i = 0; i < dependsItems.length; ++i) {
            moduleFile.writeLine("    " + dependsItems[i].trim());
            moduleFile.writeLine("");
        }
        var includePath = FileInfo.joinPaths("/",
                                             product.moduleProperty("qtc", "ide_include_path"));
        var modulePath = FileInfo.joinPaths("/",
                product.moduleProperty("qtc", "ide_qbs_modules_path"), product.name);
        var relPathToIncludes = FileInfo.relativePath(modulePath, includePath);
        moduleFile.writeLine("    cpp.includePaths: [path + '/" + relPathToIncludes + "']");
        var libInstallPath = FileInfo.joinPaths("/", input.moduleProperty("qbs", "installPrefix"),
                                                input.moduleProperty("qbs", "installDir"));
        var relPathToLibrary = FileInfo.relativePath(modulePath, libInstallPath);
        var libType = input.fileTags.contains("dynamiclibrary") ? "dynamic" : "static";
        moduleFile.writeLine("    cpp." + libType + "Libraries: [path + '/"
                             + relPathToLibrary + "/" + input.fileName + "']");
        moduleFile.writeLine("}");
    } finally {
        moduleFile.close();
    }
}
