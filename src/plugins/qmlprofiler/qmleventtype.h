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

#include "qmleventlocation.h"
#include "qmlprofilereventtypes.h"
#include <QString>
#include <QMetaType>
#include <QHash>

namespace QmlProfiler {

class QmlEventType {
public:
    QmlEventType(Message message = MaximumMessage, RangeType rangeType = MaximumRangeType,
                 int detailType = -1, const QmlEventLocation &location = QmlEventLocation(),
                 const QString &data = QString(), const QString displayName = QString()) :
        m_displayName(displayName), m_data(data), m_location(location), m_message(message),
        m_rangeType(rangeType), m_detailType(detailType)
    {}

    void setDisplayName(const QString &displayName) { m_displayName = displayName; }
    void setData(const QString &data) { m_data = data; }
    void setLocation(const QmlEventLocation &location) { m_location = location; }

    ProfileFeature feature() const;
    QString displayName() const { return m_displayName; }
    QString data() const { return m_data; }
    QmlEventLocation location() const { return m_location; }
    Message message() const { return m_message; }
    RangeType rangeType() const { return m_rangeType; }
    int detailType() const { return m_detailType; }

private:
    friend QDataStream &operator>>(QDataStream &stream, QmlEventType &type);
    friend QDataStream &operator<<(QDataStream &stream, const QmlEventType &type);

    QString m_displayName;
    QString m_data;
    QmlEventLocation m_location;
    Message m_message;
    RangeType m_rangeType;
    int m_detailType; // can be EventType, BindingType, PixmapEventType or SceneGraphFrameType
};

QDataStream &operator>>(QDataStream &stream, QmlEventType &type);
QDataStream &operator<<(QDataStream &stream, const QmlEventType &type);

inline uint qHash(const QmlEventType &type)
{
    return qHash(type.location())
            ^ (((type.message() << 12) & 0xf000)             // 4 bits of message
               | ((type.rangeType() << 24) & 0xf000000)      // 4 bits of rangeType
               | ((type.detailType() << 28) & 0xf0000000));  // 4 bits of detailType
}

inline bool operator==(const QmlEventType &type1, const QmlEventType &type2)
{
    return type1.message() == type2.message() && type1.rangeType() == type2.rangeType()
            && type1.detailType() == type2.detailType() && type1.location() == type2.location();
}

inline bool operator!=(const QmlEventType &type1, const QmlEventType &type2)
{
    return !(type1 == type2);
}

} // namespace QmlProfiler

Q_DECLARE_METATYPE(QmlProfiler::QmlEventType)

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(QmlProfiler::QmlEventType, Q_MOVABLE_TYPE);
QT_END_NAMESPACE
