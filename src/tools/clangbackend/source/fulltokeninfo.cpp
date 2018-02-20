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
#include "sourcerange.h"

#include <utils/predicates.h>

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
                || kind == CXCursor_ClassDecl || kind == CXCursor_CXXMethod) {
            Cursor cursor(cxCursor);
            const SourceRange range = cursor.sourceRange();
            if (range.start().filePath() != filePath)
                return CXChildVisit_Continue;
            if (range.contains(line, column)) {
                if (kind == CXCursor_Namespace || kind == CXCursor_StructDecl
                        || kind == CXCursor_ClassDecl) {
                    return CXChildVisit_Recurse;
                }
                // CXCursor_CXXMethod case. This is Q_PROPERTY_MAGIC_FUNCTION
                parentSpelling = Cursor(parent).type().spelling();
                return CXChildVisit_Break;
            }
        }
        return CXChildVisit_Continue;
    });
    return parentSpelling;
}

static Utf8String getPropertyType(const CXSourceLocation &cxLocation,
                                  CXTranslationUnit cxTranslationUnit,
                                  uint propertyPosition)
{
#if defined(CINDEX_VERSION_HAS_GETFILECONTENTS_BACKPORTED) || CINDEX_VERSION_MINOR >= 47
    // Extract property type from the source code
    CXFile cxFile;
    uint offset;
    clang_getFileLocation(cxLocation, &cxFile, nullptr, nullptr, &offset);
    const char *const contents = clang_getFileContents(cxTranslationUnit, cxFile, nullptr);
    const char *const lineContents = &contents[offset - propertyPosition];

    const char *typeStart = std::strstr(lineContents, "Q_PROPERTY") + 10;
    typeStart += std::strspn(typeStart, "( \t\n\r");
    if (typeStart - lineContents >= propertyPosition)
        return Utf8String();
    auto typeEnd = std::find_if(std::reverse_iterator<const char*>(lineContents + propertyPosition),
                                std::reverse_iterator<const char*>(typeStart),
                                Utils::unequalTo(' '));

    return Utf8String(typeStart, static_cast<int>(&(*typeEnd) + 1 - typeStart));
#else
    Q_UNUSED(cxLocation)
    Q_UNUSED(cxTranslationUnit)
    Q_UNUSED(propertyPosition)
    return Utf8String();
#endif
}

void FullTokenInfo::updatePropertyData()
{
    CXSourceRange cxRange(clang_getTokenExtent(m_cxTranslationUnit, *m_cxToken));
    const SourceRange range(m_cxTranslationUnit, cxRange);
    m_extraInfo.semanticParentTypeSpelling = propertyParentSpelling(m_cxTranslationUnit,
                                                                    range.start().filePath(),
                                                                    line(),
                                                                    column());
    m_extraInfo.cursorRange = range;
    m_extraInfo.declaration = true;
    m_extraInfo.definition = true;
    m_extraInfo.typeSpelling = getPropertyType(clang_getRangeStart(cxRange),
                                               m_cxTranslationUnit,
                                               column() - 1);
}

void FullTokenInfo::identifierKind(const Cursor &cursor, Recursion recursion)
{
    updateTypeSpelling(cursor);

    TokenInfo::identifierKind(cursor, recursion);

    m_extraInfo.identifier = (cursor.kind() != CXCursor_PreprocessingDirective);

    if (types().mainHighlightingType == HighlightingType::QtProperty)
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
    m_extraInfo.token = ClangString(clang_getTokenSpelling(m_cxTranslationUnit, *m_cxToken));

    auto cxTokenKind = clang_getTokenKind(*m_cxToken);
    if (cxTokenKind == CXToken_Identifier) {
        m_extraInfo.declaration = m_originalCursor.isDeclaration();
        m_extraInfo.definition = m_originalCursor.isDefinition();
    }
    m_extraInfo.includeDirectivePath = (m_originalCursor.kind() == CXCursor_InclusionDirective);

    TokenInfo::evaluate();
}

} // namespace ClangBackEnd
