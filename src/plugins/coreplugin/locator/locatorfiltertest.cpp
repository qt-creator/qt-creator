/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    QFuture<LocatorFilterEntry> locatorSearch = QtConcurrent::run(Core::Internal::runSearch,
                                                           filters, searchText);
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
