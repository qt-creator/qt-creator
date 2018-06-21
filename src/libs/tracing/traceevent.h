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

#include <QHash>
#include <QMetaType>

namespace Timeline {

class TraceEvent
{
public:
    qint64 timestamp() const { return m_timestamp; }
    void setTimestamp(qint64 timestamp) { m_timestamp = timestamp; }

    qint32 typeIndex() const { return m_typeIndex; }
    void setTypeIndex(qint32 typeIndex) { m_typeIndex = typeIndex; }

    bool isValid() const { return m_typeIndex != -1; }

protected:
    TraceEvent(qint64 timestamp = -1, qint32 typeIndex = -1)
        : m_timestamp(timestamp), m_typeIndex(typeIndex)
    {}

    TraceEvent(const TraceEvent &) = default;
    TraceEvent(TraceEvent &&) = default;
    TraceEvent &operator=(const TraceEvent &) = default;
    TraceEvent &operator=(TraceEvent &&) = default;

private:
    qint64 m_timestamp;
    qint32 m_typeIndex;
};

} // namespace Timeline

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(Timeline::TraceEvent, Q_MOVABLE_TYPE);
QT_END_NAMESPACE
