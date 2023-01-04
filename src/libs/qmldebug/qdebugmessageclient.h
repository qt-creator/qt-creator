// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldebugclient.h"
#include "qmldebug_global.h"

namespace QmlDebug {

struct QDebugContextInfo
{
    int line;
    QString file;
    QString function;
    QString category;
    qint64 timestamp;
};

class QMLDEBUG_EXPORT QDebugMessageClient : public QmlDebugClient
{
    Q_OBJECT

public:
    explicit QDebugMessageClient(QmlDebugConnection *client);

    void stateChanged(State state) override;
    void messageReceived(const QByteArray &) override;

signals:
    void newState(QmlDebug::QmlDebugClient::State);
    void message(QtMsgType, const QString &,
                 const QmlDebug::QDebugContextInfo &);

private:
    Q_DISABLE_COPY(QDebugMessageClient)
};

} // namespace QmlDebug
