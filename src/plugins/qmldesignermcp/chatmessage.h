// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QElapsedTimer>
#include <QDateTime>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVariantMap>

namespace QmlDesigner {

/**
 * @brief A single message in the chat conversation
 *
 * Exposed to QML for displaying chat history
 */
class ChatMessage : public QObject
{
    Q_OBJECT

    Q_PROPERTY(MessageType type READ type)
    Q_PROPERTY(QString content READ content)
    Q_PROPERTY(QString toolName READ toolName CONSTANT)
    Q_PROPERTY(QString serverName READ serverName CONSTANT)
    Q_PROPERTY(QDateTime timestamp READ timestamp CONSTANT)
    Q_PROPERTY(QList<int> pendingIndices READ pendingIndices CONSTANT)
    Q_PROPERTY(bool resolved READ resolved)

public:
    enum MessageType {
        UserMessage,
        AssistantMessage,
        ToolCallStarted,
        ToolCallCompleted,
        ToolCallFailed,
        ToolCallConfirmation,
        SystemMessage,
        IterationMessage
    };
    Q_ENUM(MessageType)

    ChatMessage(MessageType type, const QString &content, QObject *parent = nullptr);

    MessageType type() const;
    QString content() const;
    QString toolName() const;
    QString serverName() const;
    bool success() const;
    QDateTime timestamp() const;
    QJsonObject toolArgs() const;

    QList<int> pendingIndices() const;
    void setPendingIndices(const QList<int> &indices);

    void setToolInfo(const QString &serverName, const QString &toolName);
    void setToolArgs(const QJsonObject &args);

    QVariantList segments() const;
    void setSegments(const QVariantList &segs);

    // Called when a tool call finishes updates tool state in place to mark it done or error.
    void complete(bool success);

    void resolve(bool confirmed);
    bool resolved() const;

signals:
    void typeChanged();
    void contentChanged();
    void resolvedChanged();

private:
    MessageType m_type;
    QString m_content;
    QString m_toolName;
    QString m_serverName;
    QVariantList m_segments;
    QDateTime m_timestamp;
    QJsonObject m_toolArgs;
    QElapsedTimer m_timer;
    QList<int> m_pendingIndices;
    bool m_resolved = false;
};

} // namespace QmlDesigner
