/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "locatorfiltertest.h"

#include "locatorsearchutils.h"

#include <utils/runextensions.h>

#include <QFuture>
#include <QList>
#include <QString>
#include <QTextStream>

using namespace Core;
using namespace Core::Tests;

/*!
    \class Core::Tests::BasicLocatorFilterTest
    \inmodule QtCreator
    \internal
*/

/*!
    \class Core::Tests::TestDataDir
    \inmodule QtCreator
    \internal
*/

/*!
    \namespace Core::Tests
    \inmodule QtCreator
    \internal
*/
BasicLocatorFilterTest::BasicLocatorFilterTest(ILocatorFilter *filter) : m_filter(filter)
{
}

BasicLocatorFilterTest::~BasicLocatorFilterTest() = default;

QList<LocatorFilterEntry> BasicLocatorFilterTest::matchesFor(const QString &searchText)
{
    doBeforeLocatorRun();
    m_filter->prepareSearch(searchText);
    QFuture<LocatorFilterEntry> locatorSearch = Utils::runAsync(
        &Internal::runSearch, QList<ILocatorFilter *>({m_filter}), searchText);
    locatorSearch.waitForFinished();
    doAfterLocatorRun();
    return locatorSearch.results();
}

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

ResultData::ResultDataList ResultData::fromFilterEntryList(const QList<LocatorFilterEntry> &entries)
{
    ResultDataList result;
    for (const LocatorFilterEntry &entry : entries) {
        ResultData data(entry.displayName, entry.extraInfo);
        data.highlight = QString(entry.displayName.size(), QChar(' '));
        for (int i = 0; i < entry.highlightInfo.starts.size(); ++i) {
            const int start = entry.highlightInfo.starts.at(i);
            const int length = entry.highlightInfo.lengths.at(i);
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
