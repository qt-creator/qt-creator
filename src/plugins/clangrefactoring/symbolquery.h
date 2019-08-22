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

#include "symbolqueryinterface.h"

#include "sourcelocations.h"

#include <filepathid.h>

#include <cpptools/usages.h>

#include <algorithm>

namespace ClangRefactoring {

template <typename StatementFactory>
class SymbolQuery final : public SymbolQueryInterface
{
    using ReadStatement = typename StatementFactory::ReadStatementType;

public:
    SymbolQuery(StatementFactory &statementFactory)
        : m_statementFactory(statementFactory)
    {}

    SourceLocations locationsAt(ClangBackEnd::FilePathId filePathId,
                                int line,
                                int utf8Column) const override
    {
        ReadStatement &locationsStatement = m_statementFactory.selectLocationsForSymbolLocation;

        const std::size_t reserveSize = 128;

        return locationsStatement.template values<SourceLocation, 3>(reserveSize,
                                                                     filePathId.filePathId,
                                                                     line,
                                                                     utf8Column);
    }

    CppTools::Usages sourceUsagesAt(ClangBackEnd::FilePathId filePathId,
                                    int line,
                                    int utf8Column) const override
    {
        ReadStatement &locationsStatement = m_statementFactory.selectSourceUsagesForSymbolLocation;

        const std::size_t reserveSize = 128;

        return locationsStatement.template values<CppTools::Usage, 3>(reserveSize,
                                                                      filePathId.filePathId,
                                                                      line,
                                                                      utf8Column);
    }

    CppTools::Usages sourceUsagesAtByLocationKind(ClangBackEnd::FilePathId filePathId,
                                                  int line,
                                                  int utf8Column,
                                                  ClangBackEnd::SourceLocationKind kind) const override
    {
        ReadStatement &locationsStatement = m_statementFactory.selectSourceUsagesByLocationKindForSymbolLocation;

        const std::size_t reserveSize = 128;

        return locationsStatement.template values<CppTools::Usage, 3>(reserveSize,
                                                                      filePathId.filePathId,
                                                                      line,
                                                                      utf8Column,
                                                                      int(kind));
    }

    CppTools::Usages declarationsAt(ClangBackEnd::FilePathId filePathId,
                                    int line,
                                    int utf8Column) const override
    {
        ReadStatement &locationsStatement = m_statementFactory.selectSourceUsagesOrderedForSymbolLocation;

        const std::size_t reserveSize = 128;

        return locationsStatement.template values<CppTools::Usage, 3>(reserveSize,
                                                                      filePathId.filePathId,
                                                                      line,
                                                                      utf8Column);
    }

    Symbols symbolsWithOneSymbolKinds(ClangBackEnd::SymbolKind symbolKind,
                                      Utils::SmallStringView searchTerm) const
    {
        ReadStatement &statement = m_statementFactory.selectSymbolsForKindAndStartsWith;

        return statement.template values<Symbol, 3>(100, int(symbolKind), searchTerm);
    }

    Symbols symbolsWithTwoSymbolKinds(ClangBackEnd::SymbolKind symbolKind1,
                                      ClangBackEnd::SymbolKind symbolKind2,
                                      Utils::SmallStringView searchTerm) const
    {
        ReadStatement &statement = m_statementFactory.selectSymbolsForKindAndStartsWith2;

        return statement.template values<Symbol, 3>(100, int(symbolKind1), int(symbolKind2), searchTerm);
    }

    Symbols symbolsWithThreeSymbolKinds(ClangBackEnd::SymbolKind symbolKind1,
                                        ClangBackEnd::SymbolKind symbolKind2,
                                        ClangBackEnd::SymbolKind symbolKind3,
                                        Utils::SmallStringView searchTerm) const
    {
        ReadStatement &statement = m_statementFactory.selectSymbolsForKindAndStartsWith3;

        return statement.template values<Symbol, 3>(100, int(symbolKind1), int(symbolKind2), int(symbolKind3), searchTerm);
    }

    Symbols symbols(const ClangBackEnd::SymbolKinds &symbolKinds,
                    Utils::SmallStringView searchTerm) const override
    {
        switch (symbolKinds.size())
        {
        case 1: return symbolsWithOneSymbolKinds(symbolKinds[0], searchTerm);
        case 2: return symbolsWithTwoSymbolKinds(symbolKinds[0], symbolKinds[1], searchTerm);
        case 3: return symbolsWithThreeSymbolKinds(symbolKinds[0], symbolKinds[1], symbolKinds[2], searchTerm);
        }

        return Symbols();
    }

    Utils::optional<SourceLocation> locationForSymbolId(SymbolId symbolId,
                                                        ClangBackEnd::SourceLocationKind kind) const override
    {
        ReadStatement &statement = m_statementFactory.selectLocationOfSymbol;

        return statement.template value<SourceLocation, 3>(symbolId, int(kind));
    }
private:
    StatementFactory &m_statementFactory;
};

} // namespace ClangRefactoring
