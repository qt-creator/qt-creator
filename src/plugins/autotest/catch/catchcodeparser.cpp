// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "catchcodeparser.h"

#include <cplusplus/Token.h>

#include <utils/qtcassert.h>

#include <QRegularExpression>

namespace Autotest {
namespace Internal {

using namespace CPlusPlus;

CatchCodeParser::CatchCodeParser(const QByteArray &source, const LanguageFeatures &features)
    : m_source(source)
    , m_features(features)
{
}

static CatchTestCodeLocationAndType locationAndTypeFromToken(const Token &tkn)
{
    CatchTestCodeLocationAndType locationAndType;
    locationAndType.m_type = TestTreeItem::TestCase;
    locationAndType.m_line = tkn.lineno;
    locationAndType.m_column = 0;
    return locationAndType;
}

static Tokens tokensForSource(const QByteArray &source, const LanguageFeatures &features)
{
    SimpleLexer lexer;
    lexer.setPreprocessorMode(false); // or true? does not make a difference so far..
    lexer.setLanguageFeatures(features);
    return lexer(QString::fromUtf8(source));
}

static QStringList parseTags(const QString &tagsString)
{
    QStringList tagsList;

    static const QRegularExpression tagRegEx("\\[(.*?)\\]", QRegularExpression::CaseInsensitiveOption);
    int pos = 0;
    QRegularExpressionMatch it = tagRegEx.match(tagsString, pos);
    while (it.hasMatch()) {
        tagsList.append(it.captured(1));
        pos += it.capturedLength();
        it = tagRegEx.match(tagsString, pos);
    }
    return tagsList;
}

CatchTestCodeLocationList CatchCodeParser::findTests()
{
    m_tokens = tokensForSource(m_source, m_features);
    m_currentIndex = 0;
    for ( ; m_currentIndex < m_tokens.size(); ++m_currentIndex) {
        if (m_tokens.at(m_currentIndex).kind() == T_IDENTIFIER)
            handleIdentifier();
    }
    return m_testCases;
}

void CatchCodeParser::handleIdentifier()
{
    QTC_ASSERT(m_currentIndex < m_tokens.size(), return);
    const Token &token = m_tokens.at(m_currentIndex);
    const QByteArray &identifier = m_source.mid(int(token.bytesBegin()), int(token.bytes()));
    const QByteArray unprefixed = identifier.startsWith("CATCH_") ? identifier.mid(6) : identifier;

    if (unprefixed == "TEST_CASE") {
        handleTestCase(false);
    } else if (unprefixed == "SCENARIO") {
        handleTestCase(true);
    } else if (unprefixed == "TEMPLATE_TEST_CASE" || unprefixed == "TEMPLATE_PRODUCT_TEST_CASE"
               || unprefixed == "TEMPLATE_LIST_TEST_CASE" || unprefixed == "TEMPLATE_TEST_CASE_SIG"
               || unprefixed == "TEMPLATE_PRODUCT_TEST_CASE_SIG") {
        handleParameterizedTestCase(false);
    } else if (unprefixed == "TEST_CASE_METHOD") {
        handleFixtureOrRegisteredTestCase(true);
    } else if (unprefixed == "TEMPLATE_TEST_CASE_METHOD_SIG"
               || unprefixed == "TEMPLATE_PRODUCT_TEST_CASE_METHOD_SIG"
               || unprefixed == "TEMPLATE_TEST_CASE_METHOD"
               || unprefixed == "TEMPLATE_LIST_TEST_CASE_METHOD") {
        handleParameterizedTestCase(true);
    } else if (unprefixed == "METHOD_AS_TEST_CASE" || unprefixed == "REGISTER_TEST_CASE") {
        handleFixtureOrRegisteredTestCase(false);
    }
}

void CatchCodeParser::handleTestCase(bool isScenario)
{
    if (!skipCommentsUntil(T_LPAREN))
        return;

    CatchTestCodeLocationAndType locationAndType
            = locationAndTypeFromToken(m_tokens.at(m_currentIndex));

    Kind stoppedAt;
    ++m_currentIndex;
    QString testCaseName = getStringLiteral(stoppedAt);
    QString tagsString;  // TODO: use them

    if (stoppedAt == T_COMMA) {
        ++m_currentIndex;
        tagsString = getStringLiteral(stoppedAt);
    }

    if (stoppedAt != T_RPAREN)
        return;

    if (isScenario)
        testCaseName.prepend("Scenario: "); // use a flag?

    locationAndType.m_name = testCaseName;
    locationAndType.tags = parseTags(tagsString);
    m_testCases.append(locationAndType);
}

void CatchCodeParser::handleParameterizedTestCase(bool isFixture)
{
    if (!skipCommentsUntil(T_LPAREN))
        return;

    if (isFixture && !skipFixtureParameter())
        return;

    CatchTestCodeLocationAndType locationAndType
            = locationAndTypeFromToken(m_tokens.at(m_currentIndex));

    Kind stoppedAt;
    ++m_currentIndex;
    QString testCaseName = getStringLiteral(stoppedAt);
    QString tagsString;

    if (stoppedAt != T_COMMA)
        return;

    ++m_currentIndex;
    tagsString = getStringLiteral(stoppedAt);

    if (stoppedAt == T_COMMA)
        stoppedAt = skipUntilCorrespondingRParen();

    if (stoppedAt != T_RPAREN)
        return;
    locationAndType.m_name = testCaseName;
    locationAndType.tags = parseTags(tagsString);
    locationAndType.states = CatchTreeItem::Parameterized;
    if (isFixture)
        locationAndType.states |= CatchTreeItem::Fixture;
    m_testCases.append(locationAndType);
}

void CatchCodeParser::handleFixtureOrRegisteredTestCase(bool isFixture)
{
    if (!skipCommentsUntil(T_LPAREN))
        return;

    if (isFixture) {
        if (!skipFixtureParameter())
            return;
    } else {
        if (!skipFunctionParameter())
            return;
    }

    CatchTestCodeLocationAndType locationAndType
            = locationAndTypeFromToken(m_tokens.at(m_currentIndex));

    Kind stoppedAt;
    ++m_currentIndex;
    QString testCaseName = getStringLiteral(stoppedAt);
    QString tagsString;

    if (stoppedAt == T_COMMA) {
        ++m_currentIndex;
        tagsString = getStringLiteral(stoppedAt);
    }

    if (stoppedAt != T_RPAREN)
        return;

    locationAndType.m_name = testCaseName;
    locationAndType.tags = parseTags(tagsString);
    if (isFixture)
        locationAndType.states = CatchTreeItem::Fixture;
    m_testCases.append(locationAndType);
}

QString CatchCodeParser::getStringLiteral(Kind &stoppedAtKind)
{
    QByteArray captured;
    int end = m_tokens.size();
    while (m_currentIndex < end) {
        const Token token = m_tokens.at(m_currentIndex);
        Kind kind = token.kind();
        if (kind == T_STRING_LITERAL) {
            // store the string without its quotes
            captured.append(m_source.mid(token.bytesBegin() + 1, token.bytes() - 2));
        } else if (kind == T_RPAREN || kind == T_COMMA) {
            stoppedAtKind = kind;
            return QString::fromUtf8(captured);
        } else if (!token.isComment()) { // comments are okay - but anything else will cancel
            stoppedAtKind = kind;
            return {};
        }
        ++m_currentIndex;
    }
    stoppedAtKind = T_ERROR;
    return {};
}

bool CatchCodeParser::skipCommentsUntil(Kind nextExpectedKind)
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

Kind CatchCodeParser::skipUntilCorrespondingRParen()
{
    int openParens = 1;  // we have already one open, looking for the corresponding closing
    int end = m_tokens.size();
    while (m_currentIndex < end) {
        Kind kind = m_tokens.at(m_currentIndex).kind();
        if (kind == T_LPAREN) {
            ++openParens;
        } else if (kind == T_RPAREN) {
            --openParens;
            if (openParens == 0)
                return T_RPAREN;
        }
        ++m_currentIndex;
    }
    return T_ERROR;
}

bool CatchCodeParser::skipFixtureParameter()
{
    if (!skipCommentsUntil(T_IDENTIFIER))
        return false;
    return skipCommentsUntil(T_COMMA);
}

bool CatchCodeParser::skipFunctionParameter()
{
    if (!skipCommentsUntil(T_IDENTIFIER))
        return false;
    if (skipCommentsUntil(T_COLON_COLON))
        return skipFunctionParameter();

    return skipCommentsUntil(T_COMMA);
}

} // namespace Internal
} // namespace Autotest
