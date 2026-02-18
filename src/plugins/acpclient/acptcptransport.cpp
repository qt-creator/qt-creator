// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acptcptransport.h"

#include <QLoggingCategory>
#include <QTcpSocket>

static Q_LOGGING_CATEGORY(logTcp, "qtc.acpclient.tcp", QtWarningMsg);

namespace AcpClient::Internal {

AcpTcpTransport::AcpTcpTransport(QObject *parent)
    : AcpTransport(parent)
{}

AcpTcpTransport::~AcpTcpTransport()
{
    stop();
}

void AcpTcpTransport::setHost(const QString &host)
{
    m_host = host;
}

void AcpTcpTransport::setPort(quint16 port)
{
    m_port = port;
}

void AcpTcpTransport::start()
{
    stop();

    m_socket = new QTcpSocket(this);

    connect(m_socket, &QTcpSocket::connected, this, &AcpTransport::started);
    connect(m_socket, &QTcpSocket::disconnected, this, &AcpTransport::finished);
    connect(m_socket, &QTcpSocket::errorOccurred, this, [this] {
        emit errorOccurred(m_socket->errorString());
    });
    connect(m_socket, &QTcpSocket::readyRead, this, [this] {
        parseData(m_socket->readAll());
    });

    qCDebug(logTcp) << "Connecting to" << m_host << ":" << m_port;
    m_socket->connectToHost(m_host, m_port);
}

void AcpTcpTransport::stop()
{
    if (m_socket) {
        m_socket->disconnect(this);
        m_socket->disconnectFromHost();
        delete m_socket;
        m_socket = nullptr;
    }
}

void AcpTcpTransport::sendData(const QByteArray &data)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        emit errorOccurred(tr("Cannot send data: not connected"));
        return;
    }
    m_socket->write(data);
}

} // namespace AcpClient::Internal
