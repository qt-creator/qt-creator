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

#include "clangstring.h"
#include "cursor.h"
#include "tokeninfo.h"
#include "sourcelocation.h"
#include "sourcerange.h"
#include "sourcerangecontainer.h"

#include <utils/qtcfallthrough.h>

#include <array>

namespace ClangBackEnd {

TokenInfo::TokenInfo(const CXCursor &cxCursor,
                     CXToken *cxToken,
                     CXTranslationUnit cxTranslationUnit,
                     std::vector<CXSourceRange> &currentOutputArgumentRanges)
    : m_originalCursor(cxCursor),
      m_cxToken(cxToken),
      m_cxTranslationUnit(cxTranslationUnit),
      m_currentOutputArgumentRanges(&currentOutputArgumentRanges)
{
    const SourceRange sourceRange {cxTranslationUnit,
                                   clang_getTokenExtent(cxTranslationUnit, *cxToken)};
    const auto start = sourceRange.start();
    const auto end = sourceRange.end();

    m_line = start.line();
    m_column = start.column();
    m_offset = start.offset();
    m_length = end.offset() - start.offset();
}

bool TokenInfo::hasInvalidMainType() const
{
    return m_types.mainHighlightingType == HighlightingType::Invalid;
}

bool TokenInfo::hasMainType(HighlightingType type) const
{
    return m_types.mainHighlightingType == type;
}

unsigned TokenInfo::mixinSize() const {
    return m_types.mixinHighlightingTypes.size();
}

bool TokenInfo::hasMixinType(HighlightingType type) const
{
    auto found = std::find(m_types.mixinHighlightingTypes.begin(),
                           m_types.mixinHighlightingTypes.end(),
                           type);

    return found != m_types.mixinHighlightingTypes.end();
}

bool TokenInfo::hasMixinTypeAt(uint position, HighlightingType type) const
{
    return m_types.mixinHighlightingTypes.size() > position &&
           m_types.mixinHighlightingTypes.at(position) == type;
}

bool TokenInfo::hasOnlyType(HighlightingType type) const
{
    return m_types.mixinHighlightingTypes.size() == 0 && hasMainType(type);
}

bool TokenInfo::hasFunctionArguments() const
{
    return m_originalCursor.argumentCount() > 0;
}

TokenInfo::operator TokenInfoContainer() const
{
    return TokenInfoContainer(m_line, m_column, m_length, m_types);
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

void TokenInfo::memberReferenceKind(const Cursor &cursor)
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

void TokenInfo::overloadedDeclRefKind(const Cursor &cursor)
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

void TokenInfo::variableKind(const Cursor &cursor)
{
    if (cursor.isLocalVariable())
        m_types.mainHighlightingType = HighlightingType::LocalVariable;
    else
        m_types.mainHighlightingType = HighlightingType::GlobalVariable;

    if (isOutputArgument())
        m_types.mixinHighlightingTypes.push_back(HighlightingType::OutputArgument);
}

void TokenInfo::fieldKind(const Cursor &cursor)
{
    m_types.mainHighlightingType = HighlightingType::Field;

    const CXCursorKind kind = cursor.kind();
    switch (kind) {
        default:
            m_types.mainHighlightingType = HighlightingType::Invalid;
            return;
        case CXCursor_ObjCPropertyDecl:
            m_types.mixinHighlightingTypes.push_back(HighlightingType::ObjectiveCProperty);
            Q_FALLTHROUGH();
        case CXCursor_FieldDecl:
        case CXCursor_MemberRef:
            if (isOutputArgument())
                m_types.mixinHighlightingTypes.push_back(HighlightingType::OutputArgument);
            return;
        case CXCursor_ObjCClassMethodDecl:
            m_types.mixinHighlightingTypes.push_back(HighlightingType::ObjectiveCMethod);
            return;
        case CXCursor_ObjCIvarDecl:
        case CXCursor_ObjCInstanceMethodDecl:
        case CXCursor_ObjCSynthesizeDecl:
        case CXCursor_ObjCDynamicDecl:
            return;
    }

}

bool TokenInfo::isDefinition() const
{
    return m_originalCursor.isDefinition();
}

bool TokenInfo::isVirtualMethodDeclarationOrDefinition(const Cursor &cursor) const
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
bool TokenInfo::isRealDynamicCall(const Cursor &cursor) const
{
    return m_originalCursor.isDynamicCall() && isNotFinalFunction(cursor);
}

void TokenInfo::addExtraTypeIfFirstPass(HighlightingType type,
                                               Recursion recursion)
{
    if (recursion == Recursion::FirstPass)
        m_types.mixinHighlightingTypes.push_back(type);
}

bool TokenInfo::isArgumentInCurrentOutputArgumentLocations() const
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

bool TokenInfo::isOutputArgument() const
{
    if (m_currentOutputArgumentRanges->empty())
        return false;

    return isArgumentInCurrentOutputArgumentLocations();
}

void TokenInfo::collectOutputArguments(const Cursor &cursor)
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

void TokenInfo::filterOutPreviousOutputArguments()
{
    auto isAfterLocation = [this] (CXSourceRange outputRange) {
        return getEnd(outputRange) > m_offset;
    };

    auto precedingBegin = std::partition(m_currentOutputArgumentRanges->begin(),
                                         m_currentOutputArgumentRanges->end(),
                                         isAfterLocation);

    m_currentOutputArgumentRanges->erase(precedingBegin, m_currentOutputArgumentRanges->end());
}

void TokenInfo::functionKind(const Cursor &cursor, Recursion recursion)
{
    if (isRealDynamicCall(cursor) || isVirtualMethodDeclarationOrDefinition(cursor))
        m_types.mainHighlightingType = HighlightingType::VirtualFunction;
    else
        m_types.mainHighlightingType = HighlightingType::Function;

    if (isOutputArgument())
        m_types.mixinHighlightingTypes.push_back(HighlightingType::OutputArgument);

    addExtraTypeIfFirstPass(HighlightingType::Declaration, recursion);

    if (isDefinition())
        addExtraTypeIfFirstPass(HighlightingType::FunctionDefinition, recursion);
}

void TokenInfo::referencedTypeKind(const Cursor &cursor)
{
    typeKind(cursor.referenced());
}

void TokenInfo::typeKind(const Cursor &cursor)
{
    m_types.mainHighlightingType = HighlightingType::Type;
    const CXCursorKind kind = cursor.kind();
    switch (kind) {
        default:
            m_types.mainHighlightingType = HighlightingType::Invalid;
            return;
        case CXCursor_TemplateRef:
        case CXCursor_NamespaceRef:
        case CXCursor_TypeRef:
            referencedTypeKind(cursor);
            return;
        case CXCursor_ClassTemplate:
        case CXCursor_ClassTemplatePartialSpecialization:
        case CXCursor_ClassDecl:
            m_types.mixinHighlightingTypes.push_back(HighlightingType::Class);
            return;
        case CXCursor_UnionDecl:
            m_types.mixinHighlightingTypes.push_back(HighlightingType::Union);
            return;
        case CXCursor_StructDecl:
            m_types.mixinHighlightingTypes.push_back(HighlightingType::Struct);
            return;
        case CXCursor_EnumDecl:
            m_types.mixinHighlightingTypes.push_back(HighlightingType::Enum);
            return;
        case CXCursor_NamespaceAlias:
        case CXCursor_Namespace:
            m_types.mixinHighlightingTypes.push_back(HighlightingType::Namespace);
            return;
        case CXCursor_TypeAliasDecl:
            m_types.mixinHighlightingTypes.push_back(HighlightingType::TypeAlias);
            return;
        case CXCursor_TypedefDecl:
            m_types.mixinHighlightingTypes.push_back(HighlightingType::Typedef);
            return;
        case CXCursor_ObjCClassRef:
            m_types.mixinHighlightingTypes.push_back(HighlightingType::ObjectiveCClass);
            return;
        case CXCursor_ObjCProtocolDecl:
        case CXCursor_ObjCProtocolRef:
            m_types.mixinHighlightingTypes.push_back(HighlightingType::ObjectiveCProtocol);
            return;
        case CXCursor_ObjCInterfaceDecl:
            m_types.mixinHighlightingTypes.push_back(HighlightingType::ObjectiveCInterface);
            return;
        case CXCursor_ObjCImplementationDecl:
            m_types.mixinHighlightingTypes.push_back(HighlightingType::ObjectiveCImplementation);
            return;
        case CXCursor_ObjCCategoryDecl:
        case CXCursor_ObjCCategoryImplDecl:
            m_types.mixinHighlightingTypes.push_back(HighlightingType::ObjectiveCCategory);
            return;
        case CXCursor_ObjCSuperClassRef:
        case CXCursor_TemplateTypeParameter:
        case CXCursor_TemplateTemplateParameter:
        case CXCursor_CXXStaticCastExpr:
        case CXCursor_CXXReinterpretCastExpr:
            break;
    }
}

void TokenInfo::identifierKind(const Cursor &cursor, Recursion recursion)
{
    if (cursor.isInvalidDeclaration())
        return;

    const CXCursorKind kind = cursor.kind();
    switch (kind) {
        case CXCursor_Destructor:
        case CXCursor_Constructor:
        case CXCursor_FunctionDecl:
        case CXCursor_FunctionTemplate:
        case CXCursor_CallExpr:
        case CXCursor_CXXMethod:
            functionKind(cursor, recursion);
            break;
        case CXCursor_NonTypeTemplateParameter:
            m_types.mainHighlightingType = HighlightingType::LocalVariable;
            break;
        case CXCursor_ParmDecl:
        case CXCursor_VarDecl:
            variableKind(cursor);
            break;
        case CXCursor_DeclRefExpr:
            identifierKind(cursor.referenced(), Recursion::RecursivePass);
            break;
        case CXCursor_MemberRefExpr:
            memberReferenceKind(cursor);
            break;
        case CXCursor_FieldDecl:
        case CXCursor_MemberRef:
        case CXCursor_ObjCPropertyDecl:
        case CXCursor_ObjCIvarDecl:
        case CXCursor_ObjCClassMethodDecl:
        case CXCursor_ObjCInstanceMethodDecl:
        case CXCursor_ObjCSynthesizeDecl:
        case CXCursor_ObjCDynamicDecl:
            fieldKind(cursor);
            break;
        case CXCursor_TemplateRef:
        case CXCursor_NamespaceRef:
        case CXCursor_TypeRef:
            referencedTypeKind(cursor);
            break;
        case CXCursor_ClassDecl:
        case CXCursor_ClassTemplate:
        case CXCursor_ClassTemplatePartialSpecialization:
        case CXCursor_UnionDecl:
        case CXCursor_StructDecl:
        case CXCursor_EnumDecl:
        case CXCursor_Namespace:
            m_types.mixinHighlightingTypes.push_back(HighlightingType::Declaration);
            Q_FALLTHROUGH();
        case CXCursor_TemplateTypeParameter:
        case CXCursor_TemplateTemplateParameter:
        case CXCursor_NamespaceAlias:
        case CXCursor_TypeAliasDecl:
        case CXCursor_TypedefDecl:
        case CXCursor_CXXStaticCastExpr:
        case CXCursor_CXXReinterpretCastExpr:
        case CXCursor_ObjCCategoryDecl:
        case CXCursor_ObjCCategoryImplDecl:
        case CXCursor_ObjCImplementationDecl:
        case CXCursor_ObjCInterfaceDecl:
        case CXCursor_ObjCProtocolDecl:
        case CXCursor_ObjCProtocolRef:
        case CXCursor_ObjCClassRef:
        case CXCursor_ObjCSuperClassRef:
            typeKind(cursor);
            break;
        case CXCursor_OverloadedDeclRef:
            overloadedDeclRefKind(cursor);
            break;
        case CXCursor_EnumConstantDecl:
            m_types.mainHighlightingType = HighlightingType::Enumeration;
            break;
        case CXCursor_PreprocessingDirective:
            m_types.mainHighlightingType = HighlightingType::Preprocessor;
            break;
        case CXCursor_MacroExpansion:
            m_types.mainHighlightingType = HighlightingType::PreprocessorExpansion;
            break;
        case CXCursor_MacroDefinition:
            m_types.mainHighlightingType = HighlightingType::PreprocessorDefinition;
            break;
        case CXCursor_InclusionDirective:
            m_types.mainHighlightingType = HighlightingType::StringLiteral;
            break;
        case CXCursor_LabelRef:
        case CXCursor_LabelStmt:
            m_types.mainHighlightingType = HighlightingType::Label;
            break;
        case CXCursor_InvalidFile:
            invalidFileKind();
            break;
        default:
            break;
    }
}

namespace {
HighlightingType literalKind(const Cursor &cursor)
{
    switch (cursor.kind()) {
        case CXCursor_CharacterLiteral:
        case CXCursor_StringLiteral:
        case CXCursor_InclusionDirective:
        case CXCursor_ObjCStringLiteral:
            return HighlightingType::StringLiteral;
        case CXCursor_IntegerLiteral:
        case CXCursor_ImaginaryLiteral:
        case CXCursor_FloatingLiteral:
            return HighlightingType::NumberLiteral;
        default:
            return HighlightingType::Invalid;
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

HighlightingType TokenInfo::punctuationKind(const Cursor &cursor)
{
    HighlightingType highlightingType = HighlightingType::Invalid;

    switch (cursor.kind()) {
        case CXCursor_DeclRefExpr:
            highlightingType = operatorKind(cursor);
            break;
        case CXCursor_Constructor:
        case CXCursor_CallExpr:
            collectOutputArguments(cursor);
            break;
        default:
            break;
    }

    if (isOutputArgument())
        m_types.mixinHighlightingTypes.push_back(HighlightingType::OutputArgument);

    return highlightingType;
}

enum class PropertyPart
{
    None,
    Type,
    Property,
    Keyword,
    FunctionOrPrimitiveType
};

static PropertyPart propertyPart(CXTranslationUnit tu, CXToken *token)
{
    static constexpr const char *propertyKeywords[]
            = {"READ", "WRITE", "MEMBER", "RESET", "NOTIFY", "REVISION", "DESIGNABLE",
               "SCRIPTABLE", "STORED", "USER", "CONSTANT", "FINAL"
              };
    CXSourceLocation location = clang_getTokenLocation(tu, *token);

    // If current token is inside Q_PROPERTY then the cursor from token's position will be
    // the whole Q_PROPERTY macro cursor.
    Cursor possibleQPropertyCursor = clang_getCursor(tu, location);
    if (!(possibleQPropertyCursor.spelling() == "Q_PROPERTY"))
        return PropertyPart::None;

    const ClangString currentToken = clang_getTokenSpelling(tu, *token);
    if (std::find(std::begin(propertyKeywords), std::end(propertyKeywords), currentToken)
            != std::end(propertyKeywords)) {
        return PropertyPart::Keyword;
    }

    const ClangString nextToken = clang_getTokenSpelling(tu, *(token + 1));
    const ClangString previousToken = clang_getTokenSpelling(tu, *(token - 1));
    if (std::find(std::begin(propertyKeywords), std::end(propertyKeywords), nextToken)
            != std::end(propertyKeywords)) {
        if (std::find(std::begin(propertyKeywords), std::end(propertyKeywords), previousToken)
                == std::end(propertyKeywords)) {
            return PropertyPart::Property;
        }

        return PropertyPart::FunctionOrPrimitiveType;
    }

    if (std::find(std::begin(propertyKeywords), std::end(propertyKeywords), previousToken)
            != std::end(propertyKeywords)) {
        return PropertyPart::FunctionOrPrimitiveType;
    }
    return PropertyPart::Type;
}

void TokenInfo::invalidFileKind()
{
    const PropertyPart propPart = propertyPart(m_cxTranslationUnit, m_cxToken);

    switch (propPart) {
    case PropertyPart::None:
    case PropertyPart::Keyword:
        m_types.mainHighlightingType = HighlightingType::Invalid;
        return;
    case PropertyPart::Property:
        m_types.mainHighlightingType = HighlightingType::QtProperty;
        return;
    case PropertyPart::Type:
        m_types.mainHighlightingType = HighlightingType::Type;
        return;
    case PropertyPart::FunctionOrPrimitiveType:
        m_types.mainHighlightingType = HighlightingType::Function;
        return;
    }
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

void TokenInfo::evaluate()
{
    auto cxTokenKind = clang_getTokenKind(*m_cxToken);

    m_types = HighlightingTypes();

    switch (cxTokenKind) {
        case CXToken_Keyword:
            m_types.mainHighlightingType = highlightingTypeForKeyword(m_cxTranslationUnit,
                                                                      m_cxToken,
                                                                      m_originalCursor);
            break;
        case CXToken_Punctuation:
            m_types.mainHighlightingType = punctuationKind(m_originalCursor);
            break;
        case CXToken_Identifier:
            identifierKind(m_originalCursor, Recursion::FirstPass);
            break;
        case CXToken_Comment:
            m_types.mainHighlightingType = HighlightingType::Comment;
            break;
        case CXToken_Literal:
            m_types.mainHighlightingType = literalKind(m_originalCursor);
            break;
    }
}

} // namespace ClangBackEnd
