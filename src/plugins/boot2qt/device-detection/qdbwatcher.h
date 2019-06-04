/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QLocalSocket>
#include <QMutex>
#include <QObject>

#include <memory>

enum class RequestType;
QT_BEGIN_NAMESPACE
class QJsonDocument;
QT_END_NAMESPACE

namespace Qdb {
namespace Internal {

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

} // namespace Internal
} // namespace Qdb

