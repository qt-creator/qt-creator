/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "pythonproject.h"

#include "pythonconstants.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QTimer>

#include <coreplugin/documentmanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <utils/fileutils.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace Python {
namespace Internal {

class PythonBuildSystem : public BuildSystem
{
public:
    explicit PythonBuildSystem(Target *target);

    bool supportsAction(Node *context, ProjectAction action, const Node *node) const override;
    bool addFiles(Node *, const QStringList &filePaths, QStringList *) override;
    RemovedFilesFromProject removeFiles(Node *, const QStringList &filePaths, QStringList *) override;
    bool deleteFiles(Node *, const QStringList &) override;
    bool renameFile(Node *, const QString &filePath, const QString &newFilePath) override;

    bool saveRawFileList(const QStringList &rawFileList);
    bool saveRawList(const QStringList &rawList, const QString &fileName);
    void parse();
    QStringList processEntries(const QStringList &paths,
                               QHash<QString, QString> *map = nullptr) const;

    bool writePyProjectFile(const QString &fileName, QString &content,
                            const QStringList &rawList, QString *errorMessage);

    void triggerParsing() final;

private:
    QStringList m_rawFileList;
    QStringList m_files;
    QHash<QString, QString> m_rawListEntries;
};


/**
 * @brief Provides displayName relative to project node
 */
class PythonFileNode : public FileNode
{
public:
    PythonFileNode(const FilePath &filePath, const QString &nodeDisplayName,
                   FileType fileType = FileType::Source)
        : FileNode(filePath, fileType)
        , m_displayName(nodeDisplayName)
    {}

    QString displayName() const override { return m_displayName; }
private:
    QString m_displayName;
};

static QStringList readLines(const FilePath &projectFile)
{
    const QString projectFileName = projectFile.fileName();
    QSet<QString> visited = { projectFileName };
    QStringList lines = { projectFileName };

    QFile file(projectFile.toString());
    if (file.open(QFile::ReadOnly)) {
        QTextStream stream(&file);

        while (true) {
            const QString line = stream.readLine();
            if (line.isNull())
                break;
            if (visited.contains(line))
                continue;
            lines.append(line);
            visited.insert(line);
        }
    }

    return lines;
}

static QStringList readLinesJson(const FilePath &projectFile, QString *errorMessage)
{
    const QString projectFileName = projectFile.fileName();
    QStringList lines = { projectFileName };

    QFile file(projectFile.toString());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        *errorMessage = PythonProject::tr("Unable to open \"%1\" for reading: %2")
                        .arg(projectFile.toUserOutput(), file.errorString());
        return lines;
    }

    const QByteArray content = file.readAll();

    // This assumes the project file is formed with only one field called
    // 'files' that has a list associated of the files to include in the project.
    if (content.isEmpty()) {
        *errorMessage = PythonProject::tr("Unable to read \"%1\": The file is empty.")
                        .arg(projectFile.toUserOutput());
        return lines;
    }

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(content, &error);
    if (doc.isNull()) {
        const int line = content.left(error.offset).count('\n') + 1;
        *errorMessage = PythonProject::tr("Unable to parse \"%1\":%2: %3")
                        .arg(projectFile.toUserOutput()).arg(line)
                        .arg(error.errorString());
        return lines;
    }

    const QJsonObject obj = doc.object();
    if (obj.contains("files")) {
        const QJsonValue files = obj.value("files");
        const QJsonArray files_array = files.toArray();
        QSet<QString> visited;
        for (const auto &file : files_array)
            visited.insert(file.toString());

        lines.append(Utils::toList(visited));
    }

    return lines;
}

class PythonProjectNode : public ProjectNode
{
public:
    PythonProjectNode(const Utils::FilePath &path)
        : ProjectNode(path)
    {
        setDisplayName(path.toFileInfo().completeBaseName());
        setAddFileFilter("*.py");
    }
};

PythonProject::PythonProject(const FilePath &fileName)
    : Project(Constants::C_PY_MIMETYPE, fileName)
{
    setId(PythonProjectId);
    setProjectLanguages(Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setDisplayName(fileName.toFileInfo().completeBaseName());

    setNeedsBuildConfigurations(false);
    setBuildSystemCreator([](Target *t) { return new PythonBuildSystem(t); });
}

static FileType getFileType(const FilePath &f)
{
    if (f.endsWith(".py"))
        return FileType::Source;
    if (f.endsWith(".pyproject") || f.endsWith(".pyqtc"))
        return FileType::Project;
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

    const QDir baseDir(projectDirectory().toString());
    QList<BuildTargetInfo> appTargets;

    auto newRoot = std::make_unique<PythonProjectNode>(projectDirectory());
    for (const QString &f : qAsConst(m_files)) {
        const QString displayName = baseDir.relativeFilePath(f);
        const FilePath filePath = FilePath::fromString(f);
        const FileType fileType = getFileType(filePath);

        newRoot->addNestedNode(std::make_unique<PythonFileNode>(filePath, displayName, fileType));
        if (fileType == FileType::Source) {
            BuildTargetInfo bti;
            bti.buildKey = f;
            bti.targetFilePath = filePath;
            bti.projectFilePath = projectFilePath();
            appTargets.append(bti);
        }
    }
    setRootProjectNode(std::move(newRoot));

    setApplicationTargets(appTargets);

    guard.markAsSuccess();

    emitBuildSystemUpdated();
}

bool PythonBuildSystem::saveRawFileList(const QStringList &rawFileList)
{
    const bool result = saveRawList(rawFileList, projectFilePath().toString());
//    refresh(PythonProject::Files);
    return result;
}

bool PythonBuildSystem::saveRawList(const QStringList &rawList, const QString &fileName)
{
    FileChangeBlocker changeGuarg(fileName);
    bool result = false;

    // New project file
    if (fileName.endsWith(".pyproject")) {
        FileSaver saver(fileName, QIODevice::ReadOnly | QIODevice::Text);
        if (!saver.hasError()) {
            QString content = QTextStream(saver.file()).readAll();
            if (saver.finalize(ICore::dialogParent())) {
                QString errorMessage;
                result = writePyProjectFile(fileName, content, rawList, &errorMessage);
                if (!errorMessage.isEmpty())
                    MessageManager::write(errorMessage);
            }
        }
    } else { // Old project file
        FileSaver saver(fileName, QIODevice::WriteOnly | QIODevice::Text);
        if (!saver.hasError()) {
            QTextStream stream(saver.file());
            for (const QString &filePath : rawList)
                stream << filePath << '\n';
            saver.setResult(&stream);
            result = saver.finalize(ICore::dialogParent());
        }
    }

    return result;
}

bool PythonBuildSystem::writePyProjectFile(const QString &fileName, QString &content,
                                       const QStringList &rawList, QString *errorMessage)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        *errorMessage = PythonProject::tr("Unable to open \"%1\" for reading: %2")
                        .arg(fileName, file.errorString());
        return false;
    }

    // Build list of files with the current rawList for the JSON file
    QString files("[");
    for (const QString &f : rawList)
        if (!f.endsWith(".pyproject"))
            files += QString("\"%1\",").arg(f);
    files = files.left(files.lastIndexOf(',')); // Removing leading comma
    files += ']';

    // Removing everything inside square parenthesis
    // to replace it with the new list of files for the JSON file.
    QRegularExpression pattern(R"(\[.*\])");
    content.replace(pattern, files);
    file.write(content.toUtf8());

    return true;
}

bool PythonBuildSystem::addFiles(Node *, const QStringList &filePaths, QStringList *)
{
    QStringList newList = m_rawFileList;

    const QDir baseDir(projectDirectory().toString());
    for (const QString &filePath : filePaths)
        newList.append(baseDir.relativeFilePath(filePath));

    return saveRawFileList(newList);
}

RemovedFilesFromProject PythonBuildSystem::removeFiles(Node *, const QStringList &filePaths, QStringList *)
{
    QStringList newList = m_rawFileList;

    for (const QString &filePath : filePaths) {
        const QHash<QString, QString>::iterator i = m_rawListEntries.find(filePath);
        if (i != m_rawListEntries.end())
            newList.removeOne(i.value());
    }

    bool res = saveRawFileList(newList);

    return res ? RemovedFilesFromProject::Ok : RemovedFilesFromProject::Error;
}

bool PythonBuildSystem::deleteFiles(Node *, const QStringList &)
{
    return true;
}

bool PythonBuildSystem::renameFile(Node *, const QString &filePath, const QString &newFilePath)
{
    QStringList newList = m_rawFileList;

    const QHash<QString, QString>::iterator i = m_rawListEntries.find(filePath);
    if (i != m_rawListEntries.end()) {
        const int index = newList.indexOf(i.value());
        if (index != -1) {
            const QDir baseDir(projectDirectory().toString());
            newList.replace(index, baseDir.relativeFilePath(newFilePath));
        }
    }

    return saveRawFileList(newList);
}

void PythonBuildSystem::parse()
{
    m_rawListEntries.clear();
    const FilePath filePath = projectFilePath();
    // The PySide project file is JSON based
    if (filePath.endsWith(".pyproject")) {
        QString errorMessage;
        m_rawFileList = readLinesJson(filePath, &errorMessage);
        if (!errorMessage.isEmpty())
            MessageManager::write(errorMessage);
    }
    // To keep compatibility with PyQt we keep the compatibility with plain
    // text files as project files.
    else if (filePath.endsWith(".pyqtc"))
        m_rawFileList = readLines(filePath);

    m_files = processEntries(m_rawFileList, &m_rawListEntries);
}

/**
 * Expands environment variables in the given \a string when they are written
 * like $$(VARIABLE).
 */
static void expandEnvironmentVariables(const QProcessEnvironment &env, QString &string)
{
    const QRegularExpression candidate("\\$\\$\\((.+)\\)");

    QRegularExpressionMatch match;
    int index = string.indexOf(candidate, 0, &match);
    while (index != -1) {
        const QString value = env.value(match.captured(1));

        string.replace(index, match.capturedLength(), value);
        index += value.length();

        index = string.indexOf(candidate, index, &match);
    }
}

/**
 * Expands environment variables and converts the path from relative to the
 * project to an absolute path.
 *
 * The \a map variable is an optional argument that will map the returned
 * absolute paths back to their original \a paths.
 */
QStringList PythonBuildSystem::processEntries(const QStringList &paths,
                                          QHash<QString, QString> *map) const
{
    const QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QDir projectDir(projectDirectory().toString());

    QFileInfo fileInfo;
    QStringList absolutePaths;
    for (const QString &path : paths) {
        QString trimmedPath = path.trimmed();
        if (trimmedPath.isEmpty())
            continue;

        expandEnvironmentVariables(env, trimmedPath);

        trimmedPath = FilePath::fromUserInput(trimmedPath).toString();

        fileInfo.setFile(projectDir, trimmedPath);
        if (fileInfo.exists()) {
            const QString absPath = fileInfo.absoluteFilePath();
            absolutePaths.append(absPath);
            if (map)
                map->insert(absPath, trimmedPath);
        }
    }
    absolutePaths.removeDuplicates();
    return absolutePaths;
}

Project::RestoreResult PythonProject::fromMap(const QVariantMap &map, QString *errorMessage)
{
    Project::RestoreResult res = Project::fromMap(map, errorMessage);
    if (res == RestoreResult::Ok) {
        if (!activeTarget())
            addTargetForDefaultKit();
    }

    return res;
}

PythonBuildSystem::PythonBuildSystem(Target *target)
    : BuildSystem(target)
{
    connect(target->project(), &Project::projectFileIsDirty, this, [this]() { triggerParsing(); });
    QTimer::singleShot(0, this, &PythonBuildSystem::triggerParsing);
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

} // namespace Internal
} // namespace Python
