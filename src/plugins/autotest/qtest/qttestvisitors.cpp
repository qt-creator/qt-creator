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

#include "qttestvisitors.h"

#include <cplusplus/FullySpecifiedType.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/TypeOfExpression.h>
#include <cpptools/cppmodelmanager.h>
#include <utils/qtcassert.h>

namespace Autotest {
namespace Internal {

static QStringList specialFunctions({"initTestCase", "cleanupTestCase", "init", "cleanup"});

/************************** Cpp Test Symbol Visitor ***************************/

TestVisitor::TestVisitor(const QString &fullQualifiedClassName, const CPlusPlus::Snapshot &snapshot)
    : m_className(fullQualifiedClassName),
      m_snapshot(snapshot)
{
}

bool TestVisitor::visit(CPlusPlus::Class *symbol)
{
    const CPlusPlus::Overview o;
    CPlusPlus::LookupContext lc;

    unsigned count = symbol->memberCount();
    for (unsigned i = 0; i < count; ++i) {
        CPlusPlus::Symbol *member = symbol->memberAt(i);
        CPlusPlus::Type *type = member->type().type();

        const QString className = o.prettyName(lc.fullyQualifiedName(member->enclosingClass()));
        if (className != m_className)
            continue;

        m_valid = true;

        if (const auto func = type->asFunctionType()) {
            if (func->isSlot() && member->isPrivate()) {
                const QString name = o.prettyName(func->name());
                QtTestCodeLocationAndType locationAndType;

                CPlusPlus::Function *functionDefinition = m_symbolFinder.findMatchingDefinition(
                            func, m_snapshot, true);
                if (functionDefinition && functionDefinition->fileId()) {
                    locationAndType.m_name = QString::fromUtf8(functionDefinition->fileName());
                    locationAndType.m_line = functionDefinition->line();
                    locationAndType.m_column = functionDefinition->column() - 1;
                } else { // if we cannot find the definition use declaration as fallback
                    locationAndType.m_name = QString::fromUtf8(member->fileName());
                    locationAndType.m_line = member->line();
                    locationAndType.m_column = member->column() - 1;
                }
                if (specialFunctions.contains(name))
                    locationAndType.m_type = TestTreeItem::TestSpecialFunction;
                else if (name.endsWith("_data"))
                    locationAndType.m_type = TestTreeItem::TestDataFunction;
                else
                    locationAndType.m_type = TestTreeItem::TestFunctionOrSet;
                locationAndType.m_inherited = m_inherited;
                m_privSlots.insert(className + "::" + name, locationAndType);
            }
        }
        for (unsigned counter = 0, end = symbol->baseClassCount(); counter < end; ++counter) {
            if (CPlusPlus::BaseClass *base = symbol->baseClassAt(counter)) {
                const QString &baseClassName = o.prettyName(lc.fullyQualifiedName(base));
                if (baseClassName != "QObject")
                    m_baseClasses.insert(baseClassName);
            }
        }
    }
    return true;
}

/**************************** Cpp Test AST Visitor ****************************/

TestAstVisitor::TestAstVisitor(CPlusPlus::Document::Ptr doc, const CPlusPlus::Snapshot &snapshot)
    : ASTVisitor(doc->translationUnit()),
      m_currentDoc(doc),
      m_snapshot(snapshot)
{
}

bool TestAstVisitor::visit(CPlusPlus::CallAST *ast)
{
    if (!m_currentScope || m_currentDoc.isNull())
        return false;

    if (const auto expressionAST = ast->base_expression) {
        if (const auto idExpressionAST = expressionAST->asIdExpression()) {
            if (const auto qualifiedNameAST = idExpressionAST->name->asQualifiedName()) {
                const CPlusPlus::Overview o;
                const QString prettyName = o.prettyName(qualifiedNameAST->name);
                if (prettyName == "QTest::qExec") {
                    if (const auto expressionListAST = ast->expression_list) {
                        // first argument is the one we need
                        if (const auto argumentExpressionAST = expressionListAST->value) {
                            CPlusPlus::TypeOfExpression toe;
                            toe.init(m_currentDoc, m_snapshot);
                            QList<CPlusPlus::LookupItem> toeItems
                                    = toe(argumentExpressionAST, m_currentDoc, m_currentScope);

                            if (toeItems.size()) {
                                if (const auto pointerType = toeItems.first().type()->asPointerType())
                                    m_className = o.prettyType(pointerType->elementType());
                            }
                        }
                    }
                }
            }
        }
    }
    return false;
}

bool TestAstVisitor::visit(CPlusPlus::CompoundStatementAST *ast)
{
    if (!ast || !ast->symbol) {
        m_currentScope = nullptr;
        return false;
    }
    m_currentScope = ast->symbol->asScope();
    return true;
}

/********************** Test Data Function AST Visitor ************************/

TestDataFunctionVisitor::TestDataFunctionVisitor(CPlusPlus::Document::Ptr doc)
    : CPlusPlus::ASTVisitor(doc->translationUnit()),
      m_currentDoc(doc)
{
}

bool TestDataFunctionVisitor::visit(CPlusPlus::UsingDirectiveAST *ast)
{
    if (auto nameAST = ast->name) {
        if (m_overview.prettyName(nameAST->name) == "QTest") {
            m_insideUsingQTest = true;
            // we need the surrounding AST depth as using directive is an AST itself
            m_insideUsingQTestDepth = m_currentAstDepth - 1;
        }
    }
    return true;
}

bool TestDataFunctionVisitor::visit(CPlusPlus::FunctionDefinitionAST *ast)
{
    if (ast->declarator) {
        CPlusPlus::DeclaratorIdAST *id = ast->declarator->core_declarator->asDeclaratorId();
        if (!id || !ast->symbol || ast->symbol->argumentCount() != 0)
            return false;

        CPlusPlus::LookupContext lc;
        const QString prettyName = m_overview.prettyName(lc.fullyQualifiedName(ast->symbol));
        // do not handle functions that aren't real test data functions
        if (!prettyName.endsWith("_data"))
            return false;

        m_currentFunction = prettyName.left(prettyName.size() - 5);
        m_currentTags.clear();
        return true;
    }

    return false;
}

QString TestDataFunctionVisitor::extractNameFromAST(CPlusPlus::StringLiteralAST *ast, bool *ok) const
{
    auto token = m_currentDoc->translationUnit()->tokenAt(ast->literal_token);
    if (!token.isStringLiteral()) {
        *ok = false;
        return QString();
    }
    *ok = true;
    QString name = QString::fromUtf8(token.spell());
    if (ast->next) {
        CPlusPlus::StringLiteralAST *current = ast;
        do {
            auto nextToken = m_currentDoc->translationUnit()->tokenAt(current->next->literal_token);
            name.append(QString::fromUtf8(nextToken.spell()));
            current = current->next;
        } while (current->next);
    }
    return name;
}

bool TestDataFunctionVisitor::visit(CPlusPlus::CallAST *ast)
{
    if (m_currentFunction.isEmpty())
        return true;

    unsigned firstToken;
    if (newRowCallFound(ast, &firstToken)) {
        if (const auto expressionListAST = ast->expression_list) {
            // first argument is the one we need
            if (const auto argumentExpressionAST = expressionListAST->value) {
                if (const auto stringLiteral = argumentExpressionAST->asStringLiteral()) {
                    bool ok = false;
                    QString name = extractNameFromAST(stringLiteral, &ok);
                    if (ok) {
                        unsigned line = 0;
                        unsigned column = 0;
                        m_currentDoc->translationUnit()->getTokenStartPosition(
                                    firstToken, &line, &column);
                        QtTestCodeLocationAndType locationAndType;
                        locationAndType.m_name = name;
                        locationAndType.m_column = column - 1;
                        locationAndType.m_line = line;
                        locationAndType.m_type = TestTreeItem::TestDataTag;
                        m_currentTags.append(locationAndType);
                    }
                }
            }
        }
    }
    return true;
}

bool TestDataFunctionVisitor::preVisit(CPlusPlus::AST *)
{
    ++m_currentAstDepth;
    return true;
}

void TestDataFunctionVisitor::postVisit(CPlusPlus::AST *ast)
{
    --m_currentAstDepth;
    m_insideUsingQTest &= m_currentAstDepth >= m_insideUsingQTestDepth;

    if (!ast->asFunctionDefinition())
        return;

    if (!m_currentFunction.isEmpty() && !m_currentTags.isEmpty())
        m_dataTags.insert(m_currentFunction, m_currentTags);

    m_currentFunction.clear();
    m_currentTags.clear();
}

bool TestDataFunctionVisitor::newRowCallFound(CPlusPlus::CallAST *ast, unsigned *firstToken) const
{
    QTC_ASSERT(firstToken, return false);

    if (!ast->base_expression)
        return false;

    bool found = false;

    if (const CPlusPlus::IdExpressionAST *exp = ast->base_expression->asIdExpression()) {
        if (!exp->name)
            return false;

        if (const auto qualifiedNameAST = exp->name->asQualifiedName()) {
            found = m_overview.prettyName(qualifiedNameAST->name) == "QTest::newRow";
            *firstToken = qualifiedNameAST->firstToken();
        } else if (m_insideUsingQTest) {
            found = m_overview.prettyName(exp->name->name) == "newRow";
            *firstToken = exp->name->firstToken();
        }
    }
    return found;
}

} // namespace Internal
} // namespace Autotest
