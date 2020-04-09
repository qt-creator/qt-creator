/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "catchcodeparser.h"

#include <cplusplus/Token.h>

#include <utils/qtcassert.h>

#include <QRegularExpression>

namespace Autotest {
namespace Internal {

using namespace CPlusPlus;

CatchCodeParser::CatchCodeParser(const QByteArray &source, const LanguageFeatures &features,
                                 const Document::Ptr &doc, const Snapshot &snapshot)
    : m_source(source)
    , m_features(features)
    , m_doc(doc)
    , m_snapshot(snapshot)
{
}

static TestCodeLocationAndType locationAndTypeFromToken(const Token &tkn)
{
    TestCodeLocationAndType locationAndType;
    locationAndType.m_type = TestTreeItem::TestFunction;
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

    const QRegularExpression tagRegEx("\\[(.*?)\\]",QRegularExpression::CaseInsensitiveOption);
    int pos = 0;
    QRegularExpressionMatch it = tagRegEx.match(tagsString, pos);
    while (it.hasMatch()) {
        tagsList.append(it.captured(1));
        pos += it.capturedLength();
        it = tagRegEx.match(tagsString, pos);
    }
    return tagsList;
}

TestCodeLocationList CatchCodeParser::findTests()
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

void CatchCodeParser::handleIdentifier()
{
    QTC_ASSERT(m_currentIndex < m_tokens.size(), return);
    const Token &token = m_tokens.at(m_currentIndex);
    const QByteArray &identifier = m_source.mid(int(token.bytesBegin()), int(token.bytes()));
    if (identifier == "TEST_CASE") {
        handleTestCase(false);
    } else if (identifier == "SCENARIO") {
        handleTestCase(true);
    }
}

void CatchCodeParser::handleTestCase(bool isScenario)
{
    if (!skipCommentsUntil(T_LPAREN))
        return;

    Token token = m_tokens.at(m_currentIndex);
    TestCodeLocationAndType locationAndType = locationAndTypeFromToken(token);

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

} // namespace Internal
} // namespace Autotest
