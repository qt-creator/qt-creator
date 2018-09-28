/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "clangsupport_global.h"
#include "cursor.h"

#include <clangsupport/tokeninfocontainer.h>

#include <sqlite/utf8string.h>

#include <clang-c/Index.h>

namespace ClangBackEnd {

class Cursor;
class Token;

class TokenInfo
{
    friend bool operator==(const TokenInfo &first, const TokenInfo &second);
    template<class T> friend class TokenProcessor;
public:
    TokenInfo() = default;
    TokenInfo(const Cursor &cursor,
              const Token *token,
              std::vector<CXSourceRange> &m_currentOutputArgumentRanges);
    virtual ~TokenInfo() = default;

    virtual void evaluate();

    bool hasInvalidMainType() const;
    bool hasMainType(HighlightingType type) const;
    unsigned mixinSize() const;
    bool hasMixinType(HighlightingType type) const;
    bool hasMixinTypeAt(uint, HighlightingType type) const;
    bool hasOnlyType(HighlightingType type) const;
    bool hasFunctionArguments() const;

    virtual operator TokenInfoContainer() const;

    const HighlightingTypes &types() const
    {
        return m_types;
    }

    uint line () const
    {
        return m_line;
    }

    uint column() const
    {
        return m_column;
    }

    uint length() const
    {
        return m_length;
    }

protected:
    enum class Recursion {
        FirstPass,
        RecursivePass
    };

    virtual void identifierKind(const Cursor &cursor, Recursion recursion);
    virtual void referencedTypeKind(const Cursor &cursor);
    virtual void overloadedDeclRefKind(const Cursor &cursor);
    virtual void variableKind(const Cursor &cursor);
    virtual void fieldKind(const Cursor &cursor);
    virtual void functionKind(const Cursor &cursor, Recursion recursion);
    virtual void memberReferenceKind(const Cursor &cursor);
    virtual void typeKind(const Cursor &cursor);
    virtual void keywordKind();
    virtual void invalidFileKind();
    virtual void overloadedOperatorKind();
    virtual void punctuationOrOperatorKind();

    Cursor m_originalCursor;
    const Token *m_token;
    HighlightingTypes m_types;

private:
    bool isVirtualMethodDeclarationOrDefinition(const Cursor &cursor) const;
    bool isDefinition() const;
    bool isRealDynamicCall(const Cursor &cursor) const;
    void addExtraTypeIfFirstPass(HighlightingType type, Recursion recursion);
    bool isOutputArgument() const;
    void collectOutputArguments(const Cursor &cursor);
    void filterOutPreviousOutputArguments();
    bool isArgumentInCurrentOutputArgumentLocations() const;

    std::vector<CXSourceRange> *m_currentOutputArgumentRanges = nullptr;
    uint m_line = 0;
    uint m_column = 0;
    uint m_length = 0;
    uint m_offset = 0;
};


inline bool operator==(const TokenInfo &first, const TokenInfo &second)
{
    return first.m_line == second.m_line
        && first.m_column == second.m_column
        && first.m_length == second.m_length
        && first.m_types == second.m_types;
}

} // namespace ClangBackEnd
