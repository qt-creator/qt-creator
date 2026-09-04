// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakelexer.h"

#include "cmakeengine.h"
#include "cmakeparser.h"

using namespace CMakeLang;

enum {
    Rule_BracketOpen,
    Rule_Hash,
    Rule_ParenLeft,
    Rule_ParenRight,
    Rule_Identifier,
    Rule_Unquoted,
    Rule_Legacy,
    Rule_LeftBracket,
    Rule_Quote,
    Rule_Space,
    Rule_AnyCharacter
};

Lexer::Lexer(Engine *engine, QStringView source)
    : _engine(engine)
    , _source(source)
    , _size(int(source.size()))
{}

const char *Lexer::name(int kind)
{
    switch (kind) {
    case Parser::EOF_SYMBOL:
        return "nothing";
    case Parser::T_SPACE:
        return "space";
    case Parser::T_NEWLINE:
        return "newline";
    case Parser::T_IDENTIFIER:
        return "identifier";
    case Parser::T_LEFT_PAREN:
        return "left paren";
    case Parser::T_RIGHT_PAREN:
        return "right paren";
    case Parser::T_UNQUOTED_ARGUMENT:
        return "unquoted argument";
    case Parser::T_QUOTED_ARGUMENT:
        return "quoted argument";
    case Parser::T_BRACKET_ARGUMENT:
        return "bracket argument";
    case Parser::T_BRACKET_COMMENT:
        return "bracket comment";
    case Parser::T_COMMENT:
        return "comment";
    case Parser::T_ERROR:
        return "bad character";
    case Parser::T_UNTERMINATED_BRACKET:
        return "unterminated bracket";
    case Parser::T_UNTERMINATED_STRING:
        return "unterminated string";
    default:
        return "unknown token";
    }
}

void Lexer::setToken(Token *tk, int kind, int position) const
{
    tk->kind = kind;
    tk->position = position;
    tk->length = 0;
    tk->line = _line;
    tk->column = _column;
    tk->separation = _separation;
    tk->value = nullptr;
}

int Lexer::finish(Token *tk, int kind)
{
    tk->kind = kind;
    tk->length = _pos - tk->position;
    tk->spelling = _source.mid(tk->position, tk->length);

    switch (kind) {
    case Parser::T_SPACE:
    case Parser::T_NEWLINE:
        _separation = Token::SeparationOkay;
        break;
    case Parser::T_LEFT_PAREN:
        _separation = Token::SeparationOkay;
        break;
    case Parser::T_BRACKET_ARGUMENT:
    case Parser::T_BRACKET_COMMENT:
        _separation = Token::SeparationError;
        break;
    case Parser::T_COMMENT:
        break;
    default:
        _separation = Token::SeparationWarning;
        break;
    }
    return kind;
}

int Lexer::matchUnquotedChar(int pos) const
{
    const QChar c = at(pos);
    if (c == u'\\') {
        if (pos + 1 >= _size)
            return 0;
        const QChar n = at(pos + 1);
        if (n == u'\0' || n == u'\n')
            return 0;
        return 2;
    }
    if (pos >= _size)
        return 0;
    switch (c.unicode()) {
    case u' ':
    case u'\0':
    case u'\t':
    case u'\r':
    case u'\n':
    case u'(':
    case u')':
    case u'#':
    case u'"':
    case u'[':
    case u'=':
        return 0;
    default:
        return 1;
    }
}

int Lexer::matchMakeVar(int pos) const
{
    if (at(pos) != u'$' || at(pos + 1) != u'(')
        return 0;
    int p = pos + 2;
    while (p < _size) {
        const QChar c = at(p);
        if (c.isLetterOrNumber() && c.unicode() < 128)
            ++p;
        else if (c == u'_')
            ++p;
        else
            break;
    }
    if (at(p) != u')')
        return 0;
    return p + 1 - pos;
}

int Lexer::matchLegacyElement(int pos) const
{
    int best = matchMakeVar(pos);
    best = qMax(best, matchUnquotedChar(pos));

    if (at(pos) == u'"' && pos < _size) {
        int p = pos + 1;
        for (;;) {
            if (const int m = matchMakeVar(p)) {
                p += m;
                continue;
            }
            if (const int m = matchUnquotedChar(p)) {
                p += m;
                continue;
            }
            const QChar c = at(p);
            if (p < _size && (c == u' ' || c == u'\t' || c == u'[' || c == u'='))
                ++p;
            else
                break;
        }
        if (p < _size && at(p) == u'"')
            best = qMax(best, p + 1 - pos);
    }
    return best;
}

int Lexer::matchIdentifier(int pos) const
{
    const QChar c = at(pos);
    const bool start = pos < _size
                       && (c == u'_' || (c >= u'A' && c <= u'Z') || (c >= u'a' && c <= u'z'));
    if (!start)
        return 0;
    int p = pos + 1;
    while (p < _size) {
        const QChar n = at(p);
        if (n == u'_' || (n >= u'A' && n <= u'Z') || (n >= u'a' && n <= u'z')
            || (n >= u'0' && n <= u'9')) {
            ++p;
        } else {
            break;
        }
    }
    return p - pos;
}

int Lexer::matchUnquoted(int pos) const
{
    int p = pos;
    if (const int n = matchUnquotedChar(p)) {
        p += n;
    } else if (p < _size && at(p) == u'=') {
        ++p;
    } else if (p < _size && at(p) == u'[') {
        int q = p + 1;
        while (q < _size && at(q) == u'=')
            ++q;
        const int m = matchUnquotedChar(q);
        if (!m)
            return 0;
        p = q + m;
    } else {
        return 0;
    }

    for (;;) {
        if (const int m = matchUnquotedChar(p)) {
            p += m;
            continue;
        }
        const QChar c = at(p);
        if (p < _size && (c == u'[' || c == u'='))
            ++p;
        else
            break;
    }
    return p - pos;
}

// The legacy rule differs from the plain unquoted rule only by also accepting
// $(MAKEVAR) and "quoted" elements.  Neither can occur inside what the plain
// rule already consumed, so the legacy rule can only win where the plain rule
// stopped: on a quote, or on the "(" of a $(...) reference.
bool Lexer::canMatchLegacy(int pos, int unquotedLength) const
{
    if (unquotedLength == 0)
        return true;
    const QChar stop = at(pos + unquotedLength);
    return stop == u'"' || (stop == u'(' && at(pos + unquotedLength - 1) == u'$');
}

int Lexer::matchLegacy(int pos) const
{
    int p = pos;
    int n = matchMakeVar(p);
    if (!n)
        n = matchUnquotedChar(p);

    if (n) {
        p += n;
    } else if (p < _size && at(p) == u'=') {
        ++p;
    } else if (p < _size && at(p) == u'[') {
        int q = p + 1;
        while (q < _size && at(q) == u'=')
            ++q;
        const int m = matchLegacyElement(q);
        if (!m)
            return 0;
        p = q + m;
    } else {
        return 0;
    }

    for (;;) {
        if (const int m = matchLegacyElement(p)) {
            p += m;
            continue;
        }
        const QChar c = at(p);
        if (p < _size && (c == u'[' || c == u'='))
            ++p;
        else
            break;
    }
    return p - pos;
}

int Lexer::matchBracketOpen(int pos) const
{
    int p = pos;
    if (at(p) == u'#')
        ++p;
    if (at(p) != u'[')
        return 0;
    ++p;
    while (at(p) == u'=')
        ++p;
    if (at(p) != u'[')
        return 0;
    ++p;
    if (p < _size && at(p) == u'\n')
        ++p;
    return p - pos;
}

int Lexer::matchSpace(int pos) const
{
    int p = pos;
    while (p < _size) {
        const QChar c = at(p);
        if (c == u' ' || c == u'\t' || c == u'\r')
            ++p;
        else
            break;
    }
    return p - pos;
}

int Lexer::yylex(Token *tk)
{
    for (;;) {
        if (_pos >= _size) {
            setToken(tk, Parser::EOF_SYMBOL, _pos);
            return Parser::EOF_SYMBOL;
        }

        setToken(tk, Parser::T_ERROR, _pos);

        if (at(_pos) == u'\n') {
            ++_pos;
            ++_line;
            _column = 1;
            return finish(tk, Parser::T_NEWLINE);
        }

        int rule = Rule_AnyCharacter;
        int length = 1;
        const auto consider = [&rule, &length](int candidate, int candidateLength) {
            if (candidateLength > length) {
                length = candidateLength;
                rule = candidate;
            }
        };

        // The rules are considered in the order they appear in CMake's
        // cmListFileLexer.in.l, and consider() replaces only on a strictly
        // longer match.  The first longest match therefore wins, which is what
        // flex's earliest-rule-wins does.
        const QChar c = at(_pos);
        length = 0;
        consider(Rule_BracketOpen, matchBracketOpen(_pos));
        if (c == u'#')
            consider(Rule_Hash, 1);
        if (c == u'(')
            consider(Rule_ParenLeft, 1);
        if (c == u')')
            consider(Rule_ParenRight, 1);
        consider(Rule_Identifier, matchIdentifier(_pos));
        const int unquoted = matchUnquoted(_pos);
        consider(Rule_Unquoted, unquoted);
        if (canMatchLegacy(_pos, unquoted))
            consider(Rule_Legacy, matchLegacy(_pos));
        if (c == u'[')
            consider(Rule_LeftBracket, 1);
        if (c == u'"')
            consider(Rule_Quote, 1);
        consider(Rule_Space, matchSpace(_pos));
        consider(Rule_AnyCharacter, 1);

        switch (rule) {
        case Rule_BracketOpen:
            return scanBracketArgument(tk, length);

        case Rule_Hash: {
            int p = _pos + 1;
            while (p < _size && at(p) != u'\n' && at(p) != u'\0')
                ++p;
            _column += p - _pos;
            _pos = p;
            if (_scanComments)
                return finish(tk, Parser::T_COMMENT);
            continue;
        }

        case Rule_Quote:
            return scanQuotedArgument(tk);

        case Rule_ParenLeft:
            _pos += length;
            _column += length;
            return finish(tk, Parser::T_LEFT_PAREN);

        case Rule_ParenRight:
            _pos += length;
            _column += length;
            return finish(tk, Parser::T_RIGHT_PAREN);

        case Rule_Space:
            _pos += length;
            _column += length;
            return finish(tk, Parser::T_SPACE);

        case Rule_Identifier:
            _pos += length;
            _column += length;
            return finish(tk, Parser::T_IDENTIFIER);

        case Rule_Unquoted:
        case Rule_Legacy:
        case Rule_LeftBracket:
            _pos += length;
            _column += length;
            return finish(tk, Parser::T_UNQUOTED_ARGUMENT);

        default:
            _pos += 1;
            _column += 1;
            return finish(tk, Parser::T_ERROR);
        }
    }
}

int Lexer::scanQuotedArgument(Token *tk)
{
    _buffer.clear();
    ++_pos;
    ++_column;

    for (;;) {
        if (_pos >= _size) {
            tk->value = _engine->string(_buffer);
            return finish(tk, Parser::T_UNTERMINATED_STRING);
        }

        const QChar c = at(_pos);
        if (c == u'"') {
            ++_pos;
            ++_column;
            break;
        }
        if (c == u'\\' && at(_pos + 1) == u'\n' && _pos + 1 < _size) {
            _pos += 2;
            ++_line;
            _column = 1;
            continue;
        }
        if (c == u'\n') {
            _buffer += c;
            ++_pos;
            ++_line;
            _column = 1;
            continue;
        }

        int n = 0;
        while (_pos + n < _size) {
            const QChar ch = at(_pos + n);
            if (ch == u'\\') {
                const QChar next = at(_pos + n + 1);
                if (_pos + n + 1 >= _size || next == u'\0' || next == u'\n')
                    break;
                n += 2;
            } else if (ch == u'\0' || ch == u'\n' || ch == u'"') {
                break;
            } else {
                ++n;
            }
        }
        if (n == 0)
            n = 1;

        _buffer += _source.mid(_pos, n);
        _pos += n;
        _column += n;
    }

    tk->value = _engine->string(_buffer);
    return finish(tk, Parser::T_QUOTED_ARGUMENT);
}

int Lexer::scanBracketArgument(Token *tk, int openLength)
{
    const bool isComment = at(_pos) == u'#';

    int p = _pos + (isComment ? 1 : 0) + 1;
    int equalSigns = 0;
    while (at(p) == u'=') {
        ++equalSigns;
        ++p;
    }
    _bracket = equalSigns + 1;

    const bool openEndsWithNewline = at(_pos + openLength - 1) == u'\n';
    _pos += openLength;
    if (openEndsWithNewline) {
        ++_line;
        _column = 1;
    } else {
        _column += openLength;
    }

    _buffer.clear();
    bool atBracketEnd = false;

    for (;;) {
        if (_pos >= _size) {
            tk->value = _engine->string(_buffer);
            return finish(tk, Parser::T_UNTERMINATED_BRACKET);
        }

        const QChar c = at(_pos);

        if (c == u']') {
            if (atBracketEnd) {
                ++_pos;
                ++_column;
                _buffer.chop(_bracket);
                break;
            }
            int n = 1;
            while (at(_pos + n) == u'=')
                ++n;
            _buffer += _source.mid(_pos, n);
            _pos += n;
            _column += n;
            if (n == _bracket)
                atBracketEnd = true;
            continue;
        }

        if (c == u'\n') {
            _buffer += c;
            ++_pos;
            ++_line;
            _column = 1;
            atBracketEnd = false;
            continue;
        }

        if (!atBracketEnd) {
            int n = 0;
            while (_pos + n < _size) {
                const QChar ch = at(_pos + n);
                if (ch == u']' || ch == u'\0' || ch == u'\n')
                    break;
                ++n;
            }
            if (n > 0) {
                _buffer += _source.mid(_pos, n);
                _pos += n;
                _column += n;
                continue;
            }
        }

        _buffer += c;
        ++_pos;
        ++_column;
        atBracketEnd = false;
    }

    tk->value = _engine->string(_buffer);
    return finish(tk, isComment ? Parser::T_BRACKET_COMMENT : Parser::T_BRACKET_ARGUMENT);
}
