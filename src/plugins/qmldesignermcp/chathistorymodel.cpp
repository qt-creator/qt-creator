// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "chathistorymodel.h"

#include <aiassistantutils.h>

#include <QFileInfo>
#include <QJsonObject>
#include <QRegularExpression>

namespace QmlDesigner {

namespace {

/**
 * @brief Returns a human-readable description of a tool call.
 *
 * Maps known MCP tool names and their key arguments to natural language so that
 * the chat UI shows "Reading Button.qml" instead of "Calling tool: read_qml".
 */
QString friendlyToolMessage(const QString &toolName, const QJsonObject &args)
{
    // e.g. "components/Button.qml" → "Button.qml"
    auto fileName = [&](const QString &key) -> QString {
        const QString path = args.value(key).toString();
        if (path.isEmpty())
            return {};
        return QFileInfo(path).fileName();
    };

    // Fall back to raw path when file name is empty
    auto fileArg = [&](const QString &key) -> QString {
        const QString name = fileName(key);
        return name.isEmpty() ? args.value(key).toString() : name;
    };

    if (toolName == "read_qml") {
        const QString f = fileArg("filepath");
        return f.isEmpty() ? QObject::tr("Reading file…") : QObject::tr("Reading %1…").arg(f);
    }

    if (toolName == "modify_qml") {
        const QString f = fileArg("filepath");
        return f.isEmpty() ? QObject::tr("Editing file…") : QObject::tr("Editing %1…").arg(f);
    }

    if (toolName == "create_qml") {
        const QString f = fileArg("filepath");
        return f.isEmpty() ? QObject::tr("Creating file…") : QObject::tr("Creating %1…").arg(f);
    }

    if (toolName == "delete_qml") {
        const QString f = fileArg("filepath");
        return f.isEmpty() ? QObject::tr("Deleting file…") : QObject::tr("Deleting %1…").arg(f);
    }

    if (toolName == "move_qml") {
        const QString src = fileArg("source");
        const QString dst = fileArg("destination");
        if (!src.isEmpty() && !dst.isEmpty())
            return QObject::tr("Moving %1 → %2…").arg(src, dst);
        if (!src.isEmpty())
            return QObject::tr("Moving %1…").arg(src);
        return QObject::tr("Moving file…");
    }

    // Generic fallback: turn snake_case into "Doing thing…"
    QString display = toolName;
    display.replace('_', ' ');
    if (!display.isEmpty())
        display[0] = display[0].toUpper();
    return display + "…";
}

QVariantList parseContentSegments(const QString &markdown)
{
    QVariantList segments;

    static const QRegularExpression codeBlockRe("```([^\\n]*)\\n([\\s\\S]*?)```",
                                                QRegularExpression::MultilineOption);

    int lastEnd = 0;
    auto it = codeBlockRe.globalMatch(markdown);

    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();

        const QString prose = markdown.mid(lastEnd, match.capturedStart() - lastEnd).trimmed();
        if (!prose.isEmpty()) {
            QVariantMap seg;
            seg["type"] = QStringLiteral("prose");
            seg["html"] = AiAssistantUtils::markdownToHtml(prose);
            segments.append(seg);
        }

        QVariantMap seg;
        seg["type"]     = QStringLiteral("code");
        seg["code"]     = match.captured(2);
        seg["language"] = match.captured(1).trimmed();
        segments.append(seg);

        lastEnd = match.capturedEnd();
    }

    const QString tail = markdown.mid(lastEnd).trimmed();
    if (!tail.isEmpty()) {
        QVariantMap seg;
        seg["type"] = QStringLiteral("prose");
        seg["html"] = AiAssistantUtils::markdownToHtml(tail);
        segments.append(seg);
    }

    return segments;
}

} // anonymous namespace

ChatHistoryModel::ChatHistoryModel(QObject *parent)
    : QAbstractListModel(parent)
{}

int ChatHistoryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_messages.size();
}

QVariant ChatHistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_messages.size())
        return QVariant();

    const ChatMessage *message = m_messages.at(index.row());

    switch (role) {
    case TypeRole:
        return message->type();
    case ContentRole:
        return message->content();
    case SegmentsRole:
        return message->segments();
    case ToolNameRole:
        return message->toolName();
    case ServerNameRole:
        return message->serverName();
    case PendingIndicesRole:
        return QVariant::fromValue(message->pendingIndices());
    case Resolved:
        return message->resolved();
    case TimestampRole:
        return message->timestamp();
    default:
        return QVariant();
    }
}

QHash<int, QByteArray> ChatHistoryModel::roleNames() const
{
    return {
        {TypeRole, "messageType"},
        {ContentRole, "content"},
        {SegmentsRole, "segments"},
        {ToolNameRole, "toolName"},
        {ServerNameRole, "serverName"},
        {PendingIndicesRole, "pendingIndices"},
        {Resolved, "resolved"},
        {TimestampRole, "timestamp"}
    };
}

void ChatHistoryModel::clear()
{
    beginResetModel();
    qDeleteAll(m_messages);
    m_messages.clear();
    endResetModel();
}

void ChatHistoryModel::addUserMessage(const QString &content)
{
    beginInsertRows(QModelIndex(), m_messages.size(), m_messages.size());
    m_messages.append(new ChatMessage(ChatMessage::UserMessage, content, this));
    endInsertRows();
}

void ChatHistoryModel::addAssistantMessage(const QString &content)
{
    auto *msg = new ChatMessage(ChatMessage::AssistantMessage,
                                AiAssistantUtils::markdownToHtml(content), this);
    msg->setSegments(parseContentSegments(content));

    beginInsertRows(QModelIndex(), m_messages.size(), m_messages.size());
    m_messages.append(msg);
    endInsertRows();
}

void ChatHistoryModel::addToolCallStarted(const QString &serverName, const QString &toolName,
                                          const QJsonObject &args)
{
    beginInsertRows(QModelIndex(), m_messages.size(), m_messages.size());
    auto *msg = new ChatMessage(ChatMessage::ToolCallStarted,
                                friendlyToolMessage(toolName, args), this);
    msg->setToolInfo(serverName, toolName);
    msg->setToolArgs(args);
    m_messages.append(msg);
    endInsertRows();
}

void ChatHistoryModel::completeToolCall(const QString &serverName, const QString &toolName, bool success)
{
    for (int i = m_messages.size() - 1; i >= 0; --i) {
        ChatMessage *msg = m_messages.at(i);
        if (msg->type() == ChatMessage::ToolCallStarted
            && msg->serverName() == serverName
            && msg->toolName() == toolName) {
            msg->complete(success);
            const QModelIndex idx = index(i);
            emit dataChanged(idx, idx, {TypeRole, ContentRole});
            return;
        }
    }
}

void ChatHistoryModel::addToolCallFailed(const QString &serverName, const QString &toolName, const QString &error)
{
    // If there is a matching in-progress entry, update it; otherwise insert a
    // standalone error row (e.g. when the tool was never started successfully).
    for (int i = m_messages.size() - 1; i >= 0; --i) {
        ChatMessage *msg = m_messages.at(i);
        if (msg->type() == ChatMessage::ToolCallStarted
            && msg->serverName() == serverName
            && msg->toolName() == toolName) {
            msg->complete(false);
            const QModelIndex idx = index(i);
            emit dataChanged(idx, idx, {TypeRole, ContentRole});
            return;
        }
    }
    // No matching start row — insert a standalone failure entry.
    beginInsertRows(QModelIndex(), m_messages.size(), m_messages.size());
    auto *msg = new ChatMessage(ChatMessage::ToolCallFailed, error, this);
    msg->setToolInfo(serverName, toolName);
    m_messages.append(msg);
    endInsertRows();
}

void ChatHistoryModel::addToolCallConfirmation(const QStringList &toolNames,
                                               const QList<QJsonObject> &argsList,
                                               const QList<int> &pendingIndices)
{
    // Build a human-readable list: "Button.qml, Card.qml"
    QStringList displayNames;
    for (int i = 0; i < toolNames.size(); ++i) {
        const QString fp = argsList.at(i).value("filepath").toString();
        displayNames.append(fp.isEmpty() ? toolNames.at(i) : QFileInfo(fp).fileName());
    }

    const QString summary = displayNames.size() == 1
                                ? tr("AI wants to delete \"%1\". Proceed?").arg(displayNames.first())
                                : tr("AI wants to delete %1 files: %2. Proceed?")
                                      .arg(displayNames.size())
                                      .arg(displayNames.join(", "));
     auto *msg = new ChatMessage(ChatMessage::ToolCallConfirmation, summary, this);
    // Store the full batch on the first toolName slot; QML reads pendingIndices as a list.
    msg->setToolInfo({}, toolNames.join(','));
    msg->setPendingIndices(pendingIndices);

    beginInsertRows(QModelIndex(), m_messages.size(), m_messages.size());
    m_messages.append(msg);
    endInsertRows();
}

void ChatHistoryModel::resolveToolCallConfirmation(const QList<int> &pendingIndices, bool confirmed)
{
    // Find the confirmation message that owns this batch (any one of the indices matches).
    for (int i = m_messages.size() - 1; i >= 0; --i) {
        ChatMessage *msg = m_messages.at(i);
        if (msg->type() != ChatMessage::ToolCallConfirmation)
            continue;

        const QList<int> stored = msg->pendingIndices();

        // Check for overlap — the batch stored on the message contains these indices.
        for (int idx : pendingIndices) {
            if (stored.contains(idx)) {
               msg->resolve(confirmed);
               const QModelIndex midx = index(i);
               emit dataChanged(midx, midx, {Resolved, ContentRole});
               return;
            }
        }
    }
}

void ChatHistoryModel::addSystemMessage(const QString &content)
{
    beginInsertRows(QModelIndex(), m_messages.size(), m_messages.size());
    m_messages.append(new ChatMessage(ChatMessage::SystemMessage, content, this));
    endInsertRows();
}

void ChatHistoryModel::addIterationMessage(int current, int max)
{
    beginInsertRows(QModelIndex(), m_messages.size(), m_messages.size());
    m_messages.append(new ChatMessage(ChatMessage::IterationMessage,
                                     QString("Iteration %1/%2").arg(current).arg(max), this));
    endInsertRows();
}

QString ChatHistoryModel::lastUserPrompt() const
{
    for (int i = m_messages.size() - 1; i >= 0; --i) {
        ChatMessage *msg = m_messages.at(i);
        if (msg->type() == ChatMessage::UserMessage)
            return msg->content();
    }

    return {};
}

} // namespace QmlDesigner
