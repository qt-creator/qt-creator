// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qttestvisitors.h"

#include <cplusplus/FullySpecifiedType.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/TypeOfExpression.h>
#include <cppeditor/cppmodelmanager.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

using namespace CPlusPlus;
using namespace Utils;

namespace Autotest::Internal {

static QStringList specialFunctions({"initTestCase", "cleanupTestCase", "init", "cleanup"});

/************************** Cpp Test Symbol Visitor ***************************/

TestVisitor::TestVisitor(const QString &fullQualifiedClassName, const Snapshot &snapshot)
    : m_className(fullQualifiedClassName),
      m_snapshot(snapshot)
{
}

bool TestVisitor::visit(Class *symbol)
{
    const Overview o;
    LookupContext lc;

    int count = symbol->memberCount();
    for (int i = 0; i < count; ++i) {
        Symbol *member = symbol->memberAt(i);
        Type *type = member->type().type();

        const QString className = o.prettyName(LookupContext::fullyQualifiedName(
                                                   member->enclosingClass()));
        if (className != m_className)
            continue;

        m_valid = true;

        if (const auto func = type->asFunctionType()) {
            if (func->isSlot() && member->isPrivate()) {
                const QString name = o.prettyName(func->name());
                QtTestCodeLocationAndType locationAndType;

                if (name.endsWith("_data")) {
                    // costly.. but we need at least the correct entry for finding data tags
                    Function *functionDefinition = m_symbolFinder.findMatchingDefinition(
                                func, m_snapshot, true);
                    if (functionDefinition && functionDefinition->fileId())
                        member = functionDefinition;
                }
                locationAndType.m_filePath = FilePath::fromUtf8(member->fileName());
                locationAndType.m_line = member->line();
                locationAndType.m_column = member->column() - 1;
                if (specialFunctions.contains(name))
                    locationAndType.m_type = TestTreeItem::TestSpecialFunction;
                else if (name.endsWith("_data"))
                    locationAndType.m_type = TestTreeItem::TestDataFunction;
                else
                    locationAndType.m_type = TestTreeItem::TestFunction;
                locationAndType.m_inherited = m_inherited;
                locationAndType.m_name = className + "::" + name;
                m_privSlots.insert(locationAndType.m_name, locationAndType);
            }
        }
        for (int counter = 0, end = symbol->baseClassCount(); counter < end; ++counter) {
            if (BaseClass *base = symbol->baseClassAt(counter)) {
                const QString &baseClassName = o.prettyName(LookupContext::fullyQualifiedName(base));
                if (baseClassName != "QObject")
                    m_baseClasses.insert(baseClassName);
            }
        }
    }
    return true;
}

/**************************** Cpp Test AST Visitor ****************************/

TestAstVisitor::TestAstVisitor(Document::Ptr doc, const Snapshot &snapshot)
    : ASTVisitor(doc->translationUnit()),
      m_currentDoc(doc),
      m_snapshot(snapshot)
{
}

bool TestAstVisitor::visit(CallAST *ast)
{
    if (!m_currentScope || m_currentDoc.isNull())
        return false;

    if (const auto expressionAST = ast->base_expression) {
        if (const auto idExpressionAST = expressionAST->asIdExpression()) {
            if (const auto qualifiedNameAST = idExpressionAST->name->asQualifiedName()) {
                const Overview o;
                const QString prettyName = o.prettyName(qualifiedNameAST->name);
                if (prettyName == "QTest::qExec") {
                    if (const auto expressionListAST = ast->expression_list) {
                        // first argument is the one we need
                        if (const auto argumentExpressionAST = expressionListAST->value) {
                            TypeOfExpression toe;
                            toe.init(m_currentDoc, m_snapshot);
                            QList<LookupItem> toeItems
                                    = toe(argumentExpressionAST, m_currentDoc, m_currentScope);

                            if (!toeItems.isEmpty()) {
                                if (const auto pointerType = toeItems.first().type()->asPointerType())
                                    m_classNames.append(o.prettyType(pointerType->elementType()));
                            }
                        }
                    }
                }
            }
        }
    }
    return true;
}

bool TestAstVisitor::visit(CompoundStatementAST *ast)
{
    if (!ast || !ast->symbol) {
        m_currentScope = nullptr;
        return false;
    }
    m_currentScope = ast->symbol->asScope();
    return true;
}

TestCases TestAstVisitor::testCases() const
{
    const bool multi = m_classNames.size() > 1;
    return Utils::transform(m_classNames, [multi](const QString &className) {
        return TestCase{className, multi};
    });
}

/********************** Test Data Function AST Visitor ************************/

TestDataFunctionVisitor::TestDataFunctionVisitor(Document::Ptr doc)
    : ASTVisitor(doc->translationUnit()),
      m_currentDoc(doc)
{
}

bool TestDataFunctionVisitor::visit(UsingDirectiveAST *ast)
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

bool TestDataFunctionVisitor::visit(FunctionDefinitionAST *ast)
{
    if (ast->declarator) {
        DeclaratorIdAST *id = ast->declarator->core_declarator->asDeclaratorId();
        if (!id || !ast->symbol || ast->symbol->argumentCount() != 0)
            return false;

        LookupContext lc;
        const QString prettyName =
                m_overview.prettyName(LookupContext::fullyQualifiedName(ast->symbol));
        // do not handle functions that aren't real test data functions
        if (!prettyName.endsWith("_data"))
            return false;

        m_currentFunction = prettyName.left(prettyName.size() - 5);
        m_currentTags.clear();
        return true;
    }

    return false;
}

QString TestDataFunctionVisitor::extractNameFromAST(StringLiteralAST *ast, bool *ok) const
{
    auto token = m_currentDoc->translationUnit()->tokenAt(ast->literal_token);
    if (!token.isStringLiteral()) {
        *ok = false;
        return {};
    }
    *ok = true;
    QString name = QString::fromUtf8(token.spell());
    if (ast->next) {
        StringLiteralAST *current = ast;
        do {
            auto nextToken = m_currentDoc->translationUnit()->tokenAt(current->next->literal_token);
            name.append(QString::fromUtf8(nextToken.spell()));
            current = current->next;
        } while (current->next);
    }
    return name;
}

bool TestDataFunctionVisitor::visit(CallAST *ast)
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
                        // if it's a format string we skip as we cannot assure correct tag name
                        if (name.contains('%') && expressionListAST->next != nullptr)
                            return true;
                        int line = 0;
                        int column = 0;
                        m_currentDoc->translationUnit()->getTokenPosition(
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

bool TestDataFunctionVisitor::preVisit(AST *)
{
    ++m_currentAstDepth;
    return true;
}

void TestDataFunctionVisitor::postVisit(AST *ast)
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

bool TestDataFunctionVisitor::newRowCallFound(CallAST *ast, unsigned *firstToken) const
{
    QTC_ASSERT(firstToken, return false);

    if (!ast->base_expression)
        return false;

    bool found = false;

    if (const IdExpressionAST *exp = ast->base_expression->asIdExpression()) {
        if (!exp->name)
            return false;

        if (const auto qualifiedNameAST = exp->name->asQualifiedName()) {
            const QString name = m_overview.prettyName(qualifiedNameAST->name);
            found = (name == "QTest::newRow" || name == "QTest::addRow");
            *firstToken = qualifiedNameAST->firstToken();
        } else if (m_insideUsingQTest) {
            const QString name = m_overview.prettyName(exp->name->name);
            found = (name == "newRow" || name == "addRow");
            *firstToken = exp->name->firstToken();
        }
    }
    return found;
}

} // namespace Autotest::Internal
