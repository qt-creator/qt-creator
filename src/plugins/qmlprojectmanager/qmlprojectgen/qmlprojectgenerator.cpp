// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprojectgenerator.h"
#include "../cmakegen/generatecmakelists.h"
#include "../qmlprojectmanagertr.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/icore.h>

#include <QMessageBox>

using namespace Utils;

namespace QmlProjectManager {
namespace GenerateQmlProject {

bool QmlProjectFileGenerator::prepareForUiQmlFile(const FilePath &uiQmlFilePath)
{
    return prepare(selectTargetFile(uiQmlFilePath));
}

bool QmlProjectFileGenerator::prepare(const FilePath &targetDir)
{
    m_targetFile = targetDir.isEmpty() ? selectTargetFile() : targetDir;
    m_targetDir = m_targetFile.parentDir();

    return true;
}

const char QMLPROJECT_FILE_TEMPLATE_PATH[] = ":/projectfiletemplates/qmlproject.tpl";
const char FILE_QML[] = "*.qml";
const char FILE_JS[] = "*.js";
const char FILE_JPG[] = "*.jpg";
const char FILE_PNG[] = "*.png";
const char FILE_GIF[] = "*.gif";
const char FILE_MESH[] = "*.mesh";
const char BLOCKTITLE_QML[] = "QmlFiles";
const char BLOCKTITLE_IMAGE[] = "ImageFiles";
const char BLOCKTITLE_JS[] = "JavaScriptFiles";
const char BLOCK_CONTENTDIR[] = "\n    %1 {\n        directory: \"%2\"\n    }\n";
const char BLOCK_FILTEREDDIR[] = "\n    Files {\n        filter: \"%1\"\n        directory: \"%2\"\n    }\n";
const char BLOCK_DIRARRAY[] = "\n    %1: [ %2 ]\n";

bool QmlProjectFileGenerator::execute()
{
    static const QStringList filesQml = {FILE_QML};
    static const QStringList filesImage = {FILE_PNG, FILE_JPG, FILE_GIF};
    static const QStringList filesJs = {FILE_QML, FILE_JS};
    static const QStringList filesAsset = {FILE_MESH};

    if (m_targetFile.isEmpty() || !m_targetDir.isWritableDir())
        return false;

    const QString contentEntry = createContentDirEntries(BLOCKTITLE_QML, filesQml);
    const QString imageEntry = createContentDirEntries(BLOCKTITLE_IMAGE, filesImage);
    const QString jsEntry = createContentDirEntries(BLOCKTITLE_JS, filesJs);
    const QString assetEntry = createFilteredDirEntries(filesAsset);
    QStringList importDirs = findContentDirs(filesQml + filesAsset);
    importDirs.removeAll(".");
    importDirs.removeAll("content");
    const QString importPaths = createDirArrayEntry("importPaths", importDirs);

    const QString fileContent = GenerateCmake::readTemplate(QMLPROJECT_FILE_TEMPLATE_PATH)
            .arg(contentEntry, imageEntry, jsEntry, assetEntry, importPaths);

    QFile file(m_targetFile.toString());
    file.open(QIODevice::WriteOnly);
    if (!file.isOpen())
        return false;

    file.reset();
    file.write(fileContent.toUtf8());
    file.close();

    QMessageBox::information(Core::ICore::dialogParent(),
                             Tr::tr("Project File Generated"),
                             Tr::tr("File created:\n\n%1").arg(m_targetFile.toString()),
                             QMessageBox::Ok);

    return true;
}

const FilePath QmlProjectFileGenerator::targetDir() const
{
    return m_targetDir;
}

const FilePath QmlProjectFileGenerator::targetFile() const
{
    return m_targetFile;
}

bool QmlProjectFileGenerator::isStandardStructure(const FilePath &projectDir) const
{
    if (projectDir.pathAppended("content").isDir() &&
        projectDir.pathAppended("imports").isDir())
            return true;

    return false;
}

const QString QmlProjectFileGenerator::createContentDirEntries(const QString &containerName,
                                                               const QStringList &suffixes) const
{
    QString entries;

    const QStringList contentDirs = findContentDirs(suffixes);
    for (const QString &dir : contentDirs)
        entries.append(QString(BLOCK_CONTENTDIR).arg(containerName, dir));

    return entries;
}

const QString QmlProjectFileGenerator::createFilteredDirEntries(const QStringList &suffixes) const
{
    QString entries;
    const QString filterList = suffixes.join(';');

    const QStringList contentDirs = findContentDirs(suffixes);
    for (const QString &dir : contentDirs)
        entries.append(QString(BLOCK_FILTEREDDIR).arg(filterList, dir));

    return entries;
}

const QString QmlProjectFileGenerator::createDirArrayEntry(const QString &arrayName,
                                                           const QStringList &relativePaths) const
{
    if (relativePaths.isEmpty())
        return {};

    QStringList formattedDirs = relativePaths;
    for (QString &dir : formattedDirs)
        dir.append('"').prepend('"');

    return QString(BLOCK_DIRARRAY).arg(arrayName, formattedDirs.join(", "));
}

const FilePath QmlProjectFileGenerator::selectTargetFile(const FilePath &uiFilePath)
{
    FilePath suggestedDir;

    if (!uiFilePath.isEmpty())
        if (uiFilePath.parentDir().parentDir().exists())
            suggestedDir = uiFilePath.parentDir().parentDir();

    if (suggestedDir.isEmpty())
        suggestedDir = FilePath::fromString(QDir::homePath());

    FilePath targetFile;
    bool selectionCompleted = false;
    do {
        targetFile = Core::DocumentManager::getSaveFileNameWithExtension(
                                    Tr::tr("Select File Location"),
                                    suggestedDir,
                                    Tr::tr("Qt Design Studio Project Files (*.qmlproject)"));
        selectionCompleted = isDirAcceptable(targetFile.parentDir(), uiFilePath);
    } while (!selectionCompleted);

    return targetFile;
}

bool QmlProjectFileGenerator::isDirAcceptable(const FilePath &dir, const FilePath &uiFile)
{
    const FilePath uiFileParentDir = uiFile.parentDir();

    if (dir.isChildOf(uiFileParentDir)) {
        QMessageBox::warning(Core::ICore::dialogParent(),
                             Tr::tr("Invalid Directory"),
                             Tr::tr("Project file must be placed in a parent directory of the QML files."),
                             QMessageBox::Ok);
        return false;
    }

    if (uiFileParentDir.isChildOf(dir)) {
        const FilePath relativePath = uiFileParentDir.relativeChildPath(dir);
        QStringList components = relativePath.toString().split("/");
        if (components.size() > 2) {
            QMessageBox::StandardButton sel = QMessageBox::question(Core::ICore::dialogParent(),
                                                  Tr::tr("Problem"),
                                                  Tr::tr("Selected directory is far away from the QML file. This can cause unexpected results.\n\nAre you sure?"),
                                                  QMessageBox::Yes | QMessageBox::No);
            if (sel == QMessageBox::No)
                return false;
        }
    }

    return true;
}

const int TOO_FAR_AWAY = 4;
const QDir::Filters FILES_ONLY = QDir::Files;
const QDir::Filters DIRS_ONLY = QDir::Dirs|QDir::Readable|QDir::NoDotAndDotDot;

const FilePath QmlProjectFileGenerator::findInDirTree(const FilePath &dir, const QStringList &suffixes, int currentSearchDepth) const
{
    if (currentSearchDepth > TOO_FAR_AWAY)
        return {};

    currentSearchDepth++;

    const FilePaths files = dir.dirEntries({suffixes, FILES_ONLY});
    if (!files.isEmpty())
        return dir;

    FilePaths subdirs = dir.dirEntries(DIRS_ONLY);
    for (const FilePath &subdir : subdirs) {
        const FilePath result = findInDirTree(subdir, suffixes, currentSearchDepth);
        if (!result.isEmpty())
            return result;
    }

    return {};
}

const QStringList QmlProjectFileGenerator::findContentDirs(const QStringList &suffixes) const
{
    FilePaths results;

    if (!isStandardStructure(m_targetDir))
        if (!m_targetDir.dirEntries({suffixes, FILES_ONLY}).isEmpty())
            return {"."};

    const FilePaths dirs = m_targetDir.dirEntries(DIRS_ONLY);
    for (const FilePath &dir : dirs) {
        const FilePath result = findInDirTree(dir, suffixes);
        if (!result.isEmpty())
            results.append(result);
    }

    QStringList relativePaths;
    for (const FilePath &fullPath : results) {
        if (fullPath == m_targetDir)
            relativePaths.append(".");
        else
            relativePaths.append(fullPath.relativeChildPath(m_targetDir).toString().split('/').first());
    }

    return relativePaths;
}

} // GenerateQmlProject
} // QmlProjectManager
