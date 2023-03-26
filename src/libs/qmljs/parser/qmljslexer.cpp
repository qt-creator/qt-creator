// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljslexer_p.h"
#include "qmljsengine_p.h"
#include "qmljskeywords_p.h"

#include "qmljs/parser/qmljsdiagnosticmessage_p.h"
#include "qmljs/parser/qmljsmemorypool_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qvarlengtharray.h>
#include <QtCore/qdebug.h>
#include <QtCore/QScopedValueRollback>

QT_BEGIN_NAMESPACE
Q_CORE_EXPORT double qstrntod(const char *s00, qsizetype len, char const **se, bool *ok);
QT_END_NAMESPACE
#include <optional>

using namespace QmlJS;

static inline int regExpFlagFromChar(const QChar &ch)
{
    switch (ch.unicode()) {
    case 'g': return Lexer::RegExp_Global;
    case 'i': return Lexer::RegExp_IgnoreCase;
    case 'm': return Lexer::RegExp_Multiline;
    case 'u': return Lexer::RegExp_Unicode;
    case 'y': return Lexer::RegExp_Sticky;
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

Lexer::Lexer(Engine *engine, LexMode lexMode)
    : _engine(engine)
    , _lexMode(lexMode)
    , _endPtr(nullptr)
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

void Lexer::setCode(const QString &code,
                    int lineno,
                    bool qmlMode,
                    Lexer::CodeContinuation codeContinuation)
{
    if (codeContinuation == Lexer::CodeContinuation::Continue)
        _currentOffset += _code.size();
    else
        _currentOffset = 0;
    if (_engine)
        _engine->setCode(code);

    _qmlMode = qmlMode;
    _code = code;
    _skipLinefeed = false;

    _tokenText.clear();
    _tokenText.reserve(1024);
    _errorMessage.clear();
    _tokenSpell = QStringView();
    _rawString = QStringView();

    _codePtr = code.unicode();
    _endPtr = _codePtr + code.size();
    _tokenStartPtr = _codePtr;

    if (lineno >= 0)
        _currentLineNumber = lineno;
    _currentColumnNumber = 0;
    _tokenLine = _currentLineNumber;
    _tokenColumn = 0;
    _tokenLength = 0;

    if (codeContinuation == Lexer::CodeContinuation::Reset)
        _state = State{};
}

void Lexer::scanChar()
{
    if (_skipLinefeed) {
        Q_ASSERT(*_codePtr == u'\n');
        ++_codePtr;
        _skipLinefeed = false;
    }
    _state.currentChar = *_codePtr++;
    ++_currentColumnNumber;

    if (isLineTerminator()) {
        if (_state.currentChar == u'\r') {
            if (_codePtr < _endPtr && *_codePtr == u'\n')
                _skipLinefeed = true;
            _state.currentChar = u'\n';
        }
        ++_currentLineNumber;
        _currentColumnNumber = 0;
    }
}

QChar Lexer::peekChar()
{
    auto peekPtr = _codePtr;
    if (peekPtr < _endPtr)
        return *peekPtr;
    return QChar();
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

int hexDigit(QChar c)
{
    if (c >= u'0' && c <= u'9')
        return c.unicode() - u'0';
    if (c >= u'a' && c <= u'f')
        return c.unicode() - u'a' + 10;
    if (c >= u'A' && c <= u'F')
        return c.unicode() - u'A' + 10;
    return -1;
}

int octalDigit(QChar c)
{
    if (c >= u'0' && c <= u'7')
        return c.unicode() - u'0';
    return -1;
}

} // anonymous namespace

int Lexer::lex()
{
    const int previousTokenKind = _state.tokenKind;
    int tokenKind;
    bool firstPass = true;

  again:
    tokenKind = T_ERROR;
    _tokenSpell = QStringView();
    _rawString = QStringView();
    if (firstPass && _state.stackToken == -1) {
        firstPass = false;
        if (_codePtr > _endPtr && _lexMode == LexMode::LineByLine && !_code.isEmpty())
            return T_EOL;

        if (_state.comments == CommentState::InMultilineComment) {
            scanChar();
            _tokenStartPtr = _codePtr - 1;
            _tokenLine = _currentLineNumber;
            _tokenColumn = _currentColumnNumber;
            while (_codePtr <= _endPtr) {
                if (_state.currentChar == u'*') {
                    scanChar();
                    if (_state.currentChar == u'/') {
                        scanChar();
                        if (_engine) {
                            _engine->addComment(tokenOffset() + 2,
                                                _codePtr - _tokenStartPtr - 1 - 4,
                                                tokenStartLine(),
                                                tokenStartColumn() + 2);
                        }
                        tokenKind = T_COMMENT;
                        break;
                    }
                } else {
                    scanChar();
                }
            }
            if (tokenKind == T_ERROR)
                tokenKind = T_PARTIAL_COMMENT;
        } else {
            // handle multiline continuation
            std::optional<ScanStringMode> scanMode;
            switch (previousTokenKind) {
            case T_PARTIAL_SINGLE_QUOTE_STRING_LITERAL:
                scanMode = ScanStringMode::SingleQuote;
                break;
            case T_PARTIAL_DOUBLE_QUOTE_STRING_LITERAL:
                scanMode = ScanStringMode::DoubleQuote;
                break;
            case T_PARTIAL_TEMPLATE_HEAD:
                scanMode = ScanStringMode::TemplateHead;
                break;
            case T_PARTIAL_TEMPLATE_MIDDLE:
                scanMode = ScanStringMode::TemplateContinuation;
                break;
            default:
                break;
            }
            if (scanMode) {
                scanChar();
                _tokenStartPtr = _codePtr - 1;
                _tokenLine = _currentLineNumber;
                _tokenColumn = _currentColumnNumber;
                tokenKind = scanString(*scanMode);
            }
        }
    }
    if (tokenKind == T_ERROR)
        tokenKind = scanToken();
    _tokenLength = _codePtr - _tokenStartPtr - 1;
    switch (tokenKind) {
    // end of line and comments should not "overwrite" the old token type...
    case T_EOL:
        return tokenKind;
    case T_COMMENT:
        _state.comments = CommentState::HadComment;
        return tokenKind;
    case T_PARTIAL_COMMENT:
        _state.comments = CommentState::InMultilineComment;
        return tokenKind;
    default:
        _state.comments = CommentState::NoComment;
        break;
    }
    _state.tokenKind = tokenKind;

    _state.delimited = false;
    _state.restrictedKeyword = false;
    _state.followsClosingBrace = (previousTokenKind == T_RBRACE);

    // update the flags
    switch (_state.tokenKind) {
    case T_LBRACE:
        if (_state.bracesCount > 0)
            ++_state.bracesCount;
        Q_FALLTHROUGH();
    case T_SEMICOLON:
        _state.importState = ImportState::NoQmlImport;
        Q_FALLTHROUGH();
    case T_QUESTION:
    case T_COLON:
    case T_TILDE:
        _state.delimited = true;
        break;
    case T_AUTOMATIC_SEMICOLON:
    case T_AS:
        _state.importState = ImportState::NoQmlImport;
        Q_FALLTHROUGH();
    default:
        if (isBinop(_state.tokenKind))
            _state.delimited = true;
        break;

    case T_IMPORT:
        if (qmlMode() || (_state.handlingDirectives && previousTokenKind == T_DOT))
            _state.importState = ImportState::SawImport;
        if (isBinop(_state.tokenKind))
            _state.delimited = true;
        break;

    case T_IF:
    case T_FOR:
    case T_WHILE:
    case T_WITH:
        _state.parenthesesState = CountParentheses;
        _state.parenthesesCount = 0;
        break;

    case T_ELSE:
    case T_DO:
        _state.parenthesesState = BalancedParentheses;
        break;

    case T_CONTINUE:
    case T_BREAK:
    case T_RETURN:
    case T_YIELD:
    case T_THROW:
        _state.restrictedKeyword = true;
        break;
    case T_RBRACE:
        if (_state.bracesCount > 0)
            --_state.bracesCount;
        if (_state.bracesCount == 0)
            goto again;
    } // switch

    // update the parentheses state
    switch (_state.parenthesesState) {
    case IgnoreParentheses:
        break;

    case CountParentheses:
        if (_state.tokenKind == T_RPAREN) {
            --_state.parenthesesCount;
            if (_state.parenthesesCount == 0)
                _state.parenthesesState = BalancedParentheses;
        } else if (_state.tokenKind == T_LPAREN) {
            ++_state.parenthesesCount;
        }
        break;

    case BalancedParentheses:
        if (_state.tokenKind != T_DO && _state.tokenKind != T_ELSE)
            _state.parenthesesState = IgnoreParentheses;
        break;
    } // switch

    return _state.tokenKind;
}

uint Lexer::decodeUnicodeEscapeCharacter(bool *ok)
{
    Q_ASSERT(_state.currentChar == u'u');
    scanChar(); // skip u
    if (_codePtr + 4 <= _endPtr && isHexDigit(_state.currentChar)) {
        uint codePoint = 0;
        for (int i = 0; i < 4; ++i) {
            int digit = hexDigit(_state.currentChar);
            if (digit < 0)
                goto error;
            codePoint *= 16;
            codePoint += digit;
            scanChar();
        }

        *ok = true;
        return codePoint;
    } else if (_codePtr < _endPtr && _state.currentChar == u'{') {
        scanChar(); // skip '{'
        uint codePoint = 0;
        if (!isHexDigit(_state.currentChar))
            // need at least one hex digit
            goto error;

        while (_codePtr <= _endPtr) {
            int digit = hexDigit(_state.currentChar);
            if (digit < 0)
                break;
            codePoint *= 16;
            codePoint += digit;
            if (codePoint > 0x10ffff)
                goto error;
            scanChar();
        }

        if (_state.currentChar != u'}')
            goto error;

        scanChar(); // skip '}'


        *ok = true;
        return codePoint;
    }

error:
    _state.errorCode = IllegalUnicodeEscapeSequence;
    _errorMessage = QCoreApplication::translate("QmlParser", "Illegal unicode escape sequence");

    *ok = false;
    return 0;
}

QChar Lexer::decodeHexEscapeCharacter(bool *ok)
{
    if (isHexDigit(_codePtr[0]) && isHexDigit(_codePtr[1])) {
        scanChar();

        const QChar c1 = _state.currentChar;
        scanChar();

        const QChar c2 = _state.currentChar;
        scanChar();

        if (ok)
            *ok = true;

        return convertHex(c1, c2);
    }

    *ok = false;
    return QChar();
}

namespace QmlJS {
QDebug operator<<(QDebug dbg, const Lexer &l)
{
    dbg << "{\n"
        << "  engine:" << qsizetype(l._engine) << ",\n"
        << "  lexMode:" << int(l._lexMode) << ",\n"
        << "  code.size:" << qsizetype(l._code.unicode()) << "+" << l._code.size() << ",\n"
        << "  endPtr: codePtr + " << (l._endPtr - l._codePtr) << ",\n"
        << "  qmlMode:" << l._qmlMode << ",\n"
        << "  staticIsKeyword:" << l._staticIsKeyword << ",\n"
        << "  currentLineNumber:" << l._currentLineNumber << ",\n"
        << "  currentColumnNumber:" << l._currentColumnNumber << ",\n"
        << "  currentOffset:" << l._currentOffset << ",\n"
        << "  tokenLength:" << l._tokenLength << ",\n"
        << "  tokenLine:" << l._tokenLine << ",\n"
        << "  tokenColumn:" << l._tokenColumn << ",\n"
        << "  tokenText:" << l._tokenText << ",\n"
        << "  skipLinefeed:" << l._skipLinefeed << ",\n"
        << "  errorMessage:" << l._errorMessage << ",\n"
        << "  tokenSpell:" << l._tokenSpell << ",\n"
        << "  rawString:" << l._rawString << ",\n";
    if (l._codePtr)
        dbg << "  codePtr: code.unicode()+" << (l._codePtr - l._code.unicode()) << ",\n";
    else
        dbg << "  codePtr: *null*,\n";
    if (l._tokenStartPtr)
        dbg << "  tokenStartPtr: codePtr " << (l._tokenStartPtr - l._codePtr) << ",\n";
    else
        dbg << "  tokenStartPtr: *null*,\n";
    dbg << "  state:" << l._state << "\n}";
    return dbg;
}
} // namespace QmlJS

static inline bool isIdentifierStart(uint ch)
{
    // fast path for ascii
    if ((ch >= u'a' && ch <= u'z') ||
        (ch >= u'A' && ch <= u'Z') ||
        ch == u'$' || ch == u'_')
        return true;

    switch (QChar::category(ch)) {
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

static bool isIdentifierPart(uint ch)
{
    // fast path for ascii
    if ((ch >= u'a' && ch <= u'z') ||
        (ch >= u'A' && ch <= u'Z') ||
        (ch >= u'0' && ch <= u'9') ||
        ch == u'$' || ch == u'_' ||
        ch == 0x200c /* ZWNJ */ || ch == 0x200d /* ZWJ */)
        return true;

    switch (QChar::category(ch)) {
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
    if (_state.stackToken != -1) {
        int tk = _state.stackToken;
        _state.stackToken = -1;
        return tk;
    }

    if (_state.bracesCount == 0) {
        // we're inside a Template string
        return scanString(TemplateContinuation);
    }

    if (_state.comments == CommentState::NoComment)
        _state.terminator = false;

again:
    _state.validTokenText = false;

    while (_state.currentChar.isSpace()) {
        if (isLineTerminator()) {
            bool isAtEnd = (_codePtr + (_skipLinefeed ? 1 : 0)) == _endPtr;
            if (_state.restrictedKeyword) {
                // automatic semicolon insertion
                _tokenLine = _currentLineNumber;
                _tokenColumn = _currentColumnNumber;
                _tokenStartPtr = _codePtr - 1;
                return T_SEMICOLON;
            } else if (_lexMode == LexMode::WholeCode || !isAtEnd) {
                _state.terminator = true;
                syncProhibitAutomaticSemicolon();
            } // else we will do the previous things at the start of next line...
        }

        scanChar();
    }

    _tokenStartPtr = _codePtr - 1;
    _tokenLine = _currentLineNumber;
    _tokenColumn = _currentColumnNumber;

    if (_codePtr >= _endPtr) {
        if (_lexMode == LexMode::LineByLine) {
            if (!_code.isEmpty()) {
                _state.currentChar = *(_codePtr - 2);
                return T_EOL;
            } else {
                return EOF_SYMBOL;
            }
        } else if (_codePtr > _endPtr) {
            return EOF_SYMBOL;
        }
    }

    const QChar ch = _state.currentChar;
    scanChar();

    switch (ch.unicode()) {
    case u'~': return T_TILDE;
    case u'}': return T_RBRACE;

    case u'|':
        if (_state.currentChar == u'|') {
            scanChar();
            return T_OR_OR;
        } else if (_state.currentChar == u'=') {
            scanChar();
            return T_OR_EQ;
        }
        return T_OR;

    case u'{': return T_LBRACE;

    case u'^':
        if (_state.currentChar == u'=') {
            scanChar();
            return T_XOR_EQ;
        }
        return T_XOR;

    case u']': return T_RBRACKET;
    case u'[': return T_LBRACKET;
    case u'?': {
        if (_state.currentChar == u'?') {
            scanChar();
            return T_QUESTION_QUESTION;
        }
        if (_state.currentChar == u'.' && !peekChar().isDigit()) {
            scanChar();
            return T_QUESTION_DOT;
        }

        return T_QUESTION;
    }

    case u'>':
        if (_state.currentChar == u'>') {
            scanChar();
            if (_state.currentChar == u'>') {
                scanChar();
                if (_state.currentChar == u'=') {
                    scanChar();
                    return T_GT_GT_GT_EQ;
                }
                return T_GT_GT_GT;
            } else if (_state.currentChar == u'=') {
                scanChar();
                return T_GT_GT_EQ;
            }
            return T_GT_GT;
        } else if (_state.currentChar == u'=') {
            scanChar();
            return T_GE;
        }
        return T_GT;

    case u'=':
        if (_state.currentChar == u'=') {
            scanChar();
            if (_state.currentChar == u'=') {
                scanChar();
                return T_EQ_EQ_EQ;
            }
            return T_EQ_EQ;
        } else if (_state.currentChar == u'>') {
            scanChar();
            return T_ARROW;
        }
        return T_EQ;

    case u'<':
        if (_state.currentChar == u'=') {
            scanChar();
            return T_LE;
        } else if (_state.currentChar == u'<') {
            scanChar();
            if (_state.currentChar == u'=') {
                scanChar();
                return T_LT_LT_EQ;
            }
            return T_LT_LT;
        }
        return T_LT;

    case u';': return T_SEMICOLON;
    case u':': return T_COLON;

    case u'/':
        switch (_state.currentChar.unicode()) {
        case u'*':
            scanChar();
            while (_codePtr <= _endPtr) {
                if (_state.currentChar == u'*') {
                    scanChar();
                    if (_state.currentChar == u'/') {
                        scanChar();
                        if (_engine) {
                            _engine->addComment(tokenOffset() + 2,
                                                _codePtr - _tokenStartPtr - 1 - 4,
                                                tokenStartLine(),
                                                tokenStartColumn() + 2);
                        }
                        if (_lexMode == LexMode::LineByLine)
                            return T_COMMENT;
                        else
                            goto again;
                    }
                } else {
                    scanChar();
                }
            }
            if (_lexMode == LexMode::LineByLine)
                return T_PARTIAL_COMMENT;
            else
                goto again;
        case u'/':
            while (_codePtr <= _endPtr && !isLineTerminator()) {
                scanChar();
            }
            if (_engine) {
                _engine->addComment(tokenOffset() + 2,
                                    _codePtr - _tokenStartPtr - 1 - 2,
                                    tokenStartLine(),
                                    tokenStartColumn() + 2);
            }
            if (_lexMode == LexMode::LineByLine)
                return T_COMMENT;
            else
                goto again;
        case u'=':
            scanChar();
            return T_DIVIDE_EQ;
        default:
            return T_DIVIDE_;
        }
    case u'.':
        if (_state.importState == ImportState::SawImport)
            return T_DOT;
        if (isDecimalDigit(_state.currentChar.unicode()))
            return scanNumber(ch);
        if (_state.currentChar == u'.') {
            scanChar();
            if (_state.currentChar == u'.') {
                scanChar();
                return T_ELLIPSIS;
            } else {
                _state.errorCode = IllegalCharacter;
                _errorMessage = QCoreApplication::translate("QmlParser", "Unexpected token '.'");
                return T_ERROR;
            }
        }
        return T_DOT;

    case u'-':
        if (_state.currentChar == u'=') {
            scanChar();
            return T_MINUS_EQ;
        } else if (_state.currentChar == u'-') {
            scanChar();

            if (_state.terminator && !_state.delimited && !_state.prohibitAutomaticSemicolon
                && _state.tokenKind != T_LPAREN) {
                _state.stackToken = T_MINUS_MINUS;
                return T_SEMICOLON;
            }

            return T_MINUS_MINUS;
        }
        return T_MINUS;

    case u',': return T_COMMA;

    case u'+':
        if (_state.currentChar == u'=') {
            scanChar();
            return T_PLUS_EQ;
        } else if (_state.currentChar == u'+') {
            scanChar();

            if (_state.terminator && !_state.delimited && !_state.prohibitAutomaticSemicolon
                && _state.tokenKind != T_LPAREN) {
                _state.stackToken = T_PLUS_PLUS;
                return T_SEMICOLON;
            }

            return T_PLUS_PLUS;
        }
        return T_PLUS;

    case u'*':
        if (_state.currentChar == u'=') {
            scanChar();
            return T_STAR_EQ;
        } else if (_state.currentChar == u'*') {
            scanChar();
            if (_state.currentChar == u'=') {
                scanChar();
                return T_STAR_STAR_EQ;
            }
            return T_STAR_STAR;
        }
        return T_STAR;

    case u')': return T_RPAREN;
    case u'(': return T_LPAREN;

    case u'@': return T_AT;

    case u'&':
        if (_state.currentChar == u'=') {
            scanChar();
            return T_AND_EQ;
        } else if (_state.currentChar == u'&') {
            scanChar();
            return T_AND_AND;
        }
        return T_AND;

    case u'%':
        if (_state.currentChar == u'=') {
            scanChar();
            return T_REMAINDER_EQ;
        }
        return T_REMAINDER;

    case u'!':
        if (_state.currentChar == u'=') {
            scanChar();
            if (_state.currentChar == u'=') {
                scanChar();
                return T_NOT_EQ_EQ;
            }
            return T_NOT_EQ;
        }
        return T_NOT;

    case u'`':
        _state.outerTemplateBraceCount.push(_state.bracesCount);
        Q_FALLTHROUGH();
    case u'\'':
    case u'"':
        return scanString(ScanStringMode(ch.unicode()));
    case u'0':
    case u'1':
    case u'2':
    case u'3':
    case u'4':
    case u'5':
    case u'6':
    case u'7':
    case u'8':
    case u'9':
        if (_state.importState == ImportState::SawImport)
            return scanVersionNumber(ch);
        else
            return scanNumber(ch);

    case '#':
        if (_currentLineNumber == 1 && _currentColumnNumber == 2) {
            // shebang support
            while (_codePtr <= _endPtr && !isLineTerminator()) {
                scanChar();
            }
            if (_engine) {
                _engine->addComment(tokenOffset(),
                                    _codePtr - _tokenStartPtr - 1,
                                    tokenStartLine(),
                                    tokenStartColumn());
            }
            if (_lexMode == LexMode::LineByLine)
                return T_COMMENT;
            else
                goto again;
        }
        Q_FALLTHROUGH();

    default: {
        uint c = ch.unicode();
        bool identifierWithEscapeChars = false;
        if (QChar::isHighSurrogate(c) && QChar::isLowSurrogate(_state.currentChar.unicode())) {
            c = QChar::surrogateToUcs4(ushort(c), _state.currentChar.unicode());
            scanChar();
        } else if (c == '\\' && _state.currentChar == u'u') {
            identifierWithEscapeChars = true;
            bool ok = false;
            c = decodeUnicodeEscapeCharacter(&ok);
            if (!ok)
                return T_ERROR;
        }
        if (isIdentifierStart(c)) {
            if (identifierWithEscapeChars) {
                _tokenText.resize(0);
                if (QChar::requiresSurrogates(c)) {
                    _tokenText += QChar(QChar::highSurrogate(c));
                    _tokenText += QChar(QChar::lowSurrogate(c));
                } else {
                    _tokenText += QChar(c);
                }
                _state.validTokenText = true;
            }
            while (_codePtr <= _endPtr) {
                c = _state.currentChar.unicode();
                if (QChar::isHighSurrogate(c) && QChar::isLowSurrogate(_codePtr->unicode())) {
                    scanChar();
                    c = QChar::surrogateToUcs4(ushort(c), _state.currentChar.unicode());
                } else if (_state.currentChar == u'\\' && _codePtr[0] == u'u') {
                    if (!identifierWithEscapeChars) {
                        identifierWithEscapeChars = true;
                        _tokenText.resize(0);
                        _tokenText.insert(0, _tokenStartPtr, _codePtr - _tokenStartPtr - 1);
                        _state.validTokenText = true;
                    }

                    scanChar(); // skip '\\'
                    bool ok = false;
                    c = decodeUnicodeEscapeCharacter(&ok);
                    if (!ok)
                        return T_ERROR;

                    if (!isIdentifierPart(c))
                        break;

                    if (QChar::requiresSurrogates(c)) {
                        _tokenText += QChar(QChar::highSurrogate(c));
                        _tokenText += QChar(QChar::lowSurrogate(c));
                    } else {
                        _tokenText += QChar(c);
                    }
                    continue;
                }

                if (!isIdentifierPart(c))
                    break;

                if (identifierWithEscapeChars) {
                    if (QChar::requiresSurrogates(c)) {
                        _tokenText += QChar(QChar::highSurrogate(c));
                        _tokenText += QChar(QChar::lowSurrogate(c));
                    } else {
                        _tokenText += QChar(c);
                    }
                }
                scanChar();
            }

            _tokenLength = _codePtr - _tokenStartPtr - 1;

            int kind = T_IDENTIFIER;

            if (!identifierWithEscapeChars)
                kind = classify(_tokenStartPtr, _tokenLength, parseModeFlags());

            if (kind == T_FUNCTION) {
                continue_skipping:
                while (_codePtr < _endPtr && _state.currentChar.isSpace())
                    scanChar();
                if (_state.currentChar == u'*') {
                    _tokenLength = _codePtr - _tokenStartPtr - 1;
                    kind = T_FUNCTION_STAR;
                    scanChar();
                } else if (_state.currentChar == u'/') {
                    scanChar();
                    switch (_state.currentChar.unicode()) {
                    case u'*':
                        scanChar();
                        while (_codePtr <= _endPtr) {
                            if (_state.currentChar == u'*') {
                                scanChar();
                                if (_state.currentChar == u'/') {
                                    scanChar();
                                    if (_engine) {
                                        _engine->addComment(tokenOffset() + 2,
                                                            _codePtr - _tokenStartPtr - 1 - 4,
                                                            tokenStartLine(),
                                                            tokenStartColumn() + 2);
                                    }
                                    if (_lexMode == LexMode::LineByLine)
                                        return T_COMMENT;
                                    goto continue_skipping;
                                }
                            } else {
                                scanChar();
                            }
                        }
                        if (_lexMode == LexMode::LineByLine)
                            return T_PARTIAL_COMMENT;
                        else
                            goto continue_skipping;
                    case u'/':
                        while (_codePtr <= _endPtr && !isLineTerminator()) {
                            scanChar();
                        }
                        if (_engine) {
                            _engine->addComment(tokenOffset() + 2,
                                                _codePtr - _tokenStartPtr - 1 - 2,
                                                tokenStartLine(),
                                                tokenStartColumn() + 2);
                        }
                        if (_lexMode == LexMode::LineByLine)
                            return T_COMMENT;
                        else
                            goto continue_skipping;
                    default:
                        break;
                    }
                }
            }

            if (_engine) {
                if (kind == T_IDENTIFIER && identifierWithEscapeChars)
                    _tokenSpell = _engine->newStringRef(_tokenText);
                else
                    _tokenSpell = _engine->midRef(_tokenStartPtr - _code.unicode(), _tokenLength);
            }

            return kind;
        }
        }

        break;
    }

    return T_ERROR;
}

int Lexer::scanString(ScanStringMode mode)
{
    QChar quote = (mode == TemplateContinuation) ? QChar(TemplateHead) : QChar(mode);
    // we actually use T_STRING_LITERAL also for multiline strings, should we want to
    // change that we should set it to:
    //     _state.tokenKind == T_PARTIAL_SINGLE_QUOTE_STRING_LITERAL ||
    //     _state.tokenKind == T_PARTIAL_DOUBLE_QUOTE_STRING_LITERAL
    // here and uncomment the multilineStringLiteral = true below.
    bool multilineStringLiteral = false;

    const QChar *startCode = _codePtr - 1;
    // in case we just parsed a \r, we need to reset this flag to get things working
    // correctly in the loop below and afterwards
    _skipLinefeed = false;
    bool first = true;

    if (_engine) {
        while (_codePtr <= _endPtr) {
            if (isLineTerminator()) {
                if ((quote == u'`' || qmlMode())) {
                    if (first)
                        --_currentLineNumber; // will be read again in scanChar()
                    break;
                }
                _state.errorCode = IllegalCharacter;
                _errorMessage = QCoreApplication::translate("QmlParser",
                                                            "Stray newline in string literal");
                return T_ERROR;
            } else if (_state.currentChar == u'\\') {
                break;
            } else if (_state.currentChar == u'$' && quote == u'`') {
                break;
            } else if (_state.currentChar == quote) {
                _tokenSpell = _engine->midRef(startCode - _code.unicode(), _codePtr - startCode - 1);
                _rawString = _tokenSpell;
                scanChar();

                if (quote == u'`')
                    _state.bracesCount = _state.outerTemplateBraceCount.pop();
                if (mode == TemplateHead)
                    return T_NO_SUBSTITUTION_TEMPLATE;
                else if (mode == TemplateContinuation)
                    return T_TEMPLATE_TAIL;
                else if (multilineStringLiteral)
                    return T_MULTILINE_STRING_LITERAL;
                else
                    return T_STRING_LITERAL;
            }
            // don't use scanChar() here, that would transform \r sequences and the midRef() call would create the wrong result
            _state.currentChar = *_codePtr++;
            ++_currentColumnNumber;
            first = false;
        }
    }

    // rewind by one char, so things gets scanned correctly
    --_codePtr;
    --_currentColumnNumber;

    _state.validTokenText = true;
    _tokenText = QString(startCode, _codePtr - startCode);

    auto setRawString = [&](const QChar *end) {
        QString raw(startCode, end - startCode - 1);
        raw.replace(QLatin1String("\r\n"), QLatin1String("\n"));
        raw.replace(u'\r', u'\n');
        _rawString = _engine->newStringRef(raw);
    };

    scanChar();

    while (_codePtr <= _endPtr) {
        if (_state.currentChar == quote) {
            scanChar();

            if (_engine) {
                _tokenSpell = _engine->newStringRef(_tokenText);
                if (quote == u'`')
                    setRawString(_codePtr - 1);
            }

            if (quote == u'`')
                _state.bracesCount = _state.outerTemplateBraceCount.pop();

            if (mode == TemplateContinuation)
                return T_TEMPLATE_TAIL;
            else if (mode == TemplateHead)
                return T_NO_SUBSTITUTION_TEMPLATE;

            return multilineStringLiteral ? T_MULTILINE_STRING_LITERAL : T_STRING_LITERAL;
        } else if (quote == u'`' && _state.currentChar == u'$' && *_codePtr == u'{') {
            scanChar();
            scanChar();
            _state.bracesCount = 1;
            if (_engine) {
                _tokenSpell = _engine->newStringRef(_tokenText);
                setRawString(_codePtr - 2);
            }

            return (mode == TemplateHead ? T_TEMPLATE_HEAD : T_TEMPLATE_MIDDLE);
        } else if (_state.currentChar == u'\\') {
            scanChar();
            if (_codePtr > _endPtr) {
                _state.errorCode = IllegalEscapeSequence;
                _errorMessage
                    = QCoreApplication::translate("QmlParser",
                                                  "End of file reached at escape sequence");
                return T_ERROR;
            }

            QChar u;

            switch (_state.currentChar.unicode()) {
            // unicode escape sequence
            case u'u': {
                bool ok = false;
                uint codePoint = decodeUnicodeEscapeCharacter(&ok);
                if (!ok)
                    return T_ERROR;
                if (QChar::requiresSurrogates(codePoint)) {
                    // need to use a surrogate pair
                    _tokenText += QChar(QChar::highSurrogate(codePoint));
                    u = QChar::lowSurrogate(codePoint);
                } else {
                    u = QChar(codePoint);
                }
            } break;

            // hex escape sequence
            case u'x': {
                bool ok = false;
                u = decodeHexEscapeCharacter(&ok);
                if (!ok) {
                    _state.errorCode = IllegalHexadecimalEscapeSequence;
                    _errorMessage
                        = QCoreApplication::translate("QmlParser",
                                                      "Illegal hexadecimal escape sequence");
                    return T_ERROR;
                }
            } break;

            // single character escape sequence
            case u'\\': u = u'\\'; scanChar(); break;
            case u'\'': u = u'\''; scanChar(); break;
            case u'\"': u = u'\"'; scanChar(); break;
            case u'b':  u = u'\b'; scanChar(); break;
            case u'f':  u = u'\f'; scanChar(); break;
            case u'n':  u = u'\n'; scanChar(); break;
            case u'r':  u = u'\r'; scanChar(); break;
            case u't':  u = u'\t'; scanChar(); break;
            case u'v':  u = u'\v'; scanChar(); break;

            case u'0':
                if (!_codePtr->isDigit()) {
                    scanChar();
                    u = u'\0';
                    break;
                }
                Q_FALLTHROUGH();
            case u'1':
            case u'2':
            case u'3':
            case u'4':
            case u'5':
            case u'6':
            case u'7':
            case u'8':
            case u'9':
                _state.errorCode = IllegalEscapeSequence;
                _errorMessage
                    = QCoreApplication::translate("QmlParser",
                                                  "Octal escape sequences are not allowed");
                return T_ERROR;

            case u'\r':
            case u'\n':
            case 0x2028u:
            case 0x2029u:
                // uncomment the following to use T_MULTILINE_STRING_LITERAL
                // multilineStringLiteral = true;
                scanChar();
                continue;

            default:
                // non escape character
                u = _state.currentChar;
                scanChar();
            }

            _tokenText += u;
        } else {
            _tokenText += _state.currentChar;
            scanChar();
        }
    }
    if (_lexMode == LexMode::LineByLine && !_code.isEmpty()) {
        if (mode == TemplateContinuation)
            return T_PARTIAL_TEMPLATE_MIDDLE;
        else if (mode == TemplateHead)
            return T_PARTIAL_TEMPLATE_HEAD;
        else if (mode == SingleQuote)
            return T_PARTIAL_SINGLE_QUOTE_STRING_LITERAL;
        return T_PARTIAL_DOUBLE_QUOTE_STRING_LITERAL;
    }
    _state.errorCode = UnclosedStringLiteral;
    _errorMessage = QCoreApplication::translate("QmlParser", "Unclosed string at end of line");
    return T_ERROR;
}

int Lexer::scanNumber(QChar ch)
{
    if (ch == u'0') {
        if (_state.currentChar == u'x' || _state.currentChar == u'X') {
            ch = _state.currentChar; // remember the x or X to use it in the error message below.

            // parse hex integer literal
            scanChar(); // consume 'x'

            if (!isHexDigit(_state.currentChar)) {
                _state.errorCode = IllegalNumber;
                _errorMessage = QCoreApplication::translate(
                                    "QmlParser",
                                    "At least one hexadecimal digit is required after '0%1'")
                                    .arg(ch);
                return T_ERROR;
            }

            double d = 0.;
            while (1) {
                int digit = ::hexDigit(_state.currentChar);
                if (digit < 0)
                    break;
                d *= 16;
                d += digit;
                scanChar();
            }

            _state.tokenValue = d;
            return T_NUMERIC_LITERAL;
        } else if (_state.currentChar == u'o' || _state.currentChar == u'O') {
            ch = _state.currentChar; // remember the o or O to use it in the error message below.

            // parse octal integer literal
            scanChar(); // consume 'o'

            if (!isOctalDigit(_state.currentChar.unicode())) {
                _state.errorCode = IllegalNumber;
                _errorMessage = QCoreApplication::translate(
                                    "QmlParser", "At least one octal digit is required after '0%1'")
                                    .arg(ch);
                return T_ERROR;
            }

            double d = 0.;
            while (1) {
                int digit = ::octalDigit(_state.currentChar);
                if (digit < 0)
                    break;
                d *= 8;
                d += digit;
                scanChar();
            }

            _state.tokenValue = d;
            return T_NUMERIC_LITERAL;
        } else if (_state.currentChar == u'b' || _state.currentChar == u'B') {
            ch = _state.currentChar; // remember the b or B to use it in the error message below.

            // parse binary integer literal
            scanChar(); // consume 'b'

            if (_state.currentChar.unicode() != u'0' && _state.currentChar.unicode() != u'1') {
                _state.errorCode = IllegalNumber;
                _errorMessage = QCoreApplication::translate(
                                    "QmlParser", "At least one binary digit is required after '0%1'")
                                    .arg(ch);
                return T_ERROR;
            }

            double d = 0.;
            while (1) {
                int digit = 0;
                if (_state.currentChar.unicode() == u'1')
                    digit = 1;
                else if (_state.currentChar.unicode() != u'0')
                    break;
                d *= 2;
                d += digit;
                scanChar();
            }

            _state.tokenValue = d;
            return T_NUMERIC_LITERAL;
        } else if (_state.currentChar.isDigit() && !qmlMode()) {
            _state.errorCode = IllegalCharacter;
            _errorMessage = QCoreApplication::translate("QmlParser",
                                                        "Decimal numbers can't start with '0'");
            return T_ERROR;
        }
    }

    // decimal integer literal
    QVarLengthArray<char,32> chars;
    chars.append(ch.unicode());

    if (ch != u'.') {
        while (_state.currentChar.isDigit()) {
            chars.append(_state.currentChar.unicode());
            scanChar(); // consume the digit
        }

        if (_state.currentChar == u'.') {
            chars.append(_state.currentChar.unicode());
            scanChar(); // consume `.'
        }
    }

    while (_state.currentChar.isDigit()) {
        chars.append(_state.currentChar.unicode());
        scanChar();
    }

    if (_state.currentChar == u'e' || _state.currentChar == u'E') {
        if (_codePtr[0].isDigit()
            || ((_codePtr[0] == u'+' || _codePtr[0] == u'-') && _codePtr[1].isDigit())) {
            chars.append(_state.currentChar.unicode());
            scanChar(); // consume `e'

            if (_state.currentChar == u'+' || _state.currentChar == u'-') {
                chars.append(_state.currentChar.unicode());
                scanChar(); // consume the sign
            }

            while (_state.currentChar.isDigit()) {
                chars.append(_state.currentChar.unicode());
                scanChar();
            }
        }
    }

    const char *begin = chars.constData();
    const char *end = nullptr;
    bool ok = false;

    _state.tokenValue = qstrntod(begin, chars.size(), &end, &ok);

    if (end - begin != chars.size()) {
        _state.errorCode = IllegalExponentIndicator;
        _errorMessage = QCoreApplication::translate("QmlParser",
                                                    "Illegal syntax for exponential number");
        return T_ERROR;
    }

    return T_NUMERIC_LITERAL;
}

int Lexer::scanVersionNumber(QChar ch)
{
    if (ch == u'0') {
        _state.tokenValue = 0;
        return T_VERSION_NUMBER;
    }

    int acc = 0;
    acc += ch.digitValue();

    while (_state.currentChar.isDigit()) {
        acc *= 10;
        acc += _state.currentChar.digitValue();
        scanChar(); // consume the digit
    }

    _state.tokenValue = acc;
    return T_VERSION_NUMBER;
}

bool Lexer::scanRegExp(RegExpBodyPrefix prefix)
{
    _tokenText.resize(0);
    _state.validTokenText = true;
    _state.patternFlags = 0;

    if (prefix == EqualPrefix)
        _tokenText += u'=';

    while (true) {
        switch (_state.currentChar.unicode()) {
        case u'/':
            scanChar();

            // scan the flags
            _state.patternFlags = 0;
            while (isIdentLetter(_state.currentChar)) {
                int flag = regExpFlagFromChar(_state.currentChar);
                if (flag == 0 || _state.patternFlags & flag) {
                    _errorMessage
                        = QCoreApplication::translate("QmlParser",
                                                      "Invalid regular expression flag '%0'")
                              .arg(QChar(_state.currentChar));
                    return false;
                }
                _state.patternFlags |= flag;
                scanChar();
            }

            _tokenLength = _codePtr - _tokenStartPtr - 1;
            return true;

        case u'\\':
            // regular expression backslash sequence
            _tokenText += _state.currentChar;
            scanChar();

            if (_codePtr > _endPtr || isLineTerminator()) {
                _errorMessage = QCoreApplication::translate(
                    "QmlParser", "Unterminated regular expression backslash sequence");
                return false;
            }

            _tokenText += _state.currentChar;
            scanChar();
            break;

        case u'[':
            // regular expression class
            _tokenText += _state.currentChar;
            scanChar();

            while (_codePtr <= _endPtr && !isLineTerminator()) {
                if (_state.currentChar == u']')
                    break;
                else if (_state.currentChar == u'\\') {
                    // regular expression backslash sequence
                    _tokenText += _state.currentChar;
                    scanChar();

                    if (_codePtr > _endPtr || isLineTerminator()) {
                        _errorMessage = QCoreApplication::translate(
                            "QmlParser", "Unterminated regular expression backslash sequence");
                        return false;
                    }

                    _tokenText += _state.currentChar;
                    scanChar();
                } else {
                    _tokenText += _state.currentChar;
                    scanChar();
                }
            }

            if (_state.currentChar != u']') {
                _errorMessage
                    = QCoreApplication::translate("QmlParser",
                                                  "Unterminated regular expression class");
                return false;
            }

            _tokenText += _state.currentChar;
            scanChar(); // skip ]
            break;

        default:
            if (_codePtr > _endPtr || isLineTerminator()) {
                _errorMessage
                    = QCoreApplication::translate("QmlParser",
                                                  "Unterminated regular expression literal");
                return false;
            } else {
                _tokenText += _state.currentChar;
                scanChar();
            }
        } // switch
    } // while

    return false;
}

bool Lexer::isLineTerminator() const
{
    const ushort unicode = _state.currentChar.unicode();
    return unicode == 0x000Au
            || unicode == 0x000Du
            || unicode == 0x2028u
            || unicode == 0x2029u;
}

unsigned Lexer::isLineTerminatorSequence() const
{
    switch (_state.currentChar.unicode()) {
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
    if ((ch >= u'a' && ch <= u'z')
            || (ch >= u'A' && ch <= u'Z')
            || ch == u'$' || ch == u'_')
        return true;
    if (ch.unicode() < 128)
        return false;
    return ch.isLetterOrNumber();
}

bool Lexer::isDecimalDigit(ushort c)
{
    return (c >= u'0' && c <= u'9');
}

bool Lexer::isHexDigit(QChar c)
{
    return ((c >= u'0' && c <= u'9')
            || (c >= u'a' && c <= u'f')
            || (c >= u'A' && c <= u'F'));
}

bool Lexer::isOctalDigit(ushort c)
{
    return (c >= u'0' && c <= u'7');
}

QString Lexer::tokenText() const
{
    if (_state.validTokenText)
        return _tokenText;

    if (_state.tokenKind == T_STRING_LITERAL)
        return QString(_tokenStartPtr + 1, _tokenLength - 2);

    return QString(_tokenStartPtr, _tokenLength);
}

Lexer::Error Lexer::errorCode() const
{
    return _state.errorCode;
}

QString Lexer::errorMessage() const
{
    return _errorMessage;
}

void Lexer::syncProhibitAutomaticSemicolon()
{
    if (_state.parenthesesState == BalancedParentheses) {
        // we have seen something like "if (foo)", which means we should
        // never insert an automatic semicolon at this point, since it would
        // then be expanded into an empty statement (ECMA-262 7.9.1)
        _state.prohibitAutomaticSemicolon = true;
        _state.parenthesesState = IgnoreParentheses;
    } else {
        _state.prohibitAutomaticSemicolon = false;
    }
}

bool Lexer::prevTerminator() const
{
    return _state.terminator;
}

bool Lexer::followsClosingBrace() const
{
    return _state.followsClosingBrace;
}

bool Lexer::canInsertAutomaticSemicolon(int token) const
{
    return token == T_RBRACE || token == EOF_SYMBOL || _state.terminator
           || _state.followsClosingBrace;
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
    QmlJSGrammar::T_FUNCTION_STAR,
    QmlJSGrammar::T_IF,
    QmlJSGrammar::T_IN,
    QmlJSGrammar::T_OF,
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
    auto setError = [error, this](QString message) {
        error->message = std::move(message);
        error->loc.startLine = tokenStartLine();
        error->loc.startColumn = tokenStartColumn();
    };

    QScopedValueRollback<bool> directivesGuard(_state.handlingDirectives, true);
    Q_ASSERT(!_qmlMode);

    lex(); // fetch the first token

    if (_state.tokenKind != T_DOT)
        return true;

    do {
        const int lineNumber = tokenStartLine();
        const int column = tokenStartColumn();

        lex(); // skip T_DOT

        if (!(_state.tokenKind == T_IDENTIFIER || _state.tokenKind == T_IMPORT))
            return true; // expected a valid QML/JS directive

        const QString directiveName = tokenText();

        if (! (directiveName == QLatin1String("pragma") ||
               directiveName == QLatin1String("import"))) {
            setError(QCoreApplication::translate("QmlParser", "Syntax error"));
            return false; // not a valid directive name
        }

        // it must be a pragma or an import directive.
        if (directiveName == QLatin1String("pragma")) {
            // .pragma library
            if (! (lex() == T_IDENTIFIER && tokenText() == QLatin1String("library"))) {
                setError(QCoreApplication::translate("QmlParser", "Syntax error"));
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

            if (_state.tokenKind == T_STRING_LITERAL) {
                // .import T_STRING_LITERAL as T_IDENTIFIER

                fileImport = true;
                pathOrUri = tokenText();

                if (!pathOrUri.endsWith(QLatin1String("js"))) {
                    setError(QCoreApplication::translate("QmlParser","Imported file must be a script"));
                    return false;
                }
                lex();

            } else if (_state.tokenKind == T_IDENTIFIER) {
                // .import T_IDENTIFIER (. T_IDENTIFIER)* (T_VERSION_NUMBER (. T_VERSION_NUMBER)?)? as T_IDENTIFIER
                while (true) {
                    if (!isUriToken(_state.tokenKind)) {
                        setError(QCoreApplication::translate("QmlParser","Invalid module URI"));
                        return false;
                    }

                    pathOrUri.append(tokenText());

                    lex();
                    if (tokenStartLine() != lineNumber) {
                        setError(QCoreApplication::translate("QmlParser","Invalid module URI"));
                        return false;
                    }
                    if (_state.tokenKind != QmlJSGrammar::T_DOT)
                        break;

                    pathOrUri.append(u'.');

                    lex();
                    if (tokenStartLine() != lineNumber) {
                        setError(QCoreApplication::translate("QmlParser","Invalid module URI"));
                        return false;
                    }
                }

                if (_state.tokenKind == T_VERSION_NUMBER) {
                    version = tokenText();
                    lex();
                    if (_state.tokenKind == T_DOT) {
                        version += u'.';
                        lex();
                        if (_state.tokenKind != T_VERSION_NUMBER) {
                            setError(QCoreApplication::translate(
                                         "QmlParser", "Incomplete version number (dot but no minor)"));
                            return false; // expected the module version number
                        }
                        version += tokenText();
                        lex();
                    }
                }
            }

            //
            // recognize the mandatory `as' followed by the module name
            //
            if (!(_state.tokenKind == T_AS && tokenStartLine() == lineNumber)) {
                if (fileImport)
                    setError(QCoreApplication::translate("QmlParser", "File import requires a qualifier"));
                else
                    setError(QCoreApplication::translate("QmlParser", "Module import requires a qualifier"));
                if (tokenStartLine() != lineNumber) {
                    error->loc.startLine = lineNumber;
                    error->loc.startColumn = column;
                }
                return false; // expected `as'
            }

            if (lex() != T_IDENTIFIER || tokenStartLine() != lineNumber) {
                if (fileImport)
                    setError(QCoreApplication::translate("QmlParser", "File import requires a qualifier"));
                else
                    setError(QCoreApplication::translate("QmlParser", "Module import requires a qualifier"));
                return false; // expected module name
            }

            const QString module = tokenText();
            if (!module.at(0).isUpper()) {
                setError(QCoreApplication::translate("QmlParser","Invalid import qualifier"));
                return false;
            }

            if (fileImport)
                directives->importFile(pathOrUri, module, lineNumber, column);
            else
                directives->importModule(pathOrUri, version, module, lineNumber, column);
        }

        if (tokenStartLine() != lineNumber) {
            setError(QCoreApplication::translate("QmlParser", "Syntax error"));
            return false; // the directives cannot span over multiple lines
        }

        // fetch the first token after the .pragma/.import directive
        lex();
    } while (_state.tokenKind == T_DOT);

    return true;
}

const Lexer::State &Lexer::state() const
{
    return _state;
}
void Lexer::setState(const Lexer::State &state)
{
    _state = state;
}

namespace QmlJS {
QDebug operator<<(QDebug dbg, const Lexer::State &s)
{
    dbg << "{\n"
        << "   errorCode:" << int(s.errorCode) << ",\n"
        << "   currentChar:" << s.currentChar << ",\n"
        << "   tokenValue:" << s.tokenValue << ",\n"
        << "   parenthesesState:" << s.parenthesesState << ",\n"
        << "   parenthesesCount:" << s.parenthesesCount << ",\n"
        << "   outerTemplateBraceCount:" << s.outerTemplateBraceCount << ",\n"
        << "   bracesCount:" << s.bracesCount << ",\n"
        << "   stackToken:" << s.stackToken << ",\n"
        << "   patternFlags:" << s.patternFlags << ",\n"
        << "   tokenKind:" << s.tokenKind << ",\n"
        << "   importState:" << int(s.importState) << ",\n"
        << "   validTokenText:" << s.validTokenText << ",\n"
        << "   prohibitAutomaticSemicolon:" << s.prohibitAutomaticSemicolon << ",\n"
        << "   restrictedKeyword:" << s.restrictedKeyword << ",\n"
        << "   terminator:" << s.terminator << ",\n"
        << "   followsClosingBrace:" << s.followsClosingBrace << ",\n"
        << "   delimited:" << s.delimited << ",\n"
        << "   handlingDirectives:" << s.handlingDirectives << ",\n"
        << "   generatorLevel:" << s.generatorLevel << "\n}";
    return dbg;
}
} // namespace QmlJS

QT_QML_END_NAMESPACE
