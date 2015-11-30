// Copyright (c) 2008 Roberto Raggi <roberto.raggi@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "Lexer.h"
#include "Control.h"
#include "TranslationUnit.h"
#include "Literals.h"

#include "cppassert.h"

#include <cctype>

using namespace CPlusPlus;

/*!
    \class Lexer
    \brief The Lexer generates tokens from an UTF-8 encoded source text.

    \sa Token
*/

/*!
    \fn static void Lexer::yyinp_utf8(const char *&currentSourceChar, unsigned char &yychar, unsigned &utf16charCounter)

    Process a single unicode code point in an UTF-8 encoded source.

    \a currentSourceChar points to the UTF-8 encoded source.
    \a yychar must be the byte pointed to by \a currentSourceChar.

    Points \a currentSourceChar to the byte of the next code point
    and modifies \a yychar to the value pointed by the updated
    \a currentSourceChar. \a utf16charCounter will be incremented by
    the number of UTF-16 code units that were needed for that code
    point.
*/

Lexer::Lexer(TranslationUnit *unit)
    : _translationUnit(unit),
      _control(unit->control()),
      _state(0),
      _flags(0),
      _currentLine(1)
{
    f._scanKeywords = true;
    setSource(_translationUnit->firstSourceChar(),
              _translationUnit->lastSourceChar());
}

Lexer::Lexer(const char *firstChar, const char *lastChar)
    : _translationUnit(0),
      _control(0),
      _state(0),
      _flags(0),
      _currentLine(1)
{
    f._scanKeywords = true;
    setSource(firstChar, lastChar);
}

Lexer::~Lexer()
{ }

void Lexer::setSource(const char *firstChar, const char *lastChar)
{
    _firstChar = firstChar;
    _lastChar = lastChar;
    _currentChar = _firstChar - 1;
    _currentCharUtf16 = ~0;
    _tokenStart = _currentChar;
    _yychar = '\n';
}

void Lexer::setStartWithNewline(bool enabled)
{
    if (enabled)
        _yychar = '\n';
    else
        _yychar = ' ';
}

int Lexer::state() const
{ return _state; }

void Lexer::setState(int state)
{ _state = state; }

bool Lexer::scanCommentTokens() const
{ return f._scanCommentTokens; }

void Lexer::setScanCommentTokens(bool onoff)
{ f._scanCommentTokens = onoff; }

bool Lexer::scanKeywords() const
{ return f._scanKeywords; }

void Lexer::setScanKeywords(bool onoff)
{ f._scanKeywords = onoff; }

void Lexer::setScanAngleStringLiteralTokens(bool onoff)
{ f._scanAngleStringLiteralTokens = onoff; }

void Lexer::pushLineStartOffset()
{
    ++_currentLine;

    if (_translationUnit)
        _translationUnit->pushLineOffset(_currentCharUtf16);
}

void Lexer::scan(Token *tok)
{
    tok->reset();
    scan_helper(tok);
    tok->f.bytes = _currentChar - _tokenStart;
    tok->f.utf16chars = _currentCharUtf16 - _tokenStartUtf16;
}

static bool isRawStringLiteral(unsigned char kind)
{
    return kind >= T_FIRST_RAW_STRING_LITERAL
        && kind <= T_LAST_RAW_STRING_LITERAL;
}

static bool isMultiLineToken(unsigned char kind)
{
    return kind == T_EOF_SYMBOL
        || kind == T_COMMENT
        || kind == T_DOXY_COMMENT
        || isRawStringLiteral(kind);
}

void Lexer::scan_helper(Token *tok)
{
  again:
    while (_yychar && std::isspace(_yychar)) {
        if (_yychar == '\n') {
            tok->f.joined = s._newlineExpected;
            tok->f.newline = !s._newlineExpected;

            if (s._newlineExpected)
                s._newlineExpected = false;
            else if (!isMultiLineToken(s._tokenKind))
                _state = 0;
        } else {
            tok->f.whitespace = true;
        }
        yyinp();
    }

    if (! _translationUnit)
        tok->lineno = _currentLine;

    _tokenStart = _currentChar;
    tok->byteOffset = _currentChar - _firstChar;

    _tokenStartUtf16 = _currentCharUtf16;
    tok->utf16charOffset = _currentCharUtf16;

    if (_yychar) {
        s._newlineExpected = false;
    } else if (s._tokenKind) {
        tok->f.kind = T_EOF_SYMBOL;
        return;
    }

    if (s._tokenKind == T_EOF_SYMBOL) {
        // skip
    } else if (s._tokenKind == T_COMMENT || s._tokenKind == T_DOXY_COMMENT) {
        const int originalKind = s._tokenKind;

        while (_yychar) {
            if (_yychar != '*')
                yyinp();
            else {
                yyinp();
                if (_yychar == '/') {
                    yyinp();
                    _state = 0;
                    break;
                }
            }
        }

        if (! f._scanCommentTokens)
            goto again;

        tok->f.kind = originalKind;
        return;
    } else if (s._tokenKind == T_CPP_COMMENT || s._tokenKind == T_CPP_DOXY_COMMENT) {
        const Kind originalKind = (Kind)s._tokenKind;
        tok->f.joined = true;
        if (f._scanCommentTokens)
            tok->f.kind = originalKind;
        _state = 0;
        scanCppComment(originalKind);
        return;
    } else if (isRawStringLiteral(s._tokenKind)) {
        tok->f.kind = s._tokenKind;
        if (scanUntilRawStringLiteralEndSimple())
            _state = 0;
        return;
    } else { // non-raw strings
        tok->f.joined = true;
        tok->f.kind = s._tokenKind;
        _state = 0;
        scanUntilQuote(tok, '"');
        return;
    }

    if (! _yychar) {
        tok->f.kind = T_EOF_SYMBOL;
        return;
    }

    unsigned char ch = _yychar;
    yyinp();

    switch (ch) {
    case '\\':
        s._newlineExpected = true;
        goto again;

    case '"':
        scanStringLiteral(tok);
        break;

    case '\'':
        scanCharLiteral(tok);
        break;

    case '{':
        tok->f.kind = T_LBRACE;
        break;

    case '}':
        tok->f.kind = T_RBRACE;
        break;

    case '[':
        tok->f.kind = T_LBRACKET;
        break;

    case ']':
        tok->f.kind = T_RBRACKET;
        break;

    case '#':
        if (_yychar == '#') {
            tok->f.kind = T_POUND_POUND;
            yyinp();
        } else {
            tok->f.kind = T_POUND;
        }
        break;

    case '(':
        tok->f.kind = T_LPAREN;
        break;

    case ')':
        tok->f.kind = T_RPAREN;
        break;

    case ';':
        tok->f.kind = T_SEMICOLON;
        break;

    case ':':
        if (_yychar == ':') {
            yyinp();
            tok->f.kind = T_COLON_COLON;
        } else if (_yychar == '>') {
            yyinp();
            tok->f.kind = T_RBRACKET;
        } else {
            tok->f.kind = T_COLON;
        }
        break;

    case '.':
        if (_yychar == '*') {
            yyinp();
            tok->f.kind = T_DOT_STAR;
        } else if (_yychar == '.') {
            yyinp();
            // ### CPP_CHECK(_yychar);
            if (_yychar == '.') {
                yyinp();
                tok->f.kind = T_DOT_DOT_DOT;
            } else {
                tok->f.kind = T_ERROR;
            }
        } else if (std::isdigit(_yychar)) {
            if (f._ppMode) {
                scanPreprocessorNumber(tok, true);
                break;
            }

            const char *yytext = _currentChar - 2;
            yyinp();
            scanDigitSequence(); // this is optional: we already skipped over the first digit
            scanExponentPart();
            if (!scanOptionalFloatingSuffix())
                scanOptionalUserDefinedLiteral(tok);
            if (std::isalnum(_yychar) || _yychar == '_') {
                do {
                    yyinp();
                } while (std::isalnum(_yychar) || _yychar == '_');
                tok->f.kind = T_ERROR;
            } else {
                int yylen = _currentChar - yytext;
                tok->f.kind = T_NUMERIC_LITERAL;
                if (control())
                    tok->number = control()->numericLiteral(yytext, yylen);
            }
        } else {
            tok->f.kind = T_DOT;
        }
        break;

    case '?':
        if (_yychar == '?') {
            yyinp();
            if (_yychar == '(') {
                yyinp();
                tok->f.kind = T_LBRACKET;
            } else if (_yychar == ')') {
                yyinp();
                tok->f.kind = T_RBRACKET;
            } else if (_yychar == '<') {
                yyinp();
                tok->f.kind = T_LBRACE;
            } else if (_yychar == '>') {
                yyinp();
                tok->f.kind = T_RBRACE;
            }
        } else {
            tok->f.kind = T_QUESTION;
        }
        break;

    case '+':
        if (_yychar == '+') {
            yyinp();
            tok->f.kind = T_PLUS_PLUS;
        } else if (_yychar == '=') {
            yyinp();
            tok->f.kind = T_PLUS_EQUAL;
        } else {
            tok->f.kind = T_PLUS;
        }
        break;

    case '-':
        if (_yychar == '-') {
            yyinp();
            tok->f.kind = T_MINUS_MINUS;
        } else if (_yychar == '=') {
            yyinp();
            tok->f.kind = T_MINUS_EQUAL;
        } else if (_yychar == '>') {
            yyinp();
            if (_yychar == '*') {
                yyinp();
                tok->f.kind = T_ARROW_STAR;
            } else {
                tok->f.kind = T_ARROW;
            }
        } else {
            tok->f.kind = T_MINUS;
        }
        break;

    case '*':
        if (_yychar == '=') {
            yyinp();
            tok->f.kind = T_STAR_EQUAL;
        } else {
            tok->f.kind = T_STAR;
        }
        break;

    case '/':
        if (_yychar == '/') {
            yyinp();

            Kind commentType = T_CPP_COMMENT;

            if (_yychar == '/' || _yychar == '!') {
                yyinp();
                commentType = T_CPP_DOXY_COMMENT;
            }

            scanCppComment(commentType);

            if (! f._scanCommentTokens)
                goto again;

            tok->f.kind = commentType;

        } else if (_yychar == '*') {
            yyinp();

            Kind commentKind = T_COMMENT;

            if (_yychar == '*' || _yychar == '!') {
                const char ch = _yychar;

                yyinp();

                if (ch == '*' && _yychar == '/')
                    goto done;

                if (_yychar == '<')
                    yyinp();

                if (! _yychar || std::isspace(_yychar))
                    commentKind = T_DOXY_COMMENT;
            }

            while (_yychar) {
                if (_yychar != '*') {
                    yyinp();
                } else {
                    yyinp();
                    if (_yychar == '/')
                        break;
                }
            }

        done:
            if (_yychar)
                yyinp();
            else
                s._tokenKind = commentKind;

            if (! f._scanCommentTokens)
                goto again;

            tok->f.kind = commentKind;

        } else if (_yychar == '=') {
            yyinp();
            tok->f.kind = T_SLASH_EQUAL;
        } else {
            tok->f.kind = T_SLASH;
        }
        break;

    case '%':
        if (_yychar == '=') {
            yyinp();
            tok->f.kind = T_PERCENT_EQUAL;
        } else if (_yychar == '>') {
            yyinp();
            tok->f.kind = T_RBRACE;
        } else if (_yychar == ':') {
            yyinp();
            tok->f.kind = T_POUND;
        } else {
            tok->f.kind = T_PERCENT;
        }
        break;

    case '^':
        if (_yychar == '=') {
            yyinp();
            tok->f.kind = T_CARET_EQUAL;
        } else {
            tok->f.kind = T_CARET;
        }
        break;

    case '&':
        if (_yychar == '&') {
            yyinp();
            tok->f.kind = T_AMPER_AMPER;
        } else if (_yychar == '=') {
            yyinp();
            tok->f.kind = T_AMPER_EQUAL;
        } else {
            tok->f.kind = T_AMPER;
        }
        break;

    case '|':
        if (_yychar == '|') {
            yyinp();
            tok->f.kind = T_PIPE_PIPE;
        } else if (_yychar == '=') {
            yyinp();
            tok->f.kind = T_PIPE_EQUAL;
        } else {
            tok->f.kind = T_PIPE;
        }
        break;

    case '~':
        if (_yychar == '=') {
            yyinp();
            tok->f.kind = T_TILDE_EQUAL;
        } else {
            tok->f.kind = T_TILDE;
        }
        break;

    case '!':
        if (_yychar == '=') {
            yyinp();
            tok->f.kind = T_EXCLAIM_EQUAL;
        } else {
            tok->f.kind = T_EXCLAIM;
        }
        break;

    case '=':
        if (_yychar == '=') {
            yyinp();
            tok->f.kind = T_EQUAL_EQUAL;
        } else {
            tok->f.kind = T_EQUAL;
        }
        break;

    case '<':
        if (f._scanAngleStringLiteralTokens) {
            const char *yytext = _currentChar;
            while (_yychar && _yychar != '>')
                yyinp();
            int yylen = _currentChar - yytext;
            // ### CPP_CHECK(_yychar == '>');
            if (_yychar == '>')
                yyinp();
            if (control())
                tok->string = control()->stringLiteral(yytext, yylen);
            tok->f.kind = T_ANGLE_STRING_LITERAL;
        } else if (_yychar == '<') {
            yyinp();
            if (_yychar == '=') {
                yyinp();
                tok->f.kind = T_LESS_LESS_EQUAL;
            } else
                tok->f.kind = T_LESS_LESS;
        } else if (_yychar == '=') {
            yyinp();
            tok->f.kind = T_LESS_EQUAL;
        } else if (_yychar == ':') {
            yyinp();
            tok->f.kind = T_LBRACKET;
        } else if (_yychar == '%') {
            yyinp();
            tok->f.kind = T_LBRACE;
        } else {
            tok->f.kind = T_LESS;
        }
        break;

    case '>':
        if (_yychar == '>') {
            yyinp();
            if (_yychar == '=') {
                yyinp();
                tok->f.kind = T_GREATER_GREATER_EQUAL;
            } else
                tok->f.kind = T_LESS_LESS;
            tok->f.kind = T_GREATER_GREATER;
        } else if (_yychar == '=') {
            yyinp();
            tok->f.kind = T_GREATER_EQUAL;
        } else {
            tok->f.kind = T_GREATER;
        }
        break;

    case ',':
        tok->f.kind = T_COMMA;
        break;

    default: {
        if (_languageFeatures.objCEnabled) {
            if (ch == '@' && _yychar >= 'a' && _yychar <= 'z') {
                const char *yytext = _currentChar;

                do {
                    yyinp();
                    if (! (isalnum(_yychar) || _yychar == '_' || _yychar == '$'))
                        break;
                } while (_yychar);

                const int yylen = _currentChar - yytext;
                tok->f.kind = classifyObjCAtKeyword(yytext, yylen);
                break;
            } else if (ch == '@' && _yychar == '"') {
                yyinp();
                scanStringLiteral(tok, '"');
                break;
            }
        }

        if (ch == 'L' || ch == 'u' || ch == 'U' || ch == 'R') {
            // Either a literal or still an identifier.
            if (_yychar == '"') {
                yyinp();
                if (ch == 'R')
                    scanRawStringLiteral(tok);
                else
                    scanStringLiteral(tok, ch);
            } else if (_yychar == '\'') {
                yyinp();
                scanCharLiteral(tok, ch);
            } else if (ch != 'R' && _yychar == 'R') {
                yyinp();
                if (_yychar == '"') {
                    yyinp();
                    scanRawStringLiteral(tok, ch);
                } else {
                    scanIdentifier(tok, 1);
                }
            } else if (ch == 'u' && _yychar == '8') {
                yyinp();
                if (_yychar == '"') {
                    yyinp();
                    scanStringLiteral(tok, '8');
                } else if (_yychar == '\'') {
                    yyinp();
                    scanCharLiteral(tok, '8');
                } else if (_yychar == 'R') {
                    yyinp();
                    if (_yychar == '"') {
                        yyinp();
                        scanRawStringLiteral(tok, '8');
                    } else {
                        scanIdentifier(tok, 2);
                    }
                } else {
                    scanIdentifier(tok, 1);
                }
            } else {
                scanIdentifier(tok);
            }
        } else if (std::isalpha(ch) || ch == '_' || ch == '$' || isByteOfMultiByteCodePoint(ch)) {
            scanIdentifier(tok, _currentChar - _tokenStart - 1);
        } else if (std::isdigit(ch)) {
            if (f._ppMode)
                scanPreprocessorNumber(tok, false);
            else
                scanNumericLiteral(tok);
        } else {
            tok->f.kind = T_ERROR;
        }
        break;
    } // default

    } // switch
}

void Lexer::scanStringLiteral(Token *tok, unsigned char hint)
{
    if (hint == 'L')
        tok->f.kind = T_WIDE_STRING_LITERAL;
    else if (hint == 'U')
        tok->f.kind = T_UTF32_STRING_LITERAL;
    else if (hint == 'u')
        tok->f.kind = T_UTF16_STRING_LITERAL;
    else if (hint == '8')
        tok->f.kind = T_UTF8_STRING_LITERAL;
    else if (hint == '@')
        tok->f.kind = T_AT_STRING_LITERAL;
    else
        tok->f.kind = T_STRING_LITERAL;

    scanUntilQuote(tok, '"');
    scanOptionalUserDefinedLiteral(tok);
}

void Lexer::scanRawStringLiteral(Token *tok, unsigned char hint)
{
    const char *yytext = _currentChar;

    int delimLength = -1;
    const char *closingDelimCandidate = 0;
    while (_yychar) {
        if (_yychar == '(' && delimLength == -1) {
            delimLength = _currentChar - yytext;
            yyinp();
        } else if (_yychar == ')') {
            yyinp();
            if (delimLength == -1)
                break;
            closingDelimCandidate = _currentChar;
        } else {
            if (delimLength == -1) {
                if (_yychar == '\\' || std::isspace(_yychar))
                    break;
                yyinp();
            } else {
                if (!closingDelimCandidate) {
                    yyinp();
                } else {
                    if (_yychar == '"') {
                        if (delimLength == _currentChar - closingDelimCandidate) {
                            // Got a matching closing delimiter.
                            break;
                        }
                    }

                    // Make sure this continues to be a valid candidate.
                    if (_yychar != *(yytext + (_currentChar - closingDelimCandidate)))
                        closingDelimCandidate = 0;

                    yyinp();
                }
            }
        }
    }

    int yylen = _currentChar - yytext;

    if (_yychar == '"')
        yyinp();

    if (control())
        tok->string = control()->stringLiteral(yytext, yylen);

    if (hint == 'L')
        tok->f.kind = T_RAW_WIDE_STRING_LITERAL;
    else if (hint == 'U')
        tok->f.kind = T_RAW_UTF32_STRING_LITERAL;
    else if (hint == 'u')
        tok->f.kind = T_RAW_UTF16_STRING_LITERAL;
    else if (hint == '8')
        tok->f.kind = T_RAW_UTF8_STRING_LITERAL;
    else
        tok->f.kind = T_RAW_STRING_LITERAL;

    if (!_yychar)
        s._tokenKind = tok->f.kind;
}

// In the highlighting case we don't have any further information
// like the delimiter or its length, so just match for: ...)..."
bool Lexer::scanUntilRawStringLiteralEndSimple()
{
    bool closingParenthesisPassed = false;

    while (_yychar) {
        if (_yychar == ')') {
            yyinp();
            closingParenthesisPassed = true;
        } else {
            if (closingParenthesisPassed && _yychar == '"') {
                yyinp();
                return true;
            } else {
                yyinp();
            }
        }
    }

    return false;
}

void Lexer::scanCharLiteral(Token *tok, unsigned char hint)
{
    if (hint == 'L')
        tok->f.kind = T_WIDE_CHAR_LITERAL;
    else if (hint == 'U')
        tok->f.kind = T_UTF32_CHAR_LITERAL;
    else if (hint == 'u')
        tok->f.kind = T_UTF16_CHAR_LITERAL;
    else
        tok->f.kind = T_CHAR_LITERAL;

    scanUntilQuote(tok, '\'');
    scanOptionalUserDefinedLiteral(tok);
}

void Lexer::scanUntilQuote(Token *tok, unsigned char quote)
{
    CPP_CHECK(quote == '"' || quote == '\'');

    const char *yytext = _currentChar;
    while (_yychar
           && _yychar != quote
           && _yychar != '\n') {
        if (_yychar == '\\')
            scanBackslash((Kind)tok->f.kind);
        else
            yyinp();
    }
    int yylen = _currentChar - yytext;

    if (_yychar == quote)
        yyinp();

    if (control())
        tok->string = control()->stringLiteral(yytext, yylen);
}

bool Lexer::scanDigitSequence()
{
    if (!std::isdigit(_yychar))
        return false;
    yyinp();
    while (std::isdigit(_yychar))
        yyinp();
    return true;
}

bool Lexer::scanExponentPart()
{
    if (_yychar != 'e' && _yychar != 'E')
        return false;
    yyinp();
    if (_yychar == '+' || _yychar == '-')
        yyinp();
    return scanDigitSequence();
}

bool Lexer::scanOptionalFloatingSuffix()
{
    if (_yychar == 'f' || _yychar == 'l' || _yychar == 'F' || _yychar == 'L') {
        yyinp();
        return true;
    }
    return false;
}

bool Lexer::scanOptionalIntegerSuffix(bool allowU)
{
    switch(_yychar) {
    case 'u':
    case 'U':
        if (allowU) {
            yyinp();
            scanOptionalIntegerSuffix(false);
        }
        return true;
    case 'l':
        yyinp();
        if (_yychar == 'l')
            yyinp();
        return true;
    case 'L':
        yyinp();
        if (_yychar == 'L')
            yyinp();
        return true;
    default:
        return false;
    }
}

void Lexer::scanOptionalUserDefinedLiteral(Token *tok)
{
    if (_languageFeatures.cxx11Enabled && _yychar == '_') {
        tok->f.userDefinedLiteral = true;
        while (std::isalnum(_yychar) || _yychar == '_' || isByteOfMultiByteCodePoint(_yychar))
            yyinp();
    }
}

void Lexer::scanNumericLiteral(Token *tok)
{
    const char *yytext = _currentChar - 1;
    if (*yytext == '0' && _yychar) {
        if (_yychar == 'x' || _yychar == 'X') {
            yyinp();
            while (std::isdigit(_yychar) ||
                   (_yychar >= 'a' && _yychar <= 'f') ||
                   (_yychar >= 'A' && _yychar <= 'F')) {
                yyinp();
            }
            if (!scanOptionalIntegerSuffix())
                scanOptionalUserDefinedLiteral(tok);
            goto theEnd;
        } else if (_yychar == 'b' || _yychar == 'B') { // see n3472
            yyinp();
            while (_yychar == '0' || _yychar == '1')
                yyinp();
            if (!scanOptionalIntegerSuffix())
                scanOptionalUserDefinedLiteral(tok);
            goto theEnd;
        } else if (_yychar >= '0' && _yychar <= '7') {
            do {
                yyinp();
            } while (_yychar >= '0' && _yychar <= '7');
            if (!scanOptionalIntegerSuffix())
                scanOptionalUserDefinedLiteral(tok);
            goto theEnd;
        }
    }

    while (_yychar) {
        if (_yychar == '.') {
            yyinp();
            scanDigitSequence(); // this is optional: "1." is a valid floating point number
            scanExponentPart();
            if (!scanOptionalFloatingSuffix())
                scanOptionalUserDefinedLiteral(tok);
            break;
        } else if (_yychar == 'e' || _yychar == 'E') {
            if (scanExponentPart() && !scanOptionalFloatingSuffix())
                scanOptionalUserDefinedLiteral(tok);
            break;
        } else if (std::isdigit(_yychar)) {
            yyinp();
        } else {
            if (!scanOptionalIntegerSuffix())
                scanOptionalUserDefinedLiteral(tok);
            break;
        }
    }

theEnd:
    if (std::isalnum(_yychar) || _yychar == '_') {
        do {
            yyinp();
        } while (std::isalnum(_yychar) || _yychar == '_');
        tok->f.kind = T_ERROR;
    } else {
        int yylen = _currentChar - yytext;
        tok->f.kind = T_NUMERIC_LITERAL;
        if (control())
            tok->number = control()->numericLiteral(yytext, yylen);
    }
}

void Lexer::scanPreprocessorNumber(Token *tok, bool dotAlreadySkipped)
{
    const char *yytext = _currentChar - (dotAlreadySkipped ? 2 : 1);
    if (dotAlreadySkipped &&
            (!_yychar || (_yychar && !std::isdigit(_yychar)))) {
        tok->f.kind = T_DOT;
        return;
    }

    while (_yychar) {
        if (_yychar == 'e' || _yychar == 'E') {
            yyinp();
            if (_yychar == '+' || _yychar == '-')
                yyinp();
        } else if (std::isalnum(_yychar) || _yychar == '_' || _yychar == '.') {
            yyinp();
        } else {
            scanOptionalUserDefinedLiteral(tok);
            break;
        }
    }

    int yylen = _currentChar - yytext;
    tok->f.kind = T_NUMERIC_LITERAL;
    if (control())
        tok->number = control()->numericLiteral(yytext, yylen);
}

void Lexer::scanIdentifier(Token *tok, unsigned extraProcessedChars)
{
    const char *yytext = _currentChar - 1 - extraProcessedChars;
    while (std::isalnum(_yychar) || _yychar == '_' || _yychar == '$'
            || isByteOfMultiByteCodePoint(_yychar)) {
        yyinp();
    }
    int yylen = _currentChar - yytext;
    if (f._scanKeywords)
        tok->f.kind = classify(yytext, yylen, _languageFeatures);
    else
        tok->f.kind = T_IDENTIFIER;

    if (tok->f.kind == T_IDENTIFIER) {
        tok->f.kind = classifyOperator(yytext, yylen);

        if (control())
            tok->identifier = control()->identifier(yytext, yylen);
    }
}

void Lexer::scanBackslash(Kind type)
{
    yyinp(); // skip '\\'
    if (_yychar && !std::isspace(_yychar)) {
        yyinp();
        return;
    }
    while (_yychar != '\n' && std::isspace(_yychar))
        yyinp();
    if (!_yychar) {
        s._tokenKind = type;
        s._newlineExpected = true;
        return;
    }
    if (_yychar == '\n') {
        yyinp();
        while (_yychar != '\n' && std::isspace(_yychar))
            yyinp();
        if (!_yychar)
            s._tokenKind = type;
    }
}

void Lexer::scanCppComment(Kind type)
{
    while (_yychar && _yychar != '\n') {
        if (_yychar == '\\')
            scanBackslash(type);
        else if (_yychar)
            yyinp();
    }
}
