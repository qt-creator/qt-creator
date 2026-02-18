// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "acptransport.h"

class QTcpSocket;

namespace AcpClient::Internal {

class AcpTcpTransport : public AcpTransport
{
    Q_OBJECT

public:
    explicit AcpTcpTransport(QObject *parent = nullptr);
    ~AcpTcpTransport() override;

    void setHost(const QString &host);
    void setPort(quint16 port);

    void start() override;
    void stop() override;

protected:
    void sendData(const QByteArray &data) override;

private:
    QString m_host = QStringLiteral("localhost");
    quint16 m_port = 0;
    QTcpSocket *m_socket = nullptr;
};

} // namespace AcpClient::Internal
