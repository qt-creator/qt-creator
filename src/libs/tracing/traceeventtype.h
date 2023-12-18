// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tracing_global.h"
#include "safecastable.h"

#include <QHash>
#include <QMetaType>
#include <QString>

namespace Timeline {

class TraceEventType : public SafeCastable<TraceEventType>
{
public:
    TraceEventType(const TraceEventType &) = default;
    TraceEventType(TraceEventType &&) = default;
    TraceEventType &operator=(const TraceEventType &) = default;
    TraceEventType &operator=(TraceEventType &&) = default;

    const QString &displayName() const { return m_displayName; }
    void setDisplayName(const QString &displayName) { m_displayName = displayName; }

    quint8 feature() const { return m_feature; }
    void setFeature(quint8 feature) { m_feature = feature; }

    qint32 classId() const { return m_classId; }

protected:
    TraceEventType(qint32 classId, quint8 feature = 255, const QString &displayName = QString())
        : m_displayName(displayName), m_classId(classId), m_feature(feature)
    {}

private:
    QString m_displayName;
    qint32 m_classId;
    quint8 m_feature;
};

} // namespace Timeline

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(Timeline::TraceEventType, Q_MOVABLE_TYPE);
QT_END_NAMESPACE
