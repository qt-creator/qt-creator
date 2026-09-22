// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acptransport.h"

#include "acpclienttr.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QLoggingCategory>

static Q_LOGGING_CATEGORY(logTransport, "qtc.acpclient.transport", QtWarningMsg);

namespace AcpClient::Internal {

AcpTransport::AcpTransport(QObject *parent)
    : QObject(parent)
{}

AcpTransport::~AcpTransport() = default;

void AcpTransport::send(const QJsonObject &message)
{
    if (m_inputClosed)
        return;
    const QByteArray content = QJsonDocument(message).toJson(QJsonDocument::Compact) + '\n';
    qCDebug(logTransport) << "Sending:" << content;
    sendData(content);
}

void AcpTransport::writeConsoleInput(const QByteArray &data)
{
    if (!m_acceptsConsoleInput)
        return;
    sendConsoleInput(data);
}

void AcpTransport::closeConsoleInput()
{
    if (!m_acceptsConsoleInput)
        return;
    endConsoleInput();
    setAcceptsConsoleInput(false);
    // The initialize request is in flight on the channel just closed, and
    // every request after it would need it too. What the server writes is
    // still shown, but the protocol goes no further.
    m_inputClosed = true;
    emit errorOccurred(Tr::tr("The server's input was closed, so the connection cannot "
                              "be established."));
}

void AcpTransport::resetProtocolState()
{
    m_buffer.clear();
    m_receivedMessage = false;
    m_inputClosed = false;
    setAcceptsConsoleInput(false);
}

void AcpTransport::setAcceptsConsoleInput(bool accepts)
{
    if (m_acceptsConsoleInput == accepts)
        return;
    m_acceptsConsoleInput = accepts;
    emit acceptsConsoleInputChanged(accepts);
}

void AcpTransport::parseData(const QByteArray &data)
{
    if (m_inputClosed)
        return;
    m_buffer.append(data);

    // Process complete newline-delimited JSON messages
    int pos;
    while ((pos = m_buffer.indexOf('\n')) >= 0) {
        const QByteArray rawLine = m_buffer.left(pos + 1);
        const QByteArray line = rawLine.trimmed();
        m_buffer = m_buffer.mid(pos + 1);

        if (line.isEmpty())
            continue;

        qCDebug(logTransport) << "Received:" << line;

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(line, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            // Before its first message a server may print a banner or a
            // login URL on stdout. That is for the user, next to its stderr,
            // and the user may still have to answer it.
            if (!m_receivedMessage) {
                emit consoleOutput(rawLine);
                continue;
            }
            emit errorOccurred(Tr::tr("JSON parse error: %1").arg(parseError.errorString()));
            continue;
        }

        // The server is speaking the protocol now, so its stdin has to carry
        // nothing else from here on.
        m_receivedMessage = true;
        setAcceptsConsoleInput(false);

        // ACP v2 explicitly allows JSON-RPC 2.0 batches on stdio: a line may
        // carry an array of requests, notifications, or responses.
        if (doc.isArray()) {
            const QJsonArray batch = doc.array();
            // JSON-RPC 2.0 calls an empty batch an Invalid Request; report it
            // so that every line either produces messages or an error.
            if (batch.isEmpty()) {
                emit errorOccurred(Tr::tr("Empty JSON-RPC batch."));
                continue;
            }
            for (const QJsonValue &entry : batch) {
                if (entry.isObject())
                    emit messageReceived(entry.toObject());
                else
                    emit errorOccurred(Tr::tr("Expected JSON object in batch."));
            }
            continue;
        }

        if (!doc.isObject()) {
            emit errorOccurred(Tr::tr("Expected JSON object."));
            continue;
        }

        emit messageReceived(doc.object());
    }

    // A prompt has no newline after it, since the server reads the answer on
    // the prompt's line. Before the first message, what cannot be the start
    // of one goes to the console now rather than once a newline arrives, and
    // the response that follows the answer starts a line of its own.
    if (!m_receivedMessage) {
        const QByteArray pending = m_buffer.trimmed();
        if (!pending.isEmpty() && !pending.startsWith('{') && !pending.startsWith('[')) {
            emit consoleOutput(m_buffer);
            m_buffer.clear();
        }
    }
}

} // namespace AcpClient::Internal
