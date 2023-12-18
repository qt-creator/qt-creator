// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"
#include "safecastable.h"

#include <QHash>
#include <QMetaType>

namespace Timeline {

class TraceEvent : public SafeCastable<TraceEvent>
{
public:
    TraceEvent(const TraceEvent &) = default;
    TraceEvent(TraceEvent &&) = default;
    TraceEvent &operator=(const TraceEvent &) = default;
    TraceEvent &operator=(TraceEvent &&) = default;

    qint64 timestamp() const { return m_timestamp; }
    void setTimestamp(qint64 timestamp) { m_timestamp = timestamp; }

    qint32 typeIndex() const { return m_typeIndex; }
    void setTypeIndex(qint32 typeIndex) { m_typeIndex = typeIndex; }

    bool isValid() const { return m_typeIndex != -1; }

    qint32 classId() const { return m_classId; }

protected:
    TraceEvent(qint32 classId, qint64 timestamp = -1, qint32 typeIndex = -1)
        : m_timestamp(timestamp), m_typeIndex(typeIndex), m_classId(classId)
    {}

private:
    qint64 m_timestamp;
    qint32 m_typeIndex;
    qint32 m_classId;
};

} // namespace Timeline

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(Timeline::TraceEvent, Q_MOVABLE_TYPE);
QT_END_NAMESPACE
