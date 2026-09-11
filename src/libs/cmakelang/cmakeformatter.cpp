// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakeformatter.h"

#include "cmakeindentation.h"
#include "cmakelexer.h"
#include "cmakeparser.h"

#include <parsing/engine.h>

using namespace CMakeLang;

namespace {

bool isComment(const Token &token)
{
    return token.is(Parser::T_COMMENT) || token.is(Parser::T_BRACKET_COMMENT);
}

// The newlines of the whitespace and nothing else, so that the blank lines of
// the file stay and its line endings with them.
QString newlinesOf(QStringView whitespace)
{
    QString result;
    for (const QChar c : whitespace) {
        if (c == u'\n' || c == u'\r')
            result += c;
    }
    return result;
}

QString lineEndingOf(QStringView source)
{
    const qsizetype index = source.indexOf(u'\n');
    if (index > 0 && source.at(index - 1) == u'\r')
        return QStringLiteral("\r\n");
    return QStringLiteral("\n");
}

QString separatorFor(const Style &style, const Token *previous, const Token &current,
                     QStringView whitespace, int depth)
{
    if (!previous)
        return {};

    // A comment always stands apart, and keeps the whitespace that puts it
    // where it is when the style says so.
    if (isComment(current)) {
        if (style.keepCommentColumn && !whitespace.isEmpty())
            return whitespace.toString();
        return QStringLiteral(" ");
    }

    if (previous->is(Parser::T_LEFT_PAREN) || current.is(Parser::T_RIGHT_PAREN))
        return {};

    // The parenthesis of a call belongs to its name; one within the arguments
    // is a group of its own and stands apart.
    if (current.is(Parser::T_LEFT_PAREN) && depth == 0 && previous->is(Parser::T_IDENTIFIER)) {
        const bool space = namesControlCommand(previous->spelling)
                               ? style.spaceBeforeControlParen
                               : style.spaceBeforeCommandParen;
        return space ? QStringLiteral(" ") : QString();
    }

    return QStringLiteral(" ");
}

} // namespace

QList<Edit> CMakeLang::formattingEdits(QStringView source, const Style &style)
{
    const Indentation indentation(source, style);

    Engine engine;
    Lexer lexer(&engine, source);
    lexer.setScanComments(true);

    QList<Token> tokens;
    Token token;
    while (lexer.yylex(&token) != Parser::EOF_SYMBOL) {
        if (token.isNot(Parser::T_SPACE) && token.isNot(Parser::T_NEWLINE))
            tokens.append(token);
    }
    if (tokens.isEmpty())
        return {};

    QList<Edit> edits;
    const auto replace = [&](int position, int length, const QString &text) {
        if (source.mid(position, length) != text)
            edits.append({position, length, text});
    };

    int depth = 0;
    for (int i = 0; i < tokens.size(); ++i) {
        const Token &current = tokens.at(i);
        const Token *previous = i > 0 ? &tokens.at(i - 1) : nullptr;
        const int start = previous ? previous->end() : 0;
        const QStringView whitespace = source.mid(start, current.position - start);

        const QString newlines = newlinesOf(whitespace);
        if (newlines.isEmpty()) {
            replace(start,
                    int(whitespace.size()),
                    separatorFor(style, previous, current, whitespace, depth));
        } else if (const int level = indentation.levelAt(current.line);
                   level != Indentation::Keep) {
            replace(start, int(whitespace.size()), newlines + style.indentationFor(level));
        }

        if (current.is(Parser::T_LEFT_PAREN))
            ++depth;
        else if (current.is(Parser::T_RIGHT_PAREN) && depth > 0)
            --depth;
    }

    const int end = tokens.constLast().end();
    replace(end, int(source.size()) - end, lineEndingOf(source));

    return edits;
}
