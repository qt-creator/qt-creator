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

#pragma once

#include "ilocatorfilter.h"

#include <QTest>

namespace Core {
namespace Tests {

/// Runs a locator filter for a search text and returns the results.
class CORE_EXPORT BasicLocatorFilterTest
{
public:
    BasicLocatorFilterTest(ILocatorFilter *filter);
    virtual ~BasicLocatorFilterTest();

    QList<LocatorFilterEntry> matchesFor(const QString &searchText = QString());

private:
    virtual void doBeforeLocatorRun() {}
    virtual void doAfterLocatorRun() {}

    ILocatorFilter *m_filter;
};

class CORE_EXPORT ResultData
{
public:
    typedef QList<ResultData> ResultDataList;

    ResultData();
    ResultData(const QString &textColumn1, const QString &textColumn2);

    bool operator==(const ResultData &other) const;

    static ResultDataList fromFilterEntryList(const QList<LocatorFilterEntry> &entries);

    /// For debugging and creating reference data
    static void printFilterEntries(const ResultDataList &entries, const QString &msg = QString());

    QString textColumn1;
    QString textColumn2;
};

typedef ResultData::ResultDataList ResultDataList;

} // namespace Tests
} // namespace Core

Q_DECLARE_METATYPE(Core::Tests::ResultData)
Q_DECLARE_METATYPE(Core::Tests::ResultDataList)

QT_BEGIN_NAMESPACE
namespace QTest {

template<> inline char *toString(const Core::Tests::ResultData &data)
{
    QByteArray ba = "\"" + data.textColumn1.toUtf8() + "\", \"" + data.textColumn2.toUtf8() + "\"";
    return qstrdup(ba.data());
}

} // namespace QTest
QT_END_NAMESPACE
