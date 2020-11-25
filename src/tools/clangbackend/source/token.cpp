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


#include "token.h"

#include "clangstring.h"
#include "cursor.h"
#include "sourcelocation.h"
#include "sourcerange.h"

#include <QList>

namespace ClangBackEnd {

Token::Token(CXTranslationUnit cxTranslationUnit, CXToken *cxToken)
    : m_cxTranslationUnit(cxTranslationUnit)
    , m_cxToken(cxToken)
{
}

bool Token::isNull() const
{
    return !m_cxToken;
}

CXTokenKind Token::kind() const
{
    return clang_getTokenKind(*m_cxToken);
}

CXToken *Token::cx() const
{
    return m_cxToken;
}

SourceLocation Token::location() const
{
    return SourceLocation(m_cxTranslationUnit, clang_getTokenLocation(m_cxTranslationUnit, *m_cxToken));
}

SourceRange Token::extent() const
{
    return SourceRange(m_cxTranslationUnit, clang_getTokenExtent(m_cxTranslationUnit, *m_cxToken));
}

ClangString Token::spelling() const
{
    return clang_getTokenSpelling(m_cxTranslationUnit, *m_cxToken);
}

Tokens::Tokens(const SourceRange &range)
    : m_cxTranslationUnit(range.tu())
{
    CXSourceRange cxRange(range);
    CXToken *cxTokens;
    unsigned numTokens;
    clang_tokenize(m_cxTranslationUnit, cxRange, &cxTokens, &numTokens);

    m_tokens.reserve(numTokens);
    for (size_t i = 0; i < numTokens; ++i)
        m_tokens.push_back(Token(m_cxTranslationUnit, cxTokens + i));
}

std::vector<Cursor> Tokens::annotate() const
{
    std::vector<Cursor> cursors;
    if (m_tokens.empty())
        return cursors;

    std::vector<CXCursor> cxCursors;
    cxCursors.resize(m_tokens.size());

    clang_annotateTokens(m_cxTranslationUnit, m_tokens.front().cx(), m_tokens.size(), cxCursors.data());

    // The alias declaration "using S = struct {}" results in libclang reporting the type
    // of the surrounding scope (e.g. a namespace or class) for the cursor corresponding to
    // the token "S" (QTCREATORBUG-24875). We need to correct this manually.
    // The same goes for function attributes like "[[deprecated]]".
    // TODO: Investigate whether we can fix this in libclang itself.
    for (int i = 1; i < int(m_tokens.size()) - 2; ++i) {
        const Token &tok = m_tokens.at(i);
        if (tok.kind() != CXToken_Identifier && tok.kind() != CXToken_Keyword)
            continue;
        const Token &prevTok = m_tokens.at(i - 1);
        const Token &nextTok = m_tokens.at(i + 1);

        // QTCREATORBUG-24875
        if (prevTok.kind() == CXToken_Keyword
                && prevTok.spelling() == "using"
                && nextTok.spelling() == "="
                && cxCursors.at(i).kind != CXCursor_TypeAliasDecl) {
            // If the surrounding scope is a namespace, the correct type is reported for the token
            // after the identifier, i.e. the "=" symbol. Otherwise, it's not reported at all,
            // and we have to use the cursor after that, which should be the aliased type.
            // Note that we cannot use the next cursors in the cxCursors array, because
            // clang_annotateTokens() reports the surrounding scope for all of these.
            CXCursor nextCursor = clang_getCursor(m_cxTranslationUnit, clang_getTokenLocation(
                                                      m_cxTranslationUnit, *nextTok.cx()));
            if (nextCursor.kind != CXCursor_TypeAliasDecl) {
                nextCursor = clang_getCursor(m_cxTranslationUnit, clang_getTokenLocation(
                                                 m_cxTranslationUnit, *m_tokens.at(i + 2).cx()));
            }
            cxCursors[i] = nextCursor;
            continue;
        }

        // QTCREATORBUG-24636, QTCREATORBUG-24650
        if (i >= 2) {
            QList<int> setToNull;
            const Token &prevPrevTok = m_tokens.at(i - 2);
            if (prevTok.kind() == CXToken_Punctuation && prevTok.spelling() == "["
                    && prevPrevTok.kind() == CXToken_Punctuation && prevPrevTok.spelling() == "[") {

                int endOfAttrList = i + 2; // assume the first possible end of attribute declaration
                while (endOfAttrList < int(m_tokens.size())) {
                    const Token &last = m_tokens.at(endOfAttrList);
                    const Token &secondLast = m_tokens.at(endOfAttrList - 1);
                    const Token &current = m_tokens.at(endOfAttrList - 2);

                    if (current.kind() == CXToken_Identifier || current.kind() == CXToken_Keyword)
                        setToNull.append(endOfAttrList - 2);
                    if (last.kind() == CXToken_Punctuation && last.spelling() == "]"
                            && secondLast.kind() == CXToken_Punctuation
                            && secondLast.spelling() == "]") {
                        break;
                    }
                    ++endOfAttrList;
                }
                for (int index : qAsConst(setToNull)) {
                    for (int j = index + 3; j < int(m_tokens.size()); ++j) {
                        if (cxCursors[j] == cxCursors[j - 1])
                            continue;
                        if (cxCursors[j].kind == CXCursor_FunctionDecl
                                || cxCursors[j].kind == CXCursor_ParmDecl) {
                            cxCursors[index] = clang_getNullCursor();
                            cxCursors[index].kind = CXCursor_NotImplemented; // Our magic tag.
                        }
                        break;
                    }
                }
            }
        }
    }

    cursors.reserve(cxCursors.size());
    for (const CXCursor &cxCursor : cxCursors)
        cursors.emplace_back(cxCursor);

    return cursors;
}

const Token &Tokens::operator[](int index) const
{
    return m_tokens[index];
}

Token &Tokens::operator[](int index)
{
    return m_tokens[index];
}

std::vector<Token>::const_iterator Tokens::cbegin() const
{
    return m_tokens.cbegin();
}

std::vector<Token>::const_iterator Tokens::cend() const
{
    return m_tokens.cend();
}

std::vector<Token>::iterator Tokens::begin()
{
    return m_tokens.begin();
}

std::vector<Token>::iterator Tokens::end()
{
    return m_tokens.end();
}

int Tokens::getTokenIndex(CXTranslationUnit tu, uint line, uint column) const
{
    int tokenIndex = -1;
    for (int i = size() - 1; i >= 0; --i) {
        const SourceRange range(tu, (*this)[i].extent());
        if (range.contains(line, column)) {
            tokenIndex = i;
            break;
        }
    }
    return tokenIndex;
}

Tokens::~Tokens()
{
    if (m_tokens.empty())
        return;

    clang_disposeTokens(m_cxTranslationUnit, m_tokens.front().cx(),
                        static_cast<unsigned>(m_tokens.size()));
}

} // namespace ClangBackEnd
