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

#include "cppcompleteswitch.h"
#include "cppquickfixassistant.h"

#include <cplusplus/Overview.h>
#include <cplusplus/TypeOfExpression.h>
#include <cpptools/cpprefactoringchanges.h>
#include <utils/changeset.h>

#include <AST.h>
#include <ASTVisitor.h>
#include <CoreTypes.h>
#include <Symbols.h>

#include <QApplication>

using namespace CPlusPlus;
using namespace CppEditor;
using namespace CppEditor::Internal;
using namespace CppTools;
using namespace TextEditor;
using namespace Utils;

namespace {

class CaseStatementCollector : public ASTVisitor
{
public:
    CaseStatementCollector(Document::Ptr document, const Snapshot &snapshot,
                           Scope *scope)
        : ASTVisitor(document->translationUnit()),
        document(document),
        scope(scope)
    {
        typeOfExpression.init(document, snapshot);
    }

    QStringList operator ()(AST *ast)
    {
        values.clear();
        foundCaseStatementLevel = false;
        accept(ast);
        return values;
    }

    bool preVisit(AST *ast) {
        if (CaseStatementAST *cs = ast->asCaseStatement()) {
            foundCaseStatementLevel = true;
            if (ExpressionAST *expression = cs->expression->asIdExpression()) {
                QList<LookupItem> candidates = typeOfExpression(expression,
                                                                document,
                                                                scope);
                if (!candidates .isEmpty() && candidates.first().declaration()) {
                    Symbol *decl = candidates.first().declaration();
                    values << prettyPrint.prettyName(LookupContext::fullyQualifiedName(decl));
                }
            }
            return true;
        } else if (foundCaseStatementLevel) {
            return false;
        }
        return true;
    }

    Overview prettyPrint;
    bool foundCaseStatementLevel;
    QStringList values;
    TypeOfExpression typeOfExpression;
    Document::Ptr document;
    Scope *scope;
};

class Operation: public CppQuickFixOperation
{
public:
    Operation(const QSharedPointer<const CppEditor::Internal::CppQuickFixAssistInterface> &interface,
              int priority,
              CompoundStatementAST *compoundStatement,
              const QStringList &values)
        : CppQuickFixOperation(interface, priority)
        , compoundStatement(compoundStatement)
        , values(values)
    {
        setDescription(QApplication::translate("CppTools::QuickFix",
                                               "Complete Switch Statement"));
    }

    void perform()
    {
        CppRefactoringChanges refactoring(snapshot());
        CppRefactoringFilePtr currentFile = refactoring.file(fileName());

        ChangeSet changes;
        int start = currentFile->endOf(compoundStatement->lbrace_token);
        changes.insert(start, QLatin1String("\ncase ")
                       + values.join(QLatin1String(":\nbreak;\ncase "))
                       + QLatin1String(":\nbreak;"));
        currentFile->setChangeSet(changes);
        currentFile->appendIndentRange(currentFile->range(compoundStatement));
        currentFile->apply();
    }

    CompoundStatementAST *compoundStatement;
    QStringList values;
};

} // end of anonymous namespace

static Enum *findEnum(const QList<LookupItem> &results, const LookupContext &ctxt)
{
    foreach (const LookupItem &result, results) {
        const FullySpecifiedType fst = result.type();

        Type *type = result.declaration() ? result.declaration()->type().type()
                                          : fst.type();

        if (!type)
            continue;
        if (Enum *e = type->asEnumType())
            return e;
        if (const NamedType *namedType = type->asNamedType()) {
            const QList<LookupItem> candidates =
                    ctxt.lookup(namedType->name(), result.scope());
            return findEnum(candidates, ctxt);
        }
    }

    return 0;
}

static Enum *conditionEnum(const CppQuickFixInterface &interface, SwitchStatementAST *statement)
{
    Block *block = statement->symbol;
    Scope *scope = interface->semanticInfo().doc->scopeAt(block->line(), block->column());
    TypeOfExpression typeOfExpression;
    typeOfExpression.init(interface->semanticInfo().doc, interface->snapshot());
    const QList<LookupItem> results = typeOfExpression(statement->condition,
                                                       interface->semanticInfo().doc,
                                                       scope);

    return findEnum(results, typeOfExpression.context());
}

void CompleteSwitchCaseStatement::match(const CppQuickFixInterface &interface, QuickFixOperations &result)
{
    const QList<AST *> &path = interface->path();

    if (path.isEmpty())
        return;

    // look for switch statement
    for (int depth = path.size() - 1; depth >= 0; --depth) {
        AST *ast = path.at(depth);
        SwitchStatementAST *switchStatement = ast->asSwitchStatement();
        if (switchStatement) {
            if (!interface->isCursorOn(switchStatement->switch_token) || !switchStatement->statement)
                return;
            CompoundStatementAST *compoundStatement = switchStatement->statement->asCompoundStatement();
            if (!compoundStatement) // we ignore pathologic case "switch (t) case A: ;"
                return;
            // look if the condition's type is an enum
            if (Enum *e = conditionEnum(interface, switchStatement)) {
                // check the possible enum values
                QStringList values;
                Overview prettyPrint;
                for (unsigned i = 0; i < e->memberCount(); ++i) {
                    if (Declaration *decl = e->memberAt(i)->asDeclaration())
                        values << prettyPrint.prettyName(LookupContext::fullyQualifiedName(decl));
                }
                // Get the used values
                Block *block = switchStatement->symbol;
                CaseStatementCollector caseValues(interface->semanticInfo().doc, interface->snapshot(),
                                                  interface->semanticInfo().doc->scopeAt(block->line(), block->column()));
                QStringList usedValues = caseValues(switchStatement);
                // save the values that would be added
                foreach (const QString &usedValue, usedValues)
                    values.removeAll(usedValue);
                if (!values.isEmpty())
                    result.append(CppQuickFixOperation::Ptr(new Operation(interface, depth, compoundStatement, values)));
                return;
            }

            return;
        }
    }
}
