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

#include "projectpartqueueinterface.h"
#include "taskschedulerinterface.h"

namespace Sqlite {
class TransactionInterface;
}

namespace ClangBackEnd {

class PrecompiledHeaderStorageInterface;
class PchTaskSchedulerInterface;
class PchCreatorInterface;

class ProjectPartQueue final : public ProjectPartQueueInterface
{
public:
    using Task = std::function<void (PchCreatorInterface&)>;

    ProjectPartQueue(TaskSchedulerInterface<Task> &taskScheduler,
                     PrecompiledHeaderStorageInterface &precompiledHeaderStorage,
                     Sqlite::TransactionInterface &transactionsInterface)
        : m_taskScheduler(taskScheduler),
          m_precompiledHeaderStorage(precompiledHeaderStorage),
          m_transactionsInterface(transactionsInterface)
    {}

    void addProjectParts(V2::ProjectPartContainers &&projectParts);
    void removeProjectParts(const Utils::SmallStringVector &projectsPartIds);

    void processEntries();

    const V2::ProjectPartContainers &projectParts() const;

    std::vector<Task> createPchTasks(V2::ProjectPartContainers &&projectParts) const;

private:
    V2::ProjectPartContainers m_projectParts;
    TaskSchedulerInterface<Task> &m_taskScheduler;
    PrecompiledHeaderStorageInterface &m_precompiledHeaderStorage;
    Sqlite::TransactionInterface &m_transactionsInterface;
};

} // namespace ClangBackEnd
