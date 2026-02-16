// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "agenticrequestmanager.h" // For ToolResult
#include "aiapiutils.h"
#include "conversationmanager.h"

#include <QFileInfo>
#include <QJsonDocument>

namespace QmlDesigner {

namespace {
QJsonObject toJsonImage(const QUrl &imageUrl)
{
    const QString filePath = imageUrl.toLocalFile();
    QImage image(imageUrl.toLocalFile());

    QFileInfo fileInfo(filePath);
    const QByteArray imageFormat = fileInfo.suffix().toLatin1();

    return {
        {"type", "image"},
        {
            "source",
            QJsonObject{
                {"type", "base64"},
                {"media_type", QString("image/%1").arg(imageFormat)},
                {"data", AiApiUtils::toBase64Image(image, imageFormat)},
                },
            },
        };
}
} // namespace

ConversationManager::ConversationManager() = default;

void ConversationManager::addUserMessage(const QString &text, const QUrl &attachedImagePath)
{
    QJsonArray content;

    content.append(QJsonObject{
        {"type", "text"},
        {"text", text}
    });

    if (!attachedImagePath.isEmpty())
        content.append(toJsonImage(attachedImagePath));

    Turn turn{
        .role = "user",
        .content = content,
        .timestamp = QDateTime::currentDateTime()
    };

    m_turns.append(turn);
    pruneIfNeeded();
}

void ConversationManager::addAssistantMessage(const QJsonArray &content)
{
    Turn turn{
        .role = "assistant",
        .content = content,
        .timestamp = QDateTime::currentDateTime()
    };

    m_turns.append(turn);
    pruneIfNeeded();
}

void ConversationManager::addToolResults(const QList<ToolResult> &results)
{
    QJsonArray content;

    for (const ToolResult &result : results) {
        QJsonObject toolResult{
            {"type", "tool_result"},
            {"tool_use_id", result.toolCallId}
        };

        if (result.success) {
            toolResult["content"] = result.content;
        } else {
            toolResult["is_error"] = true;
            toolResult["content"] = result.error;
        }

        content.append(toolResult);
    }

    Turn turn{
        .role = "user",
        .content = content,
        .timestamp = QDateTime::currentDateTime()
    };

    m_turns.append(turn);
    pruneIfNeeded();
}

QJsonArray ConversationManager::getHistory(int maxTurns) const
{
    QJsonArray history;

    int turnCount = maxTurns > 0 ? qMin(maxTurns, m_turns.size()) : m_turns.size();
    int startIndex = m_turns.size() - turnCount;

    for (int i = startIndex; i < m_turns.size(); ++i) {
        const Turn &turn = m_turns[i];
        history.append(QJsonObject{
            {"role", turn.role},
            {"content", turn.content}
        });
    }

    return history;
}

void ConversationManager::clear()
{
    m_turns.clear();
}

int ConversationManager::estimateTokenCount() const
{
    int total = 0;
    for (const Turn &turn : std::as_const(m_turns))
        total += estimateTurnTokens(turn);

    return total;
}

void ConversationManager::pruneIfNeeded()
{
    if (m_maxTurns > 0 && m_turns.size() > m_maxTurns) {
        // Remove oldest turns
        int toRemove = m_turns.size() - m_maxTurns;
        m_turns.erase(m_turns.begin(), m_turns.begin() + toRemove);
    }
}

int ConversationManager::estimateTurnTokens(const Turn &turn) const
{
    // Rough estimation: ~4 characters per token
    QString jsonStr = QString::fromUtf8(
        QJsonDocument(turn.content).toJson(QJsonDocument::Compact));
    return jsonStr.length() / 4;
}

} // namespace QmlDesigner
