// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakerewriter.h"

#include <algorithm>

using namespace CMakeLang;

namespace {

bool isBlank(QChar character)
{
    return character == u' ' || character == u'\t' || character == u'\r';
}

int startOfLine(QStringView source, int position)
{
    int start = position;
    while (start > 0 && source.at(start - 1) != u'\n')
        --start;
    return start;
}

// The newline that ends the line, or the end of the source.
int endOfLine(QStringView source, int position)
{
    int end = position;
    while (end < source.size() && source.at(end) != u'\n')
        ++end;
    return end;
}

bool isBlankBetween(QStringView source, int from, int to)
{
    for (int position = from; position < to; ++position) {
        if (!isBlank(source.at(position)))
            return false;
    }
    return true;
}

bool needsQuoting(const QString &value)
{
    static const QString special = " \t\"'()#;\\";
    if (value.isEmpty())
        return true;
    for (const QChar character : value) {
        if (special.contains(character))
            return true;
    }
    return false;
}

QString escaped(const QString &value)
{
    QString result = value;
    result.replace(u'\\', "\\\\");
    result.replace(u'"', "\\\"");
    return result;
}

QString quoted(const QString &value)
{
    return u'"' + escaped(value) + u'"';
}

} // namespace

namespace CMakeLang {

Rewriter::Rewriter(const DocumentPtr &document)
    : _document(document)
    , _source(document ? document->source() : QStringView())
{}

void Rewriter::replaceValue(ArgumentAST *argument, const QString &value)
{
    if (!argument)
        return;

    const Token &token = argument->token;
    if (argument->asQuotedArgument() && token.length >= 2) {
        addEdit(token.begin() + 1, token.length - 2, escaped(value));
        return;
    }
    addEdit(token.begin(), token.length, needsQuoting(value) ? quoted(value) : value);
}

void Rewriter::remove(ArgumentAST *argument)
{
    remove(argument, argument);
}

void Rewriter::remove(ArgumentAST *first, ArgumentAST *last)
{
    if (!first || !last)
        return;

    int from = first->token.begin();
    int to = last->token.end();
    while (from > 0 && isBlank(_source.at(from - 1)))
        --from;

    const int lineStart = startOfLine(_source, from);
    const int lineEnd = endOfLine(_source, to);
    if (isBlankBetween(_source, lineStart, from) && isBlankBetween(_source, to, lineEnd)) {
        // The line ending of the line before goes, the one of this line stays.
        from = lineStart;
        if (from > 0) {
            --from;
            if (from > 0 && _source.at(from - 1) == u'\r')
                --from;
        }
        to = lineEnd;
        if (to > from && _source.at(to - 1) == u'\r')
            --to;
    }
    addEdit(from, to - from, {});
}

void Rewriter::insertAfter(ArgumentAST *argument, const QStringList &values)
{
    if (!argument || values.isEmpty())
        return;

    const int position = argument->token.end();
    addEdit(position, 0, lines(values, position));
}

void Rewriter::append(CommandAST *command, const QStringList &values)
{
    if (!command || values.isEmpty())
        return;

    ArgumentAST *last = command->arguments().last();
    const int position = last ? last->token.end() : command->rightParen.begin();
    addEdit(position, 0, lines(values, position));
}

QList<Edit> Rewriter::edits() const
{
    QList<Edit> sorted = _edits;
    std::stable_sort(sorted.begin(), sorted.end(), [](const Edit &first, const Edit &second) {
        return first.position < second.position;
    });
    return sorted;
}

void Rewriter::addEdit(int position, int length, const QString &text)
{
    if (position < 0 || position + length > _source.size())
        return;
    _edits.append({position, length, text});
}

QString Rewriter::indentationAt(int position) const
{
    const int start = startOfLine(_source, position);
    int end = start;
    while (end < _source.size() && isBlank(_source.at(end)))
        ++end;
    return _source.mid(start, end - start).toString();
}

QString Rewriter::lines(const QStringList &values, int position) const
{
    const QString indentation = indentationAt(position);
    QString text;
    for (const QString &value : values)
        text += u'\n' + indentation + value;
    return text;
}

} // namespace CMakeLang
