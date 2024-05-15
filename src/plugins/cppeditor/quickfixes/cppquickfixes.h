// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppquickfix.h"

#include <variant>

///
/// Adding New Quick Fixes
///
/// When adding new Quick Fixes, make sure that the doMatch() function is "cheap".
/// Otherwise, since the match() functions are also called to generate context menu
/// entries, the user might experience a delay opening the context menu.
///

namespace CppEditor {
namespace Internal {
using TypeOrExpr = std::variant<const CPlusPlus::ExpressionAST *, CPlusPlus::FullySpecifiedType>;

void createCppQuickFixes();
void destroyCppQuickFixes();

class ExtraRefactoringOperations : public CppQuickFixFactory
{
public:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override;
};

/*!
  Rewrite
    a op b

  As
    b flipop a

  Activates on: <= < > >= == != && ||
*/
class FlipLogicalOperands: public CppQuickFixFactory
{
public:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override;
};

/*!
  Rewrite
    a op b -> !(a invop b)
    (a op b) -> !(a invop b)
    !(a op b) -> (a invob b)

  Activates on: <= < > >= == !=
*/
class InverseLogicalComparison: public CppQuickFixFactory
{
public:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override;
};

/*!
  Rewrite
    !a && !b

  As
    !(a || b)

  Activates on: &&
*/
class RewriteLogicalAnd: public CppQuickFixFactory
{
public:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override;
};


/*!
  Base class for converting numeric literals between decimal, octal and hex.
  Does the base check for the specific ones and parses the number.

  Test cases:
    0xFA0Bu;
    0X856A;
    298.3;
    199;
    074;
    199L;
    074L;
    -199;
    -017;
    0783; // invalid octal
    0; // border case, allow only hex<->decimal

  Activates on: numeric literals
*/
class ConvertNumericLiteral: public CppQuickFixFactory
{
public:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override;
};

/*!
  Turns "an_example_symbol" into "anExampleSymbol" and
  "AN_EXAMPLE_SYMBOL" into "AnExampleSymbol".

  Activates on: identifiers
*/
class ConvertToCamelCase : public CppQuickFixFactory
{
public:
    ConvertToCamelCase(bool test = false) : CppQuickFixFactory(), m_test(test) {}

    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override;

private:
    const bool m_test;
};

/*!
  Replace
    if (Type name = foo()) {...}

  With
    Type name = foo();
    if (name) {...}

  Activates on: the name of the introduced variable
*/
class MoveDeclarationOutOfIf: public CppQuickFixFactory
{
public:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override;
};

/*!
  Replace
    while (Type name = foo()) {...}

  With
    Type name;
    while ((name = foo()) != 0) {...}

  Activates on: the name of the introduced variable
*/
class MoveDeclarationOutOfWhile: public CppQuickFixFactory
{
public:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override;
};

/*!
  Replace
     if (something && something_else) {
     }

  with
     if (something)
        if (something_else) {
        }
     }

  and
    if (something || something_else)
      x;

  with
    if (something)
      x;
    else if (something_else)
      x;

    Activates on: && or ||
*/
class SplitIfStatement: public CppQuickFixFactory
{
public:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override;
};

/*!
  Rewrite
    int *a, b;

  As
    int *a;
    int b;

  Activates on: the type or the variable names.
*/
class SplitSimpleDeclaration: public CppQuickFixFactory
{
public:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override;
};

/*!
  Add curly braces to a control statement that doesn't already contain a
  compound statement. I.e.

  if (a)
      b;
  becomes
  if (a) {
      b;
  }

  Activates on: the keyword
*/
class AddBracesToControlStatement : public CppQuickFixFactory
{
public:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override;
};

/*!
  Switches places of the parameter declaration under cursor
  with the next or the previous one in the parameter declaration list

  Activates on: parameter declarations
*/
class RearrangeParamDeclarationList : public CppQuickFixFactory
{
public:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override;
};

/*!
  Reformats a pointer, reference or rvalue reference type/declaration.

  Works also with selections (except when the cursor is not on any AST).

  Activates on: simple declarations, parameters and return types of function
                declarations and definitions, control flow statements (if,
                while, for, foreach) with declarations.
*/
class ReformatPointerDeclaration : public CppQuickFixFactory
{
public:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override;
};

/*!
  Adds missing case statements for "switch (enumVariable)"
 */
class CompleteSwitchCaseStatement: public CppQuickFixFactory
{
public:
    CompleteSwitchCaseStatement() { setClangdReplacement({12}); }

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override;
};

/*!
  Adds a declarations to a definition
 */
class InsertDeclFromDef: public CppQuickFixFactory
{
public:
    void doMatch(const CppQuickFixInterface &interface, TextEditor::QuickFixOperations &result) override;
};

class AddDeclarationForUndeclaredIdentifier : public CppQuickFixFactory
{
public:
    void doMatch(const CppQuickFixInterface &interface,
               TextEditor::QuickFixOperations &result) override;

#ifdef WITH_TESTS
    void setMembersOnly() { m_membersOnly = true; }
#endif

private:
    void collectOperations(const CppQuickFixInterface &interface,
                           TextEditor::QuickFixOperations &result);
    void handleCall(const CPlusPlus::CallAST *call, const CppQuickFixInterface &interface,
                    TextEditor::QuickFixOperations &result);

    // Returns whether to still do other checks.
    bool checkForMemberInitializer(const CppQuickFixInterface &interface,
                                   TextEditor::QuickFixOperations &result);

    void maybeAddMember(const CppQuickFixInterface &interface, CPlusPlus::Scope *scope,
                        const QByteArray &classTypeExpr, const TypeOrExpr &typeOrExpr,
                        const CPlusPlus::CallAST *call, TextEditor::QuickFixOperations &result);

    void maybeAddStaticMember(
        const CppQuickFixInterface &interface, const CPlusPlus::QualifiedNameAST *qualName,
        const TypeOrExpr &typeOrExpr, const CPlusPlus::CallAST *call,
        TextEditor::QuickFixOperations &result);

    bool m_membersOnly = false;
};

/*!
  Extracts the selected code and puts it to a function
 */
class ExtractFunction : public CppQuickFixFactory
{
public:
    using FunctionNameGetter = std::function<QString()>;

    ExtractFunction(FunctionNameGetter functionNameGetter = FunctionNameGetter());
    void doMatch(const CppQuickFixInterface &interface, TextEditor::QuickFixOperations &result) override;

private:
    FunctionNameGetter m_functionNameGetter; // For tests to avoid GUI pop-up.
};

/*!
  Extracts the selected constant and converts it to a parameter of the current function.

  Activates on numeric, bool, character, or string literal in the function body.
 */
class ExtractLiteralAsParameter : public CppQuickFixFactory
{
public:
    void doMatch(const CppQuickFixInterface &interface, TextEditor::QuickFixOperations &result) override;
};

/*!
  Converts the selected variable to a pointer if it is a stack variable or reference, or vice versa.

  Activates on variable declarations.
 */
class ConvertFromAndToPointer : public CppQuickFixFactory
{
public:
    void doMatch(const CppQuickFixInterface &interface, TextEditor::QuickFixOperations &result) override;
};

/*!
 Applies function signature changes
 */
class ApplyDeclDefLinkChanges: public CppQuickFixFactory
{
public:
    void doMatch(const CppQuickFixInterface &interface, TextEditor::QuickFixOperations &result) override;
};

/*!
  Assigns the return value of a function call or a new expression to a local variable
 */
class AssignToLocalVariable : public CppQuickFixFactory
{
public:
    void doMatch(const CppQuickFixInterface &interface, TextEditor::QuickFixOperations &result) override;
};

/*!
  Optimizes a for loop to avoid permanent condition check and forces to use preincrement
  or predecrement operators in the expression of the for loop.
 */
class OptimizeForLoop : public CppQuickFixFactory
{
public:
    void doMatch(const CppQuickFixInterface &interface, TextEditor::QuickFixOperations &result) override;
};

//! Converts C-style to C++-style comments and vice versa
class ConvertCommentStyle : public CppQuickFixFactory
{
private:
    void doMatch(const CppQuickFixInterface &interface,
               TextEditor::QuickFixOperations &result) override;
};

//! Moves function documentation between declaration and implementation.
class MoveFunctionComments : public CppQuickFixFactory
{
private:
    void doMatch(const CppQuickFixInterface &interface,
                 TextEditor::QuickFixOperations &result) override;
};

//! Converts a normal function call into a meta method invocation, if the functions is
//! marked as invokable.
class ConvertToMetaMethodCall : public CppQuickFixFactory
{
private:
    void doMatch(const CppQuickFixInterface &interface,
                 TextEditor::QuickFixOperations &result) override;
};

} // namespace Internal
} // namespace CppEditor
