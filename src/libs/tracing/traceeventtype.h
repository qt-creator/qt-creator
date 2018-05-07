/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "tracing_global.h"
#include "safecastable.h"

#include <QHash>
#include <QMetaType>
#include <QString>

namespace Timeline {

class TraceEventType : public SafeCastable<TraceEventType>
{
public:
    const QString &displayName() const { return m_displayName; }
    void setDisplayName(const QString &displayName) { m_displayName = displayName; }

    quint8 feature() const { return m_feature; }
    void setFeature(quint8 feature) { m_feature = feature; }

    qint32 classId() const { return m_classId; }

protected:
    TraceEventType(qint32 classId, quint8 feature = 255, const QString &displayName = QString())
        : m_displayName(displayName), m_classId(classId), m_feature(feature)
    {}

    TraceEventType(const TraceEventType &) = default;
    TraceEventType(TraceEventType &&) = default;
    TraceEventType &operator=(const TraceEventType &) = default;
    TraceEventType &operator=(TraceEventType &&) = default;

private:
    QString m_displayName;
    qint32 m_classId;
    quint8 m_feature;
};

} // namespace Timeline

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(Timeline::TraceEventType, Q_MOVABLE_TYPE);
QT_END_NAMESPACE
