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

#include "qmleventtype.h"
#include <QDataStream>

namespace QmlProfiler {

QDataStream &operator>>(QDataStream &stream, QmlEventType &type)
{
    quint8 message;
    quint8 rangeType;
    stream >> type.m_displayName >> type.m_data >> type.m_location >> message >> rangeType
           >> type.m_detailType;
    type.m_message = static_cast<Message>(message);
    type.m_rangeType = static_cast<RangeType>(rangeType);
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const QmlEventType &type)
{
    return stream << type.m_displayName << type.m_data << type.m_location
                  << static_cast<quint8>(type.m_message) << static_cast<quint8>(type.m_rangeType)
                  << type.m_detailType;
}

ProfileFeature QmlEventType::feature() const
{
    switch (m_message) {
    case Event: {
        switch (m_detailType) {
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
    default:
        return featureFromRangeType(m_rangeType);
    }
}


} // namespace QmlProfiler
