/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "boostcodeparser.h"
#include "boosttestconstants.h"

#include <cplusplus/Overview.h>
#include <cplusplus/Token.h>
#include <utils/qtcassert.h>

namespace Autotest {
namespace Internal {

using namespace CPlusPlus;

BoostCodeParser::BoostCodeParser(const QByteArray &source, const LanguageFeatures &features,
                                 const Document::Ptr &doc, const Snapshot &snapshot)
    : m_source(source)
    , m_features(features)
    , m_doc(doc)
    , m_snapshot(snapshot)
    , m_lookupContext(m_doc, m_snapshot)
{
    m_typeOfExpression.init(m_doc, m_snapshot);
}

static BoostTestCodeLocationAndType locationAndTypeFromToken(const Token &tkn,
                                                             const QByteArray &src,
                                                             BoostTestTreeItem::TestStates state,
                                                             const BoostTestInfoList &suitesStates)
{
    BoostTestCodeLocationAndType locationAndType;
    locationAndType.m_name = QString::fromUtf8(src.mid(int(tkn.bytesBegin()), int(tkn.bytes())));
    locationAndType.m_type = TestTreeItem::TestCase;
    locationAndType.m_line = tkn.lineno;
    locationAndType.m_column = 0;
    locationAndType.m_state = state;
    if (suitesStates.isEmpty()) {
        // should we handle renaming of master suite?
        BoostTestInfo dummy{QString(BoostTest::Constants::BOOST_MASTER_SUITE),
                    BoostTestTreeItem::Enabled, 0};
        locationAndType.m_suitesState.append(dummy);
    } else {
        locationAndType.m_suitesState.append(suitesStates);
    }
    return locationAndType;
}

static Tokens tokensForSource(const QByteArray &source, const LanguageFeatures &features)
{
    SimpleLexer lexer;
    lexer.setPreprocessorMode(false); // or true? does not make a difference so far..
    lexer.setLanguageFeatures(features);
    return lexer(QString::fromUtf8(source));
}

BoostTestCodeLocationList BoostCodeParser::findTests()
{
    m_tokens = tokensForSource(m_source, m_features);
    m_currentIndex = 0;
    for ( ; m_currentIndex < m_tokens.size(); ++m_currentIndex) {
        const Token &token = m_tokens.at(m_currentIndex);

        if (token.kind() == T_IDENTIFIER)
            handleIdentifier();

    }
    return m_testCases;
}

void BoostCodeParser::handleIdentifier()
{
    QTC_ASSERT(m_currentIndex < m_tokens.size(), return);
    const Token &token = m_tokens.at(m_currentIndex);
    const QByteArray &identifier = m_source.mid(int(token.bytesBegin()), int(token.bytes()));

    if (identifier == "BOOST_AUTO_TEST_SUITE") {
        handleSuiteBegin(false);
        m_currentState = BoostTestTreeItem::Enabled;
    } else if (identifier == "BOOST_FIXTURE_TEST_SUITE") {
        handleSuiteBegin(true);
        m_currentState = BoostTestTreeItem::Enabled;
    } else if (identifier == "BOOST_AUTO_TEST_SUITE_END") {
        handleSuiteEnd();
        m_currentState = BoostTestTreeItem::Enabled;
    } else if (identifier == "BOOST_TEST_CASE") {
        handleTestCase(TestCaseType::Functions);
        m_currentState = BoostTestTreeItem::Enabled;
    } else if (identifier == "BOOST_PARAM_TEST_CASE") {
        m_currentState.setFlag(BoostTestTreeItem::Parameterized);
        handleTestCase(TestCaseType::Parameter);
        m_currentState = BoostTestTreeItem::Enabled;
    } else if (identifier == "BOOST_AUTO_TEST_CASE") {
        handleTestCase(TestCaseType::Auto);
        m_currentState = BoostTestTreeItem::Enabled;
    } else if (identifier == "BOOST_FIXTURE_TEST_CASE") {
        m_currentState.setFlag(BoostTestTreeItem::Fixture);
        handleTestCase(TestCaseType::Fixture);
        m_currentState = BoostTestTreeItem::Enabled;
    } else if (identifier == "BOOST_DATA_TEST_CASE") {
        handleTestCase(TestCaseType::Data);
        m_currentState = BoostTestTreeItem::Enabled;
    } else if (identifier == "BOOST_DATA_TEST_CASE_F") {
        m_currentState.setFlag(BoostTestTreeItem::Fixture);
        handleTestCase(TestCaseType::Data);
        m_currentState = BoostTestTreeItem::Enabled;
    } else if (identifier == "BOOST_AUTO_TEST_CASE_TEMPLATE") {
        m_currentState.setFlag(BoostTestTreeItem::Templated);
        handleTestCase(TestCaseType::Auto);
        m_currentState = BoostTestTreeItem::Enabled;
    } else if (identifier == "BOOST_FIXTURE_TEST_CASE_TEMPLATE") {
        m_currentState.setFlag(BoostTestTreeItem::Fixture);
        m_currentState.setFlag(BoostTestTreeItem::Templated);
        handleTestCase(TestCaseType::Auto);
        m_currentState = BoostTestTreeItem::Enabled;
    } else if (identifier == "BOOST_TEST_DECORATOR") {
        handleDecorator();
    }
}

void BoostCodeParser::handleSuiteBegin(bool isFixture)
{
    m_currentSuite.clear();
    if (!skipCommentsUntil(T_LPAREN))
        return;
    if (!skipCommentsUntil(T_IDENTIFIER))
        return;

    const Token &token = m_tokens.at(m_currentIndex);
    const QByteArray &identifier = m_source.mid(int(token.bytesBegin()), int(token.bytes()));
    m_lineNo = token.lineno;
    m_currentSuite = QString::fromUtf8(identifier);
    if (!m_suites.isEmpty())
        m_currentSuite.prepend(m_suites.last().fullName + '/');

    if (isFixture) { // fixture suites have a (fixture) class name as 2nd parameter
        m_currentState.setFlag(BoostTestTreeItem::Fixture);
        if (!skipCommentsUntil(T_COMMA))
            return;
        if (!skipCommentsUntil(T_IDENTIFIER))
            return;
    }

    if (!skipCommentsUntil(T_COMMA)) {
        if (skipCommentsUntil(T_RPAREN)) {
            // we have no decorators (or we have them before this macro)
            m_suites << BoostTestInfo{m_currentSuite, m_currentState, m_lineNo};
        }
    } else {
        handleDecorators();
        m_suites << BoostTestInfo{m_currentSuite, m_currentState, m_lineNo};
    }
}

void BoostCodeParser::handleSuiteEnd()
{
    if (!skipCommentsUntil(T_LPAREN))
        return;
    skipCommentsUntil(T_RPAREN);
    if (m_suites.isEmpty())
        return;
    m_suites.removeLast();
}

void BoostCodeParser::handleTestCase(TestCaseType testCaseType)
{
    if (!skipCommentsUntil(T_LPAREN))
        return;

    Token token;
    BoostTestCodeLocationAndType locationAndType;

    switch (testCaseType) {
    case TestCaseType::Functions: // cannot have decorators passed as parameter
    case TestCaseType::Parameter:
    case TestCaseType::Data:
        if (testCaseType != TestCaseType::Data) {
            if (!skipCommentsUntil(T_AMPER)) {
                // content without the leading lparen
                const QByteArray content = contentUntil(T_RPAREN).mid(1);
                const QList<QByteArray> parts = content.split(',');
                if (parts.size() == 0)
                    return;
                token = m_tokens.at(m_currentIndex);
                locationAndType = locationAndTypeFromToken(token, m_source, m_currentState, m_suites);

                const QByteArray functionName = parts.first();
                if (isBoostBindCall(functionName))
                    locationAndType.m_name = QString::fromUtf8(content).append(')');
                else
                    locationAndType.m_name = QString::fromUtf8(functionName);
                m_testCases.append(locationAndType);
                return;
            }
        } else if (m_currentState.testFlag(BoostTestTreeItem::Fixture)) {
            // ignore first parameter (fixture) and first comma
            if (!skipCommentsUntil(T_IDENTIFIER))
                return;
            if (!skipCommentsUntil(T_COMMA))
                return;
        }
        if (!skipCommentsUntil(T_IDENTIFIER))
            return;
        token = m_tokens.at(m_currentIndex);
        locationAndType = locationAndTypeFromToken(token, m_source, m_currentState, m_suites);
        m_testCases.append(locationAndType);
        return;

    case TestCaseType::Auto:
    case TestCaseType::Fixture:
        if (!skipCommentsUntil(T_IDENTIFIER))
            return;
        token = m_tokens.at(m_currentIndex);

        if (testCaseType == TestCaseType::Fixture) { // skip fixture class parameter
            if (!skipCommentsUntil(T_COMMA))
                return;
            if (!skipCommentsUntil(T_IDENTIFIER))
                return;
        }
        break;
    }

    // check and handle decorators and add the found test case
    if (!skipCommentsUntil(T_COMMA)) {
        if (skipCommentsUntil(T_RPAREN)) {
            locationAndType = locationAndTypeFromToken(token, m_source, m_currentState, m_suites);
            m_testCases.append(locationAndType);
        }
    } else {
        if (!m_currentState.testFlag(BoostTestTreeItem::Templated))
            handleDecorators();
        locationAndType = locationAndTypeFromToken(token, m_source, m_currentState, m_suites);
        m_testCases.append(locationAndType);
    }
}

void BoostCodeParser::handleDecorator()
{
    skipCommentsUntil(T_LPAREN);
    m_currentState = BoostTestTreeItem::Enabled;
    handleDecorators();
}

void BoostCodeParser::handleDecorators()
{
    if (!skipCommentsUntil(T_STAR))
        return;
    if (!skipCommentsUntil(T_IDENTIFIER))
        return;

    QByteArray decorator = contentUntil(T_LPAREN);
    if (decorator.isEmpty())
        return;

    bool aliasedOrReal;
    QString symbolName;
    QByteArray simplifiedName;

    if (!evalCurrentDecorator(decorator, &symbolName, &simplifiedName, &aliasedOrReal))
        return;

    if (symbolName == "decorator::disabled" || (aliasedOrReal && simplifiedName == "::disabled")) {
        m_currentState.setFlag(BoostTestTreeItem::Disabled);
    } else if (symbolName == "decorator::enabled"
               || (aliasedOrReal && simplifiedName == "::enabled")) {
        m_currentState.setFlag(BoostTestTreeItem::Disabled, false);
        m_currentState.setFlag(BoostTestTreeItem::ExplicitlyEnabled);
    } else if (symbolName == "decorator::enable_if"
               || (aliasedOrReal && simplifiedName.startsWith("::enable_if<"))) {
        // figure out the passed template value
        QByteArray templateType = decorator.mid(decorator.indexOf('<') + 1);
        templateType.chop(templateType.size() - templateType.indexOf('>'));

        if (templateType == "true") {
            m_currentState.setFlag(BoostTestTreeItem::Disabled, false);
            m_currentState.setFlag(BoostTestTreeItem::ExplicitlyEnabled);
        } else if (templateType == "false") {
            m_currentState.setFlag(BoostTestTreeItem::Disabled);
        } else {
            // FIXME we have a const(expr) bool? currently not easily achievable
        }
    } else if (symbolName == "decorator::fixture"
               || (aliasedOrReal && simplifiedName.startsWith("::fixture"))){
        m_currentState.setFlag(BoostTestTreeItem::Fixture);
    }
    // TODO.. depends_on, label, precondition, timeout,...

    skipCommentsUntil(T_LPAREN);
    skipCommentsUntil(T_RPAREN);

    handleDecorators(); // check for more decorators
}

bool BoostCodeParser::skipCommentsUntil(const Kind nextExpectedKind)
{
    for (int index = m_currentIndex + 1, end = m_tokens.size(); index < end; ++index) {
        const Token &token = m_tokens.at(index);
        if (token.isComment())
            continue;
        if (token.kind() != nextExpectedKind)
            break;
        m_currentIndex = index;
        return true;
    }
    return false;
}

QByteArray BoostCodeParser::contentUntil(Kind stopKind)
{
    QByteArray result;
    for (int oldIndex = m_currentIndex, end = m_tokens.size(); oldIndex < end; ++oldIndex) {
        const Token &token = m_tokens.at(oldIndex);
        if (token.isComment())
            continue;
        if (token.kind() == stopKind)
            return result;
        result.append(m_source.mid(int(token.bytesBegin()), int(token.bytes())));
    }
    return result;
}

bool BoostCodeParser::isBoostBindCall(const QByteArray &function)
{
    if (!function.contains("bind"))
        return false;
    int index = function.indexOf('(');
    if (index == -1)
        return false;
    QByteArray funcCall = function.left(index);
    const QList<LookupItem> lookupItems = m_typeOfExpression(funcCall, m_doc->globalNamespace());
    if (lookupItems.isEmpty())
        return false;

    if (funcCall.contains("::")) {
        bool aliasedOrReal = false;
        aliasedOrRealNamespace(funcCall, "boost", nullptr, &aliasedOrReal);
        return aliasedOrReal;
    }

    Overview overview;
    for (const LookupItem &item : lookupItems) {
        if (Symbol *symbol = item.declaration()) {
            const QString fullQualified = overview.prettyName(
                        m_lookupContext.fullyQualifiedName(symbol->enclosingNamespace()));
            if (fullQualified == "boost")
                return true;
        }
    }
    return false;
}

bool BoostCodeParser::aliasedOrRealNamespace(const QByteArray &symbolName,
                                             const QString &origNamespace,
                                             QByteArray *simplifiedName, bool *aliasedOrReal)
{
    Overview overview;
    const QByteArray classOrNamespace = symbolName.left(symbolName.lastIndexOf("::"));
    const QList<LookupItem> lookup = m_typeOfExpression(classOrNamespace, m_doc->globalNamespace());
    for (const LookupItem &it : lookup) {
        if (auto classOrNamespaceSymbol = it.declaration()) {
            const QString fullQualified = overview.prettyName(
                        m_lookupContext.fullyQualifiedName(classOrNamespaceSymbol));
            if (fullQualified == origNamespace) {
                *aliasedOrReal = true;
                if (simplifiedName)
                    *simplifiedName = symbolName.mid(symbolName.lastIndexOf("::"));
                return true;
            }
            if (auto namespaceAlias = classOrNamespaceSymbol->asNamespaceAlias()) {
                if (auto aliasName = namespaceAlias->namespaceName()) {
                    if (overview.prettyName(aliasName) == origNamespace) {
                        *aliasedOrReal = true;
                        if (simplifiedName)
                            *simplifiedName = symbolName.mid(symbolName.lastIndexOf("::"));
                        return true;
                    }
                }
            }
        }
    }
    return true;
}

bool BoostCodeParser::evalCurrentDecorator(const QByteArray &decorator, QString *symbolName,
                                           QByteArray *simplifiedName, bool *aliasedOrReal)
{
    const QList<LookupItem> lookupItems = m_typeOfExpression(decorator, m_doc->globalNamespace());
    if (lookupItems.isEmpty())
        return false;

    Overview overview;
    Symbol *symbol = lookupItems.first().declaration();
    if (!symbol->name())
        return false;

    *symbolName = overview.prettyName(symbol->name());
    *aliasedOrReal = false;
    if (decorator.contains("::"))
        return aliasedOrRealNamespace(decorator, "boost::unit_test", simplifiedName, aliasedOrReal);
    return true;
}

} // namespace Internal
} // namespace Autotest
