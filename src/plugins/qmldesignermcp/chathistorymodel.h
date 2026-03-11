// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "chatmessage.h"

#include <QAbstractListModel>
#include <QList>

namespace QmlDesigner {

/**
 * @brief List model for chat conversation history
 *
 * Provides messages to QML ListView for display
 */
class ChatHistoryModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        TypeRole = Qt::UserRole + 1,
        ContentRole,
        SegmentsRole,
        ToolNameRole,
        ServerNameRole,
        SuccessRole,
        TimestampRole
    };

    explicit ChatHistoryModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void clear();

    void addUserMessage(const QString &content);
    void addAssistantMessage(const QString &content);
    void addToolCallStarted(const QString &serverName, const QString &toolName, const QJsonObject &args);
    void completeToolCall(const QString &serverName, const QString &toolName, bool success);
    void addToolCallFailed(const QString &serverName, const QString &toolName, const QString &error);
    void addSystemMessage(const QString &content);
    void addIterationMessage(int current, int max);

    QString lastUserPrompt() const;
    bool isEmpty() const { return m_messages.isEmpty(); }

private:
    QList<ChatMessage*> m_messages;
};

} // namespace QmlDesigner
