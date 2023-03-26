// Copyright (C) 2017 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "silversearcheroutputparser.h"

#include <QString>

namespace SilverSearcher {

SilverSearcherOutputParser::SilverSearcherOutputParser(
        const QString &output, const QRegularExpression &regexp)
    : output(output)
    , regexp(regexp)
    , outputSize(output.size())
{
    hasRegexp = !regexp.pattern().isEmpty();
}

Utils::FileSearchResultList SilverSearcherOutputParser::parse()
{
    while (index < outputSize - 1) {
        if (output[index] == '\n') {
            ++index;
            continue;
        }
        parseFilePath();
        while (index < outputSize && output[index] != '\n') {
            parseLineNumber();
            if (index >= outputSize - 1)
                break;
            int matches = parseMatches();
            if (index >= outputSize - 1)
                break;
            parseText();
            for (int i = 0; i < matches; ++i)
                items[items.size() - i - 1].matchingLine = item.matchingLine;
        }
    }

    return items;
}

bool SilverSearcherOutputParser::parseFilePath()
{
    int startIndex = ++index;
    while (index < outputSize && output[index] != '\n')
        ++index;
    item.fileName = Utils::FilePath::fromString(QString(output.data() + startIndex, index - startIndex));
    ++index;
    return true;
}

bool SilverSearcherOutputParser::parseLineNumber()
{
    int startIndex = index;
    while (index < outputSize && output[++index] != ';') { }

    item.lineNumber = QString(output.data() + startIndex, index - startIndex).toInt();
    ++index;
    return true;
}

bool SilverSearcherOutputParser::parseMatchIndex()
{
    int startIndex = index;
    while (index < outputSize && output[++index] != ' ') { }

    item.matchStart = QString(output.data() + startIndex, index - startIndex).toInt();
    ++index;
    return true;
}

bool SilverSearcherOutputParser::parseMatchLength()
{
    int startIndex = index;
    while (index < outputSize && output[++index] != ':' && output[index] != ',') { }

    item.matchLength = QString(output.data() + startIndex, index - startIndex).toInt();
    return true;
}

int SilverSearcherOutputParser::parseMatches()
{
    int matches = 1;
    const int colon = output.indexOf(':', index);
    QString text;
    if (colon != -1) {
        const int textStart = colon + 1;
        const int newline = output.indexOf('\n', textStart);
        text = output.mid(textStart, newline >= 0 ? newline - textStart : -1);
    }
    while (index < outputSize && output[index] != ':') {
        if (output[index] == ',') {
            ++matches;
            ++index;
        }
        parseMatchIndex();
        parseMatchLength();
        if (hasRegexp) {
            const QString part = QString(text.mid(item.matchStart, item.matchLength));
            item.regexpCapturedTexts = regexp.match(part).capturedTexts();
        }
        items << item;
    }

    ++index;
    return matches;
}

bool SilverSearcherOutputParser::parseText()
{
    int startIndex = index;
    while (index < outputSize && output[++index] != '\n') { }
    item.matchingLine = QString(output.data() + startIndex, index - startIndex);
    ++index;
    return true;
}

} // namespace SilverSearcher
