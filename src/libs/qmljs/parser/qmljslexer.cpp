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

#include "qmljslexer_p.h"
#include "qmljsengine_p.h"
#include "qmljsmemorypool_p.h"
#include "qmljskeywords_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE
Q_CORE_EXPORT double qstrtod(const char *s00, char const **se, bool *ok);
QT_END_NAMESPACE

using namespace QmlJS;

static inline int regExpFlagFromChar(const QChar &ch)
{
    switch (ch.unicode()) {
    case 'g': return Lexer::RegExp_Global;
    case 'i': return Lexer::RegExp_IgnoreCase;
    case 'm': return Lexer::RegExp_Multiline;
    }
    return 0;
}

static inline unsigned char convertHex(ushort c)
{
    if (c >= '0' && c <= '9')
        return (c - '0');
    else if (c >= 'a' && c <= 'f')
        return (c - 'a' + 10);
    else
        return (c - 'A' + 10);
}

static inline QChar convertHex(QChar c1, QChar c2)
{
    return QChar((convertHex(c1.unicode()) << 4) + convertHex(c2.unicode()));
}

static inline QChar convertUnicode(QChar c1, QChar c2, QChar c3, QChar c4)
{
    return QChar((convertHex(c3.unicode()) << 4) + convertHex(c4.unicode()),
                 (convertHex(c1.unicode()) << 4) + convertHex(c2.unicode()));
}

Lexer::Lexer(Engine *engine)
    : _engine(engine)
    , _codePtr(0)
    , _endPtr(0)
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
    _endPtr = _codePtr + code.length();
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

namespace {
inline bool isBinop(int tok)
{
    switch (tok) {
    case Lexer::T_AND:
    case Lexer::T_AND_AND:
    case Lexer::T_AND_EQ:
    case Lexer::T_DIVIDE_:
    case Lexer::T_DIVIDE_EQ:
    case Lexer::T_EQ:
    case Lexer::T_EQ_EQ:
    case Lexer::T_EQ_EQ_EQ:
    case Lexer::T_GE:
    case Lexer::T_GT:
    case Lexer::T_GT_GT:
    case Lexer::T_GT_GT_EQ:
    case Lexer::T_GT_GT_GT:
    case Lexer::T_GT_GT_GT_EQ:
    case Lexer::T_LE:
    case Lexer::T_LT:
    case Lexer::T_LT_LT:
    case Lexer::T_LT_LT_EQ:
    case Lexer::T_MINUS:
    case Lexer::T_MINUS_EQ:
    case Lexer::T_NOT_EQ:
    case Lexer::T_NOT_EQ_EQ:
    case Lexer::T_OR:
    case Lexer::T_OR_EQ:
    case Lexer::T_OR_OR:
    case Lexer::T_PLUS:
    case Lexer::T_PLUS_EQ:
    case Lexer::T_REMAINDER:
    case Lexer::T_REMAINDER_EQ:
    case Lexer::T_RETURN:
    case Lexer::T_STAR:
    case Lexer::T_STAR_EQ:
    case Lexer::T_XOR:
    case Lexer::T_XOR_EQ:
        return true;

    default:
        return false;
    }
}
} // anonymous namespace

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
    case T_QUESTION:
    case T_COLON:
    case T_TILDE:
        _delimited = true;
        break;
    default:
        if (isBinop(_tokenKind))
            _delimited = true;
        break;

    case T_IF:
    case T_FOR:
    case T_WHILE:
    case T_WITH:
        _parenthesesState = CountParentheses;
        _parenthesesCount = 0;
        break;

    case T_ELSE:
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
        if (_tokenKind != T_DO && _tokenKind != T_ELSE)
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

QChar Lexer::decodeHexEscapeCharacter(bool *ok)
{
    if (isHexDigit(_codePtr[0]) && isHexDigit(_codePtr[1])) {
        scanChar();

        const QChar c1 = _char;
        scanChar();

        const QChar c2 = _char;
        scanChar();

        if (ok)
            *ok = true;

        return convertHex(c1, c2);
    }

    *ok = false;
    return QChar();
}

static inline bool isIdentifierStart(QChar ch)
{
    // fast path for ascii
    if ((ch.unicode() >= 'a' && ch.unicode() <= 'z') ||
        (ch.unicode() >= 'A' && ch.unicode() <= 'Z') ||
        ch == '$' || ch == '_')
        return true;

    switch (ch.category()) {
    case QChar::Number_Letter:
    case QChar::Letter_Uppercase:
    case QChar::Letter_Lowercase:
    case QChar::Letter_Titlecase:
    case QChar::Letter_Modifier:
    case QChar::Letter_Other:
        return true;
    default:
        break;
    }
    return false;
}

static bool isIdentifierPart(QChar ch)
{
    // fast path for ascii
    if ((ch.unicode() >= 'a' && ch.unicode() <= 'z') ||
        (ch.unicode() >= 'A' && ch.unicode() <= 'Z') ||
        (ch.unicode() >= '0' && ch.unicode() <= '9') ||
        ch == '$' || ch == '_' ||
        ch.unicode() == 0x200c /* ZWNJ */ || ch.unicode() == 0x200d /* ZWJ */)
        return true;

    switch (ch.category()) {
    case QChar::Mark_NonSpacing:
    case QChar::Mark_SpacingCombining:

    case QChar::Number_DecimalDigit:
    case QChar::Number_Letter:

    case QChar::Letter_Uppercase:
    case QChar::Letter_Lowercase:
    case QChar::Letter_Titlecase:
    case QChar::Letter_Modifier:
    case QChar::Letter_Other:

    case QChar::Punctuation_Connector:
        return true;
    default:
        break;
    }
    return false;
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

    if (_codePtr > _endPtr)
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
            while (_codePtr <= _endPtr) {
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
            while (_codePtr <= _endPtr && !isLineTerminator()) {
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
                _errorMessage = QCoreApplication::translate("QmlParser", "Illegal syntax for exponential number.");
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
            while (_codePtr <= _endPtr) {
                if (isLineTerminator()) {
                    if (qmlMode())
                        break;
                    _errorCode = IllegalCharacter;
                    _errorMessage = QCoreApplication::translate("QmlParser", "Stray newline in string literal.");
                    return T_ERROR;
                } else if (_char == QLatin1Char('\\')) {
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

        while (_codePtr <= _endPtr) {
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

                switch (_char.unicode()) {
                // unicode escape sequence
                case 'u': {
                    bool ok = false;
                    u = decodeUnicodeEscapeCharacter(&ok);
                    if (! ok) {
                        _errorCode = IllegalUnicodeEscapeSequence;
                        _errorMessage = QCoreApplication::translate("QmlParser", "Illegal unicode escape sequence.");
                        return T_ERROR;
                    }
                } break;

                // hex escape sequence
                case 'x': {
                    bool ok = false;
                    u = decodeHexEscapeCharacter(&ok);
                    if (!ok) {
                        _errorCode = IllegalHexadecimalEscapeSequence;
                        _errorMessage = QCoreApplication::translate("QmlParser", "Illegal hexadecimal escape sequence.");
                        return T_ERROR;
                    }
                } break;

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
                    if (! _codePtr->isDigit()) {
                        scanChar();
                        u = QLatin1Char('\0');
                        break;
                    }
                    // fall through
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                    _errorCode = IllegalEscapeSequence;
                    _errorMessage = QCoreApplication::translate("QmlParser", "Octal escape sequences are not allowed.");
                    return T_ERROR;

                case '\r':
                case '\n':
                case 0x2028u:
                case 0x2029u:
                    scanChar();
                    continue;

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
        _errorMessage = QCoreApplication::translate("QmlParser", "Unclosed string at end of line.");
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

    default: {
        QChar c = ch;
        bool identifierWithEscapeChars = false;
        if (c == QLatin1Char('\\') && _char == QLatin1Char('u')) {
            identifierWithEscapeChars = true;
            bool ok = false;
            c = decodeUnicodeEscapeCharacter(&ok);
            if (! ok) {
                _errorCode = IllegalUnicodeEscapeSequence;
                _errorMessage = QCoreApplication::translate("QmlParser", "Illegal unicode escape sequence.");
                return T_ERROR;
            }
        }
        if (isIdentifierStart(c)) {
            if (identifierWithEscapeChars) {
                _tokenText.resize(0);
                _tokenText += c;
                _validTokenText = true;
            }
            while (true) {
                c = _char;
                if (_char == QLatin1Char('\\') && _codePtr[0] == QLatin1Char('u')) {
                    if (! identifierWithEscapeChars) {
                        identifierWithEscapeChars = true;
                        _tokenText.resize(0);
                        _tokenText.insert(0, _tokenStartPtr, _codePtr - _tokenStartPtr - 1);
                        _validTokenText = true;
                    }

                    scanChar(); // skip '\\'
                    bool ok = false;
                    c = decodeUnicodeEscapeCharacter(&ok);
                    if (! ok) {
                        _errorCode = IllegalUnicodeEscapeSequence;
                        _errorMessage = QCoreApplication::translate("QmlParser", "Illegal unicode escape sequence.");
                        return T_ERROR;
                    }
                    if (isIdentifierPart(c))
                        _tokenText += c;
                    continue;
                } else if (isIdentifierPart(c)) {
                    if (identifierWithEscapeChars)
                        _tokenText += c;

                    scanChar();
                    continue;
                }

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
        QVarLengthArray<char, 64> buf;
        buf += ch.toLatin1();

        QChar n = _char;
        const QChar *code = _codePtr;
        while (n.isDigit()) {
            buf += n.toLatin1();
            n = *code++;
        }

        if (n != QLatin1Char('.') && n != QLatin1Char('e') && n != QLatin1Char('E')) {
            if (code != _codePtr) {
                _codePtr = code - 1;
                scanChar();
            }
            buf.append('\0');
            _tokenValue = strtod(buf.constData(), 0);
            return T_NUMERIC_LITERAL;
        }
    } else if (_char.isDigit() && !qmlMode()) {
        _errorCode = IllegalCharacter;
        _errorMessage = QCoreApplication::translate("QmlParser", "Decimal numbers cannot start with \"0\".");
        return T_ERROR;
    }

    QVarLengthArray<char,32> chars;
    chars.append(ch.unicode());

    if (ch == QLatin1Char('0') && (_char == QLatin1Char('x') || _char == QLatin1Char('X'))) {
        ch = _char; // remember the x or X to use it in the error message below.

        // parse hex integer literal
        chars.append(_char.unicode());
        scanChar(); // consume `x'

        while (isHexDigit(_char)) {
            chars.append(_char.unicode());
            scanChar();
        }

        if (chars.size() < 3) {
            _errorCode = IllegalHexNumber;
            _errorMessage = QCoreApplication::translate("QmlParser", "At least one hexadecimal digit is required after \"0%1\".").arg(ch);
            return T_ERROR;
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
        _errorMessage = QCoreApplication::translate("QmlParser", "Illegal syntax for exponential number.");
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
                if (flag == 0 || _patternFlags & flag) {
                    _errorMessage = QCoreApplication::translate("QmlParser", "Invalid regular expression flag \"%0\".")
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

            if (_codePtr > _endPtr || isLineTerminator()) {
                _errorMessage = QCoreApplication::translate("QmlParser", "Unterminated regular expression backslash sequence.");
                return false;
            }

            _tokenText += _char;
            scanChar();
            break;

        case '[':
            // regular expression class
            _tokenText += _char;
            scanChar();

            while (_codePtr <= _endPtr && ! isLineTerminator()) {
                if (_char == QLatin1Char(']'))
                    break;
                else if (_char == QLatin1Char('\\')) {
                    // regular expression backslash sequence
                    _tokenText += _char;
                    scanChar();

                    if (_codePtr > _endPtr || isLineTerminator()) {
                        _errorMessage = QCoreApplication::translate("QmlParser", "Unterminated regular expression backslash sequence.");
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
                _errorMessage = QCoreApplication::translate("QmlParser", "Unterminated regular expression class.");
                return false;
            }

            _tokenText += _char;
            scanChar(); // skip ]
            break;

        default:
            if (_codePtr > _endPtr || isLineTerminator()) {
                _errorMessage = QCoreApplication::translate("QmlParser", "Unterminated regular expression literal.");
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

static const int uriTokens[] = {
    QmlJSGrammar::T_IDENTIFIER,
    QmlJSGrammar::T_PROPERTY,
    QmlJSGrammar::T_SIGNAL,
    QmlJSGrammar::T_READONLY,
    QmlJSGrammar::T_ON,
    QmlJSGrammar::T_BREAK,
    QmlJSGrammar::T_CASE,
    QmlJSGrammar::T_CATCH,
    QmlJSGrammar::T_CONTINUE,
    QmlJSGrammar::T_DEFAULT,
    QmlJSGrammar::T_DELETE,
    QmlJSGrammar::T_DO,
    QmlJSGrammar::T_ELSE,
    QmlJSGrammar::T_FALSE,
    QmlJSGrammar::T_FINALLY,
    QmlJSGrammar::T_FOR,
    QmlJSGrammar::T_FUNCTION,
    QmlJSGrammar::T_IF,
    QmlJSGrammar::T_IN,
    QmlJSGrammar::T_INSTANCEOF,
    QmlJSGrammar::T_NEW,
    QmlJSGrammar::T_NULL,
    QmlJSGrammar::T_RETURN,
    QmlJSGrammar::T_SWITCH,
    QmlJSGrammar::T_THIS,
    QmlJSGrammar::T_THROW,
    QmlJSGrammar::T_TRUE,
    QmlJSGrammar::T_TRY,
    QmlJSGrammar::T_TYPEOF,
    QmlJSGrammar::T_VAR,
    QmlJSGrammar::T_VOID,
    QmlJSGrammar::T_WHILE,
    QmlJSGrammar::T_CONST,
    QmlJSGrammar::T_DEBUGGER,
    QmlJSGrammar::T_RESERVED_WORD,
    QmlJSGrammar::T_WITH,

    QmlJSGrammar::EOF_SYMBOL
};
static inline bool isUriToken(int token)
{
    const int *current = uriTokens;
    while (*current != QmlJSGrammar::EOF_SYMBOL) {
        if (*current == token)
            return true;
        ++current;
    }
    return false;
}

bool Lexer::scanDirectives(Directives *directives, DiagnosticMessage *error)
{
    Q_ASSERT(!_qmlMode);

    lex(); // fetch the first token

    if (_tokenKind != T_DOT)
        return true;

    do {
        const int lineNumber = tokenStartLine();
        const int column = tokenStartColumn();

        lex(); // skip T_DOT

        if (! (_tokenKind == T_IDENTIFIER || _tokenKind == T_RESERVED_WORD))
            return true; // expected a valid QML/JS directive

        const QString directiveName = tokenText();

        if (! (directiveName == QLatin1String("pragma") ||
               directiveName == QLatin1String("import"))) {
            error->message = QCoreApplication::translate("QmlParser", "Syntax error.");
            error->loc.startLine = tokenStartLine();
            error->loc.startColumn = tokenStartColumn();
            return false; // not a valid directive name
        }

        // it must be a pragma or an import directive.
        if (directiveName == QLatin1String("pragma")) {
            // .pragma library
            if (! (lex() == T_IDENTIFIER && tokenText() == QLatin1String("library"))) {
                error->message = QCoreApplication::translate("QmlParser", "Syntax error.");
                error->loc.startLine = tokenStartLine();
                error->loc.startColumn = tokenStartColumn();
                return false; // expected `library
            }

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

                if (!pathOrUri.endsWith(QLatin1String("js"))) {
                    error->message = QCoreApplication::translate("QmlParser","Imported file must be a script.");
                    error->loc.startLine = tokenStartLine();
                    error->loc.startColumn = tokenStartColumn();
                    return false;
                }

            } else if (_tokenKind == T_IDENTIFIER) {
                // .import T_IDENTIFIER (. T_IDENTIFIER)* T_NUMERIC_LITERAL as T_IDENTIFIER

                while (true) {
                    if (!isUriToken(_tokenKind)) {
                        error->message = QCoreApplication::translate("QmlParser","Invalid module URI.");
                        error->loc.startLine = tokenStartLine();
                        error->loc.startColumn = tokenStartColumn();
                        return false;
                    }

                    pathOrUri.append(tokenText());

                    lex();
                    if (tokenStartLine() != lineNumber) {
                        error->message = QCoreApplication::translate("QmlParser","Invalid module URI.");
                        error->loc.startLine = tokenStartLine();
                        error->loc.startColumn = tokenStartColumn();
                        return false;
                    }
                    if (_tokenKind != QmlJSGrammar::T_DOT)
                        break;

                    pathOrUri.append(QLatin1Char('.'));

                    lex();
                    if (tokenStartLine() != lineNumber) {
                        error->message = QCoreApplication::translate("QmlParser","Invalid module URI.");
                        error->loc.startLine = tokenStartLine();
                        error->loc.startColumn = tokenStartColumn();
                        return false;
                    }
                }

                if (_tokenKind != T_NUMERIC_LITERAL) {
                    error->message = QCoreApplication::translate("QmlParser","Module import requires a version.");
                    error->loc.startLine = tokenStartLine();
                    error->loc.startColumn = tokenStartColumn();
                    return false; // expected the module version number
                }

                version = tokenText();
            }

            //
            // recognize the mandatory `as' followed by the module name
            //
            if (! (lex() == T_IDENTIFIER && tokenText() == QLatin1String("as") && tokenStartLine() == lineNumber)) {
                if (fileImport)
                    error->message = QCoreApplication::translate("QmlParser", "File import requires a qualifier.");
                else
                    error->message = QCoreApplication::translate("QmlParser", "Module import requires a qualifier.");
                if (tokenStartLine() != lineNumber) {
                    error->loc.startLine = lineNumber;
                    error->loc.startColumn = column;
                } else {
                    error->loc.startLine = tokenStartLine();
                    error->loc.startColumn = tokenStartColumn();
                }
                return false; // expected `as'
            }

            if (lex() != T_IDENTIFIER || tokenStartLine() != lineNumber) {
                if (fileImport)
                    error->message = QCoreApplication::translate("QmlParser", "File import requires a qualifier.");
                else
                    error->message = QCoreApplication::translate("QmlParser", "Module import requires a qualifier.");
                error->loc.startLine = tokenStartLine();
                error->loc.startColumn = tokenStartColumn();
                return false; // expected module name
            }

            const QString module = tokenText();
            if (!module.at(0).isUpper()) {
                error->message = QCoreApplication::translate("QmlParser","Invalid import qualifier.");
                error->loc.startLine = tokenStartLine();
                error->loc.startColumn = tokenStartColumn();
                return false;
            }

            if (fileImport)
                directives->importFile(pathOrUri, module, lineNumber, column);
            else
                directives->importModule(pathOrUri, version, module, lineNumber, column);
        }

        if (tokenStartLine() != lineNumber) {
            error->message = QCoreApplication::translate("QmlParser", "Syntax error.");
            error->loc.startLine = tokenStartLine();
            error->loc.startColumn = tokenStartColumn();
            return false; // the directives cannot span over multiple lines
        }

        // fetch the first token after the .pragma/.import directive
        lex();
    } while (_tokenKind == T_DOT);

    return true;
}
