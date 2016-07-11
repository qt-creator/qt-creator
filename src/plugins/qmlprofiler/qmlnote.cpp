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
