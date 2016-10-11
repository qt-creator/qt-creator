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

#include <ostream>

namespace ClangBackEnd {

Cursor::Cursor()
    : cxCursor(clang_getNullCursor())
{
}

Cursor::Cursor(CXCursor cxCursor)
    : cxCursor(cxCursor)
{
}

bool Cursor::isNull() const
{
    return clang_Cursor_isNull(cxCursor);
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
    return clang_isCursorDefinition(cxCursor);
}

bool Cursor::isDynamicCall() const
{
    return clang_Cursor_isDynamicCall(cxCursor);
}

bool Cursor::isVirtualMethod() const
{
    return clang_CXXMethod_isVirtual(cxCursor);
}

bool Cursor::isPureVirtualMethod() const
{
    return clang_CXXMethod_isPureVirtual(cxCursor);
}

bool Cursor::isConstantMethod() const
{
    return clang_CXXMethod_isConst(cxCursor);
}

bool Cursor::isStaticMethod() const
{
    return clang_CXXMethod_isStatic(cxCursor);
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

Utf8String Cursor::unifiedSymbolResolution() const
{
    return ClangString(clang_getCursorUSR(cxCursor));
}

Utf8String Cursor::mangling() const
{
    return ClangString(clang_Cursor_getMangling(cxCursor));
}

ClangString Cursor::spelling() const
{
    return ClangString(clang_getCursorSpelling(cxCursor));
}

Utf8String Cursor::displayName() const
{
    return ClangString(clang_getCursorDisplayName(cxCursor));
}

Utf8String Cursor::briefComment() const
{
    return ClangString(clang_Cursor_getBriefCommentText(cxCursor));
}

Utf8String Cursor::rawComment() const
{
    return ClangString(clang_Cursor_getRawCommentText(cxCursor));
}

int Cursor::argumentCount() const
{
    return clang_Cursor_getNumArguments(cxCursor);
}

Type Cursor::type() const
{
    return clang_getCursorType(cxCursor);
}

Type Cursor::nonPointerTupe() const
{
    auto typeResult = type();

    if (typeResult.isPointer())
        typeResult = typeResult.pointeeType();

    return typeResult;
}

SourceLocation Cursor::sourceLocation() const
{
    return clang_getCursorLocation(cxCursor);
}

CXSourceLocation Cursor::cxSourceLocation() const
{
    return clang_getCursorLocation(cxCursor);
}

SourceRange Cursor::sourceRange() const
{
    return clang_getCursorExtent(cxCursor);
}

CXSourceRange Cursor::cxSourceRange() const
{
    return clang_getCursorExtent(cxCursor);
}

SourceRange Cursor::commentRange() const
{
    return clang_Cursor_getCommentRange(cxCursor);
}

bool Cursor::hasSameSourceLocationAs(const Cursor &other) const
{
    return clang_equalLocations(clang_getCursorLocation(cxCursor),
                                clang_getCursorLocation(other.cxCursor));
}

Cursor Cursor::definition() const
{
    return clang_getCursorDefinition(cxCursor);
}

Cursor Cursor::canonical() const
{
    return clang_getCanonicalCursor(cxCursor);
}

Cursor Cursor::referenced() const
{
    return clang_getCursorReferenced(cxCursor);
}

Cursor Cursor::semanticParent() const
{
    return clang_getCursorSemanticParent(cxCursor);
}

Cursor Cursor::lexicalParent() const
{
    return clang_getCursorLexicalParent(cxCursor);
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

Cursor Cursor::argument(int index) const
{
    return clang_Cursor_getArgument(cxCursor, index);
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
    return clang_getCursorKind(cxCursor);
}

bool operator==(const Cursor &first, const Cursor &second)
{
    return clang_equalCursors(first.cxCursor, second.cxCursor);
}

bool operator!=(const Cursor &first, const Cursor &second)
{
    return !(first == second);
}

void PrintTo(CXCursorKind cursorKind, ::std::ostream *os)
{
    ClangString cursorKindSpelling(clang_getCursorKindSpelling(cursorKind));
    *os << cursorKindSpelling.cString();
}

void PrintTo(const Cursor &cursor, ::std::ostream*os)
{
    if (cursor.isValid()) {
        ClangString cursorKindSpelling(clang_getCursorKindSpelling(cursor.kind()));
        *os << cursorKindSpelling.cString() << " ";

        auto identifier = cursor.displayName();
        if (identifier.hasContent()) {
            *os  << "\""
                 << identifier.constData()
                 << "\": ";
        }

        PrintTo(cursor.sourceLocation(), os);
    } else {
        *os << "Invalid cursor!";
    }
}

} // namespace ClangBackEnd

