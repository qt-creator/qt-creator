/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPQUICKFIXES_H
#define CPPQUICKFIXES_H

#include "cppquickfix.h"

#include <cpptools/cpprefactoringchanges.h>

#include <extensionsystem/iplugin.h>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QByteArray;
class QCheckBox;
class QComboBox;
class QDialogButtonBox;
class QStandardItem;
class QSortFilterProxyModel;
class QStandardItemModel;
class QString;
class QTreeView;
template <class> class QList;
QT_END_NAMESPACE

using namespace CppTools;
using namespace CPlusPlus;
using namespace TextEditor;

///
/// Adding New Quick Fixes
///
/// When adding new Quick Fixes, make sure that the match() function is "cheap".
/// Otherwise, since the match() functions are also called to generate context menu
/// entries, the user might experience a delay opening the context menu.
///

namespace CppEditor {
namespace Internal {

void registerQuickFixes(ExtensionSystem::IPlugin *plugIn);

/*!
  Adds an include for an undefined identifier.

  Activates on: the undefined identifier
*/
class AddIncludeForUndefinedIdentifier : public CppQuickFixFactory
{
public:
    void match(const CppQuickFixInterface &interface, QuickFixOperations &result);
};

// Exposed for tests
class AddIncludeForUndefinedIdentifierOp: public CppQuickFixOperation
{
public:
    AddIncludeForUndefinedIdentifierOp(const CppQuickFixInterface &interface, int priority,
                                       const QString &include);
    void perform();

private:
    QString m_include;
};

/*!
  Can be triggered on a class forward declaration to add the matching #include.

  Activates on: the name of a forward-declared class or struct
*/
class AddIncludeForForwardDeclaration: public CppQuickFixFactory
{
public:
    void match(const CppQuickFixInterface &interface, QuickFixOperations &result);
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
    void match(const CppQuickFixInterface &interface, QuickFixOperations &result);
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
    void match(const CppQuickFixInterface &interface, QuickFixOperations &result);
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
    void match(const CppQuickFixInterface &interface, QuickFixOperations &result);

private:
    ASTMatcher matcher;
};

/*!
  Replace
     "abcd"
     QLatin1String("abcd")
     QLatin1Literal("abcd")

  With
     @"abcd"

  Activates on: the string literal, if the file type is a Objective-C(++) file.
*/
class ConvertCStringToNSString: public CppQuickFixFactory
{
public:
    void match(const CppQuickFixInterface &interface, QuickFixOperations &result);
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
    void match(const CppQuickFixInterface &interface, QuickFixOperations &result);
};

/*!
  Replace
    "abcd"

  With
    tr("abcd") or
    QCoreApplication::translate("CONTEXT", "abcd") or
    QT_TRANSLATE_NOOP("GLOBAL", "abcd")

  depending on what is available.

  Activates on: the string literal
*/
class TranslateStringLiteral: public CppQuickFixFactory
{
public:
    void match(const CppQuickFixInterface &interface, QuickFixOperations &result);
};

/*!
  Replace
    "abcd"  -> QLatin1String("abcd")
    @"abcd" -> QLatin1String("abcd") (Objective C)
    'a'     -> QLatin1Char('a')
    'a'     -> "a"
    "a"     -> 'a' or QLatin1Char('a') (Single character string constants)
    "\n"    -> '\n', QLatin1Char('\n')

  Except if they are already enclosed in
    QLatin1Char, QT_TRANSLATE_NOOP, tr,
    trUtf8, QLatin1Literal, QLatin1String

  Activates on: the string or character literal
*/

class WrapStringLiteral: public CppQuickFixFactory
{
public:
    enum ActionFlags {
        EncloseInQLatin1CharAction = 0x1,
        EncloseInQLatin1StringAction = 0x2,
        EncloseInQStringLiteralAction = 0x4,
        EncloseActionMask = EncloseInQLatin1CharAction
            | EncloseInQLatin1StringAction | EncloseInQStringLiteralAction,
        TranslateTrAction = 0x8,
        TranslateQCoreApplicationAction = 0x10,
        TranslateNoopAction = 0x20,
        TranslationMask = TranslateTrAction
            | TranslateQCoreApplicationAction | TranslateNoopAction,
        RemoveObjectiveCAction = 0x40,
        ConvertEscapeSequencesToCharAction = 0x100,
        ConvertEscapeSequencesToStringAction = 0x200,
        SingleQuoteAction = 0x400,
        DoubleQuoteAction = 0x800
    };

    enum Type { TypeString, TypeObjCString, TypeChar, TypeNone };

    void match(const CppQuickFixInterface &interface, QuickFixOperations &result);

    static QString replacement(unsigned actions);
    static QByteArray stringToCharEscapeSequences(const QByteArray &content);
    static QByteArray charToStringEscapeSequences(const QByteArray &content);

    static ExpressionAST *analyze(const QList<AST *> &path, const CppRefactoringFilePtr &file,
                                  Type *type,
                                  QByteArray *enclosingFunction = 0,
                                  CallAST **enclosingFunctionCall = 0);

private:
    static QString msgQtStringLiteralDescription(const QString &replacement, int qtVersion);
    static QString msgQtStringLiteralDescription(const QString &replacement);
};

/*!
  Turns "an_example_symbol" into "anExampleSymbol" and
  "AN_EXAMPLE_SYMBOL" into "AnExampleSymbol".

  Activates on: identifiers
*/
class ConvertToCamelCase : public CppQuickFixFactory
{
public:
    void match(const CppQuickFixInterface &interface, QuickFixOperations &result);
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
    void match(const CppQuickFixInterface &interface, QuickFixOperations &result);
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
    void match(const CppQuickFixInterface &interface, QuickFixOperations &result);
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
    void match(const CppQuickFixInterface &interface, QuickFixOperations &result);
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
    void match(const CppQuickFixInterface &interface, QuickFixOperations &result);

private:
    static bool checkDeclaration(SimpleDeclarationAST *declaration);
};

/*!
  Rewrites
    a = foo();

  As
    Type a = foo();

  Where Type is the return type of foo()

  Activates on: the assignee, if the type of the right-hand side of the assignment is known.
*/
class AddLocalDeclaration: public CppQuickFixFactory
{
public:
    void match(const CppQuickFixInterface &interface, QuickFixOperations &result);
};

/*!
  Add curly braces to a if statement that doesn't already contain a
  compound statement. I.e.

  if (a)
      b;
  becomes
  if (a) {
      b;
  }

  Activates on: the if
*/
class AddBracesToIf: public CppQuickFixFactory
{
public:
    void match(const CppQuickFixInterface &interface, QuickFixOperations &result);
};

/*!
  Switches places of the parameter declaration under cursor
  with the next or the previous one in the parameter declaration list

  Activates on: parameter declarations
*/
class RearrangeParamDeclarationList : public CppQuickFixFactory
{
public:
    void match(const CppQuickFixInterface &interface, QuickFixOperations &result);
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
    void match(const CppQuickFixInterface &interface, QuickFixOperations &result);
};

/*!
  Adds missing case statements for "switch (enumVariable)"
 */
class CompleteSwitchCaseStatement: public CppQuickFixFactory
{
public:
    void match(const CppQuickFixInterface &interface, QuickFixOperations &result);
};

/*!
  Adds a declarations to a definition
 */
class InsertDeclFromDef: public CppQuickFixFactory
{
public:
    void match(const CppQuickFixInterface &interface, TextEditor::QuickFixOperations &result);
};

/*!
  Adds a definition for a declaration.
 */
class InsertDefFromDecl: public CppQuickFixFactory
{
public:
    void match(const CppQuickFixInterface &interface, TextEditor::QuickFixOperations &result);
};

/*!
  Extracts the selected code and puts it to a function
 */
class ExtractFunction : public CppQuickFixFactory
{
public:
    void match(const CppQuickFixInterface &interface, TextEditor::QuickFixOperations &result);
};

/*!
  Adds getter and setter functions for a member variable
 */
class GenerateGetterSetter : public CppQuickFixFactory
{
public:
    void match(const CppQuickFixInterface &interface, TextEditor::QuickFixOperations &result);
};

/*!
  Adds missing members for a Q_PROPERTY
 */
class InsertQtPropertyMembers : public CppQuickFixFactory
{
public:
    void match(const CppQuickFixInterface &interface, TextEditor::QuickFixOperations &result);
};

/*!
 Applies function signature changes
 */
class ApplyDeclDefLinkChanges: public CppQuickFixFactory
{
public:
    void match(const CppQuickFixInterface &interface, TextEditor::QuickFixOperations &result);
};

/*!
 Moves the definition of a member function outside the class or moves the definition of a member
 function or a normal function to the implementation file.
 */
class MoveFuncDefOutside: public CppQuickFixFactory
{
public:
    void match(const CppQuickFixInterface &interface, TextEditor::QuickFixOperations &result);
};

/*!
 Moves the definition of a function to its declaration.
 */
class MoveFuncDefToDecl: public CppQuickFixFactory
{
public:
    void match(const CppQuickFixInterface &interface, TextEditor::QuickFixOperations &result);
};

/*!
  Assigns the return value of a function call or a new expression to a local variable
 */
class AssignToLocalVariable : public CppQuickFixFactory
{
public:
    void match(const CppQuickFixInterface &interface, TextEditor::QuickFixOperations &result);
};

/*!
 Insert (pure) virtual functions of a base class.
 Exposed for tests.
 */
class InsertVirtualMethodsDialog : public QDialog
{
    Q_OBJECT
public:
    enum CustomItemRoles {
        ClassOrFunction = Qt::UserRole + 1,
        Implemented = Qt::UserRole + 2,
        PureVirtual = Qt::UserRole + 3,
        AccessSpec = Qt::UserRole + 4
    };

    enum ImplementationMode {
        ModeOnlyDeclarations = 0x00000001,
        ModeInsideClass = 0x00000002,
        ModeOutsideClass = 0x00000004,
        ModeImplementationFile = 0x00000008
    };

    InsertVirtualMethodsDialog(QWidget *parent = 0);
    void initGui();
    void initData();
    virtual ImplementationMode implementationMode() const;
    void setImplementationsMode(ImplementationMode mode);
    virtual bool insertKeywordVirtual() const;
    void setInsertKeywordVirtual(bool insert);
    void setHasImplementationFile(bool file);
    void setHasReimplementedFunctions(bool functions);
    bool hideReimplementedFunctions() const;
    virtual bool gather();

private slots:
    void updateCheckBoxes(QStandardItem *item);
    void setHideReimplementedFunctions(bool hide);

private:
    QTreeView *m_view;
    QCheckBox *m_hideReimplementedFunctions;
    QComboBox *m_insertMode;
    QCheckBox *m_virtualKeyword;
    QDialogButtonBox *m_buttons;
    QList<bool> m_expansionStateNormal;
    QList<bool> m_expansionStateReimp;
    bool m_hasImplementationFile;
    bool m_hasReimplementedFunctions;

    void saveExpansionState();
    void restoreExpansionState();

protected:
    ImplementationMode m_implementationMode;
    bool m_insertKeywordVirtual;

public:
    QStandardItemModel *classFunctionModel;
    QSortFilterProxyModel *classFunctionFilterModel;
};

class InsertVirtualMethods: public CppQuickFixFactory
{
    Q_OBJECT
public:
    InsertVirtualMethods(InsertVirtualMethodsDialog *dialog = new InsertVirtualMethodsDialog);
    ~InsertVirtualMethods();
    void match(const CppQuickFixInterface &interface, TextEditor::QuickFixOperations &result);

private:
    InsertVirtualMethodsDialog *m_dialog;
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPQUICKFIXES_H
