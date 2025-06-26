// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "pythonbuildsystem.h"

#include "pyprojecttoml.h"
#include "pythonbuildconfiguration.h"
#include "pythonconstants.h"
#include "pythonproject.h"
#include "pythontr.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/textdocument.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <utils/algorithm.h>
#include <utils/mimeutils.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace Python::Internal {

static QJsonObject readObjJson(const FilePath &projectFile, QString *errorMessage)
{
    const Result<QByteArray> fileContentsResult = projectFile.fileContents();
    if (!fileContentsResult) {
        *errorMessage = fileContentsResult.error();
        return {};
    }

    const QByteArray content = *fileContentsResult;

    // This assumes the project file is formed with only one field called
    // 'files' that has a list associated of the files to include in the project.
    if (content.isEmpty()) {
        *errorMessage = Tr::tr("Unable to read \"%1\": The file is empty.")
                            .arg(projectFile.toUserOutput());
        return QJsonObject();
    }

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(content, &error);
    if (doc.isNull()) {
        const int line = content.left(error.offset).count('\n') + 1;
        *errorMessage = Tr::tr("Unable to parse \"%1\":%2: %3")
                            .arg(projectFile.toUserOutput()).arg(line)
                            .arg(error.errorString());
        return QJsonObject();
    }

    return doc.object();
}

static QStringList readLines(const FilePath &projectFile)
{
    QSet<QString> visited;
    QStringList lines;

    const Result<QByteArray> contents = projectFile.fileContents();
    if (contents) {
        QTextStream stream(contents.value());

        while (true) {
            const QString line = stream.readLine();
            if (line.isNull())
                break;
            if (!Utils::insert(visited, line))
                continue;
            lines.append(line);
        }
    }

    return lines;
}

static QStringList readLinesJson(const FilePath &projectFile, QString *errorMessage)
{
    QSet<QString> visited;
    QStringList lines;

    const QJsonObject obj = readObjJson(projectFile, errorMessage);
    const QJsonArray files = obj.value("files").toArray();
    for (const QJsonValue &file : files) {
        const QString fileName = file.toString();
        if (Utils::insert(visited, fileName))
            lines.append(fileName);
    }

    return lines;
}

static QStringList readImportPathsJson(const FilePath &projectFile, QString *errorMessage)
{
    QStringList importPaths;

    const QJsonObject obj = readObjJson(projectFile, errorMessage);
    if (obj.contains("qmlImportPaths")) {
        const QJsonValue dirs = obj.value("qmlImportPaths");
        const QJsonArray dirs_array = dirs.toArray();

        QSet<QString> visited;

        for (const auto &dir : dirs_array)
            visited.insert(dir.toString());

        importPaths.append(Utils::toList(visited));
    }

    return importPaths;
}

PythonBuildSystem::PythonBuildSystem(BuildConfiguration *buildConfig)
    : BuildSystem(buildConfig)
{
    connect(project(),
            &Project::projectFileIsDirty,
            this,
            &PythonBuildSystem::requestDelayedParse);
    requestParse();
}

bool PythonBuildSystem::supportsAction(Node *context, ProjectAction action, const Node *node) const
{
    if (node->asFileNode())  {
        return action == ProjectAction::Rename
               || action == ProjectAction::RemoveFile;
    }
    if (node->isFolderNodeType() || node->isProjectNodeType()) {
        return action == ProjectAction::AddNewFile
               || action == ProjectAction::RemoveFile
               || action == ProjectAction::AddExistingFile;
    }
    return BuildSystem::supportsAction(context, action, node);
}

static FileType getFileType(const FilePath &f)
{
    if (f.endsWith(".py"))
        return FileType::Source;
    if (f.endsWith(".qrc"))
        return FileType::Resource;
    if (f.endsWith(".ui"))
        return FileType::Form;
    if (f.endsWith(".qml") || f.endsWith(".js"))
        return FileType::QML;
    return Node::fileTypeForFileName(f);
}

void PythonBuildSystem::triggerParsing()
{
    ParseGuard guard = guardParsingRun();
    parse();

    QList<BuildTargetInfo> appTargets;

    auto newRoot = std::make_unique<PythonProjectNode>(projectDirectory());

    const FilePath python = static_cast<PythonBuildConfiguration *>(buildConfiguration())->python();
    const FilePath projectFile = projectFilePath();
    const QString displayName = projectFile.relativePathFromDir(projectDirectory()).toUserOutput();
    newRoot->addNestedNode(
        std::make_unique<PythonFileNode>(projectFile, displayName, FileType::Project));

    bool hasQmlFiles = false;

    for (const FileEntry &entry : std::as_const(m_files)) {
        const QString displayName = entry.filePath.relativePathFromDir(projectDirectory()).toUserOutput();
        const FileType fileType = getFileType(entry.filePath);

        hasQmlFiles |= fileType == FileType::QML;

        newRoot->addNestedNode(std::make_unique<PythonFileNode>(entry.filePath, displayName, fileType));
        const MimeType mt = mimeTypeForFile(entry.filePath, MimeMatchMode::MatchExtension);
        if (mt.matchesName(Constants::C_PY_MIMETYPE) || mt.matchesName(Constants::C_PY3_MIMETYPE)
            || mt.matchesName(Constants::C_PY_GUI_MIMETYPE)) {
            BuildTargetInfo bti;
            bti.displayName = displayName;
            bti.buildKey = entry.filePath.toUrlishString();
            bti.targetFilePath = entry.filePath;
            bti.projectFilePath = projectFile;
            bti.isQtcRunnable = entry.filePath.fileName() == "main.py";
            bti.additionalData = QVariantMap{{"python", python.toSettings()}};
            appTargets.append(bti);
        }
    }
    project()->setProjectLanguage(ProjectExplorer::Constants::QMLJS_LANGUAGE_ID, hasQmlFiles);

    setRootProjectNode(std::move(newRoot));

    setApplicationTargets(appTargets);

    updateQmlCodeModel();

    guard.markAsSuccess();

    emitBuildSystemUpdated();
}

void PythonBuildSystem::updateQmlCodeModelInfo(QmlCodeModelInfo &projectInfo)
{
    for (const FileEntry &importPath : std::as_const(m_qmlImportPaths)) {
        if (!importPath.filePath.isEmpty())
            projectInfo.qmlImportPaths.append(importPath.filePath);
    }
}

/*!
    \brief Saves the build system configuration in the corresponding project file.
    Currently, three project file formats are supported: pyproject.toml, *.pyproject and the legacy
    *.pyqtc file.
    \returns true if the save was successful, false otherwise.
*/
bool PythonBuildSystem::save()
{
    const FilePath filePath = projectFilePath();
    const QStringList projectFiles = Utils::transform(m_files, &FileEntry::rawEntry);
    const FileChangeBlocker changeGuard(filePath);

    QByteArray newContents;

    if (filePath.fileName() == "pyproject.toml") {
        Core::BaseTextDocument projectFile;
        const BaseTextDocument::ReadResult result = projectFile.read(filePath);
        if (result.code != TextFileFormat::ReadSuccess) {
            MessageManager::writeDisrupting(result.error);
            return false;
        }
        auto newPyProjectToml = updatePyProjectTomlContent(result.content, projectFiles);
        if (!newPyProjectToml) {
            MessageManager::writeDisrupting(newPyProjectToml.error());
            return false;
        }
        newContents = newPyProjectToml.value().toUtf8();
    } else if (filePath.endsWith(".pyproject")) {
        // *.pyproject project file
        Result<QByteArray> contents = filePath.fileContents();
        if (!contents) {
            MessageManager::writeDisrupting(contents.error());
            return false;
        }
        QJsonDocument doc = QJsonDocument::fromJson(*contents);
        QJsonObject project = doc.object();
        project["files"] = QJsonArray::fromStringList(projectFiles);
        doc.setObject(project);
        newContents = doc.toJson();
    } else {
        // Old project file
        newContents = projectFiles.join('\n').toUtf8();
    }

    const Result<qint64> writeResult = filePath.writeFileContents(newContents);
    if (!writeResult) {
        MessageManager::writeDisrupting(writeResult.error());
        return false;
    }
    return true;
}

bool PythonBuildSystem::addFiles(Node *, const FilePaths &filePaths, FilePaths *)
{
    const FilePath projectDir = projectDirectory();
    const auto existingPaths = Utils::transform(m_files, &FileEntry::filePath);

    auto filesComp = [](const FileEntry &left, const FileEntry &right) {
        return left.rawEntry < right.rawEntry;
    };

    const bool projectFilesWereSorted = std::is_sorted(m_files.begin(), m_files.end(), filesComp);

    for (const FilePath &filePath : filePaths) {
        if (!projectDir.isSameDevice(filePath))
            return false;
        if (existingPaths.contains(filePath))
            continue;
        m_files.append(FileEntry{filePath.relativePathFromDir(projectDir).path(), filePath});
    }

    if (projectFilesWereSorted)
        std::sort(m_files.begin(), m_files.end(), filesComp);

    return save();
}

RemovedFilesFromProject PythonBuildSystem::removeFiles(Node *, const FilePaths &filePaths, FilePaths *)
{
    for (const FilePath &filePath : filePaths) {
        Utils::eraseOne(m_files,
                        [filePath](const FileEntry &entry) { return filePath == entry.filePath; });
    }

    return save() ? RemovedFilesFromProject::Ok : RemovedFilesFromProject::Error;
}

bool PythonBuildSystem::deleteFiles(Node *, const FilePaths &)
{
    return true;
}

bool PythonBuildSystem::renameFiles(Node *, const FilePairs &filesToRename, FilePaths *notRenamed)
{
    bool success = true;
    for (const auto &[oldFilePath, newFilePath] : filesToRename) {
        bool found = false;
        for (FileEntry &entry : m_files) {
            if (entry.filePath == oldFilePath) {
                found = true;
                entry.filePath = newFilePath;
                entry.rawEntry = newFilePath.relativeChildPath(projectDirectory()).toUrlishString();
                break;
            }
        }
        if (!found) {
            success = false;
            if (notRenamed)
                *notRenamed << oldFilePath;
        }
    }

    if (!save()) {
        if (notRenamed)
            *notRenamed = firstPaths(filesToRename);
        return false;
    }

    return success;
}

void PythonBuildSystem::parse()
{
    m_files.clear();
    m_qmlImportPaths.clear();

    QStringList files;
    QStringList qmlImportPaths;

    const FilePath filePath = projectFilePath();
    QString errorMessage;
    if (filePath.endsWith(".pyproject")) {
        // The PySide .pyproject file is JSON based
        files = readLinesJson(filePath, &errorMessage);
        if (!errorMessage.isEmpty()) {
            MessageManager::writeFlashing(errorMessage);
            errorMessage.clear();
        }
        qmlImportPaths = readImportPathsJson(filePath, &errorMessage);
        if (!errorMessage.isEmpty())
            MessageManager::writeFlashing(errorMessage);
    } else if (filePath.endsWith(".pyqtc")) {
        // To keep compatibility with PyQt we keep the compatibility with plain
        // text files as project files.
        files = readLines(filePath);
    } else if (filePath.fileName() == "pyproject.toml") {
        auto pyProjectTomlParseResult = parsePyProjectToml(filePath);

        TaskHub::clearTasks(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM);
        for (const PyProjectTomlError &error : std::as_const(pyProjectTomlParseResult.errors)) {
            TaskHub::addTask<BuildSystemTask>(
                Task::TaskType::Error, error.description, filePath, error.line);
        }

        if (!pyProjectTomlParseResult.projectName.isEmpty()) {
            project()->setDisplayName(pyProjectTomlParseResult.projectName);
        }

        files = pyProjectTomlParseResult.projectFiles;
    }

    m_files = processEntries(files);
    m_qmlImportPaths = processEntries(qmlImportPaths);
}

/*!
    \brief Expands environment variables in the given \a string when they are written like
    $$(VARIABLE).
*/
static void expandEnvironmentVariables(const Environment &env, QString &string)
{
    static const QRegularExpression candidate("\\$\\$\\((.+)\\)");

    QRegularExpressionMatch match;
    int index = string.indexOf(candidate, 0, &match);
    while (index != -1) {
        const QString value = env.value(match.captured(1));

        string.replace(index, match.capturedLength(), value);
        index += value.length();

        index = string.indexOf(candidate, index, &match);
    }
}

/*!
    \brief Expands environment variables and converts the path from relative to the project root
    folder to an absolute path for all given raw paths.
    \note Duplicated resolved paths are removed
*/
QList<PythonBuildSystem::FileEntry> PythonBuildSystem::processEntries(
    const QStringList &rawPaths) const
{
    QList<FileEntry> files;
    QList<FilePath> seenResolvedPaths;

    const FilePath projectDir = projectDirectory();
    const Environment env = projectDirectory().deviceEnvironment();

    for (const QString &rawPath : rawPaths) {
        FilePath resolvedPath;
        QString path = rawPath.trimmed();
        if (!path.isEmpty()) {
            expandEnvironmentVariables(env, path);
            resolvedPath = projectDir.resolvePath(path);
        }
        if (seenResolvedPaths.contains(resolvedPath))
            continue;

        seenResolvedPaths << resolvedPath;
        files << FileEntry{rawPath, resolvedPath};
    }

    return files;
}

} // namespace Python::Internal
