// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "locatorfiltertest.h"

#include <QTextStream>

using namespace Core;
using namespace Core::Tests;

/*!
    \class Core::Tests::ResultData
    \inmodule QtCreator
    \internal
*/
ResultData::ResultData() = default;

ResultData::ResultData(const QString &textColumn1, const QString &textColumn2,
                       const QString &highlightPositions)
    : textColumn1(textColumn1), textColumn2(textColumn2), highlight(highlightPositions)
{
}

bool ResultData::operator==(const ResultData &other) const
{
    const bool highlightEqual = highlight.isEmpty()
            || other.highlight.isEmpty()
            || highlight == other.highlight;

    return textColumn1 == other.textColumn1 && textColumn2 == other.textColumn2 && highlightEqual;
}

ResultData::ResultDataList ResultData::fromFilterEntryList(const LocatorFilterEntries &entries)
{
    ResultDataList result;
    for (const LocatorFilterEntry &entry : entries) {
        ResultData data(entry.displayName, entry.extraInfo);
        data.highlight = QString(entry.displayName.size(), QChar(' '));
        for (int i = 0; i < entry.highlightInfo.startsDisplay.size(); ++i) {
            const int start = entry.highlightInfo.startsDisplay.at(i);
            const int length = entry.highlightInfo.lengthsDisplay.at(i);
            if (start < 0 || start + length > data.highlight.size())
                break;
            for (int j = 0; j < length; ++j)
                data.highlight[start + j] = '~';
        }
        result << data;
    }

    return result;
}

void ResultData::printFilterEntries(const ResultData::ResultDataList &entries, const QString &msg)
{
    QTextStream out(stdout);
    if (!msg.isEmpty())
        out << msg << '\n';
    for (const ResultData &entry : entries) {
        out << "<< ResultData(_(\"" << entry.textColumn1 << "\"), _(\"" << entry.textColumn2
            <<  "\"))\n";
    }
}
