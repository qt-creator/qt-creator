/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

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
