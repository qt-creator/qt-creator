// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "qmlevent.h"
#include "qmleventtype.h"

#include <QDataStream>

namespace QmlProfiler {

struct QmlTypedEvent
{
    QmlEvent event;
    QmlEventType type;
    qint64 serverTypeId = 0;
};

QDataStream &operator>>(QDataStream &stream, QmlTypedEvent &event);

} // namespace QmlProfiler

Q_DECLARE_METATYPE(QmlProfiler::QmlTypedEvent)

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(QmlProfiler::QmlTypedEvent, Q_MOVABLE_TYPE);
QT_END_NAMESPACE

