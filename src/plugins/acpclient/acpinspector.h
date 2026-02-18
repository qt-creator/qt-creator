// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QJsonObject>
#include <QJsonValue>
#include <QMap>
#include <QObject>
#include <QTime>

#include <list>
#include <optional>

namespace AcpClient::Internal {

class AcpLogMessage
{
public:
    enum MessageSender { ClientMessage, ServerMessage };

    AcpLogMessage() = default;
    AcpLogMessage(MessageSender sender, const QTime &time, const QJsonObject &message);

    MessageSender sender = ClientMessage;
    QTime time;
    QJsonObject message;

    QJsonValue id() const;
    QString displayText() const;

private:
    mutable std::optional<QJsonValue> m_id;
    mutable std::optional<QString> m_displayText;
};

class AcpInspector : public QObject
{
    Q_OBJECT
public:
    AcpInspector() = default;

    void show(const QString &defaultClient = {});

    void log(AcpLogMessage::MessageSender sender,
             const QString &clientName,
             const QJsonObject &message);

    std::list<AcpLogMessage> messages(const QString &clientName) const;
    QStringList clients() const;
    void clear() { m_logs.clear(); }

signals:
    void newMessage(const QString &clientName, const AcpLogMessage &message);

private:
    void onInspectorClosed();

    QMap<QString, std::list<AcpLogMessage>> m_logs;
    QWidget *m_currentWidget = nullptr;
    int m_logSize = 100;
};

} // namespace AcpClient::Internal
