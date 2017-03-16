/****************************************************************************
**
** Copyright (C) 2017 Przemyslaw Gorszkowski <pgorszkowski@gmail.com>.
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

#include "silversearcheroutputparser.h"

#include <QString>

namespace SilverSearcher {

SilverSearcherOutputParser::SilverSearcherOutputParser(
        const QByteArray &output)
    : output(output)
    , outputSize(output.size())
{
}

QList<Utils::FileSearchResult> SilverSearcherOutputParser::parse()
{
    while (index < outputSize - 1) {
        if (output[index] == '\n') {
            ++index;
            continue;
        }
        parseFilePath();
        while (output[index] != '\n') {
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
    item.fileName = QString::fromUtf8(output.data() + startIndex, index - startIndex);
    ++index;
    return true;
}

bool SilverSearcherOutputParser::parseLineNumber()
{
    int startIndex = index;
    while (index < outputSize && output[++index] != ';') { }

    item.lineNumber = QString::fromUtf8(output.data() + startIndex, index - startIndex).toInt();
    ++index;
    return true;
}

bool SilverSearcherOutputParser::parseMatchIndex()
{
    int startIndex = index;
    while (index < outputSize && output[++index] != ' ') { }

    item.matchStart = QString::fromUtf8(output.data() + startIndex, index - startIndex).toInt();
    ++index;
    return true;
}

bool SilverSearcherOutputParser::parseMatchLength()
{
    int startIndex = index;
    while (index < outputSize && output[++index] != ':' && output[index] != ',') { }

    item.matchLength = QString::fromUtf8(output.data() + startIndex, index - startIndex).toInt();
    return true;
}

int SilverSearcherOutputParser::parseMatches()
{
    int matches = 1;
    while (index < outputSize && output[index] != ':') {
        if (output[index] == ',') {
            ++matches;
            ++index;
        }
        parseMatchIndex();
        parseMatchLength();
        items << item;
    }

    ++index;
    return matches;
}

bool SilverSearcherOutputParser::parseText()
{
    int startIndex = index;
    while (index < outputSize && output[++index] != '\n') { }
    item.matchingLine = QString::fromUtf8(output.data() + startIndex, index - startIndex);
    ++index;
    return true;
}

} // namespace SilverSearcher
