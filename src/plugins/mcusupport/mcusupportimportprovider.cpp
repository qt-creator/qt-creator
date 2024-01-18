// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcusupportimportprovider.h"

#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <languageutils/componentversion.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <qmljs/qmljsbind.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljsvalueowner.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QRegularExpression>

#include <optional>

static const char uriPattern[]{R"(uri\s*:\s*[\'\"](\w+)[\'\"])"};

namespace McuSupport::Internal {
using namespace ProjectExplorer;

// Get the MCU target build folder for a file
static std::optional<FilePath> getTargetBuildFolder(const FilePath &path)
{
    const Project *project = ProjectExplorer::ProjectManager::projectForFile(path);
    if (!project)
        return std::nullopt;
    auto node = project->nodeForFilePath(path);
    if (!node)
        return std::nullopt;

    // Get the cmake target node (CMakeTargetNode is internal)
    // Go up in hierarchy until the first CMake target node
    const ProjectNode *targetNode = nullptr;
    FilePath projectBuildFolder;
    while (node) {
        targetNode = node->asProjectNode();
        if (!targetNode) {
            node = node->parentProjectNode();
            continue;
        }
        projectBuildFolder = FilePath::fromVariant(
            targetNode->data(CMakeProjectManager::Constants::BUILD_FOLDER_ROLE));
        if (!projectBuildFolder.isDir()) {
            node = node->parentProjectNode();
            targetNode = nullptr;
            continue;
        }
        break;
    }

    if (!targetNode)
        return std::nullopt;

    return projectBuildFolder / "CMakeFiles" / (project->displayName() + ".dir");
};

QList<Import> McuSupportImportProvider::imports(ValueOwner *valueOwner,
                                                const Document *context,
                                                Snapshot *snapshot) const
{
    QList<Import> ret;

    const FilePath path = context->fileName();
    const std::optional<FilePath> cmakeFilesPathOpt = getTargetBuildFolder(path);
    if (!cmakeFilesPathOpt)
        return {};

    const FilePath inputFilePath = *cmakeFilesPathOpt / "config/input.json";

    const QJsonDocument doc = QJsonDocument::fromJson(inputFilePath.fileContents().value_or(""));
    if (!doc.isObject())
        return {};
    const QJsonObject mainProjectObj = doc.object();
    for (const QJsonValue &moduleValue : mainProjectObj["modulesDependencies"].toArray()) {
        if (!moduleValue.isObject())
            continue;
        const FilePath modulePath = FilePath::fromUserInput(
            moduleValue.toObject()["qmlProjectFile"].toString());
        const QString fileContent = QString::fromLatin1(modulePath.fileContents().value_or(""));
        QRegularExpressionMatch uriMatch = QRegularExpression(uriPattern).match(fileContent);
        if (!uriMatch.hasMatch())
            continue;
        QString moduleUri = uriMatch.captured(1);
        const FilePath moduleFolder = *cmakeFilesPathOpt / moduleUri;

        Import import;
        import.valid = true;
        import.object = new ObjectValue(valueOwner, moduleUri);
        import.info = ImportInfo::moduleImport(moduleUri, {1, 0}, QString());
        for (const auto &qmlFilePath : moduleFolder.dirEntries(FileFilter({"*.qml"}, QDir::Files))) {
            Document::MutablePtr doc = Document::create(qmlFilePath, Dialect::Qml);
            doc->setSource(QString::fromLatin1(qmlFilePath.fileContents().value_or("")));
            doc->parseQml();
            snapshot->insert(doc, true);
            import.object->setMember(doc->componentName(), doc->bind()->rootObjectValue());
        }
        ret << import;
    }
    return ret;
};

void McuSupportImportProvider::loadBuiltins(ImportsPerDocument *importsPerDocument,
                                            Imports *imports,
                                            const Document *context,
                                            ValueOwner *valueOwner,
                                            Snapshot *snapshot)
{
    Import import;
    import.valid = true;
    import.object = new ObjectValue(valueOwner, "<qul>");
    import.info = ImportInfo::moduleImport("qul", {1, 0}, QString());
    getInterfacesImport(context->fileName(), importsPerDocument, import, valueOwner, snapshot);
    imports->append(import);
}

FilePaths McuSupportImportProvider::prioritizeImportPaths(const Document *context,
                                                          const FilePaths &importPaths)
{
    if (!context)
        return importPaths;
    const std::optional<FilePath> cmakeFilesPathOpt = getTargetBuildFolder(context->fileName());
    if (!cmakeFilesPathOpt)
        return importPaths;
    FilePaths ret;
    // qmltocpp uses an incomplete QtQuick folder present in the build folder
    // to avoid taking precedence over the correct qul_install/include/*/StyleDefault
    // move the import path to be last
    std::copy_if(importPaths.cbegin(),
                 importPaths.cend(),
                 std::back_inserter(ret),
                 [cmakeFilesPathOpt](const FilePath &path) { return path != *cmakeFilesPathOpt; });

    // nothing was removed
    if (ret.size() == importPaths.size())
        return importPaths;
    ret.push_back(*cmakeFilesPathOpt);
    return ret;
};

void McuSupportImportProvider::getInterfacesImport(const FilePath &path,
                                                   ImportsPerDocument *importsPerDocument,
                                                   Import &import,
                                                   ValueOwner *valueOwner,
                                                   Snapshot *snapshot) const
{
    const std::optional<FilePath> cmakeFilesPathOpt = getTargetBuildFolder(path);
    if (!cmakeFilesPathOpt)
        return;

    const FilePath inputFilePath = *cmakeFilesPathOpt / "config/input.json";

    std::optional<FilePath> fileModule = getFileModule(path, inputFilePath);
    FilePath lookupDir = *cmakeFilesPathOpt
                         / (fileModule && !fileModule->isEmpty()
                                ? QRegularExpression(uriPattern)
                                      .match(QString::fromLatin1(
                                          fileModule->fileContents().value_or("")))
                                      .captured(1)
                                : "");

    for (const auto &qmlFilePath : lookupDir.dirEntries(FileFilter({"*.qml"}, QDir::Files))) {
        Document::MutablePtr doc = Document::create(qmlFilePath, Dialect::Qml);
        doc->setSource(QString::fromLatin1(qmlFilePath.fileContents().value_or("")));
        doc->parseQml();
        snapshot->insert(doc, true);
        import.object->setMember(doc->componentName(), doc->bind()->rootObjectValue());
        importsPerDocument->insert(doc.data(), QSharedPointer<Imports>(new Imports(valueOwner)));
    }
}

std::optional<FilePath> McuSupportImportProvider::getFileModule(const FilePath &file,
                                                                const FilePath &inputFile) const
{
    const auto doc = QJsonDocument::fromJson(inputFile.fileContents().value_or(""));

    if (!doc.isObject())
        return {};

    const QJsonObject mainProjectObject = doc.object();

    // Mapping module objects starting with mainProject
    const QJsonArray mainProjectQmlFiles = mainProjectObject["QmlFiles"].toArray();
    if (std::any_of(mainProjectQmlFiles.constBegin(),
                    mainProjectQmlFiles.constEnd(),
                    [&file](const QJsonValue &value) {
                        return FilePath::fromUserInput(value.toString()) == file;
                    }))
        return std::nullopt;

    for (const QJsonValue &module : mainProjectObject["modulesDependencies"].toArray()) {
        const QJsonObject moduleObject = module.toObject();
        const QJsonArray moduleQmlFiles = moduleObject["QmlFiles"].toArray();
        if (std::any_of(moduleQmlFiles.constBegin(),
                        moduleQmlFiles.constEnd(),
                        [&file](const QJsonValue &value) {
                            return FilePath::fromUserInput(value.toString()) == file;
                        }))
            return FilePath::fromUserInput(moduleObject["qmlProjectFile"].toString());
    }
    return std::nullopt;
}
}; // namespace McuSupport::Internal
