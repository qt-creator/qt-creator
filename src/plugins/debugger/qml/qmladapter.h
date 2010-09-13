/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef QMLADAPTER_H
#define QMLADAPTER_H

#include <QtCore/QObject>
#include <QtCore/QScopedPointer>

#include "debugger_global.h"

#include <QtNetwork/QAbstractSocket>

namespace QmlJsDebugClient {
class QDeclarativeEngineDebug;
class QDeclarativeDebugConnection;
}

namespace Debugger {

class DebuggerEngine;

namespace Internal {
class QmlDebuggerClient;
} // namespace Internal

struct QmlAdapterPrivate;
class DEBUGGER_EXPORT QmlAdapter : public QObject
{
    Q_OBJECT
public:
    explicit QmlAdapter(DebuggerEngine *engine, QObject *parent = 0);
    virtual ~QmlAdapter();

    void beginConnection();
    void pauseConnection();
    void closeConnection();

    bool isConnected() const;
    bool isUnconnected() const;

    QmlJsDebugClient::QDeclarativeEngineDebug *client() const;
    QmlJsDebugClient::QDeclarativeDebugConnection *connection() const;

    // TODO move to private API b/w engine and adapter
    void setMaxConnectionAttempts(int maxAttempts);
    void setConnectionAttemptInterval(int interval);

signals:
    void aboutToDisconnect();
    void connected();
    void disconnected();
    void connectionStartupFailed();
    void connectionError(QAbstractSocket::SocketError socketError);

private slots:
    void connectionErrorOccurred(QAbstractSocket::SocketError socketError);
    void connectionStateChanged();
    void pollInferior();

private:
    bool connectToViewer();
    void createDebuggerClient();
    void showConnectionStatusMessage(const QString &message);
    void showConnectionErrorMessage(const QString &message);

private:
    QScopedPointer<QmlAdapterPrivate> d;
};

} // namespace Debugger

#endif // QMLADAPTER_H
