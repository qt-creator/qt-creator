/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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

SourceRange Cursor::sourceRange() const
{
    return clang_getCursorExtent(cxCursor);
}

SourceRange Cursor::commentRange() const
{
    return clang_Cursor_getCommentRange(cxCursor);
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
void collectOutputArguments(const Cursor &callExpression,
                            std::vector<Cursor> &outputArguments)
{
    auto callExpressionType = callExpression.referenced().type();
    auto argumentCount = callExpression.argumentCount();
    outputArguments.reserve(argumentCount);

    for (int argumentIndex = 0; argumentIndex < argumentCount; ++argumentIndex) {
        auto argument = callExpression.argument(argumentIndex);
        auto argumentType = callExpressionType.argument(argumentIndex);

        if (!argument.isUnexposed() && argumentType.isOutputParameter())
            outputArguments.push_back(callExpression.argument(argumentIndex));
    }
}
}

std::vector<Cursor> Cursor::outputArguments() const
{
    std::vector<Cursor> outputArguments;

    if (kind() == CXCursor_CallExpr)
        collectOutputArguments(*this, outputArguments);

    return outputArguments;
}

CXCursorKind Cursor::kind() const
{
    return clang_getCursorKind(cxCursor);
}

bool operator==(const Cursor &first, const Cursor &second)
{
    return clang_equalCursors(first.cxCursor, second.cxCursor);
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

