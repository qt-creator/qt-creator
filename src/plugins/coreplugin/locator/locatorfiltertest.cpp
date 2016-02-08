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

BasicLocatorFilterTest::BasicLocatorFilterTest(ILocatorFilter *filter) : m_filter(filter)
{
}

QList<LocatorFilterEntry> BasicLocatorFilterTest::matchesFor(const QString &searchText)
{
    doBeforeLocatorRun();
    const QList<ILocatorFilter *> filters = QList<ILocatorFilter *>() << m_filter;
    m_filter->prepareSearch(searchText);
    QFuture<LocatorFilterEntry> locatorSearch = Utils::runAsync(&Internal::runSearch, filters,
                                                                searchText);
    locatorSearch.waitForFinished();
    doAfterLocatorRun();
    return locatorSearch.results();
}

ResultData::ResultData()
{
}

ResultData::ResultData(const QString &textColumn1, const QString &textColumn2)
    : textColumn1(textColumn1), textColumn2(textColumn2)
{
}

bool ResultData::operator==(const ResultData &other) const
{
    return textColumn1 == other.textColumn1 && textColumn2 == other.textColumn2;
}

ResultData::ResultDataList ResultData::fromFilterEntryList(const QList<LocatorFilterEntry> &entries)
{
    ResultDataList result;
    foreach (const LocatorFilterEntry &entry, entries)
        result << ResultData(entry.displayName, entry.extraInfo);
    return result;
}

void ResultData::printFilterEntries(const ResultData::ResultDataList &entries, const QString &msg)
{
    QTextStream out(stdout);
    if (!msg.isEmpty())
        out << msg << endl;
    foreach (const ResultData entry, entries) {
        out << "<< ResultData(_(\"" << entry.textColumn1 << "\"), _(\"" << entry.textColumn2
            <<  "\"))" << endl;
    }
}
