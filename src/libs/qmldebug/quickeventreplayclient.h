// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldebugclient.h"
#include "qmlevent.h"
#include "qmleventtype.h"

namespace QmlDebug {

class QMLDEBUG_EXPORT QuickEventReplayClient : public QmlDebug::QmlDebugClient
{
    Q_OBJECT
public:
    QuickEventReplayClient(QmlDebug::QmlDebugConnection *connection);

    void sendEvent(const QmlEventType &type, const QmlEvent &event);
};

} // namespace QmlDebug
