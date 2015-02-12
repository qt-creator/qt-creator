/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "pythonscanner.h"
#include "../../pythoneditorconstants.h"
#include "../../pythoneditorplugin.h"

#include <QString>
#include <QSet>

namespace PythonEditor {
namespace Internal {

Scanner::Scanner(const QChar *text, const int length)
    : m_src(text, length)
    , m_state(0)
    , m_keywords(PythonEditorPlugin::keywords())
    , m_magics(PythonEditorPlugin::magics())
    , m_builtins(PythonEditorPlugin::builtins())
{
}

Scanner::~Scanner()
{
}

void Scanner::setState(int state)
{
    m_state = state;
}

int Scanner::state() const
{
    return m_state;
}

FormatToken Scanner::read()
{
    m_src.setAnchor();
    if (m_src.isEnd())
        return FormatToken(Format_EndOfBlock, m_src.anchor(), 0);

    State state;
    QChar saved;
    parseState(state, saved);
    switch (state) {
    case State_String:
        return readStringLiteral(saved);
    case State_MultiLineString:
        return readMultiLineStringLiteral(saved);
    default:
        return onDefaultState();
    }
}

QString Scanner::value(const FormatToken &tk) const
{
    return m_src.value(tk.begin(), tk.length());
}

FormatToken Scanner::onDefaultState()
{
    QChar first = m_src.peek();
    m_src.move();

    if (first == QLatin1Char('\\') && m_src.peek() == QLatin1Char('\n')) {
        m_src.move();
        return FormatToken(Format_Whitespace, m_src.anchor(), 2);
    }

    if (first == QLatin1Char('.') && m_src.peek().isDigit())
        return readFloatNumber();

    if (first == QLatin1Char('\'') || first == QLatin1Char('\"'))
        return readStringLiteral(first);

    if (first.isLetter() || (first == QLatin1Char('_')))
        return readIdentifier();

    if (first.isDigit())
        return readNumber();

    if (first == QLatin1Char('#')) {
        if (m_src.peek() == QLatin1Char('#'))
            return readDoxygenComment();
        return readComment();
    }

    if (first.isSpace())
        return readWhiteSpace();

    return readOperator();
}

/**
 * @brief Lexer::passEscapeCharacter
 * @return returns true if escape sequence doesn't end with newline
 */
void Scanner::checkEscapeSequence(QChar quoteChar)
{
    if (m_src.peek() == QLatin1Char('\\')) {
        m_src.move();
        QChar ch = m_src.peek();
        if (ch == QLatin1Char('\n') || ch.isNull())
            saveState(State_String, quoteChar);
    }
}

/**
  reads single-line string literal, surrounded by ' or " quotes
  */
FormatToken Scanner::readStringLiteral(QChar quoteChar)
{
    QChar ch = m_src.peek();
    if (ch == quoteChar && m_src.peek(1) == quoteChar) {
        saveState(State_MultiLineString, quoteChar);
        return readMultiLineStringLiteral(quoteChar);
    }

    while (ch != quoteChar && !ch.isNull()) {
        checkEscapeSequence(quoteChar);
        m_src.move();
        ch = m_src.peek();
    }
    if (ch == quoteChar)
        clearState();
    m_src.move();
    return FormatToken(Format_String, m_src.anchor(), m_src.length());
}

/**
  reads multi-line string literal, surrounded by ''' or """ sequencies
  */
FormatToken Scanner::readMultiLineStringLiteral(QChar quoteChar)
{
    for (;;) {
        QChar ch = m_src.peek();
        if (ch.isNull())
            break;
        if (ch == quoteChar
                && (m_src.peek(1) == quoteChar)
                && (m_src.peek(2) == quoteChar)) {
            clearState();
            m_src.move();
            m_src.move();
            m_src.move();
            break;
        }
        m_src.move();
    }

    return FormatToken(Format_String, m_src.anchor(), m_src.length());
}

/**
  reads identifier and classifies it
  */
FormatToken Scanner::readIdentifier()
{
    QChar ch = m_src.peek();
    while (ch.isLetterOrNumber() || (ch == QLatin1Char('_'))) {
        m_src.move();
        ch = m_src.peek();
    }
    QString value = m_src.value();

    Format tkFormat = Format_Identifier;
    if (value == QLatin1String("self"))
        tkFormat = Format_ClassField;
    else if (m_builtins.contains(value))
        tkFormat = Format_Type;
    else if (m_magics.contains(value))
        tkFormat = Format_MagicAttr;
    else if (m_keywords.contains(value))
        tkFormat = Format_Keyword;

    return FormatToken(tkFormat, m_src.anchor(), m_src.length());
}

inline static bool isHexDigit(QChar ch)
{
    return (ch.isDigit()
            || (ch >= QLatin1Char('a') && ch <= QLatin1Char('f'))
            || (ch >= QLatin1Char('A') && ch <= QLatin1Char('F')));
}

inline static bool isOctalDigit(QChar ch)
{
    return (ch.isDigit() && ch != QLatin1Char('8') && ch != QLatin1Char('9'));
}

inline static bool isBinaryDigit(QChar ch)
{
    return (ch == QLatin1Char('0') || ch == QLatin1Char('1'));
}

inline static bool isValidIntegerSuffix(QChar ch)
{
    return (ch == QLatin1Char('l') || ch == QLatin1Char('L'));
}

FormatToken Scanner::readNumber()
{
    if (!m_src.isEnd()) {
        QChar ch = m_src.peek();
        if (ch.toLower() == QLatin1Char('b')) {
            m_src.move();
            while (isBinaryDigit(m_src.peek()))
                m_src.move();
        } else if (ch.toLower() == QLatin1Char('o')) {
            m_src.move();
            while (isOctalDigit(m_src.peek()))
                m_src.move();
        } else if (ch.toLower() == QLatin1Char('x')) {
            m_src.move();
            while (isHexDigit(m_src.peek()))
                m_src.move();
        } else { // either integer or float number
            return readFloatNumber();
        }
        if (isValidIntegerSuffix(m_src.peek()))
            m_src.move();
    }
    return FormatToken(Format_Number, m_src.anchor(), m_src.length());
}

FormatToken Scanner::readFloatNumber()
{
    enum
    {
        State_INTEGER,
        State_FRACTION,
        State_EXPONENT
    } state;
    state = (m_src.peek(-1) == QLatin1Char('.')) ? State_FRACTION : State_INTEGER;

    for (;;) {
        QChar ch = m_src.peek();
        if (ch.isNull())
            break;

        if (state == State_INTEGER) {
            if (ch == QLatin1Char('.'))
                state = State_FRACTION;
            else if (!ch.isDigit())
                break;
        } else if (state == State_FRACTION) {
            if (ch == QLatin1Char('e') || ch == QLatin1Char('E')) {
                QChar next = m_src.peek(1);
                QChar next2 = m_src.peek(2);
                bool isExp = next.isDigit()
                        || (((next == QLatin1Char('-')) || (next == QLatin1Char('+'))) && next2.isDigit());
                if (isExp) {
                    m_src.move();
                    state = State_EXPONENT;
                } else {
                    break;
                }
            } else if (!ch.isDigit()) {
                break;
            }
        } else if (!ch.isDigit()) {
            break;
        }
        m_src.move();
    }

    QChar ch = m_src.peek();
    if ((state == State_INTEGER && (ch == QLatin1Char('l') || ch == QLatin1Char('L')))
            || (ch == QLatin1Char('j') || ch == QLatin1Char('J')))
        m_src.move();

    return FormatToken(Format_Number, m_src.anchor(), m_src.length());
}

/**
  reads single-line python comment, started with "#"
  */
FormatToken Scanner::readComment()
{
    QChar ch = m_src.peek();
    while (ch != QLatin1Char('\n') && !ch.isNull()) {
        m_src.move();
        ch = m_src.peek();
    }
    return FormatToken(Format_Comment, m_src.anchor(), m_src.length());
}

/**
  reads single-line python doxygen comment, started with "##"
  */
FormatToken Scanner::readDoxygenComment()
{
    QChar ch = m_src.peek();
    while (ch != QLatin1Char('\n') && !ch.isNull()) {
        m_src.move();
        ch = m_src.peek();
    }
    return FormatToken(Format_Doxygen, m_src.anchor(), m_src.length());
}

/**
  reads whitespace
  */
FormatToken Scanner::readWhiteSpace()
{
    while (m_src.peek().isSpace())
        m_src.move();
    return FormatToken(Format_Whitespace, m_src.anchor(), m_src.length());
}

/**
  reads punctuation symbols, excluding some special
  */
FormatToken Scanner::readOperator()
{
    const QString EXCLUDED_CHARS = QLatin1String("\'\"_#");
    QChar ch = m_src.peek();
    while (ch.isPunct() && !EXCLUDED_CHARS.contains(ch)) {
        m_src.move();
        ch = m_src.peek();
    }
    return FormatToken(Format_Operator, m_src.anchor(), m_src.length());
}

void Scanner::clearState()
{
    m_state = 0;
}

void Scanner::saveState(State state, QChar savedData)
{
    m_state = (state << 16) | static_cast<int>(savedData.unicode());
}

void Scanner::parseState(State &state, QChar &savedData) const
{
    state = static_cast<State>(m_state >> 16);
    savedData = static_cast<ushort>(m_state);
}

} // namespace Internal
} // namespace PythonEditor
