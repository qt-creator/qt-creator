/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#pragma once

#include "pchtaskqueueinterface.h"
#include "taskschedulerinterface.h"

namespace Sqlite {
class TransactionInterface;
}

namespace ClangBackEnd {
class PchCreatorInterface;
class PrecompiledHeaderStorageInterface;
class ProgressCounter;

class PchTaskQueue final : public PchTaskQueueInterface
{
public:
    using Task = std::function<void (PchCreatorInterface&)>;

    PchTaskQueue(TaskSchedulerInterface<Task> &systemPchTaskScheduler,
                 TaskSchedulerInterface<Task> &projectPchTaskScheduler,
                 ProgressCounter &progressCounter,
                 PrecompiledHeaderStorageInterface &precompiledHeaderStorage,
                 Sqlite::TransactionInterface &transactionsInterface)
        : m_systemPchTaskScheduler(systemPchTaskScheduler)
        , m_projectPchTaskScheduler(projectPchTaskScheduler)
        , m_precompiledHeaderStorage(precompiledHeaderStorage)
        , m_transactionsInterface(transactionsInterface)
        , m_progressCounter(progressCounter)
    {}

    void addSystemPchTasks(PchTasks &&pchTasks) override;
    void addProjectPchTasks(PchTasks &&pchTasks) override;
    void removePchTasks(const ProjectPartIds &projectsPartIds) override;

    void processEntries() override;

    const PchTasks &systemPchTasks() const { return m_systemPchTasks; }
    const PchTasks &projectPchTasks() const { return m_projectPchTasks; }

    std::vector<Task> createProjectTasks(PchTasks &&pchTasks) const;
    std::vector<Task> createSystemTasks(PchTasks &&pchTasks) const;

private:
    void addPchTasks(PchTasks &&pchTasks, PchTasks &destination);
    void removePchTasksByProjectPartId(const ProjectPartIds &projectsPartIds, PchTasks &destination);
    void processProjectPchTasks();
    void processSystemPchTasks();

private:
    PchTasks m_systemPchTasks;
    PchTasks m_projectPchTasks;
    TaskSchedulerInterface<Task> &m_systemPchTaskScheduler;
    TaskSchedulerInterface<Task> &m_projectPchTaskScheduler;
    PrecompiledHeaderStorageInterface &m_precompiledHeaderStorage;
    Sqlite::TransactionInterface &m_transactionsInterface;
    ProgressCounter &m_progressCounter;
};

} // namespace ClangBackEnd
