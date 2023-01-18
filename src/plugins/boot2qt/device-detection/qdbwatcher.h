// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QLocalSocket>
#include <QMutex>
#include <QObject>

#include <memory>

enum class RequestType;
QT_BEGIN_NAMESPACE
class QJsonDocument;
QT_END_NAMESPACE

namespace Qdb::Internal {

class QdbWatcher : public QObject
{
    Q_OBJECT

public:
    QdbWatcher(QObject *parent = nullptr);
    virtual ~QdbWatcher();
    void stop();
    void start(RequestType requestType);

signals:
    void incomingMessage(const QJsonDocument &);
    void watcherError(const QString &);

private:
    void startPrivate();

private:
    void handleWatchConnection();
    void handleWatchError(QLocalSocket::LocalSocketError error);
    void handleWatchMessage();
    static void forkHostServer();
    void retry();

    static QMutex s_startMutex;
    static bool s_startedServer;

    std::unique_ptr<QLocalSocket> m_socket = nullptr;
    bool m_shuttingDown = false;
    bool m_retried = false;
    RequestType m_requestType;
};

} // Qdb::Internal

