// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "qmlevent.h"
#include "qmleventtype.h"

#include <QDataStream>

namespace QmlDebug {

struct QmlTypedEvent
{
    QmlEvent event;
    QmlEventType type;
    qint64 serverTypeId = 0;
};

QDataStream &operator>>(QDataStream &stream, QmlTypedEvent &event);

} // namespace QmlDebug

Q_DECLARE_METATYPE(QmlDebug::QmlTypedEvent)

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(QmlDebug::QmlTypedEvent, Q_MOVABLE_TYPE);
QT_END_NAMESPACE

