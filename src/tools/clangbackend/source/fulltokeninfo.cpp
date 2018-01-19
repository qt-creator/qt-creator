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
#include "cursor.h"
#include "fulltokeninfo.h"

namespace ClangBackEnd {

FullTokenInfo::FullTokenInfo(const CXCursor &cxCursor,
                             CXToken *cxToken,
                             CXTranslationUnit cxTranslationUnit,
                             std::vector<CXSourceRange> &currentOutputArgumentRanges)
    : TokenInfo(cxCursor, cxToken, cxTranslationUnit, currentOutputArgumentRanges)
{
}

FullTokenInfo::operator TokenInfoContainer() const
{
    return TokenInfoContainer(line(), column(), length(), types(), m_extraInfo);
}

static Utf8String fullyQualifiedType(const Cursor &cursor)
{
    Utf8String typeSpelling = cursor.type().canonical().utf8Spelling();
    if (typeSpelling.isEmpty()) {
        // Only if it's the namespaces level.
        typeSpelling = cursor.unifiedSymbolResolution();
        typeSpelling.replace(Utf8StringLiteral("c:@N@"), Utf8StringLiteral(""));
        typeSpelling.replace(Utf8StringLiteral("@N@"), Utf8StringLiteral("::"));
    }
    return typeSpelling;
}

void FullTokenInfo::updateTypeSpelling(const Cursor &cursor, bool functionLike)
{
    m_extraInfo.typeSpelling = fullyQualifiedType(cursor);
    m_extraInfo.semanticParentTypeSpelling = fullyQualifiedType(cursor.semanticParent());
    if (!functionLike)
        return;
    Type type = cursor.type().canonical();
    m_extraInfo.resultTypeSpelling = type.resultType().utf8Spelling();
    bool hasSpaceAfterReturnType = false;
    if (m_extraInfo.resultTypeSpelling.byteSize() < m_extraInfo.typeSpelling.byteSize()) {
        const char *data = m_extraInfo.typeSpelling.constData();
        hasSpaceAfterReturnType = (data[m_extraInfo.resultTypeSpelling.byteSize()] == ' ');
    }
    m_extraInfo.typeSpelling
            = m_extraInfo.typeSpelling.mid(m_extraInfo.resultTypeSpelling.byteSize()
                                         + (hasSpaceAfterReturnType ? 1 : 0));
}

void FullTokenInfo::identifierKind(const Cursor &cursor, Recursion recursion)
{
    updateTypeSpelling(cursor);

    TokenInfo::identifierKind(cursor, recursion);

    m_extraInfo.identifier = (cursor.kind() != CXCursor_PreprocessingDirective);
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

void FullTokenInfo::evaluate()
{
    TokenInfo::evaluate();

    m_extraInfo.token = ClangString(clang_getTokenSpelling(m_cxTranslationUnit, *m_cxToken));

    auto cxTokenKind = clang_getTokenKind(*m_cxToken);
    if (cxTokenKind == CXToken_Identifier) {
        m_extraInfo.declaration = m_originalCursor.isDeclaration();
        m_extraInfo.definition = m_originalCursor.isDefinition();
    }
    m_extraInfo.includeDirectivePath = (m_originalCursor.kind() == CXCursor_InclusionDirective);
}

} // namespace ClangBackEnd
