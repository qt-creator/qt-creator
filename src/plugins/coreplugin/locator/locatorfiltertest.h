// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ilocatorfilter.h"

#include <QTest>

namespace Core {
namespace Tests {

class CORE_EXPORT ResultData
{
public:
    using ResultDataList = QList<ResultData>;

    ResultData();
    ResultData(const QString &textColumn1, const QString &textColumn2,
               const QString &highlightPositions = QString());

    bool operator==(const ResultData &other) const;

    static ResultDataList fromFilterEntryList(const LocatorFilterEntries &entries);

    /// For debugging and creating reference data
    static void printFilterEntries(const ResultDataList &entries, const QString &msg = QString());

    QString textColumn1;
    QString textColumn2;
    QString highlight;
    LocatorFilterEntry::HighlightInfo::DataType dataType;
};

using ResultDataList = ResultData::ResultDataList;

} // namespace Tests
} // namespace Core

Q_DECLARE_METATYPE(Core::Tests::ResultData)
Q_DECLARE_METATYPE(Core::Tests::ResultDataList)

QT_BEGIN_NAMESPACE
namespace QTest {

template<> inline char *toString(const Core::Tests::ResultData &data)
{
    const QByteArray ba = "\n\"" + data.textColumn1.toUtf8() + "\", \"" + data.textColumn2.toUtf8()
            + "\"\n\"" + data.highlight.toUtf8() + "\"";
    return qstrdup(ba.data());
}

} // namespace QTest
QT_END_NAMESPACE
