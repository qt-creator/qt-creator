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


#ifndef LOCATORFILTERTEST_H
#define LOCATORFILTERTEST_H

#include "ilocatorfilter.h"

#include <QTest>

namespace Core {
namespace Tests {

/// Runs a locator filter for a search text and returns the results.
class CORE_EXPORT BasicLocatorFilterTest
{
public:
    BasicLocatorFilterTest(Core::ILocatorFilter *filter);

    QList<Core::LocatorFilterEntry> matchesFor(const QString &searchText = QString());

private:
    virtual void doBeforeLocatorRun() {}
    virtual void doAfterLocatorRun() {}

    Core::ILocatorFilter *m_filter;
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

#endif // LOCATORFILTERTEST_H
