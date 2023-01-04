// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "baseenginedebugclient.h"

namespace QmlDebug {

class QmlDebugConnection;

class QMLDEBUG_EXPORT QmlEngineDebugClient : public BaseEngineDebugClient
{
    Q_OBJECT
public:
    explicit QmlEngineDebugClient(QmlDebugConnection *conn)
        : BaseEngineDebugClient(QLatin1String("QmlDebugger"), conn)
    {
    }
};

} // namespace QmlDebug
