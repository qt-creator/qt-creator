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

#include "mockbuilddependenciesprovider.h"

#include <pchtaskgenerator.h>

namespace {

using ClangBackEnd::BuildDependency;
using ClangBackEnd::BuildDependencies;
using ClangBackEnd::FilePathId;
using ClangBackEnd::PchTask;
using ClangBackEnd::SourceEntries;
using ClangBackEnd::SourceType;

class PchTaskGenerator : public testing::Test
{
protected:
    NiceMock<MockBuildDependenciesProvider> mockBuildDependenciesProvider;
    ClangBackEnd::PchTaskGenerator generator{mockBuildDependenciesProvider};
    ClangBackEnd::V2::ProjectPartContainer projectPart1{"ProjectPart1",
                                                        {"--yi"},
                                                        {{"YI","1"}},
                                                        {"/yi"},
                                                        {{1, 1}},
                                                        {{1, 2}}};
    SourceEntries firstSources{{1, SourceType::UserInclude, 1}, {2, SourceType::UserInclude, 1}, {10, SourceType::UserInclude, 1}};
    BuildDependency buildDependency{firstSources, {}};
};

TEST_F(PchTaskGenerator, Create)
{
    ON_CALL(mockBuildDependenciesProvider, create(_)).WillByDefault(Return(buildDependency));

    auto tasks = generator.create({projectPart1});

    ASSERT_THAT(tasks,
                ElementsAre(
                    AllOf(Field(&PchTask::ids, ElementsAre("ProjectPart1")),
                          Field(&PchTask::buildDependency,
                                Field(&BuildDependency::includes, firstSources)))));
}

}
