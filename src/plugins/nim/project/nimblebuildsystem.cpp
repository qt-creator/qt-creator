/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
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

#include "nimblebuildsystem.h"
#include "nimbleproject.h"
#include "nimproject.h"

#include <projectexplorer/target.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QProcess>
#include <QStandardPaths>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

const char C_NIMBLEPROJECT_TASKS[] = "Nim.NimbleProject.Tasks";

static std::vector<NimbleTask> parseTasks(const QString &nimblePath, const QString &workingDirectory)
{
    QProcess process;
    process.setWorkingDirectory(workingDirectory);
    process.start(QStandardPaths::findExecutable(nimblePath), {"tasks"});
    process.waitForFinished();

    std::vector<NimbleTask> result;

    QList<QByteArray> lines = process.readAllStandardOutput().split('\n');
    lines = Utils::transform(lines, [](const QByteArray &line){ return line.trimmed(); });
    Utils::erase(lines, [](const QByteArray &line) { return line.isEmpty(); });

    for (const QByteArray &line : lines) {
        QList<QByteArray> tokens = line.trimmed().split(' ');
        QTC_ASSERT(!tokens.empty(), continue);
        QString taskName = QString::fromUtf8(tokens.takeFirst());
        QString taskDesc = QString::fromUtf8(tokens.join(' '));
        result.push_back({std::move(taskName), std::move(taskDesc)});
    }

    return result;
}

static NimbleMetadata parseMetadata(const QString &nimblePath, const QString &workingDirectory)
{
    QProcess process;
    process.setWorkingDirectory(workingDirectory);
    process.start(QStandardPaths::findExecutable(nimblePath), {"dump"});
    process.waitForFinished();

    NimbleMetadata result = {};

    QList<QByteArray> lines = process.readAllStandardOutput().split('\n');
    lines = Utils::transform(lines, [](const QByteArray &line){ return line.trimmed(); });
    Utils::erase(lines, [](const QByteArray &line) { return line.isEmpty(); });

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

    m_metadata = parseMetadata(QStandardPaths::findExecutable("nimble"), projectDir.toString());
    const FilePath binDir = projectDir.pathAppended(m_metadata.binDir);
    const FilePath srcDir = projectDir.pathAppended("src");

    QList<BuildTargetInfo> targets = Utils::transform(m_metadata.bin, [&](const QString &bin){
        BuildTargetInfo info = {};
        info.displayName = bin;
        info.targetFilePath = binDir.pathAppended(bin);
        info.projectFilePath = projectFilePath();
        info.workingDirectory = binDir;
        info.buildKey = bin;
        return info;
    });

    setApplicationTargets(std::move(targets));

    std::vector<NimbleTask> tasks = parseTasks(QStandardPaths::findExecutable("nimble"), projectDir.toString());
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

NimbleMetadata NimbleBuildSystem::metadata() const
{
    return m_metadata;
}

void NimbleBuildSystem::saveSettings()
{
    QStringList result;
    for (const NimbleTask &task : m_tasks) {
        result.push_back(task.name);
        result.push_back(task.description);
    }

    project()->setNamedSettings(C_NIMBLEPROJECT_TASKS, result);
}

void NimbleBuildSystem::loadSettings()
{
    QStringList list = project()->namedSettings(C_NIMBLEPROJECT_TASKS).toStringList();

    m_tasks.clear();
    if (list.size() % 2 != 0)
        return;

    std::vector<NimbleTask> result;
    for (int i = 0; i < list.size(); i += 2)
        result.push_back({list[i], list[i + 1]});

    requestParse();
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

bool NimbleBuildSystem::addFiles(Node *, const QStringList &filePaths, QStringList *)
{
    return m_projectScanner.addFiles(filePaths);
}

RemovedFilesFromProject NimbleBuildSystem::removeFiles(Node *,
                                                       const QStringList &filePaths,
                                                       QStringList *)
{
    return m_projectScanner.removeFiles(filePaths);
}

bool NimbleBuildSystem::deleteFiles(Node *, const QStringList &)
{
    return true;
}

bool NimbleBuildSystem::renameFile(Node *, const QString &filePath, const QString &newFilePath)
{
    return m_projectScanner.renameFile(filePath, newFilePath);
}

} // Nim
