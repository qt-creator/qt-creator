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

#include "googletest.h"

#include "mockpchtaskqueue.h"

#include <pchtasksmerger.h>

using ClangBackEnd::PchTask;
using ClangBackEnd::PchTaskSet;

class PchTasksMerger : public testing::Test
{
protected:
    template<class T>
    T clone(T entry)
    {
        return *&entry;
    }

protected:
    NiceMock<MockPchTaskQueue> mockPchTaskQueue;
    ClangBackEnd::PchTasksMerger merger{mockPchTaskQueue};
    PchTask systemTask1{"ProjectPart1",
                        {1, 2},
                        {{"YI", "1", 1}, {"SAN", "3", 3}},
                        {{"LIANG", 0}, {"YI", 1}}};
    PchTask projectTask1{"ProjectPart1",
                         {11, 12},
                         {{"SE", "4", 4}, {"WU", "5", 5}},
                         {{"ER", 2}, {"SAN", 3}}};
    PchTask systemTask2{"ProjectPart2",
                        {1, 2},
                        {{"YI", "1", 1}, {"SAN", "3", 3}},
                        {{"LIANG", 0}, {"YI", 1}}};
    PchTask projectTask2{"ProjectPart2",
                         {11, 12},
                         {{"SE", "4", 4}, {"WU", "5", 5}},
                         {{"ER", 2}, {"SAN", 3}}};
};

TEST_F(PchTasksMerger, AddProjectTasks)
{
    InSequence s;

    EXPECT_CALL(mockPchTaskQueue, addProjectPchTasks(ElementsAre(projectTask1, projectTask2)));
    EXPECT_CALL(mockPchTaskQueue, processEntries());

    merger.mergeTasks(
        {{clone(systemTask1), clone(projectTask1)}, {clone(systemTask1), clone(projectTask2)}});

}

TEST_F(PchTasksMerger, AddSystemTasks)
{
    InSequence s;

    EXPECT_CALL(mockPchTaskQueue, addSystemPchTasks(ElementsAre(systemTask1, systemTask2)));
    EXPECT_CALL(mockPchTaskQueue, processEntries());

    merger.mergeTasks(
        {{clone(systemTask1), clone(projectTask1)}, {clone(systemTask2), clone(projectTask2)}});
}
