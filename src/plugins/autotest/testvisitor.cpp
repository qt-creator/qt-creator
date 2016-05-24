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

#include "autotest_utils.h"
#include "testvisitor.h"

#include <cplusplus/FullySpecifiedType.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/Symbols.h>
#include <cplusplus/TypeOfExpression.h>

#include <cpptools/cppmodelmanager.h>

#include <qmljs/parser/qmljsast_p.h>

#include <utils/qtcassert.h>

#include <QList>

namespace Autotest {
namespace Internal {

// names of special functions (applies for QTest as well as Quick Tests)
static QList<QString> specialFunctions = QList<QString>() << QLatin1String("initTestCase")
                                                          << QLatin1String("cleanupTestCase")
                                                          << QLatin1String("init")
                                                          << QLatin1String("cleanup");

/************************** Cpp Test Symbol Visitor ***************************/

TestVisitor::TestVisitor(const QString &fullQualifiedClassName)
    : m_className(fullQualifiedClassName)
{
}

TestVisitor::~TestVisitor()
{
}

bool TestVisitor::visit(CPlusPlus::Class *symbol)
{
    const CPlusPlus::Overview o;
    CPlusPlus::LookupContext lc;
    const CPlusPlus::Snapshot snapshot = CppTools::CppModelManager::instance()->snapshot();

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
                TestCodeLocationAndType locationAndType;

                CPlusPlus::Function *functionDefinition = m_symbolFinder.findMatchingDefinition(
                            func, snapshot, true);
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
                else if (name.endsWith(QLatin1String("_data")))
                    locationAndType.m_type = TestTreeItem::TestDataFunction;
                else
                    locationAndType.m_type = TestTreeItem::TestFunctionOrSet;
                m_privSlots.insert(name, locationAndType);
            }
        }
    }
    return true;
}

/**************************** Cpp Test AST Visitor ****************************/

TestAstVisitor::TestAstVisitor(CPlusPlus::Document::Ptr doc)
    : ASTVisitor(doc->translationUnit()),
      m_currentDoc(doc)
{
}

TestAstVisitor::~TestAstVisitor()
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
                if (prettyName == QLatin1String("QTest::qExec")) {
                    if (const auto expressionListAST = ast->expression_list) {
                        // first argument is the one we need
                        if (const auto argumentExpressionAST = expressionListAST->value) {
                            CPlusPlus::TypeOfExpression toe;
                            CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
                            toe.init(m_currentDoc, cppMM->snapshot());
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
        m_currentScope = 0;
        return false;
    }
    m_currentScope = ast->symbol->asScope();
    return true;
}

/********************** Test Data Function AST Visitor ************************/

TestDataFunctionVisitor::TestDataFunctionVisitor(CPlusPlus::Document::Ptr doc)
    : CPlusPlus::ASTVisitor(doc->translationUnit()),
      m_currentDoc(doc),
      m_currentAstDepth(0),
      m_insideUsingQTestDepth(0),
      m_insideUsingQTest(false)
{
}

TestDataFunctionVisitor::~TestDataFunctionVisitor()
{
}

bool TestDataFunctionVisitor::visit(CPlusPlus::UsingDirectiveAST *ast)
{
    if (auto nameAST = ast->name) {
        if (m_overview.prettyName(nameAST->name) == QLatin1String("QTest")) {
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
        if (!prettyName.endsWith(QLatin1String("_data")))
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
                        TestCodeLocationAndType locationAndType;
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
            found = m_overview.prettyName(qualifiedNameAST->name) == QLatin1String("QTest::newRow");
            *firstToken = qualifiedNameAST->firstToken();
        } else if (m_insideUsingQTest) {
            found = m_overview.prettyName(exp->name->name) == QLatin1String("newRow");
            *firstToken = exp->name->firstToken();
        }
    }
    return found;
}

/*************************** Quick Test AST Visitor ***************************/

TestQmlVisitor::TestQmlVisitor(QmlJS::Document::Ptr doc)
    : m_currentDoc(doc)
{
}

TestQmlVisitor::~TestQmlVisitor()
{
}

bool TestQmlVisitor::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    const QStringRef name = ast->qualifiedTypeNameId->name;
    if (name != QLatin1String("TestCase"))
        return true; // find nested TestCase items as well

    m_currentTestCaseName.clear();
    const auto sourceLocation = ast->firstSourceLocation();
    m_testCaseLocation.m_name = m_currentDoc->fileName();
    m_testCaseLocation.m_line = sourceLocation.startLine;
    m_testCaseLocation.m_column = sourceLocation.startColumn - 1;
    m_testCaseLocation.m_type = TestTreeItem::TestCase;
    return true;
}

bool TestQmlVisitor::visit(QmlJS::AST::ExpressionStatement *ast)
{
    const QmlJS::AST::ExpressionNode *expr = ast->expression;
    return expr->kind == QmlJS::AST::Node::Kind_StringLiteral;
}

bool TestQmlVisitor::visit(QmlJS::AST::UiScriptBinding *ast)
{
    const QStringRef name = ast->qualifiedId->name;
    return name == QLatin1String("name");
}

bool TestQmlVisitor::visit(QmlJS::AST::FunctionDeclaration *ast)
{
    const QStringRef name = ast->name;
    if (name.startsWith(QLatin1String("test_"))
            || name.startsWith(QLatin1String("benchmark_"))
            || name.endsWith(QLatin1String("_data"))
            || specialFunctions.contains(name.toString())) {
        const auto sourceLocation = ast->firstSourceLocation();
        TestCodeLocationAndType locationAndType;
        locationAndType.m_name = m_currentDoc->fileName();
        locationAndType.m_line = sourceLocation.startLine;
        locationAndType.m_column = sourceLocation.startColumn - 1;
        if (specialFunctions.contains(name.toString()))
            locationAndType.m_type = TestTreeItem::TestSpecialFunction;
        else if (name.endsWith(QLatin1String("_data")))
            locationAndType.m_type = TestTreeItem::TestDataFunction;
        else
            locationAndType.m_type = TestTreeItem::TestFunctionOrSet;

        m_testFunctions.insert(name.toString(), locationAndType);
    }
    return false;
}

bool TestQmlVisitor::visit(QmlJS::AST::StringLiteral *ast)
{
    m_currentTestCaseName = ast->value.toString();
    return false;
}

/********************** Google Test Function AST Visitor **********************/

GTestVisitor::GTestVisitor(CPlusPlus::Document::Ptr doc)
    : CPlusPlus::ASTVisitor(doc->translationUnit())
    , m_document(doc)
{
}

bool GTestVisitor::visit(CPlusPlus::FunctionDefinitionAST *ast)
{
    static QString disabledPrefix = QString::fromLatin1("DISABLED_");
    if (!ast || !ast->declarator || !ast->declarator->core_declarator)
        return false;

    CPlusPlus::DeclaratorIdAST *id = ast->declarator->core_declarator->asDeclaratorId();
    if (!id || !ast->symbol || ast->symbol->argumentCount() != 2)
        return false;

    CPlusPlus::LookupContext lc;
    const QString prettyName = m_overview.prettyName(lc.fullyQualifiedName(ast->symbol));
    if (!TestUtils::isGTestMacro(prettyName))
        return false;

    CPlusPlus::Argument *testCaseNameArg = ast->symbol->argumentAt(0)->asArgument();
    CPlusPlus::Argument *testNameArg = ast->symbol->argumentAt(1)->asArgument();
    if (testCaseNameArg && testNameArg) {
        const QString &testCaseName = m_overview.prettyType(testCaseNameArg->type());
        const QString &testName = m_overview.prettyType(testNameArg->type());

        const bool disabled = testName.startsWith(disabledPrefix);
        const bool disabledCase = testCaseName.startsWith(disabledPrefix);
        unsigned line = 0;
        unsigned column = 0;
        unsigned token = id->firstToken();
        m_document->translationUnit()->getTokenStartPosition(token, &line, &column);

        TestCodeLocationAndType locationAndType;
        locationAndType.m_name = testName;
        locationAndType.m_line = line;
        locationAndType.m_column = column - 1;
        locationAndType.m_type = TestTreeItem::TestFunctionOrSet;
        locationAndType.m_state = disabled ? GoogleTestTreeItem::Disabled
                                           : GoogleTestTreeItem::Enabled;
        GTestCaseSpec spec;
        spec.testCaseName = testCaseName;
        spec.parameterized = TestUtils::isGTestParameterized(prettyName);
        spec.typed = TestUtils::isGTestTyped(prettyName);
        spec.disabled = disabledCase;
        m_gtestFunctions[spec].append(locationAndType);
    }

    return false;
}

} // namespace Internal
} // namespace Autotest
