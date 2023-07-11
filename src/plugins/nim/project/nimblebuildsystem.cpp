// Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "nimblebuildsystem.h"

#include "nimbuildsystem.h"
#include "../nimconstants.h"

#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>

#include <utils/algorithm.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

const char C_NIMBLEPROJECT_TASKS[] = "Nim.NimbleProject.Tasks";

static QList<QByteArray> linesFromProcessOutput(Process *process)
{
    QList<QByteArray> lines = process->readAllRawStandardOutput().split('\n');
    lines = Utils::transform(lines, [](const QByteArray &line){ return line.trimmed(); });
    Utils::erase(lines, [](const QByteArray &line) { return line.isEmpty(); });
    return lines;
}

static std::vector<NimbleTask> parseTasks(const FilePath &nimblePath, const FilePath &workingDirectory)
{
    Process process;
    process.setCommand({nimblePath, {"tasks"}});
    process.setWorkingDirectory(workingDirectory);
    process.start();
    process.waitForFinished();

    std::vector<NimbleTask> result;

    if (process.exitCode() != 0) {
        TaskHub::addTask(ProjectExplorer::BuildSystemTask(Task::Error, process.cleanedStdOut()));
        return result;
    }

    const QList<QByteArray> &lines = linesFromProcessOutput(&process);

    for (const QByteArray &line : lines) {
        QList<QByteArray> tokens = line.trimmed().split(' ');
        QTC_ASSERT(!tokens.empty(), continue);
        QString taskName = QString::fromUtf8(tokens.takeFirst());
        QString taskDesc = QString::fromUtf8(tokens.join(' '));
        result.push_back({std::move(taskName), std::move(taskDesc)});
    }

    return result;
}

static NimbleMetadata parseMetadata(const FilePath &nimblePath, const FilePath &workingDirectory)
{
    Process process;
    process.setCommand({nimblePath, {"dump"}});
    process.setWorkingDirectory(workingDirectory);
    process.start();
    process.waitForFinished();

    NimbleMetadata result = {};

    if (process.exitCode() != 0) {
        TaskHub::addTask(ProjectExplorer::BuildSystemTask(Task::Error, process.cleanedStdOut()));
        return result;
    }
    const QList<QByteArray> &lines = linesFromProcessOutput(&process);

    for (const QByteArray &line : lines) {
        QList<QByteArray> tokens = line.trimmed().split(':');
        QTC_ASSERT(tokens.size() == 2, continue);
        QString name = QString::fromUtf8(tokens.takeFirst()).trimmed();
        QString value = QString::fromUtf8(tokens.takeFirst()).trimmed();
        QTC_ASSERT(value.size() >= 2, continue);
        QTC_ASSERT(value.front() == QChar('"'), continue);
        QTC_ASSERT(value.back() == QChar('"'), continue);
        value.remove(0, 1);
        value.remove(value.size() - 1, 1);

        if (name == "binDir")
            result.binDir = value;
        else if (name == "srcDir")
            result.srcDir = value;
        else if (name == "bin") {
            QStringList bin = value.split(',');
            bin = Utils::transform(bin, [](const QString &x){ return x.trimmed(); });
            Utils::erase(bin, [](const QString &x) { return x.isEmpty(); });
            result.bin = std::move(bin);
        }
    }

    return result;
}

NimbleBuildSystem::NimbleBuildSystem(Target *target)
    : BuildSystem(target), m_projectScanner(target->project())
{
    m_projectScanner.watchProjectFilePath();

    connect(&m_projectScanner, &NimProjectScanner::fileChanged, this, [this](const QString &path) {
        if (path == projectFilePath().toString())
            requestDelayedParse();
    });

    connect(&m_projectScanner, &NimProjectScanner::requestReparse,
            this, &NimbleBuildSystem::requestDelayedParse);

    connect(&m_projectScanner, &NimProjectScanner::finished, this, &NimbleBuildSystem::updateProject);

    connect(&m_projectScanner, &NimProjectScanner::directoryChanged, this, [this] (const QString &directory){
        // Workaround for nimble creating temporary files in project root directory
        // when querying the list of tasks.
        // See https://github.com/nim-lang/nimble/issues/720
        if (directory != projectDirectory().toString())
            requestDelayedParse();
    });

    connect(target->project(), &ProjectExplorer::Project::settingsLoaded,
            this, &NimbleBuildSystem::loadSettings);
    connect(target->project(), &ProjectExplorer::Project::aboutToSaveSettings,
            this, &NimbleBuildSystem::saveSettings);
    requestDelayedParse();
}

void NimbleBuildSystem::triggerParsing()
{
    // Only allow one parsing run at the same time:
    auto guard = guardParsingRun();
    if (!guard.guardsProject())
        return;
    m_guard = std::move(guard);

    m_projectScanner.startScan();
}

void NimbleBuildSystem::updateProject()
{
    const FilePath projectDir = projectDirectory();
    const FilePath nimble = Nim::nimblePathFromKit(kit());

    const NimbleMetadata metadata = parseMetadata(nimble, projectDir);
    const FilePath binDir = projectDir.pathAppended(metadata.binDir);
    const FilePath srcDir = projectDir.pathAppended("src");

    QList<BuildTargetInfo> targets = Utils::transform(metadata.bin, [&](const QString &bin){
        BuildTargetInfo info = {};
        info.displayName = bin;
        info.targetFilePath = binDir.pathAppended(bin);
        info.projectFilePath = projectFilePath();
        info.workingDirectory = binDir;
        info.buildKey = bin;
        return info;
    });

    setApplicationTargets(std::move(targets));

    std::vector<NimbleTask> tasks = parseTasks(nimble, projectDir);
    if (tasks != m_tasks) {
        m_tasks = std::move(tasks);
        emit tasksChanged();
    }

    // Complete scan
    m_guard.markAsSuccess();
    m_guard = {};

    emitBuildSystemUpdated();
}

std::vector<NimbleTask> NimbleBuildSystem::tasks() const
{
    return m_tasks;
}

void NimbleBuildSystem::saveSettings()
{
    // only handles nimble specific settings - NimProjectScanner handles general settings
    QStringList result;
    for (const NimbleTask &task : m_tasks) {
        result.push_back(task.name);
        result.push_back(task.description);
    }

    project()->setNamedSettings(C_NIMBLEPROJECT_TASKS, result);
}

void NimbleBuildSystem::loadSettings()
{
    // only handles nimble specific settings - NimProjectScanner handles general settings
    QStringList list = project()->namedSettings(C_NIMBLEPROJECT_TASKS).toStringList();

    m_tasks.clear();
    if (list.size() % 2 != 0)
        return;

    for (int i = 0; i < list.size(); i += 2)
        m_tasks.push_back({list[i], list[i + 1]});
}

bool NimbleBuildSystem::supportsAction(Node *context, ProjectAction action, const Node *node) const
{
    if (node->asFileNode()) {
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

bool NimbleBuildSystem::addFiles(Node *, const FilePaths &filePaths, FilePaths *)
{
    return m_projectScanner.addFiles(Utils::transform(filePaths, &FilePath::toString));
}

RemovedFilesFromProject NimbleBuildSystem::removeFiles(Node *,
                                                       const FilePaths &filePaths,
                                                       FilePaths *)
{
    return m_projectScanner.removeFiles(Utils::transform(filePaths, &FilePath::toString));
}

bool NimbleBuildSystem::deleteFiles(Node *, const FilePaths &)
{
    return true;
}

bool NimbleBuildSystem::renameFile(Node *, const FilePath &oldFilePath, const FilePath &newFilePath)
{
    return m_projectScanner.renameFile(oldFilePath.toString(), newFilePath.toString());
}

} // Nim
