/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qmljshighlighter.h"

#include <QtCore/QSet>
#include <QtCore/QtAlgorithms>
#include <QtCore/QDebug>

#include <utils/qtcassert.h>

using namespace QmlJSEditor;
using namespace QmlJS;

Highlighter::Highlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent),
      m_qmlEnabled(true)
{
    m_currentBlockParentheses.reserve(20);
    m_braceDepth = 0;
}

Highlighter::~Highlighter()
{
}

bool Highlighter::isQmlEnabled() const
{
    return m_qmlEnabled;
}

void Highlighter::setQmlEnabled(bool qmlEnabled)
{
    m_qmlEnabled = qmlEnabled;
}

QTextCharFormat Highlighter::labelTextCharFormat() const
{
    return m_formats[LabelFormat];
}

static bool checkStartOfBinding(const Token &token)
{
    switch (token.kind) {
    case Token::Semicolon:
    case Token::LeftBrace:
    case Token::RightBrace:
    case Token::LeftBracket:
    case Token::RightBracket:
        return true;

    default:
        return false;
    } // end of switch
}

void Highlighter::highlightBlock(const QString &text)
{
    const QList<Token> tokens = m_scanner(text, onBlockStart());

    int index = 0;
    while (index < tokens.size()) {
        const Token &token = tokens.at(index);

        switch (token.kind) {
            case Token::Keyword:
                setFormat(token.offset, token.length, m_formats[KeywordFormat]);
                break;

            case Token::String:
                setFormat(token.offset, token.length, m_formats[StringFormat]);
                break;

            case Token::Comment:
                setFormat(token.offset, token.length, m_formats[CommentFormat]);
                break;

            case Token::LeftParenthesis:
                onOpeningParenthesis('(', token.offset);
                break;

            case Token::RightParenthesis:
                onClosingParenthesis(')', token.offset);
                break;

            case Token::LeftBrace:
                onOpeningParenthesis('{', token.offset);
                break;

            case Token::RightBrace:
                onClosingParenthesis('}', token.offset);
                break;

            case Token::LeftBracket:
                onOpeningParenthesis('[', token.offset);
                break;

            case Token::RightBracket:
                onClosingParenthesis(']', token.offset);
                break;

            case Token::Identifier: {
                const QStringRef spell = text.midRef(token.offset, token.length);

                if (m_qmlEnabled && maybeQmlKeyword(spell)) {
                    // check the previous token
                    if (index == 0 || tokens.at(index - 1).isNot(Token::Dot)) {
                        if (index + 1 == tokens.size() || tokens.at(index + 1).isNot(Token::Colon)) {
                            setFormat(token.offset, token.length, m_formats[KeywordFormat]);
                            break;
                        }
                    }
                } else if (m_qmlEnabled && index > 0 && maybeQmlBuiltinType(spell)) {
                    const Token &previousToken = tokens.at(index - 1);
                    if (previousToken.is(Token::Identifier) && text.at(previousToken.offset) == QLatin1Char('p')
                        && text.midRef(previousToken.offset, previousToken.length) == QLatin1String("property")) {
                        setFormat(token.offset, token.length, m_formats[KeywordFormat]);
                        break;
                    }
                }

                if (index + 1 < tokens.size()) {
                    if (tokens.at(index + 1).is(Token::LeftBrace) && text.at(token.offset).isUpper()) {
                        setFormat(token.offset, token.length, m_formats[TypeFormat]);
                    } else if (index == 0 || checkStartOfBinding(tokens.at(index - 1))) {
                        const int start = index;

                        ++index; // skip the identifier.
                        while (index + 1 < tokens.size() &&
                               tokens.at(index).is(Token::Dot) &&
                               tokens.at(index + 1).is(Token::Identifier)) {
                            index += 2;
                        }

                        if (index < tokens.size() && tokens.at(index).is(Token::Colon)) {
                            // it's a binding.
                            for (int i = start; i < index; ++i) {
                                const Token &tok = tokens.at(i);
                                setFormat(tok.offset, tok.length, m_formats[LabelFormat]);
                            }
                            break;
                        } else {
                            index = start;
                        }
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
        setFormat(previousTokenEnd, token.begin() - previousTokenEnd, m_formats[VisualWhitespace]);

        switch (token.kind) {
        case Token::Comment:
        case Token::String: {
            int i = token.begin(), e = token.end();
            while (i < e) {
                const QChar ch = text.at(i);
                if (ch.isSpace()) {
                    const int start = i;
                    do {
                        ++i;
                    } while (i < e && text.at(i).isSpace());
                    setFormat(start, i - start, m_formats[VisualWhitespace]);
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

    setFormat(previousTokenEnd, text.length() - previousTokenEnd, m_formats[VisualWhitespace]);

    int firstNonSpace = 0;
    if (! tokens.isEmpty())
        firstNonSpace = tokens.first().offset;

    setCurrentBlockState(m_scanner.state());
    onBlockEnd(m_scanner.state(), firstNonSpace);
}

bool Highlighter::maybeQmlKeyword(const QStringRef &text) const
{
    if (text.isEmpty())
        return false;

    const QChar ch = text.at(0);
    if (ch == QLatin1Char('p') && text == QLatin1String("property")) {
        return true;
    } else if (ch == QLatin1Char('a') && text == QLatin1String("alias")) {
        return true;
    } else if (ch == QLatin1Char('s') && text == QLatin1String("signal")) {
        return true;
    } else if (ch == QLatin1Char('p') && text == QLatin1String("property")) {
        return true;
    } else if (ch == QLatin1Char('r') && text == QLatin1String("readonly")) {
        return true;
    } else if (ch == QLatin1Char('i') && text == QLatin1String("import")) {
        return true;
    } else {
        return false;
    }
}

bool Highlighter::maybeQmlBuiltinType(const QStringRef &text) const
{
    if (text.isEmpty())
        return false;

    const QChar ch = text.at(0);

    if (ch == QLatin1Char('i') && text == QLatin1String("int")) {
        return true;
    } else if (ch == QLatin1Char('b') && text == QLatin1String("bool")) {
        return true;
    } else if (ch == QLatin1Char('d') && text == QLatin1String("double")) {
        return true;
    } else if (ch == QLatin1Char('r') && text == QLatin1String("real")) {
        return true;
    } else if (ch == QLatin1Char('s') && text == QLatin1String("string")) {
        return true;
    } else if (ch == QLatin1Char('u') && text == QLatin1String("url")) {
        return true;
    } else if (ch == QLatin1Char('c') && text == QLatin1String("color")) {
        return true;
    } else if (ch == QLatin1Char('d') && text == QLatin1String("date")) {
        return true;
    } else if (ch == QLatin1Char('v') && text == QLatin1String("var")) {
        return true;
    } else if (ch == QLatin1Char('v') && text == QLatin1String("variant")) {
        return true;
    } else {
        return false;
    }
}

int Highlighter::onBlockStart()
{
    m_currentBlockParentheses.clear();
    m_braceDepth = 0;

    int state = 0;
    int previousState = previousBlockState();
    if (previousState != -1) {
        state = previousState & 0xff;
        m_braceDepth = previousState >> 8;
    }

    return state;
}

void Highlighter::onBlockEnd(int state, int firstNonSpace)
{
    typedef TextEditor::TextBlockUserData TextEditorBlockData;

    setCurrentBlockState((m_braceDepth << 8) | state);

    // Set block data parentheses. Force creation of block data unless empty
    TextEditorBlockData *blockData = 0;

    if (QTextBlockUserData *userData = currentBlockUserData())
        blockData = static_cast<TextEditorBlockData *>(userData);

    if (!blockData && !m_currentBlockParentheses.empty()) {
        blockData = new TextEditorBlockData;
        setCurrentBlockUserData(blockData);
    }
    if (blockData) {
        blockData->setParentheses(m_currentBlockParentheses);
        blockData->setClosingCollapseMode(TextEditor::TextBlockUserData::NoClosingCollapse);
        blockData->setCollapseMode(TextEditor::TextBlockUserData::NoCollapse);
    }
    if (!m_currentBlockParentheses.isEmpty()) {
        QTC_ASSERT(blockData, return);
        int collapse = Parenthesis::collapseAtPos(m_currentBlockParentheses);
        if (collapse >= 0) {
            if (collapse == firstNonSpace)
                blockData->setCollapseMode(TextEditor::TextBlockUserData::CollapseThis);
            else
                blockData->setCollapseMode(TextEditor::TextBlockUserData::CollapseAfter);
        }
        if (Parenthesis::hasClosingCollapse(m_currentBlockParentheses))
            blockData->setClosingCollapseMode(TextEditor::TextBlockUserData::NoClosingCollapse);
    }
}

void Highlighter::onOpeningParenthesis(QChar parenthesis, int pos)
{
    if (parenthesis == QLatin1Char('{') || parenthesis == QLatin1Char('['))
        ++m_braceDepth;
    m_currentBlockParentheses.push_back(Parenthesis(Parenthesis::Opened, parenthesis, pos));
}

void Highlighter::onClosingParenthesis(QChar parenthesis, int pos)
{
    if (parenthesis == QLatin1Char('}') || parenthesis == QLatin1Char(']'))
        --m_braceDepth;
    m_currentBlockParentheses.push_back(Parenthesis(Parenthesis::Closed, parenthesis, pos));
}

