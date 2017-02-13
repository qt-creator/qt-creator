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
    if (!id || !ast->symbol || ast->symbol->argumentCount() != 2)
        return false;

    CPlusPlus::LookupContext lc;
    QString prettyName = m_overview.prettyName(lc.fullyQualifiedName(ast->symbol));

    // get surrounding namespace(s) and strip them out
    const QString namespaces = enclosingNamespaces(ast->symbol);
    if (!namespaces.isEmpty()) {
        QTC_CHECK(prettyName.startsWith(namespaces));
        prettyName = prettyName.mid(namespaces.length());
    }

    if (!GTestUtils::isGTestMacro(prettyName))
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

        GTestCodeLocationAndType locationAndType;
        locationAndType.m_name = testName;
        locationAndType.m_line = line;
        locationAndType.m_column = column - 1;
        locationAndType.m_type = TestTreeItem::TestFunctionOrSet;
        locationAndType.m_state = disabled ? GTestTreeItem::Disabled
                                           : GTestTreeItem::Enabled;
        GTestCaseSpec spec;
        spec.testCaseName = testCaseName;
        spec.parameterized = GTestUtils::isGTestParameterized(prettyName);
        spec.typed = GTestUtils::isGTestTyped(prettyName);
        spec.disabled = disabledCase;
        m_gtestFunctions[spec].append(locationAndType);
    }

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
