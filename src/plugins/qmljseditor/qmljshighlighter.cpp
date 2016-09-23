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

#include "qmljshighlighter.h"

#include <QSet>

#include <utils/qtcassert.h>

using namespace QmlJS;
using namespace TextEditor;

namespace QmlJSEditor {

QmlJSHighlighter::QmlJSHighlighter(QTextDocument *parent)
    : SyntaxHighlighter(parent),
      m_qmlEnabled(true),
      m_braceDepth(0),
      m_foldingIndent(0),
      m_inMultilineComment(false)
{
    m_currentBlockParentheses.reserve(20);
    static const QVector<TextStyle> categories({
        C_NUMBER, C_STRING, C_TYPE, C_KEYWORD, C_FIELD, C_COMMENT, C_VISUAL_WHITESPACE
    });
    setTextFormatCategories(categories);
}

QmlJSHighlighter::~QmlJSHighlighter()
{
}

bool QmlJSHighlighter::isQmlEnabled() const
{
    return m_qmlEnabled;
}

void QmlJSHighlighter::setQmlEnabled(bool qmlEnabled)
{
    m_qmlEnabled = qmlEnabled;
}

void QmlJSHighlighter::highlightBlock(const QString &text)
{
    const QList<Token> tokens = m_scanner(text, onBlockStart());

    int index = 0;
    while (index < tokens.size()) {
        const Token &token = tokens.at(index);

        switch (token.kind) {
            case Token::Keyword:
                setFormat(token.offset, token.length, formatForCategory(KeywordFormat));
                break;

            case Token::String:
                setFormat(token.offset, token.length, formatForCategory(StringFormat));
                break;

            case Token::Comment:
                if (m_inMultilineComment && text.midRef(token.end() - 2, 2) == QLatin1String("*/")) {
                    onClosingParenthesis(QLatin1Char('-'), token.end() - 1, index == tokens.size()-1);
                    m_inMultilineComment = false;
                } else if (!m_inMultilineComment
                           && (m_scanner.state() & Scanner::MultiLineMask) == Scanner::MultiLineComment
                           && index == tokens.size() - 1) {
                    onOpeningParenthesis(QLatin1Char('+'), token.offset, index == 0);
                    m_inMultilineComment = true;
                }
                setFormat(token.offset, token.length, formatForCategory(CommentFormat));
                break;

            case Token::RegExp:
                setFormat(token.offset, token.length, formatForCategory(StringFormat));
                break;

            case Token::LeftParenthesis:
                onOpeningParenthesis(QLatin1Char('('), token.offset, index == 0);
                break;

            case Token::RightParenthesis:
                onClosingParenthesis(QLatin1Char(')'), token.offset, index == tokens.size()-1);
                break;

            case Token::LeftBrace:
                onOpeningParenthesis(QLatin1Char('{'), token.offset, index == 0);
                break;

            case Token::RightBrace:
                onClosingParenthesis(QLatin1Char('}'), token.offset, index == tokens.size()-1);
                break;

            case Token::LeftBracket:
                onOpeningParenthesis(QLatin1Char('['), token.offset, index == 0);
                break;

            case Token::RightBracket:
                onClosingParenthesis(QLatin1Char(']'), token.offset, index == tokens.size()-1);
                break;

            case Token::Identifier: {
                if (!m_qmlEnabled)
                    break;

                const QStringRef spell = text.midRef(token.offset, token.length);

                if (maybeQmlKeyword(spell)) {
                    // check the previous token
                    if (index == 0 || tokens.at(index - 1).isNot(Token::Dot)) {
                        if (index + 1 == tokens.size() || tokens.at(index + 1).isNot(Token::Colon)) {
                            setFormat(token.offset, token.length, formatForCategory(KeywordFormat));
                            break;
                        }
                    }
                } else if (index > 0 && maybeQmlBuiltinType(spell)) {
                    const Token &previousToken = tokens.at(index - 1);
                    if (previousToken.is(Token::Identifier) && text.at(previousToken.offset) == QLatin1Char('p')
                        && text.midRef(previousToken.offset, previousToken.length) == QLatin1String("property")) {
                        setFormat(token.offset, token.length, formatForCategory(KeywordFormat));
                        break;
                    }
                }
            }   break;

            case Token::Delimiter:
                break;

            default:
                break;
        } // end swtich

        ++index;
    }

    int previousTokenEnd = 0;
    for (int index = 0; index < tokens.size(); ++index) {
        const Token &token = tokens.at(index);
        setFormat(previousTokenEnd, token.begin() - previousTokenEnd, formatForCategory(VisualWhitespace));

        switch (token.kind) {
        case Token::Comment:
        case Token::String:
        case Token::RegExp: {
            int i = token.begin(), e = token.end();
            while (i < e) {
                const QChar ch = text.at(i);
                if (ch.isSpace()) {
                    const int start = i;
                    do {
                        ++i;
                    } while (i < e && text.at(i).isSpace());
                    setFormat(start, i - start, formatForCategory(VisualWhitespace));
                } else {
                    ++i;
                }
            }
        } break;

        default:
            break;
        } // end of switch

        previousTokenEnd = token.end();
    }

    setFormat(previousTokenEnd, text.length() - previousTokenEnd, formatForCategory(VisualWhitespace));

    setCurrentBlockState(m_scanner.state());
    onBlockEnd(m_scanner.state());
}

bool QmlJSHighlighter::maybeQmlKeyword(const QStringRef &text) const
{
    if (text.isEmpty())
        return false;

    const QChar ch = text.at(0);
    if (ch == QLatin1Char('p') && text == QLatin1String("property"))
        return true;
    else if (ch == QLatin1Char('a') && text == QLatin1String("alias"))
        return true;
    else if (ch == QLatin1Char('s') && text == QLatin1String("signal"))
        return true;
    else if (ch == QLatin1Char('p') && text == QLatin1String("property"))
        return true;
    else if (ch == QLatin1Char('r') && text == QLatin1String("readonly"))
        return true;
    else if (ch == QLatin1Char('i') && text == QLatin1String("import"))
        return true;
    else if (ch == QLatin1Char('o') && text == QLatin1String("on"))
        return true;
    else
        return false;
}

bool QmlJSHighlighter::maybeQmlBuiltinType(const QStringRef &text) const
{
    if (text.isEmpty())
        return false;

    const QChar ch = text.at(0);

    if (ch == QLatin1Char('a') && text == QLatin1String("action"))
            return true;
    else if (ch == QLatin1Char('b') && text == QLatin1String("bool"))
        return true;
    else if (ch == QLatin1Char('c') && text == QLatin1String("color"))
        return true;
    else if (ch == QLatin1Char('d') && text == QLatin1String("date"))
        return true;
    else if (ch == QLatin1Char('d') && text == QLatin1String("double"))
        return true;
    else if (ch == QLatin1Char('e') && text == QLatin1String("enumeration"))
        return true;
    else if (ch == QLatin1Char('f') && text == QLatin1String("font"))
        return true;
    else if (ch == QLatin1Char('i') && text == QLatin1String("int"))
        return true;
    else if (ch == QLatin1Char('l') && text == QLatin1String("list"))
        return true;
    else if (ch == QLatin1Char('m') && text == QLatin1String("matrix4x4"))
        return true;
    else if (ch == QLatin1Char('p') && text == QLatin1String("point"))
        return true;
    else if (ch == QLatin1Char('q') && text == QLatin1String("quaternion"))
        return true;
    else if (ch == QLatin1Char('r') && text == QLatin1String("real"))
        return true;
    else if (ch == QLatin1Char('r') && text == QLatin1String("rect"))
        return true;
    else if (ch == QLatin1Char('s') && text == QLatin1String("size"))
        return true;
    else if (ch == QLatin1Char('s') && text == QLatin1String("string"))
        return true;
    else if (ch == QLatin1Char('t') && text == QLatin1String("time"))
        return true;
    else if (ch == QLatin1Char('u') && text == QLatin1String("url"))
        return true;
    else if (ch == QLatin1Char('v') && text == QLatin1String("variant"))
        return true;
    else if (ch == QLatin1Char('v') && text == QLatin1String("var"))
        return true;
    else if (ch == QLatin1Char('v') && text == QLatin1String("vector2d"))
        return true;
    else if (ch == QLatin1Char('v') && text == QLatin1String("vector3d"))
        return true;
    else if (ch == QLatin1Char('v') && text == QLatin1String("vector4d"))
        return true;
    else
        return false;
}

int QmlJSHighlighter::onBlockStart()
{
    m_currentBlockParentheses.clear();
    m_braceDepth = 0;
    m_foldingIndent = 0;
    m_inMultilineComment = false;
    if (TextBlockUserData *userData = TextDocumentLayout::testUserData(currentBlock())) {
        userData->setFoldingIndent(0);
        userData->setFoldingStartIncluded(false);
        userData->setFoldingEndIncluded(false);
    }

    int state = 0;
    int previousState = previousBlockState();
    if (previousState != -1) {
        state = previousState & 0xff;
        m_braceDepth = (previousState >> 8);
        m_inMultilineComment = ((state & Scanner::MultiLineMask) == Scanner::MultiLineComment);
    }
    m_foldingIndent = m_braceDepth;

    return state;
}

void QmlJSHighlighter::onBlockEnd(int state)
{
    setCurrentBlockState((m_braceDepth << 8) | state);
    TextDocumentLayout::setParentheses(currentBlock(), m_currentBlockParentheses);
    TextDocumentLayout::setFoldingIndent(currentBlock(), m_foldingIndent);
}

void QmlJSHighlighter::onOpeningParenthesis(QChar parenthesis, int pos, bool atStart)
{
    if (parenthesis == QLatin1Char('{') || parenthesis == QLatin1Char('[') || parenthesis == QLatin1Char('+')) {
        ++m_braceDepth;
        // if a folding block opens at the beginning of a line, treat the entire line
        // as if it were inside the folding block
        if (atStart)
            TextDocumentLayout::userData(currentBlock())->setFoldingStartIncluded(true);
    }
    m_currentBlockParentheses.push_back(Parenthesis(Parenthesis::Opened, parenthesis, pos));
}

void QmlJSHighlighter::onClosingParenthesis(QChar parenthesis, int pos, bool atEnd)
{
    if (parenthesis == QLatin1Char('}') || parenthesis == QLatin1Char(']') || parenthesis == QLatin1Char('-')) {
        --m_braceDepth;
        if (atEnd)
            TextDocumentLayout::userData(currentBlock())->setFoldingEndIncluded(true);
        else
            m_foldingIndent = qMin(m_braceDepth, m_foldingIndent); // folding indent is the minimum brace depth of a block
    }
    m_currentBlockParentheses.push_back(Parenthesis(Parenthesis::Closed, parenthesis, pos));
}

} // namespace QmlJSEditor
