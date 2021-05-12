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

#include "gtestvisitors.h"
#include "gtest_utils.h"

#include <cplusplus/LookupContext.h>
#include <utils/qtcassert.h>

#include <QRegularExpression>

namespace Autotest {
namespace Internal {

GTestVisitor::GTestVisitor(CPlusPlus::Document::Ptr doc)
    : CPlusPlus::ASTVisitor(doc->translationUnit())
    , m_document(doc)
{
}

bool GTestVisitor::visit(CPlusPlus::FunctionDefinitionAST *ast)
{
    static const QString disabledPrefix("DISABLED_");
    if (!ast || !ast->declarator || !ast->declarator->core_declarator)
        return false;

    CPlusPlus::DeclaratorIdAST *id = ast->declarator->core_declarator->asDeclaratorId();
    if (!id || !ast->symbol)
        return false;

    QString prettyName =
            m_overview.prettyName(CPlusPlus::LookupContext::fullyQualifiedName(ast->symbol));

    // get surrounding namespace(s) and strip them out
    const QString namespaces = enclosingNamespaces(ast->symbol);
    if (!namespaces.isEmpty()) {
        QTC_CHECK(prettyName.startsWith(namespaces));
        prettyName = prettyName.mid(namespaces.length());
    }

    if (!GTestUtils::isGTestMacro(prettyName))
        return false;

    QString testSuiteName;
    QString testCaseName;
    if (ast->symbol->argumentCount() != 2 && ast->declarator->initializer) {
        // we might have a special case when second parameter is a literal starting with a number
        if (auto expressionListParenAST = ast->declarator->initializer->asExpressionListParen()) {
            // only try if we have 3 tokens between left and right paren (2 parameters and a comma)
            if (expressionListParenAST->rparen_token - expressionListParenAST->lparen_token != 4)
                return false;

            const CPlusPlus::Token parameter1
                    = translationUnit()->tokenAt(expressionListParenAST->lparen_token + 1);
            const CPlusPlus::Token parameter2
                    = translationUnit()->tokenAt(expressionListParenAST->rparen_token - 1);
            const CPlusPlus::Token comma
                    = translationUnit()->tokenAt(expressionListParenAST->lparen_token + 2);
            if (!comma.is(CPlusPlus::T_COMMA))
                return false;

            testSuiteName = QString::fromUtf8(parameter1.spell());
            testCaseName = QString::fromUtf8(parameter2.spell());
            // test (suite) name needs to be a alpha numerical literal ( _ and $ allowed)
            const QRegularExpression alnum("^[[:alnum:]_$]+$");
            // test suite must not start with a number, test case may
            if (!alnum.match(testSuiteName).hasMatch()
                    || (!testSuiteName.isEmpty() && testSuiteName.at(0).isNumber())) {
                testSuiteName.clear();
            }
            if (!alnum.match(testCaseName).hasMatch())
                testCaseName.clear();
        }
    } else {
        const CPlusPlus::Symbol *firstArg = ast->symbol->argumentAt(0);
        const CPlusPlus::Symbol *secondArg = ast->symbol->argumentAt(1);
        if (!firstArg || !secondArg)
            return false;
        const CPlusPlus::Argument *testSuiteNameArg = firstArg->asArgument();
        const CPlusPlus::Argument *testCaseNameArg = secondArg->asArgument();
        if (testSuiteNameArg && testCaseNameArg) {
            testSuiteName = m_overview.prettyType(testSuiteNameArg->type());
            testCaseName = m_overview.prettyType(testCaseNameArg->type());
        }
    }
    if (testSuiteName.isEmpty() || testCaseName.isEmpty())
        return false;

    const bool disabled = testCaseName.startsWith(disabledPrefix);
    const bool disabledCase = testSuiteName.startsWith(disabledPrefix);
    int line = 0;
    int column = 0;
    unsigned token = id->firstToken();
    m_document->translationUnit()->getTokenStartPosition(token, &line, &column);

    GTestCodeLocationAndType locationAndType;
    locationAndType.m_name = testCaseName;
    locationAndType.m_line = line;
    locationAndType.m_column = column - 1;
    locationAndType.m_type = TestTreeItem::TestCase;
    locationAndType.m_state = disabled ? GTestTreeItem::Disabled
                                       : GTestTreeItem::Enabled;
    GTestCaseSpec spec;
    // FIXME GTestCaseSpec structure wrong nowadays (suite vs case / case vs function)
    spec.testCaseName = testSuiteName;
    spec.parameterized = GTestUtils::isGTestParameterized(prettyName);
    spec.typed = GTestUtils::isGTestTyped(prettyName);
    spec.disabled = disabledCase;
    m_gtestFunctions[spec].append(locationAndType);

    return false;
}

QString GTestVisitor::enclosingNamespaces(CPlusPlus::Symbol *symbol) const
{
    QString enclosing;
    if (!symbol)
        return enclosing;

    CPlusPlus::Symbol *currentSymbol = symbol;
    while (CPlusPlus::Namespace *ns = currentSymbol->enclosingNamespace()) {
        if (ns->name()) // handle anonymous namespaces as well
            enclosing.prepend(m_overview.prettyName(ns->name()).append("::"));
        currentSymbol = ns;
    }
    return enclosing;
}

} // namespace Internal
} // namespace Autotest
