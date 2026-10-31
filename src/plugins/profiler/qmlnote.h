// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>
#include <QMetaType>

#include <chrono>

namespace Profiler {

class QmlNote {

public:
    QmlNote(int typeIndex = -1, int collapsedRow = -1,
            std::chrono::nanoseconds startTime = std::chrono::nanoseconds(-1),
            std::chrono::nanoseconds duration = std::chrono::nanoseconds(0),
            const QString &text = QString()) :
        m_typeIndex(typeIndex), m_collapsedRow(collapsedRow), m_startTime(startTime),
        m_duration(duration), m_text(text), m_loaded(false)
    {}

    int typeIndex() const { return m_typeIndex; }
    int collapsedRow() const { return m_collapsedRow; }
    std::chrono::nanoseconds startTime() const { return m_startTime; }
    std::chrono::nanoseconds duration() const { return m_duration; }
    QString text() const { return m_text; }
    bool loaded() const { return m_loaded; }

    void setText(const QString &text) { m_text = text; }
    void setLoaded(bool loaded) { m_loaded = loaded; }

    friend bool operator==(const QmlNote &note1, const QmlNote &note2);

    friend QDataStream &operator>>(QDataStream &stream, QmlNote &note);
    friend QDataStream &operator<<(QDataStream &stream, const QmlNote &note);

private:
    int m_typeIndex;
    int m_collapsedRow;
    std::chrono::nanoseconds m_startTime;
    std::chrono::nanoseconds m_duration;
    QString m_text;
    bool m_loaded;
};

} // namespace Profiler

Q_DECLARE_METATYPE(Profiler::QmlNote)

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(Profiler::QmlNote, Q_MOVABLE_TYPE);
QT_END_NAMESPACE
