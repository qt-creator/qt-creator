// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldebug_global.h"
#include <qtcpsocket.h>

#include <QDataStream>

namespace QmlDebug {

class QmlDebugConnection;
class QmlDebugClientPrivate;
class QMLDEBUG_EXPORT QmlDebugClient : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QmlDebugClient)
    Q_DISABLE_COPY(QmlDebugClient)

public:
    enum State { NotConnected, Unavailable, Enabled };

    QmlDebugClient(const QString &, QmlDebugConnection *parent);
    ~QmlDebugClient() override;

    QString name() const;
    float serviceVersion() const;
    State state() const;
    QmlDebugConnection *connection() const;
    int dataStreamVersion() const;

    virtual void sendMessage(const QByteArray &);
    virtual void stateChanged(State);
    virtual void messageReceived(const QByteArray &);

private:
    QScopedPointer<QmlDebugClientPrivate> d_ptr;
};

} // namespace QmlDebug
