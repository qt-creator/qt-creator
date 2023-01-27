// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "haskelltokenizer.h"

#include <QSet>

#include <algorithm>
#include <functional>

Q_GLOBAL_STATIC_WITH_ARGS(QSet<QString>, RESERVED_OP, ({
    "..",
    ":",
    "::",
    "=",
    "\\",
    "|",
    "<-",
    "->",
    "@",
    "~",
    "=>",

    // Arrows GHC extension
    "-<",
    "-<<",
    ">-",
    ">>-",
    "(|",
    "|)"
}));

Q_GLOBAL_STATIC_WITH_ARGS(QSet<QString>, RESERVED_ID, ({
    "case",
    "class",
    "data",
    "default",
    "deriving",
    "do",
    "else",
    "foreign",
    "if",
    "import",
    "in",
    "infix",
    "infixl",
    "infixr",
    "instance",
    "let",
    "module",
    "newtype",
    "of",
    "then",
    "type",
    "where",
    "_",

    // from GHC extensions
    "family",
    "forall",
    "mdo",
    "proc",
    "rec"
}));

Q_GLOBAL_STATIC_WITH_ARGS(QSet<QChar>, SPECIAL, ({
    '(',
    ')',
    ',',
    ';',
    '[',
    ']',
    '`',
    '{',
    '}',
}));

Q_GLOBAL_STATIC_WITH_ARGS(QSet<QChar>, CHAR_ESCAPES,
                          ({'a', 'b', 'f', 'n', 'r', 't', 'v', '\\', '"', '\'', '&'}));

Q_GLOBAL_STATIC_WITH_ARGS(QVector<QString>, ASCII_ESCAPES, ({
    "NUL",
    "SOH", // must be before "SO" to match
    "STX",
    "ETX",
    "EOT",
    "ENQ",
    "ACK",
    "BEL",
    "BS",
    "HT",
    "LF",
    "VT",
    "FF",
    "CR",
    "SO",
    "SI",
    "DLE",
    "DC1",
    "DC2",
    "DC3",
    "DC4",
    "NAK",
    "SYN",
    "ETB",
    "CAN",
    "EM",
    "SUB",
    "ESC",
    "FS",
    "GS",
    "RS",
    "US",
    "SP",
    "DEL"
}));

namespace Haskell {
namespace Internal {

Token token(TokenType type, std::shared_ptr<QString> line, int start, int end)
{
    return {type, start, end - start, QStringView(*line).mid(start, end - start), line};
}

Tokens::Tokens(std::shared_ptr<QString> source)
    : source(source)
{
}

Token Tokens::tokenAtColumn(int col) const
{
    auto it = std::upper_bound(begin(), end(), col, [](int c, const Token &i) {
        return c < i.startCol;
    });
    if (it == begin())
        return Token();
    --it;
    if (it->startCol + it->length > col)
        return *it;
    return Token();
}

static int grab(const QString &line, int begin,
                const std::function<bool(const QChar&)> &test)
{
    const int length = line.length();
    int current = begin;
    while (current < length && test(line.at(current)))
        ++current;
    return current - begin;
};

static bool isIdentifierChar(const QChar &c)
{
    return c.isLetterOrNumber() || c == '\'' || c == '_';
}

static bool isVariableIdentifierStart(const QChar &c)
{
    return c == '_' || c.isLower();
}

static bool isAscSymbol(const QChar &c)
{
    return c == '!'
            || c == '#'
            || c == '$'
            || c == '%'
            || c == '&'
            || c == '*'
            || c == '+'
            || c == '.'
            || c == '/'
            || c == '<'
            || c == '='
            || c == '>'
            || c == '?'
            || c == '@'
            || c == '\\'
            || c == '^'
            || c == '|'
            || c == '-'
            || c == '~'
            || c == ':';
}

static bool isSymbol(const QChar &c)
{
    return isAscSymbol(c)
            || ((c.isSymbol() || c.isPunct()) && c != '_' && c != '"' && c != '\''
                                              && !SPECIAL->contains(c));
}

static bool isDigit(const QChar &c)
{
    return c.isDigit();
}

static bool isOctit(const QChar &c)
{
    return c >= '0' && c <= '7';
}

static bool isHexit(const QChar &c)
{
    return c.isDigit()
            || (c >= 'A' && c <= 'F')
            || (c >= 'a' && c <= 'f');
}

static bool isCntrl(const QChar &c)
{
    return (c >= 'A' && c <= 'Z')
            || c == '@'
            || c == '['
            || c == '\\'
            || c == ']'
            || c == '^'
            || c == '_';
}

static QVector<Token> getSpace(std::shared_ptr<QString> line, int start)
{
    const auto lineEnd = line->cend();
    const auto tokenStart = line->cbegin() + start;
    auto current = tokenStart;
    while (current != lineEnd && (*current).isSpace())
        ++current;
    const int length = int(std::distance(tokenStart, current));
    if (current > tokenStart)
        return {{TokenType::Whitespace, start, length, QStringView(*line).mid(start, length), line}};
    return {};
}

static QVector<Token> getNumber(std::shared_ptr<QString> line, int start)
{
    const QChar &startC = line->at(start);
    if (!startC.isDigit())
        return {};
    const int length = line->length();
    int current = start + 1;
    TokenType type = TokenType::Integer;
    if (current < length) {
        if (startC == '0') {
            // check for octal or hexadecimal
            const QChar &secondC = line->at(current);
            if (secondC == 'o' || secondC == 'O') {
                const int numLen = grab(*line, current + 1, isOctit);
                if (numLen > 0)
                    return {token(TokenType::Integer, line, start, current + numLen + 1)};
            } else if (secondC == 'x' || secondC == 'X') {
                const int numLen = grab(*line, current + 1, isHexit);
                if (numLen > 0)
                    return {token(TokenType::Integer, line, start, current + numLen + 1)};
            }
        }
        // starts with decimal
        const int numLen = grab(*line, start, isDigit);
        current = start + numLen;
        // check for floating point
        if (current < length && line->at(current) == '.') {
            const int numLen = grab(*line, current + 1, isDigit);
            if (numLen > 0) {
                current += numLen + 1;
                type = TokenType::Float;
            }
        }
        // check for exponent
        if (current + 1 < length /*for at least 'e' and digit*/
                && (line->at(current) == 'e' || line->at(current) == 'E')) {
            int expEnd = current + 1;
            if (line->at(expEnd) == '+' || line->at(expEnd) == '-')
                ++expEnd;
            const int numLen = grab(*line, expEnd, isDigit);
            if (numLen > 0) {
                current = expEnd + numLen;
                type = TokenType::Float;
            }
        }
    }
    return {token(type, line, start, current)};
}

static QVector<Token> getIdOrOpOrSingleLineComment(std::shared_ptr<QString> line, int start)
{
    const int length = line->length();
    if (start >= length)
        return {};
    int current = start;
    // check for {conid.}conid
    int conidEnd = start;
    bool canOnlyBeConstructor = false;
    while (current < length && line->at(current).isUpper()) {
        current += grab(*line, current, isIdentifierChar);
        conidEnd = current;
        // it is definitely a constructor id if it is not followed by a '.'
        canOnlyBeConstructor = current >= length || line->at(current) != '.';
        // otherwise it might be a module id, and we skip the dot to check for qualified thing
        if (!canOnlyBeConstructor)
            ++current;
    }
    if (canOnlyBeConstructor)
        return {token(TokenType::Constructor, line, start, conidEnd)};

    // check for variable or reserved id
    if (current < length && isVariableIdentifierStart(line->at(current))) {
        const int varLen = grab(*line, current, isIdentifierChar);
        // check for reserved id
        if (RESERVED_ID->contains(line->mid(current, varLen))) {
            QVector<Token> result;
            // possibly add constructor + op '.'
            if (conidEnd > start) {
                result.append(token(TokenType::Constructor, line, start, conidEnd));
                result.append(token(TokenType::Operator, line, conidEnd, current));
            }
            result.append(token(TokenType::Keyword, line, current, current + varLen));
            return result;
        }
        return {token(TokenType::Variable, line, start, current + varLen)};
    }
    // check for operator
    if (current < length && isSymbol(line->at(current))) {
        const int opLen = grab(*line, current, isSymbol);
        // check for reserved op
        if (RESERVED_OP->contains(line->mid(current, opLen))) {
            // because of the case of F... (constructor + op '...') etc
            // we only add conid if we have one, handling the rest in next iteration
            if (conidEnd > start)
                return {token(TokenType::Constructor, line, start, conidEnd)};
            return {token(TokenType::Keyword, line, start, current + opLen)};
        }
        // check for single line comment
        if (opLen >= 2 && std::all_of(line->begin() + current, line->begin() + current + opLen,
                                      [](const QChar c) { return c == '-'; })) {
            QVector<Token> result;
            // possibly add constructor + op '.'
            if (conidEnd > start) {
                result.append(token(TokenType::Constructor, line, start, conidEnd));
                result.append(token(TokenType::Operator, line, conidEnd, current));
            }
            // rest is comment
            result.append(token(TokenType::SingleLineComment, line, current, length));
            return result;
        }
        // check for (qualified?) operator constructor
        if (line->at(current) == ':')
            return {token(TokenType::OperatorConstructor, line, start, current + opLen)};
        return {token(TokenType::Operator, line, start, current + opLen)};
    }
    // Foo.Blah.
    if (conidEnd > start)
        return {token(TokenType::Constructor, line, start, conidEnd)};
    return {};
}

static int getEscape(const QString &line, int start)
{
    if (CHAR_ESCAPES->contains(line.at(start)))
        return 1;

    // decimal
    if (line.at(start).isDigit())
        return grab(line, start + 1, isDigit) + 1;
    // octal
    if (line.at(start) == 'o') {
        const int count = grab(line, start + 1, isOctit);
        if (count < 1) // no octal number after 'o'
            return 0;
        return count + 1;
    }
    // hexadecimal
    if (line.at(start) == 'x') {
        const int count = grab(line, start + 1, isHexit);
        if (count < 1) // no octal number after 'o'
            return 0;
        return count + 1;
    }
    // ascii cntrl
    if (line.at(start) == '^') {
        const int count = grab(line, start + 1, isCntrl);
        if (count < 1) // no octal number after 'o'
            return 0;
        return count + 1;
    }
    const QStringView s = QStringView(line).mid(start);
    for (const QString &esc : *ASCII_ESCAPES) {
        if (s.startsWith(esc))
            return esc.length();
    }
    return 0;
}

static QVector<Token> getString(std::shared_ptr<QString> line, int start, bool *inStringGap/*in-out*/)
{
    // Haskell has the specialty of using \<whitespace>\ within strings for multiline strings
    const int length = line->length();
    if (start >= length)
        return {};
    QVector<Token> result;
    int tokenStart = start;
    int current = tokenStart;
    bool inString = *inStringGap;
    do {
        const QChar c = line->at(current);
        if (*inStringGap && !c.isSpace() && c != '\\') {
            // invalid non-whitespace in string gap
            // add previous string as token, this is at least a whitespace
            result.append(token(TokenType::String, line, tokenStart, current));
            // then add wrong non-whitespace
            tokenStart = current;
            do { ++current; } while (current < length && !line->at(current).isSpace());
            result.append(token(TokenType::StringError, line, tokenStart, current));
            tokenStart = current;
        } else if (c == '"') {
            inString = !inString;
            ++current;
        } else if (inString) {
            if (c == '\\') {
                ++current;
                if (*inStringGap) {
                    // ending string gap
                    *inStringGap = false;
                } else if (current >= length || line->at(current).isSpace()) {
                    // starting string gap
                    *inStringGap = true;
                    current = std::min(current + 1, length);
                } else { // there is at least one character after current
                    const int escapeLength = getEscape(*line, current);
                    if (escapeLength > 0) {
                        // valid escape
                        // add previous string as token without backslash, if necessary
                        if (tokenStart < current - 1/*backslash*/)
                            result.append(token(TokenType::String, line, tokenStart, current - 1));
                        tokenStart = current - 1; // backslash
                        current += escapeLength;
                        result.append(token(TokenType::EscapeSequence, line, tokenStart, current));
                        tokenStart = current;
                    } else { // invalid escape sequence
                        // add previous string as token, this is at least backslash
                        result.append(token(TokenType::String, line, tokenStart, current));
                        result.append(token(TokenType::StringError, line, current, current + 1));
                        ++current;
                        tokenStart = current;
                    }
                }
            } else {
                ++current;
            }
        }
    } while (current < length && inString);
    if (current > tokenStart)
        result.append(token(TokenType::String, line, tokenStart, current));
    if (inString && !*inStringGap) { // unterminated string
        // mark last character of last token as Unknown as an error hint
        if (!result.isEmpty()) { // should actually never be different
            Token &lastRef = result.last();
            if (lastRef.length == 1) {
                lastRef.type = TokenType::StringError;
            } else {
                --lastRef.length;
                lastRef.text = QStringView(*line).mid(lastRef.startCol, lastRef.length);
                result.append(token(TokenType::StringError, line, current - 1, current));
            }
        }
    }
    return result;
}

static QVector<Token> getMultiLineComment(std::shared_ptr<QString> line, int start,
                                          int *commentLevel/*in_out*/)
{
    // Haskell multiline comments can be nested {- foo {- bar -} blah -}
    const int length = line->length();
    int current = start;
    do {
        const QStringView test = QStringView(*line).mid(current, 2);
        if (test == QLatin1String("{-")) {
            ++(*commentLevel);
            current += 2;
        } else if (test == QLatin1String("-}") && *commentLevel > 0) {
            --(*commentLevel);
            current += 2;
        } else if (*commentLevel > 0) {
            ++current;
        }
    } while (current < length && *commentLevel > 0);
    if (current > start) {
        return {token(TokenType::MultiLineComment, line, start, current)};
    }
    return {};
}

static QVector<Token> getChar(std::shared_ptr<QString> line, int start)
{
    if (line->at(start) != '\'')
        return {};
    QVector<Token> result;
    const int length = line->length();
    int tokenStart = start;
    int current = tokenStart + 1;
    bool inChar = true;
    int count = 0;
    while (current < length && inChar) {
        if (line->at(current) == '\'') {
            inChar = false;
            ++current;
        } else if (count == 1) {
            // we already have one character, so start Unknown token
            if (current > tokenStart)
                result.append(token(TokenType::Char, line, tokenStart, current));
            tokenStart = current;
            ++count;
            ++current;
        } else if (count > 1) {
            ++count;
            ++current;
        } else if (line->at(current) == '\\') {
            if (current + 1 < length) {
                ++current;
                ++count;
                const int escapeLength = getEscape(*line, current);
                if (line->at(current) != '&' && escapeLength > 0) { // no & escape for chars
                    // valid escape
                    // add previous string as token without backslash, if necessary
                    if (tokenStart < current - 1/*backslash*/)
                        result.append(token(TokenType::Char, line, tokenStart, current - 1));
                    tokenStart = current - 1; // backslash
                    current += escapeLength;
                    result.append(token(TokenType::EscapeSequence, line, tokenStart, current));
                    tokenStart = current;
                } else { // invalid escape sequence
                    // add previous string as token, this is at least backslash
                    result.append(token(TokenType::Char, line, tokenStart, current));
                    result.append(token(TokenType::CharError, line, current, current + 1));
                    ++current;
                    tokenStart = current;
                }
            } else {
                ++current;
            }
        } else {
            ++count;
            ++current;
        }
    }
    if (count > 1 && inChar) {
        // too long and unterminated, just add Unknown token till end
        result.append(token(TokenType::CharError, line, tokenStart, current));
    } else if (count > 1) {
        // too long but terminated, add Unknown up to ending quote, then quote
        result.append(token(TokenType::CharError, line, tokenStart, current - 1));
        result.append(token(TokenType::Char, line, current - 1, current));
    } else if (inChar || count < 1) {
        // unterminated, or no character inside, mark last character as error
        if (current > tokenStart + 1)
            result.append(token(TokenType::Char, line, tokenStart, current - 1));
        result.append(token(TokenType::CharError, line, current - 1, current));
    } else {
        result.append(token(TokenType::Char, line, tokenStart, current));
    }
    return result;
}

static QVector<Token> getSpecial(std::shared_ptr<QString> line, int start)
{
    if (SPECIAL->contains(line->at(start)))
        return {{TokenType::Special, start, 1, QStringView(*line).mid(start, 1), line}};
    return {};
}

Tokens HaskellTokenizer::tokenize(const QString &line, int startState)
{
    Tokens result(std::make_shared<QString>(line));
    const int length = result.source->length();
    bool inStringGap = startState == int(Tokens::State::StringGap);
    int multiLineCommentLevel = std::max(startState - int(Tokens::State::MultiLineCommentGuard), 0);
    int currentStart = 0;
    QVector<Token> tokens;
    while (currentStart < length) {
        if (multiLineCommentLevel <= 0 &&
                !(tokens = getString(result.source, currentStart, &inStringGap)).isEmpty()) {
            result.append(tokens);
        } else if (!(tokens = getMultiLineComment(result.source, currentStart,
                                                &multiLineCommentLevel)).isEmpty()) {
            result.append(tokens);
        } else if (!(tokens = getChar(result.source, currentStart)).isEmpty()) {
            result.append(tokens);
        } else if (!(tokens = getSpace(result.source, currentStart)).isEmpty()) {
            result.append(tokens);
        } else if (!(tokens = getNumber(result.source, currentStart)).isEmpty()) {
            result.append(tokens);
        } else if (!(tokens = getIdOrOpOrSingleLineComment(result.source, currentStart)).isEmpty()) {
            result.append(tokens);
        } else if (!(tokens = getSpecial(result.source, currentStart)).isEmpty()) {
            result.append(tokens);
        } else {
            tokens = {{TokenType::Unknown,
                       currentStart,
                       1,
                       QStringView(*result.source).mid(currentStart, 1),
                       result.source}};
            result.append(tokens);
        }
        currentStart += std::accumulate(tokens.cbegin(), tokens.cend(), 0,
                                        [](int s, const Token &t) { return s + t.length; });
    }
    if (inStringGap)
        result.state = int(Tokens::State::StringGap);
    else if (multiLineCommentLevel > 0)
        result.state = int(Tokens::State::MultiLineCommentGuard) + multiLineCommentLevel;
    return result;
}

bool Token::isValid() const
{
    return type != TokenType::Unknown;
}

} // Internal
} // Haskell
