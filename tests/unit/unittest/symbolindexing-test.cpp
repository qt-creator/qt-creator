/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <projectpartcontainerv2.h>

#include <symbolindexing.h>
#include <symbolquery.h>
#include <querysqlitestatementfactory.h>

#include <QDir>

namespace {

using Sqlite::Database;
using Sqlite::ReadStatement;
using ClangBackEnd::SymbolIndexer;
using ClangBackEnd::SymbolsCollector;
using ClangBackEnd::SymbolStorage;
using ClangBackEnd::FilePathCache;
using ClangBackEnd::FilePathCache;
using ClangBackEnd::V2::ProjectPartContainer;
using ClangBackEnd::V2::ProjectPartContainer;
using ClangRefactoring::SymbolQuery;
using ClangRefactoring::QuerySqliteStatementFactory;
using Utils::PathString;
using SL = ClangRefactoring::SourceLocations;

using StatementFactory = QuerySqliteStatementFactory<Database, ReadStatement>;
using Query = SymbolQuery<StatementFactory>;

MATCHER_P3(IsLocation, sourceId, line, column,
           std::string(negation ? "isn't" : "is")
           + " source id " + PrintToString(sourceId)
           + " line " + PrintToString(line)
           + " and column " + PrintToString(column)
           )
{
    const SL::Location &location = arg;

    return location.sourceId == sourceId
        && location.line == line
        && location.column == column;
};

class SymbolIndexing : public testing::Test
{
protected:
    FilePathCache<std::mutex> filePathCache;
    ClangBackEnd::SymbolIndexing indexing{filePathCache, QDir::tempPath() + "/symbol.db"};
    StatementFactory queryFactory{indexing.database()};
    Query query{queryFactory};
    PathString main1Path = TESTDATA_DIR "/symbolindexing_main1.cpp";
    ProjectPartContainer projectPart1{"project1",
                                      {"cc", "-I", TESTDATA_DIR, "-std=c++1z"},
                                      {},
                                      {main1Path.clone()}};
};

TEST_F(SymbolIndexing, Locations)
{
    indexing.indexer().updateProjectParts({projectPart1}, {});

    auto locations = query.locationsAt(TESTDATA_DIR "/symbolindexing_main1.cpp", 6, 5);
    ASSERT_THAT(locations.locations,
                ElementsAre(
                    IsLocation(0, 5, 9),
                    IsLocation(0, 6, 5)));
}

TEST_F(SymbolIndexing, Sources)
{
    indexing.indexer().updateProjectParts({projectPart1}, {});

    auto locations = query.locationsAt(TESTDATA_DIR "/symbolindexing_main1.cpp", 6, 5);
    ASSERT_THAT(locations.sources, ElementsAre(Pair(0, Eq(TESTDATA_DIR "/symbolindexing_main1.cpp"))));
}

TEST_F(SymbolIndexing, DISABLED_TemplateFunction)
{
    indexing.indexer().updateProjectParts({projectPart1}, {});

    auto locations = query.locationsAt(TESTDATA_DIR "/symbolindexing_main1.cpp", 21, 24);
    ASSERT_THAT(locations.locations,
                ElementsAre(
                    IsLocation(0, 5, 9),
                    IsLocation(0, 6, 5)));
}
}
