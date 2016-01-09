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
#include "highlightinginformation.h"
#include "sourcelocation.h"
#include "sourcerange.h"

#include <cstring>
#include <ostream>

#include <QDebug>

namespace ClangBackEnd {

HighlightingInformation::HighlightingInformation(const CXCursor &cxCursor,
                                                 CXToken *cxToken,
                                                 CXTranslationUnit cxTranslationUnit)
{
    const SourceRange sourceRange = clang_getTokenExtent(cxTranslationUnit, *cxToken);
    const auto start = sourceRange.start();
    const auto end = sourceRange.end();

    originalCursor = cxCursor;
    line = start.line();
    column = start.column();
    length = end.offset() - start.offset();
    type = kind(cxToken, originalCursor);
}

HighlightingInformation::HighlightingInformation(uint line, uint column, uint length, HighlightingType type)
    : line(line),
      column(column),
      length(length),
      type(type)
{
}

bool HighlightingInformation::hasType(HighlightingType type) const
{
    return this->type == type;
}

bool HighlightingInformation::hasFunctionArguments() const
{
    return originalCursor.argumentCount() > 0;
}

QVector<HighlightingInformation> HighlightingInformation::outputFunctionArguments() const
{
    QVector<HighlightingInformation> outputFunctionArguments;

    return outputFunctionArguments;
}

HighlightingInformation::operator HighlightingMarkContainer() const
{
    return HighlightingMarkContainer(line, column, length, type);
}

namespace {

bool isFinalFunction(const Cursor &cursor)
{
    auto referencedCursor = cursor.referenced();
    if (referencedCursor.hasFinalFunctionAttribute())
        return true;

    else return false;
}

bool isFunctionInFinalClass(const Cursor &cursor)
{
    auto functionBase = cursor.functionBaseDeclaration();
    if (functionBase.isValid() && functionBase.hasFinalClassAttribute())
        return true;

    return false;
}
}

HighlightingType HighlightingInformation::memberReferenceKind(const Cursor &cursor) const
{
    if (cursor.isDynamicCall()) {
        if (isFinalFunction(cursor) || isFunctionInFinalClass(cursor))
            return HighlightingType::Function;
        else
            return HighlightingType::VirtualFunction;
    }

    return identifierKind(cursor.referenced());

}

HighlightingType HighlightingInformation::referencedTypeKind(const Cursor &cursor) const
{
    const Cursor referencedCursor = cursor.referenced();

    switch (referencedCursor.kind()) {
        case CXCursor_ClassDecl:
        case CXCursor_StructDecl:
        case CXCursor_UnionDecl:
        case CXCursor_TypedefDecl:
        case CXCursor_TemplateTypeParameter:
        case CXCursor_TypeAliasDecl:
        case CXCursor_EnumDecl:              return HighlightingType::Type;
        default:                             return HighlightingType::Invalid;
    }

    Q_UNREACHABLE();
}

HighlightingType HighlightingInformation::variableKind(const Cursor &cursor) const
{
    if (cursor.isLocalVariable())
        return HighlightingType::LocalVariable;
    else
        return HighlightingType::GlobalVariable;
}

bool HighlightingInformation::isVirtualMethodDeclarationOrDefinition(const Cursor &cursor) const
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
bool HighlightingInformation::isRealDynamicCall(const Cursor &cursor) const
{

    return originalCursor.isDynamicCall() && isNotFinalFunction(cursor);
}

HighlightingType HighlightingInformation::functionKind(const Cursor &cursor) const
{
    if (isRealDynamicCall(cursor) || isVirtualMethodDeclarationOrDefinition(cursor))
        return HighlightingType::VirtualFunction;
    else
        return HighlightingType::Function;
}

HighlightingType HighlightingInformation::identifierKind(const Cursor &cursor) const
{
    switch (cursor.kind()) {
        case CXCursor_Destructor:
        case CXCursor_Constructor:
        case CXCursor_FunctionDecl:
        case CXCursor_CallExpr:
        case CXCursor_CXXMethod:                 return functionKind(cursor);
        case CXCursor_NonTypeTemplateParameter:
        case CXCursor_ParmDecl:                  return HighlightingType::LocalVariable;
        case CXCursor_VarDecl:                   return variableKind(cursor);
        case CXCursor_VariableRef:
        case CXCursor_DeclRefExpr:               return identifierKind(cursor.referenced());
        case CXCursor_MemberRefExpr:             return memberReferenceKind(cursor);
        case CXCursor_FieldDecl:
        case CXCursor_MemberRef:
        case CXCursor_ObjCIvarDecl:
        case CXCursor_ObjCPropertyDecl:
        case CXCursor_ObjCClassMethodDecl:
        case CXCursor_ObjCInstanceMethodDecl:
        case CXCursor_ObjCSynthesizeDecl:
        case CXCursor_ObjCDynamicDecl:           return HighlightingType::Field;
        case CXCursor_TypeRef:                   return referencedTypeKind(cursor);
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
        case CXCursor_ObjCSuperClassRef:         return HighlightingType::Type;
        case CXCursor_FunctionTemplate:          return HighlightingType::Function;
        case CXCursor_EnumConstantDecl:          return HighlightingType::Enumeration;
        case CXCursor_PreprocessingDirective:    return HighlightingType::Preprocessor;
        case CXCursor_MacroExpansion:            return HighlightingType::PreprocessorExpansion;
        case CXCursor_MacroDefinition:           return HighlightingType::PreprocessorDefinition;
        case CXCursor_InclusionDirective:        return HighlightingType::StringLiteral;
        case CXCursor_LabelRef:
        case CXCursor_LabelStmt:                 return HighlightingType::Label;
        default:                                 return HighlightingType::Invalid;
    }

    Q_UNREACHABLE();
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

HighlightingType punctationKind(const Cursor &cursor)
{
    switch (cursor.kind()) {
        case CXCursor_DeclRefExpr: return operatorKind(cursor);
        default:                   return HighlightingType::Invalid;
    }
}

}
HighlightingType HighlightingInformation::kind(CXToken *cxToken, const Cursor &cursor) const
{
    auto cxTokenKind = clang_getTokenKind(*cxToken);

    switch (cxTokenKind) {
        case CXToken_Keyword:     return HighlightingType::Keyword;
        case CXToken_Punctuation: return punctationKind(cursor);
        case CXToken_Identifier:  return identifierKind(cursor);
        case CXToken_Comment:     return HighlightingType::Comment;
        case CXToken_Literal:     return literalKind(cursor);
    }

    Q_UNREACHABLE();
}

void PrintTo(const HighlightingInformation &information, ::std::ostream *os)
{
    *os << "type: ";
    PrintTo(information.type, os);
    *os << " line: " << information.line
        << " column: " << information.column
        << " length: " << information.length;
}

} // namespace ClangBackEnd
