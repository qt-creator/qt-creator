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

#include "clangfollowsymbol.h"
#include "clangfollowsymboljob.h"
#include "commandlinearguments.h"
#include "cursor.h"
#include "clangstring.h"
#include "sourcerange.h"
#include "token.h"
#include "clangsupportdebugutils.h"

#include <utils/qtcassert.h>

#include <future>

namespace ClangBackEnd {

static SourceRange getOperatorRange(const Tokens &tokens,
                                    std::vector<Token>::const_iterator currentToken)
{
    const SourceLocation start = currentToken->location();
    currentToken += 2;
    while (currentToken != tokens.cend() && !(currentToken->spelling() == "("))
        ++currentToken;

    return SourceRange(start, currentToken->location());
}

static SourceRangeContainer extractMatchingTokenRange(const Cursor &cursor,
                                                      const Utf8String &tokenStr)
{
    Tokens tokens(cursor.sourceRange());
    for (auto it = tokens.cbegin(); it != tokens.cend(); ++it) {
        const Token &currentToken = *it;
        if (!(tokenStr == currentToken.spelling()))
            continue;

        if (cursor.isFunctionLike() || cursor.isConstructorOrDestructor()) {
            if (tokenStr == "operator")
                return getOperatorRange(tokens, it);

            auto nextIt = it + 1;
            if (nextIt == tokens.cend() || !(nextIt->spelling() == "("))
                continue;
        }
        return currentToken.extent();
    }
    return SourceRangeContainer();
}

FollowSymbolResult FollowSymbol::followSymbol(CXTranslationUnit tu,
                                              const Cursor &fullCursor,
                                              uint line,
                                              uint column)
{
    Tokens tokens(fullCursor.sourceRange());

    if (!tokens.size()) {
        const Cursor tuCursor(clang_getTranslationUnitCursor(tu));
        tokens = Tokens(tuCursor.sourceRange());
    }

    if (!tokens.size())
        return SourceRangeContainer();

    std::vector<Cursor> cursors = tokens.annotate();
    const int tokenIndex = tokens.getTokenIndex(tu, line, column);
    QTC_ASSERT(tokenIndex >= 0, return SourceRangeContainer());

    const Utf8String tokenSpelling = tokens[tokenIndex].spelling();
    if (tokenSpelling.isEmpty())
        return SourceRangeContainer();

    Cursor cursor{cursors[tokenIndex]};
    if (cursor.kind() == CXCursor_CXXThisExpr && tokenSpelling != "this") { // QTCREATORBUG-25342
        cursor.semanticParent().visit([&cursor](CXCursor current, CXCursor parent) {
            if (current == cursor && parent.kind == CXCursor_MemberRefExpr
                    && cursor.sourceLocation() == Cursor(parent).sourceLocation()) {
                cursor = parent;
                return CXChildVisit_Break;
            }
            return CXChildVisit_Recurse;
        });
    }

    if (cursor.kind() == CXCursor_InclusionDirective) {
        CXFile file = clang_getIncludedFile(cursors[tokenIndex].cx());
        const ClangString filename(clang_getFileName(file));
        const SourceLocation loc(tu, clang_getLocation(tu, file, 1, 1));
        FollowSymbolResult result;
        result.range = SourceRangeContainer(SourceRange(loc, loc));
        // CLANG-UPGRADE-CHECK: Remove if we don't use empty generated ui_* headers anymore.
        if (Utf8String(filename).contains("ui_"))
            result.isResultOnlyForFallBack = true;
        return result;
    }

    // For definitions we can always find a declaration in current TU
    if (cursor.isDefinition()) {
        if (tokenSpelling == "auto") {
            Type type = cursor.type().pointeeType();
            if (!type.isValid())
                type = cursor.type();
            const Cursor declCursor = type.declaration();
            return extractMatchingTokenRange(declCursor, declCursor.spelling());
        }

        const Cursor declCursor = cursor.canonical();
        FollowSymbolResult result;
        result.range = extractMatchingTokenRange(declCursor, tokenSpelling);
        result.isResultOnlyForFallBack = cursor.isFunctionLike() && declCursor == cursor;
        return result;
    }

    if (!cursor.isDeclaration()) {
        // This is the symbol usage
        // We want to return definition
        cursor = cursor.referenced();
        if (cursor.isNull())
            return SourceRangeContainer();

        FollowSymbolResult result;
        // We can't find definition in this TU or it's a virtual method call
        if (!cursor.isDefinition() || cursor.isVirtualMethod())
            result.isResultOnlyForFallBack = true;

        result.range = extractMatchingTokenRange(cursor, tokenSpelling);
        return result;
    }

    const bool isFunction = cursor.isFunctionLike();
    cursor = cursor.definition();
    // If we are able to find a definition in current TU
    if (!cursor.isNull())
        return extractMatchingTokenRange(cursor, tokenSpelling);

    return FollowSymbolResult({}, isFunction);
}

} // namespace ClangBackEnd
