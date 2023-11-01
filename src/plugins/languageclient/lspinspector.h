// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "dynamiccapabilities.h"

#include <QPointer>
#include <QTime>
#include <QWidget>

#include <languageserverprotocol/basemessage.h>
#include <languageserverprotocol/servercapabilities.h>

#include <list>

namespace LanguageClient {

class LspLogMessage
{
public:
    enum MessageSender { ClientMessage, ServerMessage } sender = ClientMessage;

    LspLogMessage();
    LspLogMessage(MessageSender sender,
                  const QTime &time,
                  const LanguageServerProtocol::JsonRpcMessage &message);
    QTime time;
    LanguageServerProtocol::JsonRpcMessage message;

    LanguageServerProtocol::MessageId id() const;
    QString displayText() const;

private:
    mutable std::optional<LanguageServerProtocol::MessageId> m_id;
    mutable std::optional<QString> m_displayText;
};

struct Capabilities
{
    LanguageServerProtocol::ServerCapabilities capabilities;
    DynamicCapabilities dynamicCapabilities;
};

class LspInspector : public QObject
{
    Q_OBJECT
public:
    LspInspector() {}

    void show(const QString &defaultClient = {});

    void log(const LspLogMessage::MessageSender sender,
             const QString &clientName,
             const LanguageServerProtocol::JsonRpcMessage &message);
    void clientInitialized(const QString &clientName,
                           const LanguageServerProtocol::ServerCapabilities &capabilities);
    void updateCapabilities(const QString &clientName,
                            const DynamicCapabilities &dynamicCapabilities);

    std::list<LspLogMessage> messages(const QString &clientName) const;
    Capabilities capabilities(const QString &clientName) const;
    QList<QString> clients() const;
    void clear() { m_logs.clear(); }

signals:
    void newMessage(const QString &clientName, const LspLogMessage &message);
    void capabilitiesUpdated(const QString &clientName);

private:
    void onInspectorClosed();

    QMap<QString, std::list<LspLogMessage>> m_logs;
    QMap<QString, Capabilities> m_capabilities;
    QWidget *m_currentWidget = nullptr;
    int m_logSize = 100; // default log size if no widget is currently visible
};

} // namespace LanguageClient
