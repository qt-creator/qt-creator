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

class FollowSymbolData {
public:
    FollowSymbolData() = delete;
    FollowSymbolData(const Utf8String &usr, const Utf8String &tokenSpelling, bool isFunctionLike,
                     std::atomic<bool> &ready)
        : m_usr(usr)
        , m_spelling(tokenSpelling)
        , m_isFunctionLike(isFunctionLike)
        , m_ready(ready)
    {}
    FollowSymbolData(const FollowSymbolData &other)
        : m_usr(other.m_usr)
        , m_spelling(other.m_spelling)
        , m_isFunctionLike(other.m_isFunctionLike)
        , m_ready(other.m_ready)
    {}

    const Utf8String &usr() const { return m_usr; }
    const Utf8String &spelling() const { return m_spelling; }
    bool isFunctionLike() const { return m_isFunctionLike; }
    bool ready() const { return m_ready; }
    const SourceRangeContainer &result() const { return m_result; }

    void setReady(bool ready = true) { m_ready = ready; }
    void setResult(const SourceRangeContainer &result) { m_result = result; }
private:
    const Utf8String &m_usr;
    const Utf8String &m_spelling;
    SourceRangeContainer m_result;
    bool m_isFunctionLike;
    std::atomic<bool> &m_ready;
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
    return SourceRange(clang_getRange(start, end));
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
        return SourceRange(clang_getTokenExtent(tu, tokens.data[i]));
    }
    return SourceRangeContainer();
}

static void handleDeclaration(CXClientData client_data, const CXIdxDeclInfo *declInfo)
{
    if (!declInfo || !declInfo->isDefinition)
        return;

    const Cursor currentCursor(declInfo->cursor);
    auto* data = reinterpret_cast<FollowSymbolData*>(client_data);
    if (data->ready())
        return;

    if (data->usr() != currentCursor.canonical().unifiedSymbolResolution())
        return;

    QString str = Utf8String(currentCursor.displayName());
    if (currentCursor.isFunctionLike() || currentCursor.isConstructorOrDestructor()) {
        if (!data->isFunctionLike())
            return;
        str = str.mid(0, str.indexOf('('));
    } else if (data->isFunctionLike()) {
        return;
    }
    if (!str.endsWith(data->spelling()))
        return;
    const CXTranslationUnit tu = clang_Cursor_getTranslationUnit(declInfo->cursor);
    Tokens tokens(currentCursor);

    for (uint i = 0; i < tokens.tokenCount; ++i) {
        Utf8String curSpelling = ClangString(clang_getTokenSpelling(tu, tokens.data[i]));
        if (data->spelling() == curSpelling) {
            if (data->isFunctionLike()
                    && (i+1 >= tokens.tokenCount
                        || !(ClangString(clang_getTokenSpelling(tu, tokens.data[i+1])) == "("))) {
                continue;
            }
            data->setResult(SourceRange(clang_getTokenExtent(tu, tokens.data[i])));
            data->setReady();
            return;
        }
    }
}

static int getTokenIndex(CXTranslationUnit tu, const Tokens &tokens, uint line, uint column)
{
    int tokenIndex = -1;
    for (int i = static_cast<int>(tokens.tokenCount - 1); i >= 0; --i) {
        const SourceRange range = clang_getTokenExtent(tu, tokens.data[i]);
        if (range.contains(line, column)) {
            tokenIndex = i;
            break;
        }
    }
    return tokenIndex;
}

static IndexerCallbacks createIndexerCallbacks()
{
    return {
        [](CXClientData client_data, void *) {
            auto* data = reinterpret_cast<FollowSymbolData*>(client_data);
            return data->ready() ? 1 : 0;
        },
        [](CXClientData, CXDiagnosticSet, void *) {},
        [](CXClientData, CXFile, void *) { return CXIdxClientFile(); },
        [](CXClientData, const CXIdxIncludedFileInfo *) { return CXIdxClientFile(); },
        [](CXClientData, const CXIdxImportedASTFileInfo *) { return CXIdxClientASTFile(); },
        [](CXClientData, void *) { return CXIdxClientContainer(); },
        handleDeclaration,
        [](CXClientData, const CXIdxEntityRefInfo *) {}
    };
}

static SourceRangeContainer followSymbolInDependentFiles(CXIndex index,
                                                         const Cursor &cursor,
                                                         const Utf8String &tokenSpelling,
                                                         const QVector<Utf8String> &dependentFiles,
                                                         const CommandLineArguments &currentArgs)
{
    int argsCount = 0;
    if (currentArgs.data())
        argsCount = currentArgs.count() - 1;

    const Utf8String usr = cursor.canonical().unifiedSymbolResolution();

    // ready is shared for all data in vector
    std::atomic<bool> ready {false};
    std::vector<FollowSymbolData> dataVector(
                dependentFiles.size(),
                FollowSymbolData(usr, tokenSpelling,
                                 cursor.isFunctionLike() || cursor.isConstructorOrDestructor(),
                                 ready));

    std::vector<std::future<void>> indexFutures;

    for (int i = 0; i < dependentFiles.size(); ++i) {
        if (i > 0 && ready)
            break;
        indexFutures.emplace_back(std::async([&, i]() {
            TIME_SCOPE_DURATION("Dependent file " + dependentFiles.at(i) + " indexer runner");

            const CXIndexAction indexAction = clang_IndexAction_create(index);
            IndexerCallbacks callbacks = createIndexerCallbacks();
            clang_indexSourceFile(indexAction,
                                  &dataVector[i],
                                  &callbacks,
                                  sizeof(callbacks),
                                  CXIndexOpt_SkipParsedBodiesInSession
                                      | CXIndexOpt_SuppressRedundantRefs
                                      | CXIndexOpt_SuppressWarnings,
                                  dependentFiles.at(i).constData(),
                                  currentArgs.data(),
                                  argsCount,
                                  nullptr,
                                  0,
                                  nullptr,
                                  CXTranslationUnit_SkipFunctionBodies
                                      | CXTranslationUnit_KeepGoing);
            clang_IndexAction_dispose(indexAction);
        }));
    }

    for (const std::future<void> &future: indexFutures)
        future.wait();

    for (const FollowSymbolData &data: dataVector) {
        if (!data.result().start().filePath().isEmpty()) {
            return data.result();
        }
    }
    return SourceRangeContainer();
}

SourceRangeContainer FollowSymbol::followSymbol(CXTranslationUnit tu,
                                                CXIndex index,
                                                const Cursor &fullCursor,
                                                uint line,
                                                uint column,
                                                const QVector<Utf8String> &dependentFiles,
                                                const CommandLineArguments &currentArgs)
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
        return SourceRange(loc, loc);
    }

    // For definitions we can always find a declaration in current TU
    if (cursor.isDefinition())
        return extractMatchingTokenRange(cursor.canonical(), tokenSpelling);

    SourceRangeContainer result;
    if (!cursor.isDeclaration()) {
        // This is the symbol usage
        // We want to return definition or at least declaration of this symbol
        const Cursor referencedCursor = cursor.referenced();
        if (referencedCursor.isNull() || referencedCursor == cursor)
            return SourceRangeContainer();
        result = extractMatchingTokenRange(referencedCursor, tokenSpelling);

        // We've already found what we need
        if (referencedCursor.isDefinition())
            return result;
        cursor = referencedCursor;
    }

    const Cursor definitionCursor = cursor.definition();
    if (!definitionCursor.isNull() && definitionCursor != cursor) {
        // If we are able to find a definition in current TU
        return extractMatchingTokenRange(definitionCursor, tokenSpelling);
    }

    // Search for the definition in the dependent files
    SourceRangeContainer dependentFilesResult = followSymbolInDependentFiles(index,
                                                                             cursor,
                                                                             tokenSpelling,
                                                                             dependentFiles,
                                                                             currentArgs);
    return dependentFilesResult.start().filePath().isEmpty() ?
                result : dependentFilesResult;
}

} // namespace ClangBackEnd
