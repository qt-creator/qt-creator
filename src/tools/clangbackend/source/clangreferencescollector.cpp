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

#include "clangreferencescollector.h"

#include "clangstring.h"
#include "cursor.h"
#include "sourcerange.h"

#include <clangsupport/sourcerangecontainer.h>
#include <utils/qtcassert.h>

#include <utf8string.h>

#include <QDebug>

namespace ClangBackEnd {

namespace {

class ReferencedCursor
{
public:
    static ReferencedCursor find(const Cursor &cursor)
    {
        // Query the referenced cursor directly instead of first testing with cursor.isReference().
        // cursor.isReference() reports false for e.g. CXCursor_DeclRefExpr or CXCursor_CallExpr
        // although it returns a valid cursor.
        const Cursor referenced = cursor.referenced();
        if (referenced.isValid())
            return handleReferenced(referenced);

        const Cursor definition = cursor.definition();
        if (definition.isValid())
            return definition;

        return cursor;
    }

    Utf8String usr() const
    {
        return cursor.unifiedSymbolResolution() + usrSuffix;
    }

    bool isLocalVariable() const
    {
        return cursor.isLocalVariable();
    }

private:
    ReferencedCursor(const Cursor &cursor, const Utf8String &usrSuffix = Utf8String())
        : cursor(cursor)
        , usrSuffix(usrSuffix)
    {}

    static ReferencedCursor handleReferenced(const Cursor &cursor)
    {
        if (cursor.kind() == CXCursor_OverloadedDeclRef) {
            // e.g. Text cursor is on "App" of "using N::App;".
            if (cursor.overloadedDeclarationsCount() >= 1)
                return cursor.overloadedDeclaration(0);
        }

        if (cursor.isConstructorOrDestructor()) {
            const Type type = cursor.type();
            if (type.isValid()) {
                const Cursor typeDeclaration = type.declaration();
                if (typeDeclaration.isValid()) {
                    // A CXCursor_CallExpr like "new Foo" has a type of CXType_Record and its
                    // declaration is e.g. CXCursor_ClassDecl.
                    return typeDeclaration;
                } else {
                    // A CXCursor_Constructor like "Foo();" has a type of CXType_FunctionProto
                    // and its type declaration is invalid, so use the semantic parent.
                    const Cursor parent = cursor.semanticParent();
                    if (parent.isValid())
                        return parent;
                }
            }
        }

        if (cursor.isFunctionLike() || cursor.isTemplateLike()) {
            const Cursor parent = cursor.semanticParent();
            const ClangString spelling = cursor.spelling();
            return {parent, Utf8StringLiteral("_qtc_") + Utf8String(spelling)};
        }

        return cursor;
    }

private:
    Cursor cursor;
    Utf8String usrSuffix;
};

class ReferencesCollector
{
public:
    ReferencesCollector(CXTranslationUnit cxTranslationUnit);
    ~ReferencesCollector();

    ReferencesResult collect(uint line, uint column) const;

private:
    bool isWithinTokenRange(CXToken token, uint line, uint column) const;
    bool pointsToIdentifier(uint line, uint column, unsigned *tokenIndex) const;
    bool matchesIdentifier(const CXToken &token, const Utf8String &identifier) const;
    bool checkToken(unsigned index, const Utf8String &identifier, const Utf8String &usr) const;

private:
    CXTranslationUnit m_cxTranslationUnit = nullptr;
    CXToken *m_cxTokens = nullptr;
    uint m_cxTokenCount = 0;

    QVector<CXCursor> m_cxCursors;
};

ReferencesCollector::ReferencesCollector(CXTranslationUnit cxTranslationUnit)
    : m_cxTranslationUnit(cxTranslationUnit)
{
    const CXSourceRange range
            = clang_getCursorExtent(clang_getTranslationUnitCursor(m_cxTranslationUnit));
    clang_tokenize(cxTranslationUnit, range, &m_cxTokens, &m_cxTokenCount);

    m_cxCursors.resize(static_cast<int>(m_cxTokenCount));
    clang_annotateTokens(cxTranslationUnit, m_cxTokens, m_cxTokenCount, m_cxCursors.data());
}

ReferencesCollector::~ReferencesCollector()
{
    clang_disposeTokens(m_cxTranslationUnit, m_cxTokens, m_cxTokenCount);
}

bool ReferencesCollector::isWithinTokenRange(CXToken token, uint line, uint column) const
{
    const SourceRange range = clang_getTokenExtent(m_cxTranslationUnit, token);
    return range.contains(line, column);
}

bool ReferencesCollector::pointsToIdentifier(uint line, uint column, unsigned *tokenIndex) const
{
    for (uint i = 0; i < m_cxTokenCount; ++i) {
        const CXToken token = m_cxTokens[i];
        if (clang_getTokenKind(token) == CXToken_Identifier
                && isWithinTokenRange(token, line, column)) {
            *tokenIndex = i;
            return true;
        }
    }

    return false;
}

bool ReferencesCollector::matchesIdentifier(const CXToken &token,
                                            const Utf8String &identifier) const
{
    const CXTokenKind tokenKind = clang_getTokenKind(token);
    if (tokenKind == CXToken_Identifier) {
        const Utf8String candidateIdentifier
                = ClangString(clang_getTokenSpelling(m_cxTranslationUnit, token));
        return candidateIdentifier == identifier;
    }

    return false;
}

bool ReferencesCollector::checkToken(unsigned index, const Utf8String &identifier,
                                     const Utf8String &usr) const
{
    const CXToken token = m_cxTokens[index];
    if (!matchesIdentifier(token, identifier))
        return false;

    { // For debugging only
//        const SourceRange range = clang_getTokenExtent(m_cxTranslationUnit, token);
//        const uint line = range.start().line();
//        const ClangString spellingCs = clang_getTokenSpelling(m_cxTranslationUnit, token);
//        const Utf8String spelling = spellingCs;
//        qWarning() << "ReferencesCollector::checkToken:" << line << spelling;
    }

    const Cursor currentCursor(m_cxCursors[static_cast<int>(index)]);
    const ReferencedCursor candidate = ReferencedCursor::find(currentCursor);

    return candidate.usr() == usr;
}

ReferencesResult ReferencesCollector::collect(uint line, uint column) const
{
    ReferencesResult result;

    unsigned index = 0;
    if (!pointsToIdentifier(line, column, &index))
        return result;

    const Cursor cursorFromUser = m_cxCursors[static_cast<int>(index)];
    const ReferencedCursor refCursor = ReferencedCursor::find(cursorFromUser);
    const Utf8String usr = refCursor.usr();
    if (usr.isEmpty())
        return result;

    const CXToken token = m_cxTokens[index];
    const Utf8String identifier = ClangString(clang_getTokenSpelling(m_cxTranslationUnit, token));
    for (uint i = 0; i < m_cxTokenCount; ++i) {
        if (checkToken(i, identifier, usr)) {
            const SourceRange range = clang_getTokenExtent(m_cxTranslationUnit, m_cxTokens[i]);
            result.references.append(range);
        }
    }

    result.isLocalVariable = refCursor.isLocalVariable();

    return result;
}

} // anonymous namespace

ReferencesResult collectReferences(CXTranslationUnit cxTranslationUnit,
                                   uint line,
                                   uint column)
{
    ReferencesCollector collector(cxTranslationUnit);
    return collector.collect(line, column);
}

} // namespace ClangBackEnd
