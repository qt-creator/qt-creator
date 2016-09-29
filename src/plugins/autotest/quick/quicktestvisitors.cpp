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

#include <qmljs/parser/qmljsast_p.h>

namespace Autotest {
namespace Internal {

static QStringList specialFunctions({ "initTestCase", "cleanupTestCase", "init", "cleanup" });

TestQmlVisitor::TestQmlVisitor(QmlJS::Document::Ptr doc)
    : m_currentDoc(doc)
{
}

bool TestQmlVisitor::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    const QStringRef name = ast->qualifiedTypeNameId->name;
    if (name != "TestCase")
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
    return name == "name";
}

bool TestQmlVisitor::visit(QmlJS::AST::FunctionDeclaration *ast)
{
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

} // namespace Internal
} // namespace Autotest
