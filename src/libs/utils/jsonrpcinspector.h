// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QMap>
#include <QObject>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QTime>

#include <functional>
#include <list>
#include <optional>

QT_BEGIN_NAMESPACE
class QComboBox;
class QWidget;
QT_END_NAMESPACE

namespace Utils {

// A single JSON-RPC message (request, response or notification) as logged by an endpoint.
class QTCREATOR_UTILS_EXPORT JsonRpcLogMessage
{
public:
    enum MessageSender { ClientMessage, ServerMessage };

    JsonRpcLogMessage() = default;
    JsonRpcLogMessage(MessageSender sender, const QTime &time, const QJsonObject &message);

    MessageSender sender = ClientMessage;
    QTime time;
    QJsonObject message;

    // Value of the "id" member, used to pair requests with responses.
    QJsonValue id() const;
    // "hh:mm:ss.zzz" followed by the method name or "Response #<id>".
    QString displayText() const;

private:
    mutable std::optional<QJsonValue> m_id;
    mutable std::optional<QString> m_displayText;
};

// Generic JSON-RPC traffic inspector. Stores a capped per-endpoint message log and shows it
// in a dialog (left/right detail panes around a message list). Shared by the LanguageClient,
// MCP server and ACP client plugins.
//
// "Endpoint" is the protocol-specific connection identity: a language server, an MCP session
// or an ACP client. Set Settings::endpointLabel to the term that fits the protocol.
class QTCREATOR_UTILS_EXPORT JsonRpcInspector : public QObject
{
    Q_OBJECT

public:
    struct Settings
    {
        QString windowTitle;
        QString endpointLabel; // e.g. "Language Server:", "Session:", "ACP Client:"
        int maxLogSize = 100;

        // JSON member names that are expanded by default when a message is shown in the
        // detail tree (e.g. {"params", "result"}). The implicit top-level node is always
        // expanded when this is non-empty so the message's members become visible.
        QStringList defaultExpandedKeys;

        // Utils cannot depend on Core, so the owner registers the dialog as an application
        // window (Core::ICore::registerWindow, close on coreAboutToClose) via this hook.
        std::function<void(QWidget *)> registerWindow;

        // Optional widget inserted between the endpoint selector row and the log view
        // (e.g. the LanguageClient message editor). Receives the endpoint selector combo box.
        std::function<QWidget *(QComboBox *endpointSelector)> headerWidget;

        // Optional extra tabs shown next to the log for the selected endpoint
        // (e.g. LanguageClient capabilities and custom client tabs). When set, the log is
        // wrapped in a tab widget. Rebuilt whenever the endpoint changes or refreshEndpoint()
        // is called.
        std::function<QList<QPair<QWidget *, QString>>(const QString &endpoint)> extraTabs;
    };

    explicit JsonRpcInspector(Settings settings, QObject *parent = nullptr);

    void show(const QString &defaultEndpoint = {});

    void log(JsonRpcLogMessage::MessageSender sender,
             const QString &endpoint,
             const QJsonObject &message);

    std::list<JsonRpcLogMessage> messages(const QString &endpoint) const;
    QStringList endpoints() const;
    void clear() { m_logs.clear(); }

    const Settings &settings() const { return m_settings; }

    // Rebuild the extra tabs of the open dialog if it currently shows the given endpoint.
    void refreshEndpoint(const QString &endpoint);

signals:
    void newMessage(const QString &endpoint, const Utils::JsonRpcLogMessage &message);
    void endpointRefreshRequested(const QString &endpoint);

private:
    void onInspectorClosed();

    const Settings m_settings;
    QMap<QString, std::list<JsonRpcLogMessage>> m_logs;
    QWidget *m_currentWidget = nullptr;
};

} // namespace Utils
