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

#include <symbolindexing.h>
#include <symbolquery.h>
#include <querysqlitestatementfactory.h>

#include <filepathcaching.h>
#include <projectpartcontainerv2.h>
#include <refactoringdatabaseinitializer.h>

#include <QDir>

namespace {

using Sqlite::Database;
using Sqlite::ReadStatement;
using ClangBackEnd::SymbolIndexer;
using ClangBackEnd::SymbolsCollector;
using ClangBackEnd::SymbolStorage;
using ClangBackEnd::FilePathCaching;
using ClangBackEnd::FilePathId;
using ClangBackEnd::RefactoringDatabaseInitializer;
using ClangBackEnd::V2::ProjectPartContainer;
using ClangBackEnd::V2::ProjectPartContainer;
using ClangRefactoring::SymbolQuery;
using ClangRefactoring::QuerySqliteStatementFactory;
using Utils::PathString;
using SL = ClangRefactoring::SourceLocations;

using StatementFactory = QuerySqliteStatementFactory<Database, ReadStatement>;
using Query = SymbolQuery<StatementFactory>;

MATCHER_P3(IsLocation, filePathId, line, column,
           std::string(negation ? "isn't" : "is")
           + " file path id " + PrintToString(filePathId)
           + " line " + PrintToString(line)
           + " and column " + PrintToString(column)
           )
{
    const ClangRefactoring::SourceLocation &location = arg;

    return location.filePathId == filePathId
        && location.line == line
        && location.column == column;
};

class SymbolIndexing : public testing::Test
{
protected:
    FilePathId filePathId(Utils::SmallString filePath);

protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    RefactoringDatabaseInitializer<Sqlite::Database> initializer{database};
    FilePathCaching filePathCache{database};
    ClangBackEnd::SymbolIndexing indexing{database, filePathCache};
    StatementFactory queryFactory{database};
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

    auto locations = query.locationsAt(filePathId(TESTDATA_DIR "/symbolindexing_main1.cpp"), 6, 5);
    ASSERT_THAT(locations,
                ElementsAre(
                    IsLocation(filePathId(TESTDATA_DIR "/symbolindexing_main1.cpp"), 5, 9),
                    IsLocation(filePathId(TESTDATA_DIR "/symbolindexing_main1.cpp"), 6, 5)));
}

TEST_F(SymbolIndexing, DISABLED_TemplateFunction)
{
    indexing.indexer().updateProjectParts({projectPart1}, {});

    auto locations = query.locationsAt(filePathId(TESTDATA_DIR "/symbolindexing_main1.cpp"), 21, 24);
    ASSERT_THAT(locations,
                ElementsAre(
                    IsLocation(filePathId(TESTDATA_DIR "/symbolindexing_main1.cpp"), 5, 9),
                    IsLocation(filePathId(TESTDATA_DIR "/symbolindexing_main1.cpp"), 6, 5)));
}

ClangBackEnd::FilePathId SymbolIndexing::filePathId(Utils::SmallString filePath)
{
    return filePathCache.filePathId(filePath);
}
}
