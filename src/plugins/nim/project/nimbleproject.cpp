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

#include "nimbleproject.h"
#include "nimconstants.h"
#include "nimblebuildsystem.h"

#include <coreplugin/icontext.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/qtcassert.h>

using namespace Nim;
using namespace ProjectExplorer;

NimbleProject::NimbleProject(const Utils::FilePath &fileName)
    : ProjectExplorer::Project(Constants::C_NIMBLE_MIMETYPE, fileName)
{
    setId(Constants::C_NIMBLEPROJECT_ID);
    setDisplayName(fileName.toFileInfo().completeBaseName());
    // ensure debugging is enabled (Nim plugin translates nim code to C code)
    setProjectLanguages(Core::Context(ProjectExplorer::Constants::CXX_LANGUAGE_ID));
    setBuildSystemCreator([] (Project *p) { return new NimbleBuildSystem(p); });
}

std::vector<NimbleTask> NimbleProject::tasks() const
{
    return m_tasks;
}

NimbleMetadata NimbleProject::metadata() const
{
    return m_metadata;
}

void NimbleProject::setTasks(std::vector<NimbleTask> tasks)
{
    if (tasks == m_tasks)
        return;
    m_tasks = std::move(tasks);
    emit tasksChanged(m_tasks);
}

void NimbleProject::setMetadata(NimbleMetadata metadata)
{
    if (m_metadata == metadata)
        return;
    m_metadata = std::move(metadata);
    emit metadataChanged(m_metadata);
}

QVariantMap NimbleProject::toMap() const
{
    QVariantMap result = Project::toMap();
    result[Constants::C_NIMPROJECT_EXCLUDEDFILES] = static_cast<NimBuildSystem *>(buildSystem())
                                                        ->excludedFiles();
    result[Constants::C_NIMBLEPROJECT_TASKS] = toStringList(m_tasks);
    return result;
}

Project::RestoreResult NimbleProject::fromMap(const QVariantMap &map, QString *errorMessage)
{
    static_cast<NimBuildSystem *>(buildSystem())
        ->setExcludedFiles(map.value(Constants::C_NIMPROJECT_EXCLUDEDFILES).toStringList());
    Project::RestoreResult result = Project::RestoreResult::Error;
    std::tie(result, m_tasks) = fromStringList(map.value(Constants::C_NIMBLEPROJECT_TASKS).toStringList());
    return result == Project::RestoreResult::Ok ? Project::fromMap(map, errorMessage) : result;
}

QStringList NimbleProject::toStringList(const std::vector<NimbleTask> &tasks)
{
    QStringList result;
    for (const NimbleTask &task : tasks) {
        result.push_back(task.name);
        result.push_back(task.description);
    }
    return result;
}

std::tuple<Project::RestoreResult, std::vector<NimbleTask>> NimbleProject::fromStringList(const QStringList &list)
{
    if (list.size() % 2 != 0)
        return std::make_tuple(Project::RestoreResult::Error, std::vector<NimbleTask>());

    std::vector<NimbleTask> result;
    for (int i = 0; i < list.size(); i += 2)
        result.push_back({list[i], list[i + 1]});
    return std::make_tuple(Project::RestoreResult::Ok, result);
}
