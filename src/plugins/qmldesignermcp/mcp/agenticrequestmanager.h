// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "aiproviderconfig.h"

#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QPointer>

QT_FORWARD_DECLARE_CLASS(QNetworkAccessManager)
QT_FORWARD_DECLARE_CLASS(QNetworkReply)
QT_FORWARD_DECLARE_CLASS(QUrl)

namespace QmlDesigner {

class AiApiAdapter;
class AiResponse;
class ConversationManager;
class McpHost;
class ToolRegistry;

/**
 * @brief Request data for AI assistant
 */
struct RequestData {
    QString instructions;
    QString userPrompt;
    QString currentQml;
    QString currentFilePath; // e.g. "Main.ui.qml"
    QString projectStructure; // plain-text file tree from qmlproject://structure resource
    QUrl attachedImageUrl;

    // Configuration
    int maxIterations = 50;
    int maxTokens = 32768;
};

/**
 * @brief Tool call request from LLM
 */
struct ToolCall {
    QString id;           // Unique tool call id
    QString toolName;     // Name of the tool
    QString serverName;   // MCP server
    QJsonObject arguments;
};

/**
 * @brief Tool execution result
 */
struct ToolResult {
    QString toolCallId;   // Matches ToolCall::id
    QString toolName;
    bool success;
    QString content;      // Result content (JSON or text)
    QString error;        // Error message if failed
};

/**
 * @brief Orchestrates the MCP agentic loop
 *
 * - Manages multi-turn conversations with LLM
 * - Executes tool calls via MCP
 * - Handles iteration control
 * - Coordinates all components via dependency injection
 */
class AgenticRequestManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @param mcpHost MCP server manager
     * @param toolRegistry Tool discovery and caching
     * @param conversation Conversation history manager
     * @param parent Parent QObject
     */
    explicit AgenticRequestManager(
        McpHost *mcpHost,
        ToolRegistry *toolRegistry,
        ConversationManager *conversation,
        QObject *parent = nullptr);

    ~AgenticRequestManager();

    /**
     * @brief Start an agentic request
     *
     * Kicks off the multi-turn conversation loop.
     */
    void request(const RequestData &data, const AiModelInfo &modelInfo);

    /**
     * @brief Cancel the current request
     */
    void cancel();

    /**
     * @brief Check if a request is currently running
     */
    bool isRunning() const { return m_isRunning; }

    /**
     * @brief Clear apdaters states
     */
    void clearAdapters();

signals:
    // Lifecycle
    void started();
    void finished();

    // Results
    void responseReady(const QmlDesigner::AiResponse &response);
    void errorOccurred(const QString &error);

    // Progress
    void iterationStarted(int iteration, int maxIterations);
    void toolCallStarted(const QString &serverName, const QString &toolName, const QJsonObject &arguments);
    void toolCallFinished(const QString &serverName, const QString &toolName, bool success);

    // Emitted when the LLM produces a text block alongside tool calls
    // e.g. "I'll read the file first to understand its current state."
    // Not emitted for the final response (responseReady covers that case).
    void toolCallTextReady(const QString &text);

    // Logging/debugging
    void logMessage(const QString &message);

private slots:
    void onNetworkReplyFinished();
    void onToolCallSucceeded(const QString &serverName, const QJsonObject &result, qint64 requestId);
    void onToolCallFailed(const QString &serverName, const QString &message,
                          const QJsonObject &errorObj, qint64 requestId);

private:
    // Core loop methods
    void executeNextIteration();
    void sendLlmRequest();
    void handleLlmResponse(const QByteArray &responseData);
    void executeToolCalls(const QList<ToolCall> &calls);
    void onAllToolCallsCompleted();
    void finishWithResponse(const AiResponse &response);
    void finishWithError(const QString &error);

    // Retry logic
    void retryLastRequest();

    // Helper to get appropriate API adapter for provider
    AiApiAdapter *adapterForProvider(const QUrl &url) const;

    McpHost *m_mcpHost = nullptr;
    ToolRegistry *m_toolRegistry = nullptr;
    ConversationManager *m_conversation = nullptr;

    QNetworkAccessManager *m_network = nullptr;
    QList<AiApiAdapter *> m_adapters;

    // Current request state
    RequestData m_currentRequestData;
    AiModelInfo m_currentModelInfo;
    int m_currentIteration = 0;
    bool m_isRunning = false;

    // Network state
    QPointer<QNetworkReply> m_currentNetworkReply;
    QByteArray m_lastLlmResponse;
    QByteArray m_lastRequestContent; // For retry

    // Retry configuration
    int m_retryCount = 0;
    int m_maxRetries = 2;
    int m_retryDelayMs = 1000;

    // Tool call tracking
    struct PendingToolCall {
        ToolCall call;
        qint64 requestId;
        bool completed = false;
        ToolResult result;
    };
    QList<PendingToolCall> m_pendingToolCalls;

    // Statistics
    QElapsedTimer m_requestTimer;
};

} // namespace QmlDesigner
