// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmlprofiler_global.h"

#include <QString>
#include <QHash>

namespace QmlProfiler {

class QMLPROFILER_EXPORT QmlEventLocation
{
public:
    QmlEventLocation() = default;
    QmlEventLocation(const QString &file, int lineNumber, int columnNumber) : m_filename(file),
        m_line(lineNumber), m_column(columnNumber)
    {}

    void clear()
    {
        m_filename.clear();
        m_line = m_column = -1;
    }

    bool isValid() const
    {
        return !m_filename.isEmpty();
    }

    QString filename() const { return m_filename; }
    int line() const { return m_line; }
    int column() const { return m_column; }

    friend bool operator==(const QmlEventLocation &location1, const QmlEventLocation &location2)
    {
        // compare filename last as it's expensive.
        return location1.line() == location2.line() && location1.column() == location2.column()
                && location1.filename() == location2.filename();
    }

    friend bool operator!=(const QmlEventLocation &location1, const QmlEventLocation &location2)
    {
        return !(location1 == location2);
    }

    friend auto qHash(const QmlEventLocation &location)
    {
        return qHash(location.filename())
                ^ ((location.line() & 0xfff)                   // 12 bits of line number
                   | ((location.column() << 16) & 0xff0000));  // 8 bits of column

    }

private:
    friend QDataStream &operator>>(QDataStream &stream, QmlEventLocation &location);
    friend QDataStream &operator<<(QDataStream &stream, const QmlEventLocation &location);

    QString m_filename;
    int m_line = -1;
    int m_column = -1;
};

} // namespace QmlProfiler

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(QmlProfiler::QmlEventLocation, Q_MOVABLE_TYPE);
QT_END_NAMESPACE
