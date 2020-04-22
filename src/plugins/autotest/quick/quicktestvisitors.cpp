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

#include "quicktestvisitors.h"

#include <cplusplus/Overview.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsbind.h>
#include <qmljs/qmljslink.h>
#include <qmljs/qmljsutils.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QDebug>

namespace Autotest {
namespace Internal {

static QStringList specialFunctions({"initTestCase", "cleanupTestCase", "init", "cleanup"});

TestQmlVisitor::TestQmlVisitor(QmlJS::Document::Ptr doc, const QmlJS::Snapshot &snapshot)
    : m_currentDoc(doc)
    , m_snapshot(snapshot)
{
}

static bool documentImportsQtTest(const QmlJS::Document *doc)
{
    if (const QmlJS::Bind *bind = doc->bind()) {
        return Utils::anyOf(bind->imports(), [] (const QmlJS::ImportInfo &info) {
            return info.isValid() && info.name() == "QtTest";
        });
    }
    return false;
}

static bool isDerivedFromTestCase(QmlJS::AST::UiQualifiedId *id, const QmlJS::Document::Ptr &doc,
                                  const QmlJS::Snapshot &snapshot)
{
    if (!id)
        return false;
    QmlJS::Link link(snapshot, QmlJS::ViewerContext(), QmlJS::LibraryInfo());
    const QmlJS::ContextPtr context = link();

    const QmlJS::ObjectValue *value = context->lookupType(doc.data(), id);
    if (!value)
        return false;

    QmlJS::PrototypeIterator protoIterator(value, context);
    const QList<const QmlJS::ObjectValue *> prototypes = protoIterator.all();
    for (const QmlJS::ObjectValue *val : prototypes) {
        if (auto prototype = val->prototype()) {
            if (auto qmlPrototypeRef = prototype->asQmlPrototypeReference()) {
                if (auto qmlTypeName = qmlPrototypeRef->qmlTypeName()) {
                    if (qmlTypeName->name == "TestCase") {
                        if (auto astObjVal = val->asAstObjectValue())
                            return documentImportsQtTest(astObjVal->document());
                    }
                }
            }
        }
    }
    return false;
}

bool TestQmlVisitor::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    const QStringRef name = ast->qualifiedTypeNameId->name;
    m_objectIsTestStack.push(false);
    if (name != "TestCase") {
        if (!isDerivedFromTestCase(ast->qualifiedTypeNameId, m_currentDoc, m_snapshot))
            return true;
    } else if (!documentImportsQtTest(m_currentDoc.data())) {
        return true; // find nested TestCase items as well
    }

    m_objectIsTestStack.top() = true;
    const auto sourceLocation = ast->firstSourceLocation();
    QuickTestCaseSpec currentSpec;
    currentSpec.m_locationAndType.m_name = m_currentDoc->fileName();
    currentSpec.m_locationAndType.m_line = sourceLocation.startLine;
    currentSpec.m_locationAndType.m_column = sourceLocation.startColumn - 1;
    currentSpec.m_locationAndType.m_type = TestTreeItem::TestCase;
    m_caseParseStack.push(currentSpec);
    return true;
}

void TestQmlVisitor::endVisit(QmlJS::AST::UiObjectDefinition *)
{
    if (!m_objectIsTestStack.isEmpty() && m_objectIsTestStack.pop() && !m_caseParseStack.isEmpty())
        m_testCases << m_caseParseStack.pop();
}

bool TestQmlVisitor::visit(QmlJS::AST::ExpressionStatement *ast)
{
    const QmlJS::AST::ExpressionNode *expr = ast->expression;
    return expr->kind == QmlJS::AST::Node::Kind_StringLiteral;
}

bool TestQmlVisitor::visit(QmlJS::AST::UiScriptBinding *ast)
{
    if (m_objectIsTestStack.top())
        m_expectTestCaseName = ast->qualifiedId->name == "name";
    return m_expectTestCaseName;
}

void TestQmlVisitor::endVisit(QmlJS::AST::UiScriptBinding *)
{
    if (m_expectTestCaseName)
        m_expectTestCaseName = false;
}

bool TestQmlVisitor::visit(QmlJS::AST::FunctionDeclaration *ast)
{
    if (m_caseParseStack.isEmpty())
        return false;

    const QStringRef name = ast->name;
    if (name.startsWith("test_")
            || name.startsWith("benchmark_")
            || name.endsWith("_data")
            || specialFunctions.contains(name.toString())) {
        const auto sourceLocation = ast->firstSourceLocation();
        TestCodeLocationAndType locationAndType;
        locationAndType.m_name = m_currentDoc->fileName();
        locationAndType.m_line = sourceLocation.startLine;
        locationAndType.m_column = sourceLocation.startColumn - 1;
        if (specialFunctions.contains(name.toString()))
            locationAndType.m_type = TestTreeItem::TestSpecialFunction;
        else if (name.endsWith("_data"))
            locationAndType.m_type = TestTreeItem::TestDataFunction;
        else
            locationAndType.m_type = TestTreeItem::TestFunction;

        const QString nameStr = name.toString();
        // identical test functions inside the same file are not working - will fail at runtime
        if (!Utils::anyOf(m_caseParseStack.top().m_functions,
                          [nameStr, locationAndType](const QuickTestFunctionSpec func) {
                          return func.m_locationAndType.m_type == locationAndType.m_type
                          && func.m_functionName == nameStr
                          && func.m_locationAndType.m_name == locationAndType.m_name;
        })) {
            m_caseParseStack.top().m_functions.append(
                        QuickTestFunctionSpec{nameStr, locationAndType});
        }
    }
    return false;
}

bool TestQmlVisitor::visit(QmlJS::AST::StringLiteral *ast)
{
    if (m_expectTestCaseName) {
        QTC_ASSERT(!m_caseParseStack.isEmpty(), return false);
        m_caseParseStack.top().m_caseName = ast->value.toString();
        m_expectTestCaseName = false;
    }
    return false;
}

void TestQmlVisitor::throwRecursionDepthError()
{
    qWarning("Warning: Hit maximum recursion depth while visiting AST in TestQmlVisitor");
}

/************************************** QuickTestAstVisitor *************************************/

QuickTestAstVisitor::QuickTestAstVisitor(CPlusPlus::Document::Ptr doc,
                                         const CPlusPlus::Snapshot &snapshot)
    : ASTVisitor(doc->translationUnit())
    , m_currentDoc(doc)
    , m_snapshot(snapshot)
{
}

bool QuickTestAstVisitor::visit(CPlusPlus::CallAST *ast)
{
    if (m_currentDoc.isNull())
        return false;

    if (const auto expressionAST = ast->base_expression) {
        if (const auto idExpressionAST = expressionAST->asIdExpression()) {
            if (const auto simpleNameAST = idExpressionAST->name->asSimpleName()) {
                const CPlusPlus::Overview o;
                const QString prettyName = o.prettyName(simpleNameAST->name);
                if (prettyName == "quick_test_main" || prettyName == "quick_test_main_with_setup") {
                    if (auto expressionListAST = ast->expression_list) {
                        // third argument is the one we need, so skip current and next
                        expressionListAST = expressionListAST->next; // argv
                        expressionListAST = expressionListAST ? expressionListAST->next : nullptr; // testcase literal

                        if (expressionListAST && expressionListAST->value) {
                            const auto *stringLitAST = expressionListAST->value->asStringLiteral();
                            if (!stringLitAST)
                                return false;
                            const auto *string
                                    = translationUnit()->stringLiteral(stringLitAST->literal_token);
                            if (string) {
                                m_testBaseName = QString::fromUtf8(string->chars(),
                                                                   int(string->size()));
                            }
                        }
                    }
                }
            }
        }
    }
    return false;
}

} // namespace Internal
} // namespace Autotest
