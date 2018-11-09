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

#include "mockeditormanager.h"
#include "mocksymbolquery.h"

#include <locatorfilter.h>
#include <sourcelocationentry.h>

namespace {

using ClangBackEnd::SymbolKind;
using ClangBackEnd::SourceLocationKind;
using ClangRefactoring::Symbol;

MATCHER_P2(HasNameAndSymbol, name, symbol,
          std::string(negation ? "hasn't " : "has ")
          + PrintToString(name)
          + " and "
          + PrintToString(symbol))
{
    const Core::LocatorFilterEntry &entry = arg;

    return entry.displayName == QString::fromUtf8(name) && entry.internalData.value<Symbol>() == symbol;
}

class LocatorFilter : public ::testing::Test
{
protected:
    static Core::LocatorFilterEntry createSymbolLocatorFilterEntry()
    {
        ClangRefactoring::Symbol symbol{42, "symbolName"};
        Core::LocatorFilterEntry entry;
        entry.internalData = QVariant::fromValue(symbol);

        return entry;
    }

protected:
    MockEditorManager mockEditorManager;
    NiceMock<MockSymbolQuery> mockSymbolQuery;
    ClangRefactoring::LocatorFilter filter{mockSymbolQuery, mockEditorManager, {SymbolKind::Record}, "", "", ""};
    Core::LocatorFilterEntry entry{createSymbolLocatorFilterEntry()};
    int start = 0;
    int length = 0;
    QString newText;
    Utils::LineColumn lineColumn{4, 3};
    ClangBackEnd::FilePathId filePathId{64};
    ClangRefactoring::SourceLocation sourceLocation{filePathId, lineColumn};
};

TEST_F(LocatorFilter, MatchesForCallSymbols)
{
    QFutureInterface<Core::LocatorFilterEntry> dummy;

    EXPECT_CALL(mockSymbolQuery, symbols(ElementsAre(ClangBackEnd::SymbolKind::Record), Eq("search%Term%")));

    filter.matchesFor(dummy, "search*Term");
}

TEST_F(LocatorFilter, MatchesForReturnsLocatorFilterEntries)
{
    ClangRefactoring::Symbols symbols{{12, "Foo", "class Foo final : public Bar"},
                                      {22, "Poo", "class Poo final : public Bar"}};
    ON_CALL(mockSymbolQuery, symbols(ElementsAre(ClangBackEnd::SymbolKind::Record), Eq("search%Term%")))
            .WillByDefault(Return(symbols));
    QFutureInterface<Core::LocatorFilterEntry> dummy;

    auto entries = filter.matchesFor(dummy, "search*Term");

    ASSERT_THAT(entries,
                ElementsAre(
                    HasNameAndSymbol("Foo", symbols[0]),
                    HasNameAndSymbol("Poo", symbols[1])));
}

TEST_F(LocatorFilter, AcceptDontCallsLocationAndOpensEditorIfItGetInvalidResultFromDatabase)
{
    InSequence s;

    EXPECT_CALL(mockSymbolQuery, locationForSymbolId(42, SourceLocationKind::Definition))
            .WillOnce(Return(Utils::optional<ClangRefactoring::SourceLocation>{}));
    EXPECT_CALL(mockEditorManager, openEditorAt(Eq(filePathId), Eq(lineColumn))).Times(0);

    filter.accept(entry, &newText, &start, &length);
}

TEST_F(LocatorFilter, AcceptCallsLocationAndOpensEditor)
{
    InSequence s;

    EXPECT_CALL(mockSymbolQuery, locationForSymbolId(42, SourceLocationKind::Definition))
            .WillOnce(Return(sourceLocation));
    EXPECT_CALL(mockEditorManager, openEditorAt(Eq(filePathId), Eq(lineColumn)));

    filter.accept(entry, &newText, &start, &length);
}

}
