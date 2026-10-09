// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QJsonObject>
#include <QObject>

namespace AcpClient::Internal {

class AcpTransport : public QObject
{
    Q_OBJECT

public:
    explicit AcpTransport(QObject *parent = nullptr);
    ~AcpTransport() override;

    void send(const QJsonObject &message);

    virtual void start() = 0;
    virtual void stop() = 0;

    // Console input reaches the server's stdin with no protocol around it,
    // behind the initialize request, which is written as soon as the server
    // runs. It is for a server that reads that request and asks for something
    // before it answers; from its first message on, stdin is a message stream
    // and a raw line in it is a malformed message, so the input stops there.
    // Until then stdout that is not JSON is console output, not an error,
    // and so is a prompt the server writes without a newline.
    // Closing the input closes the protocol's channel with it, so the
    // connection is given up, and reported as such.
    bool acceptsConsoleInput() const { return m_acceptsConsoleInput; }
    void writeConsoleInput(const QByteArray &data);
    void closeConsoleInput();

signals:
    void messageReceived(const QJsonObject &message);
    void consoleOutput(const QByteArray &data);
    void acceptsConsoleInputChanged(bool accepts);
    void started();
    void finished();
    void errorOccurred(const QString &message);

protected:
    virtual void sendData(const QByteArray &data) = 0;
    virtual void sendConsoleInput(const QByteArray &data) { Q_UNUSED(data) }
    virtual void endConsoleInput() {}
    void setAcceptsConsoleInput(bool accepts);
    void parseData(const QByteArray &data);
    void resetProtocolState();
    bool hasReceivedMessage() const { return m_receivedMessage; }
    bool isInputClosed() const { return m_inputClosed; }

private:
    QByteArray m_buffer;
    bool m_acceptsConsoleInput = false;
    bool m_receivedMessage = false;
    bool m_inputClosed = false;
};

} // namespace AcpClient::Internal
