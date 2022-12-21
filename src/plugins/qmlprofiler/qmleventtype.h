// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "qmleventlocation.h"
#include "qmlprofilereventtypes.h"

#include <tracing/traceeventtype.h>

#include <QString>
#include <QMetaType>
#include <QHash>

namespace QmlProfiler {

class QmlEventType : public Timeline::TraceEventType {
public:
    static const qint32 staticClassId = 0x716d6c74; // 'qmlt';

    QmlEventType(Message message = UndefinedMessage, RangeType rangeType = UndefinedRangeType,
                 int detailType = -1, const QmlEventLocation &location = QmlEventLocation(),
                 const QString &data = QString(), const QString &displayName = QString());

    void setData(const QString &data) { m_data = data; }
    void setLocation(const QmlEventLocation &location) { m_location = location; }

    QString data() const { return m_data; }
    QmlEventLocation location() const { return m_location; }
    Message message() const { return m_message; }
    RangeType rangeType() const { return m_rangeType; }
    int detailType() const { return m_detailType; }

private:
    friend QDataStream &operator>>(QDataStream &stream, QmlEventType &type);
    friend QDataStream &operator<<(QDataStream &stream, const QmlEventType &type);

    QString m_data;
    QmlEventLocation m_location;
    Message m_message;
    RangeType m_rangeType;
    int m_detailType; // can be EventType, BindingType, PixmapEventType or SceneGraphFrameType
};

} // namespace QmlProfiler

Q_DECLARE_METATYPE(QmlProfiler::QmlEventType)

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(QmlProfiler::QmlEventType, Q_MOVABLE_TYPE);
QT_END_NAMESPACE
