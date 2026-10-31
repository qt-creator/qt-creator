// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlnote.h"

#include <QDataStream>

namespace Profiler {

QDataStream &operator>>(QDataStream &stream, QmlNote &note)
{
    qint64 startTime = 0;
    qint64 duration = 0;
    stream >> note.m_typeIndex >> note.m_collapsedRow >> startTime >> duration >> note.m_text;
    note.m_startTime = std::chrono::nanoseconds(startTime);
    note.m_duration = std::chrono::nanoseconds(duration);
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const QmlNote &note)
{
    return stream << note.m_typeIndex << note.m_collapsedRow << qint64(note.m_startTime.count())
                  << qint64(note.m_duration.count()) << note.m_text;
}

bool operator==(const QmlNote &note1, const QmlNote &note2)
{
    return note1.typeIndex() == note2.typeIndex() && note1.collapsedRow() == note2.collapsedRow()
            && note1.startTime() == note2.startTime() && note1.duration() == note2.duration()
            && note1.text() == note2.text();
}

} // namespace Profiler
