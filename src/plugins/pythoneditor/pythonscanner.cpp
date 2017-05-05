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

#include "pythonscanner.h"

#include "pythoneditorconstants.h"
#include "pythoneditorplugin.h"

namespace PythonEditor {
namespace Internal {

Scanner::Scanner(const QChar *text, const int length)
    : m_text(text), m_textLength(length), m_state(0)
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
    setAnchor();
    if (isEnd())
        return FormatToken();

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
    return QString(m_text + tk.begin(), tk.length());
}

FormatToken Scanner::onDefaultState()
{
    QChar first = peek();
    move();

    if (first == '\\' && peek() == '\n') {
        move();
        return FormatToken(Format_Whitespace, anchor(), 2);
    }

    if (first == '.' && peek().isDigit())
        return readFloatNumber();

    if (first == '\'' || first == '\"')
        return readStringLiteral(first);

    if (first.isLetter() || first == '_')
        return readIdentifier();

    if (first.isDigit())
        return readNumber();

    if (first == '#') {
        if (peek() == '#')
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
    if (peek() == '\\') {
        move();
        QChar ch = peek();
        if (ch == '\n' || ch.isNull())
            saveState(State_String, quoteChar);
    }
}

/**
  reads single-line string literal, surrounded by ' or " quotes
  */
FormatToken Scanner::readStringLiteral(QChar quoteChar)
{
    QChar ch = peek();
    if (ch == quoteChar && peek(1) == quoteChar) {
        saveState(State_MultiLineString, quoteChar);
        return readMultiLineStringLiteral(quoteChar);
    }

    while (ch != quoteChar && !ch.isNull()) {
        checkEscapeSequence(quoteChar);
        move();
        ch = peek();
    }
    if (ch == quoteChar)
        clearState();
    move();
    return FormatToken(Format_String, anchor(), length());
}

/**
  reads multi-line string literal, surrounded by ''' or """ sequences
  */
FormatToken Scanner::readMultiLineStringLiteral(QChar quoteChar)
{
    for (;;) {
        QChar ch = peek();
        if (ch.isNull())
            break;
        if (ch == quoteChar && peek(1) == quoteChar && peek(2) == quoteChar) {
            clearState();
            move();
            move();
            move();
            break;
        }
        move();
    }

    return FormatToken(Format_String, anchor(), length());
}

/**
  reads identifier and classifies it
  */
FormatToken Scanner::readIdentifier()
{
    static const QSet<QString> keywords = {
        "and", "as", "assert", "break", "class", "continue", "def", "del", "elif",
        "else", "except", "exec", "finally", "for", "from", "global", "if", "import",
        "in", "is", "lambda", "not", "or", "pass", "print", "raise", "return", "try",
        "while", "with", "yield"
    };

    // List of Python magic methods and attributes
    static const QSet<QString> magics = {
        // ctor & dtor
        "__init__", "__del__",
        // string conversion functions
        "__str__", "__repr__", "__unicode__",
        // attribute access functions
        "__setattr__", "__getattr__", "__delattr__",
        // binary operators
        "__add__", "__sub__", "__mul__", "__truediv__", "__floordiv__", "__mod__",
        "__pow__", "__and__", "__or__", "__xor__", "__eq__", "__ne__", "__gt__",
        "__lt__", "__ge__", "__le__", "__lshift__", "__rshift__", "__contains__",
        // unary operators
        "__pos__", "__neg__", "__inv__", "__abs__", "__len__",
        // item operators like []
        "__getitem__", "__setitem__", "__delitem__", "__getslice__", "__setslice__",
        "__delslice__",
        // other functions
        "__cmp__", "__hash__", "__nonzero__", "__call__", "__iter__", "__reversed__",
        "__divmod__", "__int__", "__long__", "__float__", "__complex__", "__hex__",
        "__oct__", "__index__", "__copy__", "__deepcopy__", "__sizeof__", "__trunc__",
        "__format__",
        // magic attributes
        "__name__", "__module__", "__dict__", "__bases__", "__doc__"
    };

    // List of python built-in functions and objects
    static const QSet<QString> builtins = {
        "range", "xrange", "int", "float", "long", "hex", "oct" "chr", "ord",
        "len", "abs", "None", "True", "False"
    };

    QChar ch = peek();
    while (ch.isLetterOrNumber() || ch == '_') {
        move();
        ch = peek();
    }

    const QString v = QString(m_text + m_markedPosition, length());
    Format tkFormat = Format_Identifier;
    if (v == "self")
        tkFormat = Format_ClassField;
    else if (builtins.contains(v))
        tkFormat = Format_Type;
    else if (magics.contains(v))
        tkFormat = Format_MagicAttr;
    else if (keywords.contains(v))
        tkFormat = Format_Keyword;

    return FormatToken(tkFormat, anchor(), length());
}

inline static bool isHexDigit(QChar ch)
{
    return ch.isDigit()
            || (ch >= 'a' && ch <= 'f')
            || (ch >= 'A' && ch <= 'F');
}

inline static bool isOctalDigit(QChar ch)
{
    return ch.isDigit() && ch != '8' && ch != '9';
}

inline static bool isBinaryDigit(QChar ch)
{
    return ch == '0' || ch == '1';
}

inline static bool isValidIntegerSuffix(QChar ch)
{
    return ch == 'l' || ch == 'L';
}

FormatToken Scanner::readNumber()
{
    if (!isEnd()) {
        QChar ch = peek();
        if (ch.toLower() == 'b') {
            move();
            while (isBinaryDigit(peek()))
                move();
        } else if (ch.toLower() == 'o') {
            move();
            while (isOctalDigit(peek()))
                move();
        } else if (ch.toLower() == 'x') {
            move();
            while (isHexDigit(peek()))
                move();
        } else { // either integer or float number
            return readFloatNumber();
        }
        if (isValidIntegerSuffix(peek()))
            move();
    }
    return FormatToken(Format_Number, anchor(), length());
}

FormatToken Scanner::readFloatNumber()
{
    enum
    {
        State_INTEGER,
        State_FRACTION,
        State_EXPONENT
    } state;
    state = (peek(-1) == '.') ? State_FRACTION : State_INTEGER;

    for (;;) {
        QChar ch = peek();
        if (ch.isNull())
            break;

        if (state == State_INTEGER) {
            if (ch == '.')
                state = State_FRACTION;
            else if (!ch.isDigit())
                break;
        } else if (state == State_FRACTION) {
            if (ch == 'e' || ch == 'E') {
                QChar next = peek(1);
                QChar next2 = peek(2);
                bool isExp = next.isDigit()
                        || ((next == '-' || next == '+') && next2.isDigit());
                if (isExp) {
                    move();
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
        move();
    }

    QChar ch = peek();
    if ((state == State_INTEGER && (ch == 'l' || ch == 'L'))
            || (ch == 'j' || ch =='J'))
        move();

    return FormatToken(Format_Number, anchor(), length());
}

/**
  reads single-line python comment, started with "#"
  */
FormatToken Scanner::readComment()
{
    QChar ch = peek();
    while (ch != '\n' && !ch.isNull()) {
        move();
        ch = peek();
    }
    return FormatToken(Format_Comment, anchor(), length());
}

/**
  reads single-line python doxygen comment, started with "##"
  */
FormatToken Scanner::readDoxygenComment()
{
    QChar ch = peek();
    while (ch != '\n' && !ch.isNull()) {
        move();
        ch = peek();
    }
    return FormatToken(Format_Doxygen, anchor(), length());
}

/**
  reads whitespace
  */
FormatToken Scanner::readWhiteSpace()
{
    while (peek().isSpace())
        move();
    return FormatToken(Format_Whitespace, anchor(), length());
}

/**
  reads punctuation symbols, excluding some special
  */
FormatToken Scanner::readOperator()
{
    static const QString EXCLUDED_CHARS = "\'\"_#";
    QChar ch = peek();
    while (ch.isPunct() && !EXCLUDED_CHARS.contains(ch)) {
        move();
        ch = peek();
    }
    return FormatToken(Format_Operator, anchor(), length());
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
