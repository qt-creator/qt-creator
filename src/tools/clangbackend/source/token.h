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

#pragma once

#include <clang-c/Index.h>

#include <vector>

namespace ClangBackEnd {

class ClangString;
class Cursor;
class SourceLocation;
class SourceRange;

class Token
{
    friend class Tokens;
public:
    bool isNull() const;

    CXTokenKind kind() const;

    SourceLocation location() const;
    SourceRange extent() const;
    ClangString spelling() const;

    CXToken *cx() const;
    CXTranslationUnit tu() const { return m_cxTranslationUnit; }

private:
    Token(CXTranslationUnit m_cxTranslationUnit, CXToken *cxToken);

    CXTranslationUnit m_cxTranslationUnit;
    CXToken *m_cxToken;
};

class Tokens
{
public:
    Tokens() = default;
    Tokens(const SourceRange &range);
    ~Tokens();

    Tokens(Tokens &&other) = default;
    Tokens(const Tokens &other) = delete;
    Tokens &operator=(Tokens &&other) = default;
    Tokens &operator=(const Tokens &other) = delete;

    std::vector<Cursor> annotate() const;

    size_t size() const { return m_tokens.size(); }
    const Token &operator[](size_t index) const;
    Token &operator[](size_t index);

    std::vector<Token>::const_iterator cbegin() const;
    std::vector<Token>::const_iterator cend() const;
    std::vector<Token>::iterator begin();
    std::vector<Token>::iterator end();

    CXTranslationUnit tu() const { return m_cxTranslationUnit; }
private:
    CXTranslationUnit m_cxTranslationUnit;
    std::vector<Token> m_tokens;
};

} // namespace ClangBackEnd
