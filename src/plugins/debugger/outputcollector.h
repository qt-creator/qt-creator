// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

QT_BEGIN_NAMESPACE
class QLocalServer;
class QLocalSocket;
class QSocketNotifier;
QT_END_NAMESPACE

namespace Debugger::Internal {

class OutputCollector : public QObject
{
    Q_OBJECT

public:
    OutputCollector() = default;
    ~OutputCollector() override;
    bool listen();
    void shutdown();
    QString serverName() const;
    QString errorString() const;

signals:
    void byteDelivery(const QByteArray &data);

private:
    void bytesAvailable();
#ifdef Q_OS_WIN
    void newConnectionAvailable();
    QLocalServer *m_server = nullptr;
    QLocalSocket *m_socket = nullptr;
#else
    void closeFileDescriptors();
    QString m_serverPath;
    int m_serverFd = -1;  // Master side of the pseudo terminal, read by us.
    int m_slaveFd = -1;   // Kept open only to hold the pseudo terminal alive.
    QSocketNotifier *m_serverNotifier = nullptr;
    QString m_errorString;
#endif
};

} // Debugger::Internal
