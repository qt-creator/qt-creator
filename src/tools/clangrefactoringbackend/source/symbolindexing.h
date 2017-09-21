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

#pragma once

#include "symbolindexinginterface.h"

#include "storagesqlitestatementfactory.h"
#include "symbolindexer.h"
#include "symbolscollector.h"
#include "symbolstorage.h"

#include <refactoringdatabaseinitializer.h>
#include <filepathcachingfwd.h>

#include <sqlitedatabase.h>
#include <sqlitereadstatement.h>
#include <sqlitewritestatement.h>

namespace ClangBackEnd {

class SymbolIndexing final : public SymbolIndexingInterface
{
public:
    using StatementFactory = ClangBackEnd::StorageSqliteStatementFactory<Sqlite::Database,
                                                                         Sqlite::ReadStatement,
                                                                         Sqlite::WriteStatement>;
    using Storage = ClangBackEnd::SymbolStorage<StatementFactory>;

    SymbolIndexing(Sqlite::Database &database,
                   FilePathCachingInterface &filePathCache)
        : m_filePathCache(filePathCache),
          m_statementFactory(database)
    {
    }

    SymbolIndexer &indexer()
    {
        return m_indexer;
    }

    void updateProjectParts(V2::ProjectPartContainers &&projectParts,
                            V2::FileContainers &&generatedFiles)
    {
        m_indexer.updateProjectParts(std::move(projectParts), std::move(generatedFiles));
    }

private:
    FilePathCachingInterface &m_filePathCache;
    SymbolsCollector m_collector{m_filePathCache};
    StatementFactory m_statementFactory;
    Storage m_symbolStorage{m_statementFactory, m_filePathCache};
    SymbolIndexer m_indexer{m_collector, m_symbolStorage};
};

} // namespace ClangBackEnd
