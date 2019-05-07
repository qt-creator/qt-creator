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

#include "googletest.h"

#include <clangindexingsettingsmanager.h>
#include <projectexplorer/project.h>

namespace {
class ClangIndexingSettingsManager : public testing::Test
{
protected:
    ClangPchManager::ClangIndexingSettingsManager manager;
    ProjectExplorer::Project project;
    ProjectExplorer::Project project2;
};

TEST_F(ClangIndexingSettingsManager, FetchSettings)
{
    auto setting = manager.settings(&project);

    ASSERT_THAT(setting, Not(IsNull()));
}

TEST_F(ClangIndexingSettingsManager, SettingsAreTheSameForTheSameProject)
{
    auto setting1 = manager.settings(&project);

    auto setting2 = manager.settings(&project);

    ASSERT_THAT(setting1, Eq(setting2));
}

TEST_F(ClangIndexingSettingsManager, SettingsAreTheDifferentForDifferentProjects)
{
    manager.settings(&project);
    manager.settings(&project2);

    auto setting1 = manager.settings(&project);

    auto setting2 = manager.settings(&project2);

    ASSERT_THAT(setting1, Not(Eq(setting2)));
}

TEST_F(ClangIndexingSettingsManager, RemoveSettings)
{
    manager.settings(&project);

    manager.remove(&project);

    ASSERT_FALSE(manager.hasSettings(&project));
}

TEST_F(ClangIndexingSettingsManager, RemoveNonExistingSettings)
{
    manager.remove(&project);

    ASSERT_FALSE(manager.hasSettings(&project));
}
} // namespace
