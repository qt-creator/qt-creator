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

Utils::SearchResultItems SilverSearcherOutputParser::parse()
{
    while (index < outputSize - 1) {
        if (output[index] == '\n') {
            ++index;
            continue;
        }
        parseFilePath();
        while (index < outputSize && output[index] != '\n') {
            const int lineNumber = parseLineNumber();
            if (index >= outputSize - 1)
                break;
            int matches = parseMatches(lineNumber);
            if (index >= outputSize - 1)
                break;
            parseText();
            for (int i = 0; i < matches; ++i)
                items[items.size() - i - 1].setDisplayText(item.lineText());
        }
    }

    return items;
}

bool SilverSearcherOutputParser::parseFilePath()
{
    int startIndex = ++index;
    while (index < outputSize && output[index] != '\n')
        ++index;
    item.setFilePath(Utils::FilePath::fromString(QString(output.data() + startIndex,
                                                         index - startIndex)));
    ++index;
    return true;
}

int SilverSearcherOutputParser::parseLineNumber()
{
    int startIndex = index;
    while (index < outputSize && output[++index] != ';') { }

    const int lineNumber = QString(output.data() + startIndex, index - startIndex).toInt();
    ++index;
    return lineNumber;
}

int SilverSearcherOutputParser::parseMatchIndex()
{
    int startIndex = index;
    while (index < outputSize && output[++index] != ' ') { }

    const int lineStart = QString(output.data() + startIndex, index - startIndex).toInt();
    ++index;
    return lineStart;
}

int SilverSearcherOutputParser::parseMatchLength()
{
    int startIndex = index;
    while (index < outputSize && output[++index] != ':' && output[index] != ',') { }

    return QString(output.data() + startIndex, index - startIndex).toInt();
}

int SilverSearcherOutputParser::parseMatches(int lineNumber)
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
        const int lineStart = parseMatchIndex();
        const int lineLength = parseMatchLength();
        item.setMainRange(lineNumber, lineStart, lineLength);
        if (hasRegexp) {
            const QString part = QString(text.mid(lineStart, lineLength));
            item.setUserData(regexp.match(part).capturedTexts());
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

    item.setLineText(QString(output.data() + startIndex, index - startIndex));
    ++index;
    return true;
}

} // namespace SilverSearcher
