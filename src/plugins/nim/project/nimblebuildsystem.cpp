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

#include <projectexplorer/target.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QProcess>
#include <QStandardPaths>

using namespace Nim;
using namespace ProjectExplorer;
using namespace Utils;

namespace {

const char C_NIMBLEPROJECT_TASKS[] = "Nim.NimbleProject.Tasks";
const char C_NIMBLEPROJECT_METADATA[] = "Nim.NimbleProject.Metadata";

std::vector<NimbleTask> parseTasks(const QString &nimblePath, const QString &workingDirectory)
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

NimbleMetadata parseMetadata(const QString &nimblePath, const QString &workingDirectory) {
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

}

NimbleBuildSystem::NimbleBuildSystem(Target *target)
    : NimBuildSystem(target)
{
    // Not called in parseProject due to nimble behavior to create temporary
    // files in project directory. This creation in turn stimulate the fs watcher
    // that in turn causes project parsing (thus a loop if invoke in parseProject).
    // For this reason we call this function manually during project creation
    // See https://github.com/nim-lang/nimble/issues/720
    m_directoryWatcher.addFile(projectFilePath().toString(), FileSystemWatcher::WatchModifiedDate);

    connect(&m_directoryWatcher, &FileSystemWatcher::fileChanged, this, [this](const QString &path) {
        if (path == projectFilePath().toString()) {
            updateProject();
        }
    });

    updateProject();
}


void NimbleBuildSystem::updateProject()
{
    updateProjectMetaData();
    updateProjectTasks();
}

void NimbleBuildSystem::updateProjectTasks()
{
    setTasks(parseTasks(QStandardPaths::findExecutable("nimble"), projectDirectory().toString()));
}

void NimbleBuildSystem::updateProjectMetaData()
{
    setMetadata(parseMetadata(QStandardPaths::findExecutable("nimble"), projectDirectory().toString()));
}

void NimbleBuildSystem::updateApplicationTargets()
{
    const NimbleMetadata &metaData = metadata();
    const FilePath &projectDir = project()->projectDirectory();
    const FilePath binDir = projectDir.pathAppended(metaData.binDir);
    const FilePath srcDir = projectDir.pathAppended("src");

    QList<BuildTargetInfo> targets = Utils::transform(metaData.bin, [&](const QString &bin){
        BuildTargetInfo info = {};
        info.displayName = bin;
        info.targetFilePath = binDir.pathAppended(HostOsInfo::withExecutableSuffix(bin));
        info.projectFilePath = srcDir.pathAppended(bin).stringAppended(".nim");
        info.workingDirectory = binDir;
        info.buildKey = bin;
        return info;
    });

    setApplicationTargets(std::move(targets));
}

std::vector<NimbleTask> NimbleBuildSystem::tasks() const
{
    return m_tasks;
}

NimbleMetadata NimbleBuildSystem::metadata() const
{
    return m_metadata;
}

void NimbleBuildSystem::setTasks(std::vector<NimbleTask> tasks)
{
    if (tasks == m_tasks)
        return;
    m_tasks = std::move(tasks);
    emit tasksChanged();

    emitBuildSystemUpdated();
}

void NimbleBuildSystem::setMetadata(NimbleMetadata metadata)
{
    if (m_metadata == metadata)
        return;
    m_metadata = std::move(metadata);

    updateApplicationTargets();
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
}
