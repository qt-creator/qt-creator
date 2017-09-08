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

#include <highlightingmarkcontainer.h>

#include "clangstring.h"
#include "cursor.h"
#include "highlightingmark.h"
#include "sourcelocation.h"
#include "sourcerange.h"
#include "sourcerangecontainer.h"

#include <cstring>
#include <ostream>

#include <QDebug>

namespace ClangBackEnd {

HighlightingMark::HighlightingMark(const CXCursor &cxCursor,
                                   CXToken *cxToken,
                                   CXTranslationUnit cxTranslationUnit,
                                   std::vector<CXSourceRange> &currentOutputArgumentRanges)
    : m_currentOutputArgumentRanges(&currentOutputArgumentRanges),
      m_originalCursor(cxCursor)
{
    const SourceRange sourceRange = clang_getTokenExtent(cxTranslationUnit, *cxToken);
    const auto start = sourceRange.start();
    const auto end = sourceRange.end();

    m_line = start.line();
    m_column = start.column();
    m_offset = start.offset();
    m_length = end.offset() - start.offset();
    collectKinds(cxTranslationUnit, cxToken, m_originalCursor);
}

HighlightingMark::HighlightingMark(uint line, uint column, uint length, HighlightingTypes types)
    : m_line(line),
      m_column(column),
      m_length(length),
      m_types(types)
{
}

HighlightingMark::HighlightingMark(uint line, uint column, uint length, HighlightingType type)
    : m_line(line),
      m_column(column),
      m_length(length),
      m_types(HighlightingTypes())
{
    m_types.mainHighlightingType = type;
}

bool HighlightingMark::hasInvalidMainType() const
{
    return m_types.mainHighlightingType == HighlightingType::Invalid;
}

bool HighlightingMark::hasMainType(HighlightingType type) const
{
    return m_types.mainHighlightingType == type;
}

bool HighlightingMark::hasMixinType(HighlightingType type) const
{
    auto found = std::find(m_types.mixinHighlightingTypes.begin(),
                           m_types.mixinHighlightingTypes.end(),
                           type);

    return found != m_types.mixinHighlightingTypes.end();
}

bool HighlightingMark::hasOnlyType(HighlightingType type) const
{
    return m_types.mixinHighlightingTypes.size() == 0 && hasMainType(type);
}

bool HighlightingMark::hasFunctionArguments() const
{
    return m_originalCursor.argumentCount() > 0;
}

HighlightingMark::operator HighlightingMarkContainer() const
{
    return HighlightingMarkContainer(m_line, m_column, m_length, m_types, m_isIdentifier,
                                     m_isInclusion);
}

namespace {

bool isFinalFunction(const Cursor &cursor)
{
    auto referencedCursor = cursor.referenced();
    if (referencedCursor.hasFinalFunctionAttribute())
        return true;
    else
        return false;
}

bool isFunctionInFinalClass(const Cursor &cursor)
{
    auto functionBase = cursor.functionBaseDeclaration();
    if (functionBase.isValid() && functionBase.hasFinalClassAttribute())
        return true;

    return false;
}
}

void HighlightingMark::memberReferenceKind(const Cursor &cursor)
{
    if (cursor.isDynamicCall()) {
        if (isFinalFunction(cursor) || isFunctionInFinalClass(cursor))
            m_types.mainHighlightingType = HighlightingType::Function;
        else
            m_types.mainHighlightingType = HighlightingType::VirtualFunction;
    } else {
        identifierKind(cursor.referenced(), Recursion::RecursivePass);
    }
}

void HighlightingMark::referencedTypeKind(const Cursor &cursor)
{
    const Cursor referencedCursor = cursor.referenced();

    switch (referencedCursor.kind()) {
        case CXCursor_ClassDecl:
        case CXCursor_StructDecl:
        case CXCursor_UnionDecl:
        case CXCursor_TypedefDecl:
        case CXCursor_TemplateTypeParameter:
        case CXCursor_TypeAliasDecl:
        case CXCursor_EnumDecl:              m_types.mainHighlightingType = HighlightingType::Type; break;
        default:                             m_types.mainHighlightingType = HighlightingType::Invalid; break;
    }
}

void HighlightingMark::overloadedDeclRefKind(const Cursor &cursor)
{
    m_types.mainHighlightingType = HighlightingType::Function;

    // CLANG-UPGRADE-CHECK: Workaround still needed?
    // Workaround https://bugs.llvm.org//show_bug.cgi?id=33256 - SomeType in
    // "using N::SomeType" is mistakenly considered as a CXCursor_OverloadedDeclRef.
    if (cursor.overloadedDeclarationsCount() >= 1
            && cursor.overloadedDeclaration(0).kind() != CXCursor_FunctionDecl
            && cursor.overloadedDeclaration(0).kind() != CXCursor_FunctionTemplate) {
        m_types.mainHighlightingType = HighlightingType::Type;
    }
}

void HighlightingMark::variableKind(const Cursor &cursor)
{
    if (cursor.isLocalVariable())
        m_types.mainHighlightingType = HighlightingType::LocalVariable;
    else
        m_types.mainHighlightingType = HighlightingType::GlobalVariable;

    if (isOutputArgument())
        m_types.mixinHighlightingTypes.push_back(HighlightingType::OutputArgument);
}

void HighlightingMark::fieldKind(const Cursor &)
{
    m_types.mainHighlightingType = HighlightingType::Field;

    if (isOutputArgument())
        m_types.mixinHighlightingTypes.push_back(HighlightingType::OutputArgument);
}

bool HighlightingMark::isVirtualMethodDeclarationOrDefinition(const Cursor &cursor) const
{
    return cursor.isVirtualMethod()
        && (m_originalCursor.isDeclaration() || m_originalCursor.isDefinition());
}
namespace {
bool isNotFinalFunction(const Cursor &cursor)
{
    return !cursor.hasFinalFunctionAttribute();
}

}
bool HighlightingMark::isRealDynamicCall(const Cursor &cursor) const
{
    return m_originalCursor.isDynamicCall() && isNotFinalFunction(cursor);
}

void HighlightingMark::addExtraTypeIfFirstPass(HighlightingType type,
                                               Recursion recursion)
{
    if (recursion == Recursion::FirstPass)
        m_types.mixinHighlightingTypes.push_back(type);
}

bool HighlightingMark::isArgumentInCurrentOutputArgumentLocations() const
{
    auto originalSourceLocation = m_originalCursor.cxSourceLocation();

    const auto isNotSameOutputArgument = [&] (const CXSourceRange &currentSourceRange) {
        return originalSourceLocation.int_data >= currentSourceRange.begin_int_data
            && originalSourceLocation.int_data <= currentSourceRange.end_int_data;
    };

    auto found = std::find_if(m_currentOutputArgumentRanges->begin(),
                              m_currentOutputArgumentRanges->end(),
                              isNotSameOutputArgument);

    bool isOutputArgument = found != m_currentOutputArgumentRanges->end();

    return isOutputArgument;
}

bool HighlightingMark::isOutputArgument() const
{
    if (m_currentOutputArgumentRanges->empty())
        return false;

    return isArgumentInCurrentOutputArgumentLocations();
}

void HighlightingMark::collectOutputArguments(const Cursor &cursor)
{
    cursor.collectOutputArgumentRangesTo(*m_currentOutputArgumentRanges);
    filterOutPreviousOutputArguments();
}

namespace {

uint getEnd(CXSourceRange cxSourceRange)
{
    CXSourceLocation startSourceLocation = clang_getRangeEnd(cxSourceRange);

    uint endOffset;

    clang_getFileLocation(startSourceLocation, nullptr, nullptr, nullptr, &endOffset);

    return endOffset;
}
}

void HighlightingMark::filterOutPreviousOutputArguments()
{
    auto isAfterLocation = [this] (CXSourceRange outputRange) {
        return getEnd(outputRange) > m_offset;
    };

    auto precedingBegin = std::partition(m_currentOutputArgumentRanges->begin(),
                                         m_currentOutputArgumentRanges->end(),
                                         isAfterLocation);

    m_currentOutputArgumentRanges->erase(precedingBegin, m_currentOutputArgumentRanges->end());
}

void HighlightingMark::functionKind(const Cursor &cursor, Recursion recursion)
{
    if (isRealDynamicCall(cursor) || isVirtualMethodDeclarationOrDefinition(cursor))
        m_types.mainHighlightingType = HighlightingType::VirtualFunction;
    else
        m_types.mainHighlightingType = HighlightingType::Function;

    if (isOutputArgument())
        m_types.mixinHighlightingTypes.push_back(HighlightingType::OutputArgument);

    addExtraTypeIfFirstPass(HighlightingType::Declaration, recursion);
}

void HighlightingMark::identifierKind(const Cursor &cursor, Recursion recursion)
{
    m_isIdentifier = (cursor.kind() != CXCursor_PreprocessingDirective);

    switch (cursor.kind()) {
        case CXCursor_Destructor:
        case CXCursor_Constructor:
        case CXCursor_FunctionDecl:
        case CXCursor_CallExpr:
        case CXCursor_CXXMethod:                 functionKind(cursor, recursion); break;
        case CXCursor_NonTypeTemplateParameter:
        case CXCursor_CompoundStmt:              m_types.mainHighlightingType = HighlightingType::LocalVariable; break;
        case CXCursor_ParmDecl:
        case CXCursor_VarDecl:                   variableKind(cursor); break;
        case CXCursor_DeclRefExpr:               identifierKind(cursor.referenced(), Recursion::RecursivePass); break;
        case CXCursor_MemberRefExpr:             memberReferenceKind(cursor); break;
        case CXCursor_FieldDecl:
        case CXCursor_MemberRef:                 fieldKind(cursor); break;
        case CXCursor_ObjCIvarDecl:
        case CXCursor_ObjCPropertyDecl:
        case CXCursor_ObjCClassMethodDecl:
        case CXCursor_ObjCInstanceMethodDecl:
        case CXCursor_ObjCSynthesizeDecl:
        case CXCursor_ObjCDynamicDecl:           m_types.mainHighlightingType = HighlightingType::Field; break;
        case CXCursor_TypeRef:                   referencedTypeKind(cursor); break;
        case CXCursor_ClassDecl:
        case CXCursor_ClassTemplatePartialSpecialization:
        case CXCursor_TemplateTypeParameter:
        case CXCursor_TemplateTemplateParameter:
        case CXCursor_UnionDecl:
        case CXCursor_StructDecl:
        case CXCursor_TemplateRef:
        case CXCursor_Namespace:
        case CXCursor_NamespaceRef:
        case CXCursor_NamespaceAlias:
        case CXCursor_TypeAliasDecl:
        case CXCursor_TypedefDecl:
        case CXCursor_ClassTemplate:
        case CXCursor_EnumDecl:
        case CXCursor_CXXStaticCastExpr:
        case CXCursor_CXXReinterpretCastExpr:
        case CXCursor_ObjCCategoryDecl:
        case CXCursor_ObjCCategoryImplDecl:
        case CXCursor_ObjCImplementationDecl:
        case CXCursor_ObjCInterfaceDecl:
        case CXCursor_ObjCProtocolDecl:
        case CXCursor_ObjCProtocolRef:
        case CXCursor_ObjCClassRef:
        case CXCursor_ObjCSuperClassRef:         m_types.mainHighlightingType = HighlightingType::Type; break;
        case CXCursor_OverloadedDeclRef:         overloadedDeclRefKind(cursor); break;
        case CXCursor_FunctionTemplate:          m_types.mainHighlightingType = HighlightingType::Function; break;
        case CXCursor_EnumConstantDecl:          m_types.mainHighlightingType = HighlightingType::Enumeration; break;
        case CXCursor_PreprocessingDirective:    m_types.mainHighlightingType = HighlightingType::Preprocessor; break;
        case CXCursor_MacroExpansion:            m_types.mainHighlightingType = HighlightingType::PreprocessorExpansion; break;
        case CXCursor_MacroDefinition:           m_types.mainHighlightingType = HighlightingType::PreprocessorDefinition; break;
        case CXCursor_InclusionDirective:        m_types.mainHighlightingType = HighlightingType::StringLiteral; break;
        case CXCursor_LabelRef:
        case CXCursor_LabelStmt:                 m_types.mainHighlightingType = HighlightingType::Label; break;
        default:                                 break;
    }
}

namespace {
HighlightingType literalKind(const Cursor &cursor)
{
    switch (cursor.kind()) {
        case CXCursor_CharacterLiteral:
        case CXCursor_StringLiteral:
        case CXCursor_InclusionDirective:
        case CXCursor_ObjCStringLiteral: return HighlightingType::StringLiteral;
        case CXCursor_IntegerLiteral:
        case CXCursor_ImaginaryLiteral:
        case CXCursor_FloatingLiteral:   return HighlightingType::NumberLiteral;
        default:                         return HighlightingType::Invalid;
    }

    Q_UNREACHABLE();
}

bool hasOperatorName(const char *operatorString)
{
    return std::strncmp(operatorString, "operator", 8) == 0;
}

HighlightingType operatorKind(const Cursor &cursor)
{
    if (hasOperatorName(cursor.spelling().cString()))
        return HighlightingType::Operator;
    else
        return HighlightingType::Invalid;
}

}

HighlightingType HighlightingMark::punctuationKind(const Cursor &cursor)
{
    HighlightingType highlightingType = HighlightingType::Invalid;

    switch (cursor.kind()) {
        case CXCursor_DeclRefExpr: highlightingType = operatorKind(cursor); break;
        case CXCursor_Constructor:
        case CXCursor_CallExpr:    collectOutputArguments(cursor); break;
        default:                   break;
    }

    if (isOutputArgument())
        m_types.mixinHighlightingTypes.push_back(HighlightingType::OutputArgument);

    return highlightingType;
}

static HighlightingType highlightingTypeForKeyword(CXTranslationUnit cxTranslationUnit,
                                                   CXToken *cxToken,
                                                   const Cursor &cursor)
{
    switch (cursor.kind()) {
        case CXCursor_PreprocessingDirective: return HighlightingType::Preprocessor;
        case CXCursor_InclusionDirective: return HighlightingType::StringLiteral;
        default: break;
    }

    const ClangString spelling = clang_getTokenSpelling(cxTranslationUnit, *cxToken);
    if (spelling == "bool"
            || spelling == "char"
            || spelling == "char16_t"
            || spelling == "char32_t"
            || spelling == "double"
            || spelling == "float"
            || spelling == "int"
            || spelling == "long"
            || spelling == "short"
            || spelling == "signed"
            || spelling == "unsigned"
            || spelling == "void"
            || spelling == "wchar_t") {
        return HighlightingType::PrimitiveType;
    }

    return HighlightingType::Keyword;
}

void HighlightingMark::collectKinds(CXTranslationUnit cxTranslationUnit,
                                    CXToken *cxToken, const Cursor &cursor)
{
    auto cxTokenKind = clang_getTokenKind(*cxToken);

    m_types = HighlightingTypes();

    switch (cxTokenKind) {
        case CXToken_Keyword:     m_types.mainHighlightingType = highlightingTypeForKeyword(cxTranslationUnit, cxToken, m_originalCursor); break;
        case CXToken_Punctuation: m_types.mainHighlightingType = punctuationKind(cursor); break;
        case CXToken_Identifier:  identifierKind(cursor, Recursion::FirstPass); break;
        case CXToken_Comment:     m_types.mainHighlightingType = HighlightingType::Comment; break;
        case CXToken_Literal:     m_types.mainHighlightingType = literalKind(cursor); break;
    }

    m_isInclusion = (cursor.kind() == CXCursor_InclusionDirective);
}

std::ostream &operator<<(std::ostream &os, const HighlightingMark& highlightingMark)
{
    os << "(type: " << highlightingMark.m_types << ", "
       << " line: " << highlightingMark.m_line << ", "
       << " column: " << highlightingMark.m_column << ", "
       << " length: " << highlightingMark.m_length
       << ")";

    return  os;
}

} // namespace ClangBackEnd
