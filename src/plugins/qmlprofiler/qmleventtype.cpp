// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qmleventtype.h"
#include <QDataStream>

namespace QmlProfiler {

static ProfileFeature qmlFeatureFromType(Message message, RangeType rangeType, int detailType)
{
    switch (message) {
    case Event: {
        switch (detailType) {
        case Mouse:
        case Key:
            return ProfileInputEvents;
        case AnimationFrame:
            return ProfileAnimations;
        default:
            return MaximumProfileFeature;
        }
    }
    case PixmapCacheEvent:
        return ProfilePixmapCache;
    case SceneGraphFrame:
        return ProfileSceneGraph;
    case MemoryAllocation:
        return ProfileMemory;
    case DebugMessage:
        return ProfileDebugMessages;
    case Quick3DEvent:
        return ProfileQuick3D;
    default:
        return featureFromRangeType(rangeType);
    }
}

QDataStream &operator>>(QDataStream &stream, QmlEventType &type)
{
    quint8 message;
    quint8 rangeType;
    QString displayName;
    stream >> displayName >> type.m_data >> type.m_location >> message >> rangeType
           >> type.m_detailType;
    type.setDisplayName(displayName);
    type.m_message = static_cast<Message>(message);
    type.m_rangeType = static_cast<RangeType>(rangeType);
    type.setFeature(qmlFeatureFromType(type.m_message, type.m_rangeType, type.m_detailType));
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const QmlEventType &type)
{
    return stream << type.displayName() << type.m_data << type.m_location
                  << static_cast<quint8>(type.m_message) << static_cast<quint8>(type.m_rangeType)
                  << type.m_detailType;
}

QmlEventType::QmlEventType(Message message, RangeType rangeType, int detailType,
                           const QmlEventLocation &location, const QString &data,
                           const QString &displayName) :
    TraceEventType(staticClassId, qmlFeatureFromType(message, rangeType, detailType)),
    m_data(data), m_location(location), m_message(message),
    m_rangeType(rangeType), m_detailType(detailType)
{
    setDisplayName(displayName);
}

} // namespace QmlProfiler
