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

#include <QString>
#include <QMetaType>

namespace QmlProfiler {

class QmlNote {

public:
    QmlNote(int typeIndex = -1, qint64 startTime = -1, qint64 duration = -1,
                     const QString &text = QString()) :
        m_typeIndex(typeIndex), m_startTime(startTime), m_duration(duration), m_text(text)
    {}

    int typeIndex() const { return m_typeIndex; }
    qint64 startTime() const { return m_startTime; }
    qint64 duration() const { return m_duration; }
    QString text() const { return m_text; }

    void setText(const QString &text) { m_text = text; }

private:
    friend QDataStream &operator>>(QDataStream &stream, QmlNote &note);
    friend QDataStream &operator<<(QDataStream &stream, const QmlNote &note);

    int m_typeIndex;
    qint64 m_startTime;
    qint64 m_duration;
    QString m_text;
};

QDataStream &operator>>(QDataStream &stream, QmlNote &note);
QDataStream &operator<<(QDataStream &stream, const QmlNote &note);

} // namespace QmlProfiler

Q_DECLARE_METATYPE(QmlProfiler::QmlNote)
