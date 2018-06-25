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
#include "clangsupportdebugutils.h"

#include <utils/qtcassert.h>

#include <future>

namespace ClangBackEnd {

namespace {

struct Tokens
{
    Tokens(const Tokens &) = delete;
    Tokens(const Cursor &cursor) {
        tu = cursor.cxTranslationUnit();
        clang_tokenize(tu, cursor.cxSourceRange(), &data, &tokenCount);
    }
    Tokens(const CXTranslationUnit &tu) {
        const CXSourceRange range
                = clang_getCursorExtent(clang_getTranslationUnitCursor(tu));
        clang_tokenize(tu, range, &data, &tokenCount);
    }
    ~Tokens() {
        clang_disposeTokens(tu, data, tokenCount);
    }

    CXToken *data = nullptr;
    uint tokenCount = 0;
private:
    CXTranslationUnit tu;
};

} // anonymous namespace

static SourceRange getOperatorRange(const CXTranslationUnit tu,
                                    const Tokens &tokens,
                                    uint operatorIndex)
{
    const CXSourceLocation start = clang_getTokenLocation(tu, tokens.data[operatorIndex]);
    operatorIndex += 2;
    while (operatorIndex < tokens.tokenCount
           && !(ClangString(clang_getTokenSpelling(tu, tokens.data[operatorIndex])) == "(")) {
        ++operatorIndex;
    }
    const CXSourceLocation end = clang_getTokenLocation(tu, tokens.data[operatorIndex]);
    return SourceRange(tu, clang_getRange(start, end));
}

static SourceRangeContainer extractMatchingTokenRange(const Cursor &cursor,
                                                      const Utf8String &tokenStr)
{
    Tokens tokens(cursor);
    const CXTranslationUnit tu = cursor.cxTranslationUnit();
    for (uint i = 0; i < tokens.tokenCount; ++i) {
        if (!(tokenStr == ClangString(clang_getTokenSpelling(tu, tokens.data[i]))))
            continue;

        if (cursor.isFunctionLike() || cursor.isConstructorOrDestructor()) {
            if (tokenStr == "operator")
                return getOperatorRange(tu, tokens, i);

            if (i+1 > tokens.tokenCount
                    || !(ClangString(clang_getTokenSpelling(tu, tokens.data[i+1])) == "(")) {
                continue;
            }
        }
        return SourceRange(tu, clang_getTokenExtent(tu, tokens.data[i]));
    }
    return SourceRangeContainer();
}

static int getTokenIndex(CXTranslationUnit tu, const Tokens &tokens, uint line, uint column)
{
    int tokenIndex = -1;
    for (int i = static_cast<int>(tokens.tokenCount - 1); i >= 0; --i) {
        const SourceRange range(tu, clang_getTokenExtent(tu, tokens.data[i]));
        if (range.contains(line, column)) {
            tokenIndex = i;
            break;
        }
    }
    return tokenIndex;
}

FollowSymbolResult FollowSymbol::followSymbol(CXTranslationUnit tu,
                                              const Cursor &fullCursor,
                                              uint line,
                                              uint column)
{
    std::unique_ptr<Tokens> tokens(new Tokens(fullCursor));

    if (!tokens->tokenCount)
        tokens.reset(new Tokens(tu));

    if (!tokens->tokenCount)
        return SourceRangeContainer();

    QVector<CXCursor> cursors(static_cast<int>(tokens->tokenCount));
    clang_annotateTokens(tu, tokens->data, tokens->tokenCount, cursors.data());
    int tokenIndex = getTokenIndex(tu, *tokens, line, column);
    QTC_ASSERT(tokenIndex >= 0, return SourceRangeContainer());

    const Utf8String tokenSpelling = ClangString(
                clang_getTokenSpelling(tu, tokens->data[tokenIndex]));
    if (tokenSpelling.isEmpty())
        return SourceRangeContainer();

    Cursor cursor{cursors[tokenIndex]};

    if (cursor.kind() == CXCursor_InclusionDirective) {
        CXFile file = clang_getIncludedFile(cursors[tokenIndex]);
        const ClangString filename(clang_getFileName(file));
        const SourceLocation loc(tu, filename, 1, 1);
        FollowSymbolResult result;
        result.range = SourceRangeContainer(SourceRange(loc, loc));
        // CLANG-UPGRADE-CHECK: Remove if we don't use empty generated ui_* headers anymore.
        if (Utf8String(filename).contains("ui_"))
            result.isResultOnlyForFallBack = true;
        return result;
    }

    // For definitions we can always find a declaration in current TU
    if (cursor.isDefinition())
        return extractMatchingTokenRange(cursor.canonical(), tokenSpelling);

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

    cursor = cursor.definition();
    // If we are able to find a definition in current TU
    if (!cursor.isNull())
        return extractMatchingTokenRange(cursor, tokenSpelling);

    return SourceRangeContainer();
}

} // namespace ClangBackEnd
