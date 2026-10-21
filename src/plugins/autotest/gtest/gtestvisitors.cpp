// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gtestvisitors.h"
#include "gtest_utils.h"

#include <cplusplus/LookupContext.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QRegularExpression>

namespace Autotest::Internal {

GTestVisitor::GTestVisitor(CPlusPlus::Document::Ptr doc,
                           const QMap<GTestCaseSpec, GTestCodeLocationList> &knownTests,
                           const QByteArray &originalSource)
    : CPlusPlus::ASTVisitor(doc->translationUnit())
    , m_document(doc)
    , m_originalSource(originalSource)
    , m_gtestFunctions(knownTests)
{
}

bool GTestVisitor::visit(CPlusPlus::FunctionDefinitionAST *ast)
{
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
        prettyName = prettyName.mid(namespaces.size());
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
            static const QRegularExpression alnum("^[[:alnum:]_$]+$");
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

    int line = 0;
    int column = 0;
    tokenPosition(id->firstToken(), &line, &column);
    GTestCaseSpec spec;
    // FIXME GTestCaseSpec structure wrong nowadays (suite vs case / case vs function)
    spec.testCaseName = testSuiteName;
    spec.parameterized = GTestUtils::isGTestParameterized(prettyName);
    spec.typed = GTestUtils::isGTestTyped(prettyName);
    addTest(spec, testCaseName, line, column);
    return false;
}

bool GTestVisitor::visit(CPlusPlus::CallAST *ast)
{
    if (!ast || !ast->base_expression || !ast->expression_list)
        return true;

    CPlusPlus::IdExpressionAST *idExpression = ast->base_expression->asIdExpression();
    if (!idExpression || !idExpression->name)
        return true;
    CPlusPlus::NameAST *name = idExpression->name;
    if (CPlusPlus::QualifiedNameAST *qualifiedName = name->asQualifiedName())
        name = qualifiedName->unqualified_name;
    CPlusPlus::SimpleNameAST *simpleName = name ? name->asSimpleName() : nullptr;
    if (!simpleName)
        return true;
    const CPlusPlus::Identifier *identifier
        = translationUnit()->identifier(simpleName->identifier_token);
    if (!identifier || !identifier->match(translationUnit()->control()->identifier(
                           "MakeAndRegisterTestInfo"))) {
        return true;
    }

    const auto stringArgument = [this](CPlusPlus::ExpressionListAST *argument) {
        if (!argument || !argument->value)
            return QString();
        CPlusPlus::StringLiteralAST *literal = argument->value->asStringLiteral();
        if (!literal || literal->next)
            return QString();
        const CPlusPlus::Token &token = translationUnit()->tokenAt(literal->literal_token);
        if (token.kind() != CPlusPlus::T_STRING_LITERAL)
            return QString();
        const QString raw = QString::fromUtf8(token.spell());
        QString text;
        for (int i = 0; i < raw.size(); ++i) {
            if (raw.at(i) != '\\') {
                text.append(raw.at(i));
                continue;
            }
            // other escape sequences are unlikely inside a test name
            if (++i == raw.size() || !QStringView(u"\"'?\\").contains(raw.at(i)))
                return QString();
            text.append(raw.at(i));
        }
        return text;
    };
    const QString testSuiteName = stringArgument(ast->expression_list);
    const QString testCaseName = stringArgument(ast->expression_list->next);
    if (testSuiteName.isEmpty() || testCaseName.isEmpty())
        return false;

    // tests registered by a gtest macro may have been found already
    for (auto it = m_gtestFunctions.cbegin(); it != m_gtestFunctions.cend(); ++it) {
        if (it.key().testCaseName == testSuiteName
            && Utils::anyOf(it.value(), [&testCaseName](const GTestCodeLocationAndType &location) {
                   return location.m_name == testCaseName;
               })) {
            return false;
        }
    }

    int line = 0;
    int column = 0;
    tokenPosition(ast->firstToken(), &line, &column);
    GTestCaseSpec spec;
    spec.testCaseName = testSuiteName;
    addTest(spec, testCaseName, line, column);
    return false;
}

void GTestVisitor::addTest(GTestCaseSpec spec, const QString &testCaseName, int line, int column)
{
    static const QString disabledPrefix("DISABLED_");
    const bool disabled = testCaseName.startsWith(disabledPrefix);
    GTestCodeLocationAndType locationAndType;
    locationAndType.m_name = testCaseName;
    locationAndType.m_line = line;
    locationAndType.m_column = column - 1;
    locationAndType.m_type = TestTreeItem::TestCase;
    locationAndType.m_state = disabled ? GTestTreeItem::Disabled
                                       : GTestTreeItem::Enabled;
    spec.disabled = spec.testCaseName.startsWith(disabledPrefix);
    m_gtestFunctions[spec].append(locationAndType);
}

void GTestVisitor::tokenPosition(int token, int *line, int *column) const
{
    if (!translationUnit()->tokenAt(token).generated() || m_originalSource.isEmpty()) {
        translationUnit()->getTokenPosition(token, line, column);
        return;
    }
    // use the position of the macro invocation inside the original source
    const int offset = translationUnit()->getExpansionPosition(token).first;
    const QByteArrayView source(m_originalSource);
    const QByteArrayView before = source.first(qMin<qsizetype>(offset, source.size()));
    *line = before.count('\n') + 1;
    *column = QString::fromUtf8(before.sliced(before.lastIndexOf('\n') + 1)).size() + 1;
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

} // namespace Autotest::Internal
