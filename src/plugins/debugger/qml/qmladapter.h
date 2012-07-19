/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef QMLADAPTER_H
#define QMLADAPTER_H

#include "debugger_global.h"

#include <qmldebug/qmldebugclient.h>

#include <QAbstractSocket>
#include <QObject>
#include <QPointer>
#include <QTimer>

namespace QmlDebug {
class BaseEngineDebugClient;
class QmlDebugConnection;
class QDebugMessageClient;
}

namespace Debugger {

class DebuggerEngine;

namespace Internal {
class BaseQmlDebuggerClient;
class QmlAdapterPrivate;

class QmlAdapter : public QObject
{
    Q_OBJECT

public:
    explicit QmlAdapter(DebuggerEngine *engine, QObject *parent = 0);
    virtual ~QmlAdapter();

    void beginConnectionTcp(const QString &address, quint16 port);
    void beginConnectionOst(const QString &port);
    void closeConnection();

    QmlDebug::QmlDebugConnection *connection() const;
    DebuggerEngine *debuggerEngine() const;

    BaseQmlDebuggerClient *activeDebuggerClient() const;
    QHash<QString, BaseQmlDebuggerClient*> debuggerClients() const;

    QmlDebug::QDebugMessageClient *messageClient() const;

public slots:
    void logServiceStatusChange(const QString &service, float version,
                                QmlDebug::ClientStatus newStatus);
    void logServiceActivity(const QString &service, const QString &logMessage);

signals:
    void connected();
    void disconnected();
    void connectionStartupFailed();
    void connectionError(QAbstractSocket::SocketError socketError);
    void serviceConnectionError(const QString serviceName);

private slots:
    void connectionErrorOccurred(QAbstractSocket::SocketError socketError);
    void clientStatusChanged(QmlDebug::ClientStatus status);
    void debugClientStatusChanged(QmlDebug::ClientStatus status);
    void connectionStateChanged();
    void checkConnectionState();

private:
    bool isConnected() const;
    void createDebuggerClients();
    void showConnectionStatusMessage(const QString &message);
    void showConnectionErrorMessage(const QString &message);

private:
    QPointer<DebuggerEngine> m_engine;
    BaseQmlDebuggerClient *m_qmlClient;
    QTimer m_connectionTimer;
    QmlDebug::QmlDebugConnection *m_conn;
    QHash<QString, BaseQmlDebuggerClient*> m_debugClients;
    QmlDebug::QDebugMessageClient *m_msgClient;
};

} // namespace Internal
} // namespace Debugger

#endif // QMLADAPTER_H
