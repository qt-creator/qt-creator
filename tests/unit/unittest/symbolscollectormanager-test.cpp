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

#include "mocksymbolscollector.h"

#include <sqlitedatabase.h>
#include <refactoringdatabaseinitializer.h>
#include <symbolscollectormanager.h>

namespace {

class SymbolsCollectorManager : public testing::Test
{
protected:
    Sqlite::Database database{":memory:", Sqlite::JournalMode::Memory};
    ClangBackEnd::RefactoringDatabaseInitializer<Sqlite::Database> initializer{database};
    ClangBackEnd::GeneratedFiles generatedFiles;
    ClangBackEnd::SymbolsCollectorManager<NiceMock<MockSymbolsCollector>> manager{database, generatedFiles};
};

TEST_F(SymbolsCollectorManager, CreateUnsedSystemCollector)
{
    manager.unusedSymbolsCollector();

    manager.unusedSymbolsCollector();

    ASSERT_THAT(manager.collectors(), SizeIs(2));
}

TEST_F(SymbolsCollectorManager, ReuseUnsedSystemCollector)
{
    auto &collector = manager.unusedSymbolsCollector();
    collector.setIsUsed(false);

    manager.unusedSymbolsCollector();

    ASSERT_THAT(manager.collectors(), SizeIs(1));
}

TEST_F(SymbolsCollectorManager, AsGetNewUnusedSymbolsCollectorItIsSetUsed)
{
    auto &collector = manager.unusedSymbolsCollector();

    ASSERT_TRUE(collector.isUsed());
}

TEST_F(SymbolsCollectorManager, AsGetReusedUnusedSymbolsCollectorItIsSetUsed)
{
    auto &collector = manager.unusedSymbolsCollector();
    collector.setIsUsed(false);

    auto &collector2 = manager.unusedSymbolsCollector();

    ASSERT_TRUE(collector2.isUsed());
}

TEST_F(SymbolsCollectorManager, UnusedSystemCollectorIsInitializedWithUnsavedFiles)
{
    auto &collector = manager.unusedSymbolsCollector();

    ASSERT_TRUE(collector.hasUnsavedFiles);
}

TEST_F(SymbolsCollectorManager, ReusedSystemCollectorIsInitializedWithUnsavedFiles)
{
    auto &collector = manager.unusedSymbolsCollector();
    collector.setIsUsed(false);

    auto &collector2 = manager.unusedSymbolsCollector();

    ASSERT_TRUE(collector2.hasUnsavedFiles);
}

}
