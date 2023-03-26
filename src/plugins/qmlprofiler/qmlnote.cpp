// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlnote.h"
#include <QDataStream>

namespace QmlProfiler {

QDataStream &operator>>(QDataStream &stream, QmlNote &note)
{
    return stream >> note.m_typeIndex >> note.m_collapsedRow >> note.m_startTime >> note.m_duration
                  >> note.m_text;
}

QDataStream &operator<<(QDataStream &stream, const QmlNote &note)
{
    return stream << note.m_typeIndex << note.m_collapsedRow << note.m_startTime << note.m_duration
                  << note.m_text;
}

bool operator==(const QmlNote &note1, const QmlNote &note2)
{
    return note1.typeIndex() == note2.typeIndex() && note1.collapsedRow() == note2.collapsedRow()
            && note1.startTime() == note2.startTime() && note1.duration() == note2.duration()
            && note1.text() == note2.text();
}

bool operator!=(const QmlNote &note1, const QmlNote &note2)
{
    return !(note1 == note2);
}

} // namespace QmlProfiler
