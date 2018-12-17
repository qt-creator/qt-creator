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

#include "pchtasksmerger.h"

#include "pchtaskqueueinterface.h"

namespace ClangBackEnd {

void PchTasksMerger::mergeTasks(PchTaskSets &&taskSets,
                                Utils::SmallStringVector &&/*toolChainArguments*/)
{
    PchTasks systemTasks;
    systemTasks.reserve(taskSets.size());
    PchTasks projectTasks;
    projectTasks.reserve(taskSets.size());

    for (PchTaskSet &taskSet : taskSets) {
        projectTasks.push_back(std::move(taskSet.project));
        systemTasks.push_back(std::move(taskSet.system));
    }

    m_pchTaskQueue.addSystemPchTasks(std::move(systemTasks));
    m_pchTaskQueue.addProjectPchTasks(std::move(projectTasks));
    m_pchTaskQueue.processEntries();
}

void PchTasksMerger::removePchTasks(const Utils::SmallStringVector &projectPartIds)
{
    m_pchTaskQueue.removePchTasks(projectPartIds);
}

} // namespace ClangBackEnd
