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
    : currentOutputArgumentRanges(&currentOutputArgumentRanges),
      originalCursor(cxCursor)
{
    const SourceRange sourceRange = clang_getTokenExtent(cxTranslationUnit, *cxToken);
    const auto start = sourceRange.start();
    const auto end = sourceRange.end();

    line = start.line();
    column = start.column();
    offset = start.offset();
    length = end.offset() - start.offset();
    collectKinds(cxToken, originalCursor);
}

HighlightingMark::HighlightingMark(uint line, uint column, uint length, HighlightingTypes types)
    : line(line),
      column(column),
      length(length),
      types(types)
{
}

HighlightingMark::HighlightingMark(uint line, uint column, uint length, HighlightingType type)
    : line(line),
      column(column),
      length(length),
      types(HighlightingTypes())
{
    types.mainHighlightingType = type;
}

bool HighlightingMark::hasInvalidMainType() const
{
    return types.mainHighlightingType == HighlightingType::Invalid;
}

bool HighlightingMark::hasMainType(HighlightingType type) const
{
    return types.mainHighlightingType == type;
}

bool HighlightingMark::hasMixinType(HighlightingType type) const
{
    auto found = std::find(types.mixinHighlightingTypes.begin(),
                           types.mixinHighlightingTypes.end(),
                           type);

    return found != types.mixinHighlightingTypes.end();
}

bool HighlightingMark::hasOnlyType(HighlightingType type) const
{
    return types.mixinHighlightingTypes.size() == 0 && hasMainType(type);
}

bool HighlightingMark::hasFunctionArguments() const
{
    return originalCursor.argumentCount() > 0;
}

HighlightingMark::operator HighlightingMarkContainer() const
{
    return HighlightingMarkContainer(line, column, length, types);
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
            types.mainHighlightingType = HighlightingType::Function;
        else
            types.mainHighlightingType = HighlightingType::VirtualFunction;
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
        case CXCursor_EnumDecl:              types.mainHighlightingType = HighlightingType::Type; break;
        default:                             types.mainHighlightingType = HighlightingType::Invalid; break;
    }
}

void HighlightingMark::variableKind(const Cursor &cursor)
{
    if (cursor.isLocalVariable())
        types.mainHighlightingType = HighlightingType::LocalVariable;
    else
        types.mainHighlightingType = HighlightingType::GlobalVariable;

    if (isOutputArgument())
        types.mixinHighlightingTypes.push_back(HighlightingType::OutputArgument);
}

void HighlightingMark::fieldKind(const Cursor &)
{
    types.mainHighlightingType = HighlightingType::Field;

    if (isOutputArgument())
        types.mixinHighlightingTypes.push_back(HighlightingType::OutputArgument);
}

bool HighlightingMark::isVirtualMethodDeclarationOrDefinition(const Cursor &cursor) const
{
    return cursor.isVirtualMethod()
        && (originalCursor.isDeclaration() || originalCursor.isDefinition());
}
namespace {
bool isNotFinalFunction(const Cursor &cursor)
{
    return !cursor.hasFinalFunctionAttribute();
}

}
bool HighlightingMark::isRealDynamicCall(const Cursor &cursor) const
{
    return originalCursor.isDynamicCall() && isNotFinalFunction(cursor);
}

void HighlightingMark::addExtraTypeIfFirstPass(HighlightingType type,
                                               Recursion recursion)
{
    if (recursion == Recursion::FirstPass)
        types.mixinHighlightingTypes.push_back(type);
}

bool HighlightingMark::isArgumentInCurrentOutputArgumentLocations() const
{
    auto originalSourceLocation = originalCursor.cxSourceLocation();

    const auto isNotSameOutputArgument = [&] (const CXSourceRange &currentSourceRange) {
        return originalSourceLocation.int_data >= currentSourceRange.begin_int_data
            && originalSourceLocation.int_data <= currentSourceRange.end_int_data;
    };

    auto found = std::find_if(currentOutputArgumentRanges->begin(),
                                       currentOutputArgumentRanges->end(),
                                       isNotSameOutputArgument);

    bool isOutputArgument = found != currentOutputArgumentRanges->end();

    return isOutputArgument;
}

bool HighlightingMark::isOutputArgument() const
{
    if (currentOutputArgumentRanges->empty())
        return false;

    return isArgumentInCurrentOutputArgumentLocations();
}

void HighlightingMark::collectOutputArguments(const Cursor &cursor)
{
    cursor.collectOutputArgumentRangesTo(*currentOutputArgumentRanges);
    filterOutPreviousOutputArguments();
}

namespace {

uint getStart(CXSourceRange cxSourceRange)
{
    CXSourceLocation startSourceLocation = clang_getRangeStart(cxSourceRange);

    uint startOffset;

    clang_getFileLocation(startSourceLocation, nullptr, nullptr, nullptr, &startOffset);

    return startOffset;
}
}

void HighlightingMark::filterOutPreviousOutputArguments()
{
    auto isAfterLocation = [this] (CXSourceRange outputRange) {
        return getStart(outputRange) > offset;
    };

    auto precedingBegin = std::partition(currentOutputArgumentRanges->begin(),
                                         currentOutputArgumentRanges->end(),
                                         isAfterLocation);

    currentOutputArgumentRanges->erase(precedingBegin, currentOutputArgumentRanges->end());
}

void HighlightingMark::functionKind(const Cursor &cursor, Recursion recursion)
{
    if (isRealDynamicCall(cursor) || isVirtualMethodDeclarationOrDefinition(cursor))
        types.mainHighlightingType = HighlightingType::VirtualFunction;
    else
        types.mainHighlightingType = HighlightingType::Function;

    addExtraTypeIfFirstPass(HighlightingType::Declaration, recursion);
}

void HighlightingMark::identifierKind(const Cursor &cursor, Recursion recursion)
{
    switch (cursor.kind()) {
        case CXCursor_Destructor:
        case CXCursor_Constructor:
        case CXCursor_FunctionDecl:
        case CXCursor_CallExpr:
        case CXCursor_CXXMethod:                 functionKind(cursor, recursion); break;
        case CXCursor_NonTypeTemplateParameter:
        case CXCursor_CompoundStmt:              types.mainHighlightingType = HighlightingType::LocalVariable; break;
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
        case CXCursor_ObjCDynamicDecl:           types.mainHighlightingType = HighlightingType::Field; break;
        case CXCursor_TypeRef:                   referencedTypeKind(cursor); break;
        case CXCursor_ClassDecl:
        case CXCursor_TemplateTypeParameter:
        case CXCursor_TemplateTemplateParameter:
        case CXCursor_UnionDecl:
        case CXCursor_StructDecl:
        case CXCursor_OverloadedDeclRef:
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
        case CXCursor_ObjCSuperClassRef:         types.mainHighlightingType = HighlightingType::Type; break;
        case CXCursor_FunctionTemplate:          types.mainHighlightingType = HighlightingType::Function; break;
        case CXCursor_EnumConstantDecl:          types.mainHighlightingType = HighlightingType::Enumeration; break;
        case CXCursor_PreprocessingDirective:    types.mainHighlightingType = HighlightingType::Preprocessor; break;
        case CXCursor_MacroExpansion:            types.mainHighlightingType = HighlightingType::PreprocessorExpansion; break;
        case CXCursor_MacroDefinition:           types.mainHighlightingType = HighlightingType::PreprocessorDefinition; break;
        case CXCursor_InclusionDirective:        types.mainHighlightingType = HighlightingType::StringLiteral; break;
        case CXCursor_LabelRef:
        case CXCursor_LabelStmt:                 types.mainHighlightingType = HighlightingType::Label; break;
        default:                                 break;
    }
}

namespace {
HighlightingType literalKind(const Cursor &cursor)
{
    switch (cursor.kind()) {
        case CXCursor_CharacterLiteral:
        case CXCursor_StringLiteral:
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
    switch (cursor.kind()) {
        case CXCursor_DeclRefExpr: return operatorKind(cursor);
        case CXCursor_Constructor:
        case CXCursor_CallExpr:    collectOutputArguments(cursor);
        default:                   return HighlightingType::Invalid;
    }
}

void HighlightingMark::collectKinds(CXToken *cxToken, const Cursor &cursor)
{
    auto cxTokenKind = clang_getTokenKind(*cxToken);

    types = HighlightingTypes();

    switch (cxTokenKind) {
        case CXToken_Keyword:     types.mainHighlightingType = HighlightingType::Keyword; break;
        case CXToken_Punctuation: types.mainHighlightingType = punctuationKind(cursor); break;
        case CXToken_Identifier:  identifierKind(cursor, Recursion::FirstPass); break;
        case CXToken_Comment:     types.mainHighlightingType = HighlightingType::Comment; break;
        case CXToken_Literal:     types.mainHighlightingType = literalKind(cursor); break;
    }
}

void PrintTo(const HighlightingMark &information, ::std::ostream *os)
{
    *os << "type: ";
    PrintTo(information.types, os);
    *os << " line: " << information.line
        << " column: " << information.column
        << " length: " << information.length;
}

} // namespace ClangBackEnd
