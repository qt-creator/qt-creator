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

#include "clangstring.h"
#include "clangtooltipinfocollector.h"
#include "cursor.h"
#include "fulltokeninfo.h"
#include "sourcerange.h"
#include "token.h"
#include "tokenprocessor.h"

#include <utils/predicates.h>

namespace ClangBackEnd {

FullTokenInfo::FullTokenInfo(const Cursor &cursor,
                             const Token *token,
                             std::vector<CXSourceRange> &currentOutputArgumentRanges)
    : TokenInfo(cursor, token, currentOutputArgumentRanges)
{
}

FullTokenInfo::operator TokenInfoContainer() const
{
    return TokenInfoContainer(line(), column(), length(), m_types, m_extraInfo);
}

static Utf8String fullyQualifiedType(const Cursor &cursor) {
    Utf8String prefix;
    if (cursor.kind() == CXCursor_ClassTemplate || cursor.kind() == CXCursor_Namespace)
        return qualificationPrefix(cursor) + cursor.displayName();

    return cursor.type().canonical().spelling();
}

void FullTokenInfo::updateTypeSpelling(const Cursor &cursor, bool functionLike)
{
    m_extraInfo.semanticParentTypeSpelling = fullyQualifiedType(cursor.semanticParent());
    if (!functionLike) {
        m_extraInfo.typeSpelling = fullyQualifiedType(cursor);
        return;
    }

    m_extraInfo.token = cursor.displayName();
    // On the client side full type is typeSpelling + token.
    m_extraInfo.typeSpelling = cursor.type().resultType().utf8Spelling();
}

static Utf8String propertyParentSpelling(CXTranslationUnit cxTranslationUnit,
                                         const Utf8String &filePath,
                                         uint line, uint column)
{
    // Q_PROPERTY expands into QPropertyMagicFunction which can be found as a child of
    // the containing class.
    Cursor tuCursor = clang_getTranslationUnitCursor(cxTranslationUnit);
    Utf8String parentSpelling;
    tuCursor.visit([&filePath, line, column, &parentSpelling](CXCursor cxCursor, CXCursor parent) {
        const CXCursorKind kind = clang_getCursorKind(cxCursor);
        if (kind == CXCursor_Namespace || kind == CXCursor_StructDecl
                || kind == CXCursor_ClassDecl || kind == CXCursor_StaticAssert) {
            Cursor cursor(cxCursor);
            const SourceRange range = cursor.sourceRange();
            if (range.start().filePath() != filePath)
                return CXChildVisit_Continue;
            if (range.contains(line, column)) {
                if (kind == CXCursor_Namespace || kind == CXCursor_StructDecl
                        || kind == CXCursor_ClassDecl) {
                    return CXChildVisit_Recurse;
                }
                // CXCursor_StaticAssert case. This is Q_PROPERTY static_assert
                parentSpelling = Cursor(parent).type().spelling();
                return CXChildVisit_Break;
            }
        }
        return CXChildVisit_Continue;
    });
    return parentSpelling;
}

static Utf8String getPropertyType(const SourceLocation &location, uint propertyPosition)
{
    // Extract property type from the source code
    CXFile cxFile;
    uint offset;
    clang_getFileLocation(location.cx(), &cxFile, nullptr, nullptr, &offset);
    const char *const contents = clang_getFileContents(location.tu(), cxFile, nullptr);
    const char *const lineContents = &contents[offset - propertyPosition];

    const char *typeStart = std::strstr(lineContents, "Q_PROPERTY") + 10;
    typeStart += std::strspn(typeStart, "( \t\n\r");
    if (typeStart - lineContents >= propertyPosition)
        return Utf8String();
    auto typeEnd = std::find_if(std::reverse_iterator<const char*>(lineContents + propertyPosition),
                                std::reverse_iterator<const char*>(typeStart),
                                Utils::unequalTo(' '));

    return Utf8String(typeStart, static_cast<int>(&(*typeEnd) + 1 - typeStart));
}

void FullTokenInfo::updatePropertyData()
{
    const SourceRange range = m_token->extent();
    m_extraInfo.semanticParentTypeSpelling = propertyParentSpelling(m_token->tu(),
                                                                    range.start().filePath(),
                                                                    line(),
                                                                    column());
    m_extraInfo.cursorRange = range;
    m_extraInfo.declaration = true;
    m_extraInfo.definition = true;
    m_extraInfo.typeSpelling = getPropertyType(range.start(), column() - 1);
}

void FullTokenInfo::identifierKind(const Cursor &cursor, Recursion recursion)
{
    updateTypeSpelling(cursor);

    TokenInfo::identifierKind(cursor, recursion);

    m_extraInfo.identifier = (cursor.kind() != CXCursor_PreprocessingDirective);

    if (m_types.mainHighlightingType == HighlightingType::QtProperty)
        updatePropertyData();
    else
        m_extraInfo.cursorRange = cursor.sourceRange();
}

void FullTokenInfo::referencedTypeKind(const Cursor &cursor)
{
    updateTypeSpelling(cursor.referenced());

    TokenInfo::referencedTypeKind(cursor);
}

void FullTokenInfo::functionKind(const Cursor &cursor, Recursion recursion)
{
    updateTypeSpelling(cursor, true);

    TokenInfo::functionKind(cursor, recursion);

    m_extraInfo.accessSpecifier = cursor.accessSpecifier();
    m_extraInfo.storageClass = cursor.storageClass();

    bool isSignal = false;
    bool isSlot = false;
    m_originalCursor.visit([&isSignal, &isSlot](CXCursor cursor, CXCursor) {
        Cursor cur(cursor);
        ClangString spelling = cur.spelling();
        if (spelling == "qt_signal")
            isSignal = true;
        else if (spelling == "qt_slot")
            isSlot = true;
        return CXChildVisit_Break;
    });
    m_extraInfo.signal = isSignal;
    m_extraInfo.slot = isSlot;
}

void FullTokenInfo::variableKind(const Cursor &cursor)
{
    TokenInfo::variableKind(cursor);

    m_extraInfo.accessSpecifier = cursor.accessSpecifier();
    m_extraInfo.storageClass = cursor.storageClass();
}

void FullTokenInfo::fieldKind(const Cursor &cursor)
{
    TokenInfo::fieldKind(cursor);

    m_extraInfo.accessSpecifier = cursor.accessSpecifier();
    m_extraInfo.storageClass = cursor.storageClass();
}

void FullTokenInfo::memberReferenceKind(const Cursor &cursor)
{
    TokenInfo::memberReferenceKind(cursor);
    if (cursor.isDynamicCall()) {
        m_extraInfo.storageClass = cursor.storageClass();
        m_extraInfo.accessSpecifier = cursor.accessSpecifier();
    }
}

void FullTokenInfo::keywordKind()
{
    TokenInfo::keywordKind();

    if (m_originalCursor.isAnonymous()) {
        CXCursorKind cursorKind = m_originalCursor.kind();
        if (cursorKind == CXCursor_EnumDecl)
            m_types.mixinHighlightingTypes.push_back(HighlightingType::Enum);
        else if (cursorKind == CXCursor_ClassDecl)
            m_types.mixinHighlightingTypes.push_back(HighlightingType::Class);
        else if (cursorKind == CXCursor_StructDecl)
            m_types.mixinHighlightingTypes.push_back(HighlightingType::Struct);
        else if (cursorKind == CXCursor_Namespace)
            m_types.mixinHighlightingTypes.push_back(HighlightingType::Namespace);
        m_extraInfo.declaration = m_extraInfo.definition = true;
        m_extraInfo.token = m_originalCursor.displayName();
        updateTypeSpelling(m_originalCursor);
        m_extraInfo.cursorRange = m_originalCursor.sourceRange();
    }
}

void FullTokenInfo::overloadedOperatorKind()
{
    TokenInfo::overloadedOperatorKind();

    if (!m_types.mixinHighlightingTypes.contains(HighlightingType::OverloadedOperator))
        return;

    // Overloaded operator
    m_extraInfo.identifier = true;
    if (!m_originalCursor.isDeclaration())
        return;

    // Overloaded operator declaration
    m_extraInfo.declaration = true;
    m_extraInfo.definition = m_originalCursor.isDefinition();

    updateTypeSpelling(m_originalCursor, true);
    m_extraInfo.cursorRange = m_originalCursor.sourceRange();
    m_extraInfo.accessSpecifier = m_originalCursor.accessSpecifier();
    m_extraInfo.storageClass = m_originalCursor.storageClass();
}

void FullTokenInfo::evaluate()
{
    m_extraInfo.token = m_token->spelling();
    CXTokenKind cxTokenKind = m_token->kind();
    if (cxTokenKind == CXToken_Identifier) {
        m_extraInfo.declaration = m_originalCursor.isDeclaration();
        m_extraInfo.definition = m_originalCursor.isDefinition();
    }
    m_extraInfo.includeDirectivePath = (m_originalCursor.kind() == CXCursor_InclusionDirective);

    TokenInfo::evaluate();
}

} // namespace ClangBackEnd
