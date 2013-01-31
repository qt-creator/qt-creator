/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "qmljslexer_p.h"
#include "qmljsengine_p.h"
#include "qmljsmemorypool_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QVarLengthArray>
#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE
Q_CORE_EXPORT double qstrtod(const char *s00, char const **se, bool *ok);
QT_END_NAMESPACE

using namespace QmlJS;

static int regExpFlagFromChar(const QChar &ch)
{
    switch (ch.unicode()) {
    case 'g': return Lexer::RegExp_Global;
    case 'i': return Lexer::RegExp_IgnoreCase;
    case 'm': return Lexer::RegExp_Multiline;
    }
    return 0;
}

static unsigned char convertHex(ushort c)
{
    if (c >= '0' && c <= '9')
        return (c - '0');
    else if (c >= 'a' && c <= 'f')
        return (c - 'a' + 10);
    else
        return (c - 'A' + 10);
}

static QChar convertHex(QChar c1, QChar c2)
{
    return QChar((convertHex(c1.unicode()) << 4) + convertHex(c2.unicode()));
}

static QChar convertUnicode(QChar c1, QChar c2, QChar c3, QChar c4)
{
    return QChar((convertHex(c3.unicode()) << 4) + convertHex(c4.unicode()),
                 (convertHex(c1.unicode()) << 4) + convertHex(c2.unicode()));
}

Lexer::Lexer(Engine *engine)
    : _engine(engine)
    , _codePtr(0)
    , _lastLinePtr(0)
    , _tokenLinePtr(0)
    , _tokenStartPtr(0)
    , _char(QLatin1Char('\n'))
    , _errorCode(NoError)
    , _currentLineNumber(0)
    , _tokenValue(0)
    , _parenthesesState(IgnoreParentheses)
    , _parenthesesCount(0)
    , _stackToken(-1)
    , _patternFlags(0)
    , _tokenKind(0)
    , _tokenLength(0)
    , _tokenLine(0)
    , _validTokenText(false)
    , _prohibitAutomaticSemicolon(false)
    , _restrictedKeyword(false)
    , _terminator(false)
    , _followsClosingBrace(false)
    , _delimited(true)
    , _qmlMode(true)
{
    if (engine)
        engine->setLexer(this);
}

bool Lexer::qmlMode() const
{
    return _qmlMode;
}

QString Lexer::code() const
{
    return _code;
}

void Lexer::setCode(const QString &code, int lineno, bool qmlMode)
{
    if (_engine)
        _engine->setCode(code);

    _qmlMode = qmlMode;
    _code = code;
    _tokenText.clear();
    _tokenText.reserve(1024);
    _errorMessage.clear();
    _tokenSpell = QStringRef();

    _codePtr = code.unicode();
    _lastLinePtr = _codePtr;
    _tokenLinePtr = _codePtr;
    _tokenStartPtr = _codePtr;

    _char = QLatin1Char('\n');
    _errorCode = NoError;

    _currentLineNumber = lineno;
    _tokenValue = 0;

    // parentheses state
    _parenthesesState = IgnoreParentheses;
    _parenthesesCount = 0;

    _stackToken = -1;

    _patternFlags = 0;
    _tokenLength = 0;
    _tokenLine = lineno;

    _validTokenText = false;
    _prohibitAutomaticSemicolon = false;
    _restrictedKeyword = false;
    _terminator = false;
    _followsClosingBrace = false;
    _delimited = true;
}

void Lexer::scanChar()
{
    unsigned sequenceLength = isLineTerminatorSequence();
    _char = *_codePtr++;
    if (sequenceLength == 2)
        _char = *_codePtr++;

    if (unsigned sequenceLength = isLineTerminatorSequence()) {
        _lastLinePtr = _codePtr + sequenceLength - 1; // points to the first character after the newline
        ++_currentLineNumber;
    }
}

int Lexer::lex()
{
    const int previousTokenKind = _tokenKind;

    _tokenSpell = QStringRef();
    _tokenKind = scanToken();
    _tokenLength = _codePtr - _tokenStartPtr - 1;

    _delimited = false;
    _restrictedKeyword = false;
    _followsClosingBrace = (previousTokenKind == T_RBRACE);

    // update the flags
    switch (_tokenKind) {
    case T_LBRACE:
    case T_SEMICOLON:
    case T_COLON:
        _delimited = true;
        break;

    case T_IF:
    case T_FOR:
    case T_WHILE:
    case T_WITH:
        _parenthesesState = CountParentheses;
        _parenthesesCount = 0;
        break;

    case T_DO:
        _parenthesesState = BalancedParentheses;
        break;

    case T_CONTINUE:
    case T_BREAK:
    case T_RETURN:
    case T_THROW:
        _restrictedKeyword = true;
        break;
    } // switch

    // update the parentheses state
    switch (_parenthesesState) {
    case IgnoreParentheses:
        break;

    case CountParentheses:
        if (_tokenKind == T_RPAREN) {
            --_parenthesesCount;
            if (_parenthesesCount == 0)
                _parenthesesState = BalancedParentheses;
        } else if (_tokenKind == T_LPAREN) {
            ++_parenthesesCount;
        }
        break;

    case BalancedParentheses:
        _parenthesesState = IgnoreParentheses;
        break;
    } // switch

    return _tokenKind;
}

bool Lexer::isUnicodeEscapeSequence(const QChar *chars)
{
    if (isHexDigit(chars[0]) && isHexDigit(chars[1]) && isHexDigit(chars[2]) && isHexDigit(chars[3]))
        return true;

    return false;
}

QChar Lexer::decodeUnicodeEscapeCharacter(bool *ok)
{
    if (_char == QLatin1Char('u') && isUnicodeEscapeSequence(&_codePtr[0])) {
        scanChar(); // skip u

        const QChar c1 = _char;
        scanChar();

        const QChar c2 = _char;
        scanChar();

        const QChar c3 = _char;
        scanChar();

        const QChar c4 = _char;
        scanChar();

        if (ok)
            *ok = true;

        return convertUnicode(c1, c2, c3, c4);
    }

    *ok = false;
    return QChar();
}

int Lexer::scanToken()
{
    if (_stackToken != -1) {
        int tk = _stackToken;
        _stackToken = -1;
        return tk;
    }

    _terminator = false;

again:
    _validTokenText = false;
    _tokenLinePtr = _lastLinePtr;

    while (_char.isSpace()) {
        if (unsigned sequenceLength = isLineTerminatorSequence()) {
            _tokenLinePtr = _codePtr + sequenceLength - 1;

            if (_restrictedKeyword) {
                // automatic semicolon insertion
                _tokenLine = _currentLineNumber;
                _tokenStartPtr = _codePtr - 1;
                return T_SEMICOLON;
            } else {
                _terminator = true;
                syncProhibitAutomaticSemicolon();
            }
        }

        scanChar();
    }

    _tokenStartPtr = _codePtr - 1;
    _tokenLine = _currentLineNumber;

    if (_char.isNull())
        return EOF_SYMBOL;

    const QChar ch = _char;
    scanChar();

    switch (ch.unicode()) {
    case '~': return T_TILDE;
    case '}': return T_RBRACE;

    case '|':
        if (_char == QLatin1Char('|')) {
            scanChar();
            return T_OR_OR;
        } else if (_char == QLatin1Char('=')) {
            scanChar();
            return T_OR_EQ;
        }
        return T_OR;

    case '{': return T_LBRACE;

    case '^':
        if (_char == QLatin1Char('=')) {
            scanChar();
            return T_XOR_EQ;
        }
        return T_XOR;

    case ']': return T_RBRACKET;
    case '[': return T_LBRACKET;
    case '?': return T_QUESTION;

    case '>':
        if (_char == QLatin1Char('>')) {
            scanChar();
            if (_char == QLatin1Char('>')) {
                scanChar();
                if (_char == QLatin1Char('=')) {
                    scanChar();
                    return T_GT_GT_GT_EQ;
                }
                return T_GT_GT_GT;
            } else if (_char == QLatin1Char('=')) {
                scanChar();
                return T_GT_GT_EQ;
            }
            return T_GT_GT;
        } else if (_char == QLatin1Char('=')) {
            scanChar();
            return T_GE;
        }
        return T_GT;

    case '=':
        if (_char == QLatin1Char('=')) {
            scanChar();
            if (_char == QLatin1Char('=')) {
                scanChar();
                return T_EQ_EQ_EQ;
            }
            return T_EQ_EQ;
        }
        return T_EQ;

    case '<':
        if (_char == QLatin1Char('=')) {
            scanChar();
            return T_LE;
        } else if (_char == QLatin1Char('<')) {
            scanChar();
            if (_char == QLatin1Char('=')) {
                scanChar();
                return T_LT_LT_EQ;
            }
            return T_LT_LT;
        }
        return T_LT;

    case ';': return T_SEMICOLON;
    case ':': return T_COLON;

    case '/':
        if (_char == QLatin1Char('*')) {
            scanChar();
            while (!_char.isNull()) {
                if (_char == QLatin1Char('*')) {
                    scanChar();
                    if (_char == QLatin1Char('/')) {
                        scanChar();

                        if (_engine) {
                            _engine->addComment(tokenOffset() + 2, _codePtr - _tokenStartPtr - 1 - 4,
                                                tokenStartLine(), tokenStartColumn() + 2);
                        }

                        goto again;
                    }
                } else {
                    scanChar();
                }
            }
        } else if (_char == QLatin1Char('/')) {
            while (!_char.isNull() && !isLineTerminator()) {
                scanChar();
            }
            if (_engine) {
                _engine->addComment(tokenOffset() + 2, _codePtr - _tokenStartPtr - 1 - 2,
                                    tokenStartLine(), tokenStartColumn() + 2);
            }
            goto again;
        } if (_char == QLatin1Char('=')) {
            scanChar();
            return T_DIVIDE_EQ;
        }
        return T_DIVIDE_;

    case '.':
        if (_char.isDigit()) {
            QVarLengthArray<char,32> chars;

            chars.append(ch.unicode()); // append the `.'

            while (_char.isDigit()) {
                chars.append(_char.unicode());
                scanChar();
            }

            if (_char == QLatin1Char('e') || _char == QLatin1Char('E')) {
                if (_codePtr[0].isDigit() || ((_codePtr[0] == QLatin1Char('+') || _codePtr[0] == QLatin1Char('-')) &&
                                              _codePtr[1].isDigit())) {

                    chars.append(_char.unicode());
                    scanChar(); // consume `e'

                    if (_char == QLatin1Char('+') || _char == QLatin1Char('-')) {
                        chars.append(_char.unicode());
                        scanChar(); // consume the sign
                    }

                    while (_char.isDigit()) {
                        chars.append(_char.unicode());
                        scanChar();
                    }
                }
            }

            chars.append('\0');

            const char *begin = chars.constData();
            const char *end = 0;
            bool ok = false;

            _tokenValue = qstrtod(begin, &end, &ok);

            if (end - begin != chars.size() - 1) {
                _errorCode = IllegalExponentIndicator;
                _errorMessage = QCoreApplication::translate("QmlParser", "Illegal syntax for exponential number");
                return T_ERROR;
            }

            return T_NUMERIC_LITERAL;
        }
        return T_DOT;

    case '-':
        if (_char == QLatin1Char('=')) {
            scanChar();
            return T_MINUS_EQ;
        } else if (_char == QLatin1Char('-')) {
            scanChar();

            if (_terminator && !_delimited && !_prohibitAutomaticSemicolon) {
                _stackToken = T_MINUS_MINUS;
                return T_SEMICOLON;
            }

            return T_MINUS_MINUS;
        }
        return T_MINUS;

    case ',': return T_COMMA;

    case '+':
        if (_char == QLatin1Char('=')) {
            scanChar();
            return T_PLUS_EQ;
        } else if (_char == QLatin1Char('+')) {
            scanChar();

            if (_terminator && !_delimited && !_prohibitAutomaticSemicolon) {
                _stackToken = T_PLUS_PLUS;
                return T_SEMICOLON;
            }

            return T_PLUS_PLUS;
        }
        return T_PLUS;

    case '*':
        if (_char == QLatin1Char('=')) {
            scanChar();
            return T_STAR_EQ;
        }
        return T_STAR;

    case ')': return T_RPAREN;
    case '(': return T_LPAREN;

    case '&':
        if (_char == QLatin1Char('=')) {
            scanChar();
            return T_AND_EQ;
        } else if (_char == QLatin1Char('&')) {
            scanChar();
            return T_AND_AND;
        }
        return T_AND;

    case '%':
        if (_char == QLatin1Char('=')) {
            scanChar();
            return T_REMAINDER_EQ;
        }
        return T_REMAINDER;

    case '!':
        if (_char == QLatin1Char('=')) {
            scanChar();
            if (_char == QLatin1Char('=')) {
                scanChar();
                return T_NOT_EQ_EQ;
            }
            return T_NOT_EQ;
        }
        return T_NOT;

    case '\'':
    case '"': {
        const QChar quote = ch;
        bool multilineStringLiteral = false;

        const QChar *startCode = _codePtr;

        if (_engine) {
            while (!_char.isNull()) {
                if (isLineTerminator() || _char == QLatin1Char('\\')) {
                    break;
                } else if (_char == quote) {
                    _tokenSpell = _engine->midRef(startCode - _code.unicode() - 1, _codePtr - startCode);
                    scanChar();

                    return T_STRING_LITERAL;
                }
                scanChar();
            }
        }

        _validTokenText = true;
        _tokenText.resize(0);
        startCode--;
        while (startCode != _codePtr - 1) 
            _tokenText += *startCode++;

        while (! _char.isNull()) {
            if (unsigned sequenceLength = isLineTerminatorSequence()) {
                multilineStringLiteral = true;
                _tokenText += _char;
                if (sequenceLength == 2)
                    _tokenText += *_codePtr;
                scanChar();
            } else if (_char == quote) {
                scanChar();

                if (_engine)
                    _tokenSpell = _engine->newStringRef(_tokenText);

                return multilineStringLiteral ? T_MULTILINE_STRING_LITERAL : T_STRING_LITERAL;
            } else if (_char == QLatin1Char('\\')) {
                scanChar();

                QChar u;
                bool ok = false;

                switch (_char.unicode()) {
                // unicode escape sequence
                case 'u':
                    u = decodeUnicodeEscapeCharacter(&ok);
                    if (! ok)
                        u = _char;
                    break;

                // hex escape sequence
                case 'x':
                    if (isHexDigit(_codePtr[0]) && isHexDigit(_codePtr[1])) {
                        scanChar();

                        const QChar c1 = _char;
                        scanChar();

                        const QChar c2 = _char;
                        scanChar();

                        u = convertHex(c1, c2);
                    } else {
                        u = _char;
                    }
                    break;

                // single character escape sequence
                case '\\': u = QLatin1Char('\\'); scanChar(); break;
                case '\'': u = QLatin1Char('\''); scanChar(); break;
                case '\"': u = QLatin1Char('\"'); scanChar(); break;
                case 'b':  u = QLatin1Char('\b'); scanChar(); break;
                case 'f':  u = QLatin1Char('\f'); scanChar(); break;
                case 'n':  u = QLatin1Char('\n'); scanChar(); break;
                case 'r':  u = QLatin1Char('\r'); scanChar(); break;
                case 't':  u = QLatin1Char('\t'); scanChar(); break;
                case 'v':  u = QLatin1Char('\v'); scanChar(); break;

                case '0':
                    if (! _codePtr[1].isDigit()) {
                        scanChar();
                        u = QLatin1Char('\0');
                    } else {
                        // ### parse deprecated octal escape sequence ?
                        u = _char;
                    }
                    break;

                case '\r':
                    if (isLineTerminatorSequence() == 2) {
                        _tokenText += QLatin1Char('\r');
                        u = QLatin1Char('\n');
                    } else {
                        u = QLatin1Char('\r');
                    }
                    scanChar();
                    break;

                case '\n':
                case 0x2028u:
                case 0x2029u:
                    u = _char;
                    scanChar();
                    break;


                default:
                    // non escape character
                    u = _char;
                    scanChar();
                }

                _tokenText += u;
            } else {
                _tokenText += _char;
                scanChar();
            }
        }

        _errorCode = UnclosedStringLiteral;
        _errorMessage = QCoreApplication::translate("QmlParser", "Unclosed string at end of line");
        return T_ERROR;
    }
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return scanNumber(ch);

    default:
        if (ch.isLetter() || ch == QLatin1Char('$') || ch == QLatin1Char('_') || (ch == QLatin1Char('\\') && _char == QLatin1Char('u'))) {
            bool identifierWithEscapeChars = false;
            if (ch == QLatin1Char('\\')) {
                identifierWithEscapeChars = true;
                _tokenText.resize(0);
                bool ok = false;
                _tokenText += decodeUnicodeEscapeCharacter(&ok);
                _validTokenText = true;
                if (! ok) {
                    _errorCode = IllegalUnicodeEscapeSequence;
                    _errorMessage = QCoreApplication::translate("QmlParser", "Illegal unicode escape sequence");
                    return T_ERROR;
                }
            }
            while (true) {
                if (_char.isLetterOrNumber() || _char == QLatin1Char('$') || _char == QLatin1Char('_')) {
                    if (identifierWithEscapeChars)
                        _tokenText += _char;

                    scanChar();
                } else if (_char == QLatin1Char('\\') && _codePtr[0] == QLatin1Char('u')) {
                    if (! identifierWithEscapeChars) {
                        identifierWithEscapeChars = true;
                        _tokenText.resize(0);
                        _tokenText.insert(0, _tokenStartPtr, _codePtr - _tokenStartPtr - 1);
                        _validTokenText = true;
                    }

                    scanChar(); // skip '\\'
                    bool ok = false;
                    _tokenText += decodeUnicodeEscapeCharacter(&ok);
                    if (! ok) {
                        _errorCode = IllegalUnicodeEscapeSequence;
                        _errorMessage = QCoreApplication::translate("QmlParser", "Illegal unicode escape sequence");
                        return T_ERROR;
                    }
                } else {
                    _tokenLength = _codePtr - _tokenStartPtr - 1;

                    int kind = T_IDENTIFIER;

                    if (! identifierWithEscapeChars)
                        kind = classify(_tokenStartPtr, _tokenLength, _qmlMode);

                    if (_engine) {
                        if (kind == T_IDENTIFIER && identifierWithEscapeChars)
                            _tokenSpell = _engine->newStringRef(_tokenText);
                        else
                            _tokenSpell = _engine->midRef(_tokenStartPtr - _code.unicode(), _tokenLength);
                    }

                    return kind;
                }
            }
        }

        break;
    }

    return T_ERROR;
}

int Lexer::scanNumber(QChar ch)
{
    if (ch != QLatin1Char('0')) {
        double integer = ch.unicode() - '0';

        QChar n = _char;
        const QChar *code = _codePtr;
        while (n.isDigit()) {
            integer = integer * 10 + (n.unicode() - '0');
            n = *code++;
        }

        if (n != QLatin1Char('.') && n != QLatin1Char('e') && n != QLatin1Char('E')) {
            if (code != _codePtr) {
                _codePtr = code - 1;
                scanChar();
            }
            _tokenValue = integer;
            return T_NUMERIC_LITERAL;
        }
    }

    QVarLengthArray<char,32> chars;
    chars.append(ch.unicode());

    if (ch == QLatin1Char('0') && (_char == QLatin1Char('x') || _char == QLatin1Char('X'))) {
        // parse hex integer literal

        chars.append(_char.unicode());
        scanChar(); // consume `x'

        while (isHexDigit(_char)) {
            chars.append(_char.unicode());
            scanChar();
        }

        _tokenValue = integerFromString(chars.constData(), chars.size(), 16);
        return T_NUMERIC_LITERAL;
    }

    // decimal integer literal
    while (_char.isDigit()) {
        chars.append(_char.unicode());
        scanChar(); // consume the digit
    }

    if (_char == QLatin1Char('.')) {
        chars.append(_char.unicode());
        scanChar(); // consume `.'

        while (_char.isDigit()) {
            chars.append(_char.unicode());
            scanChar();
        }

        if (_char == QLatin1Char('e') || _char == QLatin1Char('E')) {
            if (_codePtr[0].isDigit() || ((_codePtr[0] == QLatin1Char('+') || _codePtr[0] == QLatin1Char('-')) &&
                                          _codePtr[1].isDigit())) {

                chars.append(_char.unicode());
                scanChar(); // consume `e'

                if (_char == QLatin1Char('+') || _char == QLatin1Char('-')) {
                    chars.append(_char.unicode());
                    scanChar(); // consume the sign
                }

                while (_char.isDigit()) {
                    chars.append(_char.unicode());
                    scanChar();
                }
            }
        }
    } else if (_char == QLatin1Char('e') || _char == QLatin1Char('E')) {
        if (_codePtr[0].isDigit() || ((_codePtr[0] == QLatin1Char('+') || _codePtr[0] == QLatin1Char('-')) &&
                                      _codePtr[1].isDigit())) {

            chars.append(_char.unicode());
            scanChar(); // consume `e'

            if (_char == QLatin1Char('+') || _char == QLatin1Char('-')) {
                chars.append(_char.unicode());
                scanChar(); // consume the sign
            }

            while (_char.isDigit()) {
                chars.append(_char.unicode());
                scanChar();
            }
        }
    }

    if (chars.size() == 1) {
        // if we ended up with a single digit, then it was a '0'
        _tokenValue = 0;
        return T_NUMERIC_LITERAL;
    }

    chars.append('\0');

    const char *begin = chars.constData();
    const char *end = 0;
    bool ok = false;

    _tokenValue = qstrtod(begin, &end, &ok);

    if (end - begin != chars.size() - 1) {
        _errorCode = IllegalExponentIndicator;
        _errorMessage = QCoreApplication::translate("QmlParser", "Illegal syntax for exponential number");
        return T_ERROR;
    }

    return T_NUMERIC_LITERAL;
}

bool Lexer::scanRegExp(RegExpBodyPrefix prefix)
{
    _tokenText.resize(0);
    _validTokenText = true;
    _patternFlags = 0;

    if (prefix == EqualPrefix)
        _tokenText += QLatin1Char('=');

    while (true) {
        switch (_char.unicode()) {
        case '/':
            scanChar();

            // scan the flags
            _patternFlags = 0;
            while (isIdentLetter(_char)) {
                int flag = regExpFlagFromChar(_char);
                if (flag == 0) {
                    _errorMessage = QCoreApplication::translate("QmlParser", "Invalid regular expression flag '%0'")
                             .arg(QChar(_char));
                    return false;
                }
                _patternFlags |= flag;
                scanChar();
            }

            _tokenLength = _codePtr - _tokenStartPtr - 1;
            return true;

        case '\\':
            // regular expression backslash sequence
            _tokenText += _char;
            scanChar();

            if (_char.isNull() || isLineTerminator()) {
                _errorMessage = QCoreApplication::translate("QmlParser", "Unterminated regular expression backslash sequence");
                return false;
            }

            _tokenText += _char;
            scanChar();
            break;

        case '[':
            // regular expression class
            _tokenText += _char;
            scanChar();

            while (! _char.isNull() && ! isLineTerminator()) {
                if (_char == QLatin1Char(']'))
                    break;
                else if (_char == QLatin1Char('\\')) {
                    // regular expression backslash sequence
                    _tokenText += _char;
                    scanChar();

                    if (_char.isNull() || isLineTerminator()) {
                        _errorMessage = QCoreApplication::translate("QmlParser", "Unterminated regular expression backslash sequence");
                        return false;
                    }

                    _tokenText += _char;
                    scanChar();
                } else {
                    _tokenText += _char;
                    scanChar();
                }
            }

            if (_char != QLatin1Char(']')) {
                _errorMessage = QCoreApplication::translate("QmlParser", "Unterminated regular expression class");
                return false;
            }

            _tokenText += _char;
            scanChar(); // skip ]
            break;

        default:
            if (_char.isNull() || isLineTerminator()) {
                _errorMessage = QCoreApplication::translate("QmlParser", "Unterminated regular expression literal");
                return false;
            } else {
                _tokenText += _char;
                scanChar();
            }
        } // switch
    } // while

    return false;
}

bool Lexer::isLineTerminator() const
{
    const ushort unicode = _char.unicode();
    return unicode == 0x000Au
            || unicode == 0x000Du
            || unicode == 0x2028u
            || unicode == 0x2029u;
}

unsigned Lexer::isLineTerminatorSequence() const
{
    switch (_char.unicode()) {
    case 0x000Au:
    case 0x2028u:
    case 0x2029u:
        return 1;
    case 0x000Du:
        if (_codePtr->unicode() == 0x000Au)
            return 2;
        else
            return 1;
    default:
        return 0;
    }
}

bool Lexer::isIdentLetter(QChar ch)
{
    // ASCII-biased, since all reserved words are ASCII, aand hence the
    // bulk of content to be parsed.
    if ((ch >= QLatin1Char('a') && ch <= QLatin1Char('z'))
            || (ch >= QLatin1Char('A') && ch <= QLatin1Char('Z'))
            || ch == QLatin1Char('$')
            || ch == QLatin1Char('_'))
        return true;
    if (ch.unicode() < 128)
        return false;
    return ch.isLetterOrNumber();
}

bool Lexer::isDecimalDigit(ushort c)
{
    return (c >= '0' && c <= '9');
}

bool Lexer::isHexDigit(QChar c)
{
    return ((c >= QLatin1Char('0') && c <= QLatin1Char('9'))
            || (c >= QLatin1Char('a') && c <= QLatin1Char('f'))
            || (c >= QLatin1Char('A') && c <= QLatin1Char('F')));
}

bool Lexer::isOctalDigit(ushort c)
{
    return (c >= '0' && c <= '7');
}

int Lexer::tokenEndLine() const
{
    return _currentLineNumber;
}

int Lexer::tokenEndColumn() const
{
    return _codePtr - _lastLinePtr;
}

QString Lexer::tokenText() const
{
    if (_validTokenText)
        return _tokenText;

    if (_tokenKind == T_STRING_LITERAL)
        return QString(_tokenStartPtr + 1, _tokenLength - 2);

    return QString(_tokenStartPtr, _tokenLength);
}

Lexer::Error Lexer::errorCode() const
{
    return _errorCode;
}

QString Lexer::errorMessage() const
{
    return _errorMessage;
}

void Lexer::syncProhibitAutomaticSemicolon()
{
    if (_parenthesesState == BalancedParentheses) {
        // we have seen something like "if (foo)", which means we should
        // never insert an automatic semicolon at this point, since it would
        // then be expanded into an empty statement (ECMA-262 7.9.1)
        _prohibitAutomaticSemicolon = true;
        _parenthesesState = IgnoreParentheses;
    } else {
        _prohibitAutomaticSemicolon = false;
    }
}

bool Lexer::prevTerminator() const
{
    return _terminator;
}

bool Lexer::followsClosingBrace() const
{
    return _followsClosingBrace;
}

bool Lexer::canInsertAutomaticSemicolon(int token) const
{
    return token == T_RBRACE
            || token == EOF_SYMBOL
            || _terminator
            || _followsClosingBrace;
}

bool Lexer::scanDirectives(Directives *directives)
{
    if (_qmlMode) {
        // the directives are a Javascript-only extension.
        return false;
    }

    lex(); // fetch the first token

    if (_tokenKind != T_DOT)
        return true;

    do {
        lex(); // skip T_DOT

        const int lineNumber = tokenStartLine();

        if (! (_tokenKind == T_IDENTIFIER || _tokenKind == T_RESERVED_WORD))
            return false; // expected a valid QML/JS directive

        const QString directiveName = tokenText();

        if (! (directiveName == QLatin1String("pragma") ||
               directiveName == QLatin1String("import")))
            return false; // not a valid directive name

        // it must be a pragma or an import directive.
        if (directiveName == QLatin1String("pragma")) {
            // .pragma library
            if (! (lex() == T_IDENTIFIER && tokenText() == QLatin1String("library")))
                return false; // expected `library

            // we found a .pragma library directive
            directives->pragmaLibrary();

        } else {
            Q_ASSERT(directiveName == QLatin1String("import"));
            lex(); // skip .import

            QString pathOrUri;
            QString version;
            bool fileImport = false; // file or uri import

            if (_tokenKind == T_STRING_LITERAL) {
                // .import T_STRING_LITERAL as T_IDENTIFIER

                fileImport = true;
                pathOrUri = tokenText();

            } else if (_tokenKind == T_IDENTIFIER) {
                // .import T_IDENTIFIER (. T_IDENTIFIER)* T_NUMERIC_LITERAL as T_IDENTIFIER

                pathOrUri = tokenText();

                lex(); // skip the first T_IDENTIFIER
                for (; _tokenKind == T_DOT; lex()) {
                    if (lex() != T_IDENTIFIER)
                        return false;

                    pathOrUri += QLatin1Char('.');
                    pathOrUri += tokenText();
                }

                if (_tokenKind != T_NUMERIC_LITERAL)
                    return false; // expected the module version number

                version = tokenText();
            }

            //
            // recognize the mandatory `as' followed by the module name
            //
            if (! (lex() == T_IDENTIFIER && tokenText() == QLatin1String("as")))
                return false; // expected `as'

            if (lex() != T_IDENTIFIER)
                return false; // expected module name

            const QString module = tokenText();

            if (fileImport)
                directives->importFile(pathOrUri, module);
            else
                directives->importModule(pathOrUri, version, module);
        }

        if (tokenStartLine() != lineNumber)
            return false; // the directives cannot span over multiple lines

        // fetch the first token after the .pragma/.import directive
        lex();
    } while (_tokenKind == T_DOT);

    return true;
}

#include "qmljskeywords_p.h"
