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

#include "cursor.h"

#include "clangstring.h"
#include "sourcelocation.h"
#include "sourcerange.h"

#include "clangbackend_global.h"

#include <ostream>

namespace ClangBackEnd {

Cursor::Cursor()
    : m_cxCursor(clang_getNullCursor())
{
}

Cursor::Cursor(CXCursor cxCursor)
    : m_cxCursor(cxCursor)
{
}

bool Cursor::isNull() const
{
    return clang_Cursor_isNull(m_cxCursor);
}

bool Cursor::isValid() const
{
    return !clang_isInvalid(kind());
}

bool Cursor::isTranslationUnit() const
{
    return clang_isTranslationUnit(kind());
}

bool Cursor::isDefinition() const
{
    return clang_isCursorDefinition(m_cxCursor);
}

bool Cursor::isDynamicCall() const
{
    return clang_Cursor_isDynamicCall(m_cxCursor);
}

bool Cursor::isVirtualMethod() const
{
    return clang_CXXMethod_isVirtual(m_cxCursor);
}

bool Cursor::isPureVirtualMethod() const
{
    return clang_CXXMethod_isPureVirtual(m_cxCursor);
}

bool Cursor::isConstantMethod() const
{
    return clang_CXXMethod_isConst(m_cxCursor);
}

bool Cursor::isStaticMethod() const
{
    return clang_CXXMethod_isStatic(m_cxCursor);
}

bool Cursor::isCompoundType() const
{
    switch (kind()) {
        case CXCursor_ClassDecl:
        case CXCursor_StructDecl:
        case CXCursor_UnionDecl: return true;
        default: return false;
    }
}

bool Cursor::isDeclaration() const
{
    return clang_isDeclaration(kind());
}

bool Cursor::isInvalidDeclaration() const
{
#ifdef IS_INVALIDDECL_SUPPORTED
    return clang_isInvalidDeclaration(m_cxCursor);
#else
    return false;
#endif
}

bool Cursor::isLocalVariable() const
{
    switch (semanticParent().kind()) {
        case CXCursor_FunctionDecl:
        case CXCursor_CXXMethod:
        case CXCursor_Constructor:
        case CXCursor_Destructor:
        case CXCursor_ConversionFunction:
        case CXCursor_FunctionTemplate:
        case CXCursor_ObjCInstanceMethodDecl: return true;
        default:
            return false;
    }
}

bool Cursor::isReference() const
{
    return clang_isReference(kind());
}

bool Cursor::isExpression() const
{
    return clang_isExpression(kind());
}

bool Cursor::isFunctionLike() const
{
    const CXCursorKind k = kind();
    return k == CXCursor_FunctionDecl
        || k == CXCursor_CXXMethod
        || k == CXCursor_FunctionTemplate;
}

bool Cursor::isConstructorOrDestructor() const
{
    const CXCursorKind k = kind();
    return k == CXCursor_Constructor
        || k == CXCursor_Destructor;
}

bool Cursor::isTemplateLike() const
{
    switch (kind()) {
    case CXCursor_ClassTemplate:
    case CXCursor_ClassTemplatePartialSpecialization:
        return  true;
    case CXCursor_ClassDecl:
        return specializedCursorTemplate().isValid();
    default:
        return false;
    }

    Q_UNREACHABLE();
}

bool Cursor::isAnyTypeAlias() const
{
    const CXCursorKind k = kind();
    return k == CXCursor_TypeAliasDecl
        || k == CXCursor_TypedefDecl
        || k == CXCursor_TypeAliasTemplateDecl;
}

bool Cursor::hasFinalFunctionAttribute() const
{
    bool hasFinal = false;

    visit([&] (Cursor cursor, Cursor /*parent*/) {
        if (cursor.kind() == CXCursor_CXXFinalAttr) {
            hasFinal = true;
            return CXChildVisit_Break;
        } else {
            return CXChildVisit_Recurse;
        }
    });

    return hasFinal;
}

bool Cursor::hasFinalClassAttribute() const
{
      bool hasFinal = false;

      visit([&] (Cursor cursor, Cursor /*parent*/) {
          switch (cursor.kind()) {
              case CXCursor_CXXFinalAttr:
                  hasFinal = true;
                  return CXChildVisit_Break;
              case CXCursor_CXXMethod:
                  return CXChildVisit_Break;
              default:
                  return CXChildVisit_Recurse;
          }
      });

      return hasFinal;
}

bool Cursor::isUnexposed() const
{
    return clang_isUnexposed(kind());
}

bool Cursor::isAnonymous() const
{
    return clang_Cursor_isAnonymous(m_cxCursor);
}

ClangString Cursor::unifiedSymbolResolution() const
{
    return ClangString(clang_getCursorUSR(m_cxCursor));
}

ClangString Cursor::mangling() const
{
    return ClangString(clang_Cursor_getMangling(m_cxCursor));
}

ClangString Cursor::spelling() const
{
    return ClangString(clang_getCursorSpelling(m_cxCursor));
}

Utf8String Cursor::displayName() const
{
    Utf8String result = ClangString(clang_getCursorDisplayName(m_cxCursor));
    if (!result.hasContent() && isAnonymous())
        result = Utf8String("(anonymous)");
    return result;
}

ClangString Cursor::briefComment() const
{
    return ClangString(clang_Cursor_getBriefCommentText(m_cxCursor));
}

ClangString Cursor::rawComment() const
{
    return ClangString(clang_Cursor_getRawCommentText(m_cxCursor));
}

int Cursor::argumentCount() const
{
    return clang_Cursor_getNumArguments(m_cxCursor);
}

Type Cursor::type() const
{
    return clang_getCursorType(m_cxCursor);
}

Type Cursor::nonPointerTupe() const
{
    auto typeResult = type();

    if (typeResult.isPointer())
        typeResult = typeResult.pointeeType();

    return typeResult;
}

Type Cursor::enumType() const
{
    return clang_getEnumDeclIntegerType(m_cxCursor);
}

long long Cursor::enumConstantValue() const
{
    return clang_getEnumConstantDeclValue(m_cxCursor);
}

unsigned long long Cursor::enumConstantUnsignedValue() const
{
    return clang_getEnumConstantDeclUnsignedValue(m_cxCursor);
}

Cursor Cursor::specializedCursorTemplate() const
{
    return clang_getSpecializedCursorTemplate(m_cxCursor);
}

CXFile Cursor::includedFile() const
{
    return clang_getIncludedFile(m_cxCursor);
}

SourceLocation Cursor::sourceLocation() const
{
    return {cxTranslationUnit(), clang_getCursorLocation(m_cxCursor)};
}

CXSourceLocation Cursor::cxSourceLocation() const
{
    return clang_getCursorLocation(m_cxCursor);
}

SourceRange Cursor::sourceRange() const
{
    return {cxTranslationUnit(), clang_getCursorExtent(m_cxCursor)};
}

CXSourceRange Cursor::cxSourceRange() const
{
    return clang_getCursorExtent(m_cxCursor);
}

CXTranslationUnit Cursor::cxTranslationUnit() const
{
    return clang_Cursor_getTranslationUnit(m_cxCursor);
}

SourceRange Cursor::commentRange() const
{
    return {cxTranslationUnit(), clang_Cursor_getCommentRange(m_cxCursor)};
}

bool Cursor::hasSameSourceLocationAs(const Cursor &other) const
{
    return clang_equalLocations(clang_getCursorLocation(m_cxCursor),
                                clang_getCursorLocation(other.m_cxCursor));
}

Cursor Cursor::definition() const
{
    return clang_getCursorDefinition(m_cxCursor);
}

Cursor Cursor::canonical() const
{
    return clang_getCanonicalCursor(m_cxCursor);
}

Cursor Cursor::referenced() const
{
    return clang_getCursorReferenced(m_cxCursor);
}

Cursor Cursor::semanticParent() const
{
    return clang_getCursorSemanticParent(m_cxCursor);
}

Cursor Cursor::lexicalParent() const
{
    return clang_getCursorLexicalParent(m_cxCursor);
}

Cursor Cursor::functionBaseDeclaration() const
{
    auto functionBaseCursor = functionBase();

    if (functionBaseCursor.isValid())
        return functionBaseCursor.nonPointerTupe().canonical().declaration();
    else
        return semanticParent().semanticParent();
}

Cursor Cursor::functionBase() const
{
    Cursor functionBaseCursor;

    visit([&] (Cursor cursor, Cursor /*parentCursor*/) {
        switch (cursor.kind()) {
            case CXCursor_DeclRefExpr:
                functionBaseCursor = cursor;                                                       ;
                return CXChildVisit_Break;
            default:
                 return CXChildVisit_Recurse;
        }
    });

    return functionBaseCursor;
}

Type Cursor::resultType() const
{
    return clang_getResultType(type().m_cxType);
}

Cursor Cursor::argument(int index) const
{
    return clang_Cursor_getArgument(m_cxCursor, index);
}

unsigned Cursor::overloadedDeclarationsCount() const
{
    return clang_getNumOverloadedDecls(m_cxCursor);
}

Cursor Cursor::overloadedDeclaration(unsigned index) const
{
    return clang_getOverloadedDecl(m_cxCursor, index);
}

namespace {

bool isNotUnexposedLValueReference(const Cursor &argument, const Type &argumentType)
{
    return !(argument.isUnexposed() && argumentType.isLValueReference());
}

}

void Cursor::collectOutputArgumentRangesTo(std::vector<CXSourceRange> &outputArgumentRanges) const
{
    const Type callExpressionType = referenced().type();
    const int argumentCount = this->argumentCount();
    const std::size_t maxSize = std::size_t(std::max(0, argumentCount))
            + outputArgumentRanges.size();
    outputArgumentRanges.reserve(maxSize);

    for (int argumentIndex = 0; argumentIndex < argumentCount; ++argumentIndex) {
        const Cursor argument = this->argument(argumentIndex);
        const Type argumentType = callExpressionType.argument(argumentIndex);

        if (isNotUnexposedLValueReference(argument, argumentType)
                && argumentType.isOutputArgument()) {
            outputArgumentRanges.push_back(argument.cxSourceRange());
        }
    }
}

std::vector<CXSourceRange> Cursor::outputArgumentRanges() const
{
    std::vector<CXSourceRange> outputArgumentRanges;

    collectOutputArgumentRangesTo(outputArgumentRanges);

    return outputArgumentRanges;
}

CXCursorKind Cursor::kind() const
{
    return clang_getCursorKind(m_cxCursor);
}

CXCursor Cursor::cx() const
{
    return m_cxCursor;
}

StorageClass Cursor::storageClass() const
{
    CXCursor cursor = m_cxCursor;
    if (!isDeclaration())
        cursor = referenced().m_cxCursor;
    const CX_StorageClass cxStorageClass = clang_Cursor_getStorageClass(cursor);
    switch (cxStorageClass) {
    case CX_SC_Invalid:
    case CX_SC_OpenCLWorkGroupLocal:
        break;
    case CX_SC_None:
        return StorageClass::None;
    case CX_SC_Extern:
        return StorageClass::Extern;
    case CX_SC_Static:
        return StorageClass::Static;
    case CX_SC_PrivateExtern:
        return StorageClass::PrivateExtern;
    case CX_SC_Auto:
        return StorageClass::Auto;
    case CX_SC_Register:
        return StorageClass::Register;
    }
    return StorageClass::Invalid;
}

AccessSpecifier Cursor::accessSpecifier() const
{
    CXCursor cursor = m_cxCursor;
    if (!isDeclaration())
        cursor = referenced().m_cxCursor;
    const CX_CXXAccessSpecifier cxAccessSpecifier = clang_getCXXAccessSpecifier(cursor);
    switch (cxAccessSpecifier) {
    case CX_CXXInvalidAccessSpecifier:
        break;
    case CX_CXXPublic:
        return AccessSpecifier::Public;
    case CX_CXXProtected:
        return AccessSpecifier::Protected;
    case CX_CXXPrivate:
        return AccessSpecifier::Private;
    }
    return AccessSpecifier::Invalid;
}

bool operator==(const Cursor &first, const Cursor &second)
{
    return clang_equalCursors(first.m_cxCursor, second.m_cxCursor);
}

bool operator!=(const Cursor &first, const Cursor &second)
{
    return !(first == second);
}

std::ostream &operator<<(std::ostream &os, CXCursorKind cursorKind)
{
    ClangString cursorKindSpelling(clang_getCursorKindSpelling(cursorKind));
    return os << cursorKindSpelling.cString();
}

std::ostream &operator<<(std::ostream &os, const Cursor &cursor)
{
    if (cursor.isValid()) {
        ClangString cursorKindSpelling(clang_getCursorKindSpelling(cursor.kind()));
        os << cursorKindSpelling << " ";

        auto identifier = cursor.displayName();
        if (identifier.hasContent()) {
            os  << "\""
                << identifier
                << "\": ";
        }

        os << cursor.sourceLocation();
    } else {
        os << "Invalid cursor!";
    }

    return os;
}

} // namespace ClangBackEnd

