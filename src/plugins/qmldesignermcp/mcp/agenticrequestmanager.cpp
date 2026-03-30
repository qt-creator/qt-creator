// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "agenticrequestmanager.h"

#include "aiapiadapter.h"
#include "aiassistantconstants.h"
#include "airesponse.h"
#include "claudeapiadapter.h"
#include "conversationmanager.h"
#include "geminiapiadapter.h"
#include "mcphost.h"
#include "openaiapiresponseadapter.h"
#include "toolregistry.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>

namespace QmlDesigner {

AgenticRequestManager::AgenticRequestManager(
    McpHost *mcpHost,
    ToolRegistry *toolRegistry,
    ConversationManager *conversation,
    QObject *parent)
    : QObject(parent)
    , m_mcpHost(mcpHost)
    , m_toolRegistry(toolRegistry)
    , m_conversation(conversation)
    , m_network(new QNetworkAccessManager(this))
{
    // Create API adapters for different providers
    m_adapters << new ClaudeApiAdapter();
    m_adapters << new GeminiApiAdapter();
    m_adapters << new OpenAiResponseApiAdapter();

    // Connect to MCP host signals for tool execution results
    connect(m_mcpHost, &McpHost::toolCallSucceeded, this, &AgenticRequestManager::onToolCallSucceeded);
    connect(m_mcpHost, &McpHost::toolCallFailed, this, &AgenticRequestManager::onToolCallFailed);
    connect(m_mcpHost, &McpHost::errorOccurred,
            this, [this](const QString &serverName, const QString &message) {
        emit logMessage(QString("[MCP Error] %1: %2").arg(serverName, message));
    });
}

AgenticRequestManager::~AgenticRequestManager()
{
    qDeleteAll(m_adapters);
    m_adapters.clear();
}

void AgenticRequestManager::request(const RequestData &data, const AiModelInfo &modelInfo)
{
    if (m_isRunning) {
        emit logMessage("Warning: Request already running, cancelling previous");
        cancel();
    }

    m_currentRequestData = data;
    m_currentModelInfo = modelInfo;
    m_currentIteration = 0;
    m_retryCount = 0;
    m_isRunning = true;

    AiApiAdapter *adapter = adapterForProvider(modelInfo.url);
    if (!adapter) {
        finishWithError("Invalid model configuration: no adapter found");
        return;
    }

    // Add initial user message
    m_conversation->addUserMessage(
        adapter->buildUserMessage(QString("Current QML:\n```qml\n%1\n```\n\nRequest: %2")
                                      .arg(data.currentQml, data.userPrompt),
                                  data.attachedImageUrl));

    m_requestTimer.start();
    emit started();
    emit logMessage(QString("Starting agentic request (max %1 iterations)")
                   .arg(m_currentRequestData.maxIterations));

    executeNextIteration();
}

void AgenticRequestManager::cancel()
{
    if (!m_isRunning)
        return;

    if (m_currentNetworkReply) {
        QNetworkReply *reply = m_currentNetworkReply;
        m_currentNetworkReply = nullptr;  // clear first, before abort() fires finished()
        reply->abort();
        reply->deleteLater();
    }

    QList<int> existingConfirmations;
    for (int i = 0; i < m_pendingToolCalls.size(); ++i) {
        if (m_pendingToolCalls.at(i).awaitingConfirmation)
            existingConfirmations.append(i);
    }
    if (!existingConfirmations.isEmpty())
        emit confirmationsCancelled(existingConfirmations);

    m_pendingToolCalls.clear();
    m_isRunning = false;

    emit logMessage("Request cancelled");
    emit finished();
}

void AgenticRequestManager::executeNextIteration()
{
    if (!m_isRunning)
        return;

    ++m_currentIteration;

    if (m_currentIteration > m_currentRequestData.maxIterations) {
        finishWithError(QString("Maximum iterations (%1) reached")
                       .arg(m_currentRequestData.maxIterations));
        return;
    }

    emit iterationStarted(m_currentIteration, m_currentRequestData.maxIterations);
    emit logMessage(QString("--- Iteration %1/%2 ---")
                   .arg(m_currentIteration)
                   .arg(m_currentRequestData.maxIterations));

    sendLlmRequest();
}

void AgenticRequestManager::sendLlmRequest()
{
    AiApiAdapter *adapter = adapterForProvider(m_currentModelInfo.url);
    if (!adapter) {
        finishWithError(QString("No adapter for provider: %1").arg(m_currentModelInfo.provider));
        return;
    }

    // Prepare instructions with project context before passing to adapter
    RequestData reqData = m_currentRequestData;
    if (!reqData.projectStructure.isEmpty()) {
        reqData.instructions += QString("\n\n<project_structure>\n%1\n</project_structure>")
                                    .arg(reqData.projectStructure);
    }
    if (!reqData.currentFilePath.isEmpty()) {
        reqData.instructions += QString("\n\n<current_file>\n%1\n</current_file>")
                                    .arg(reqData.currentFilePath);
    }

    // Create request
    const QUrl resolvedUrl = adapter->resolveUrl(m_currentModelInfo.url, m_currentModelInfo);
    QNetworkRequest networkRequest(resolvedUrl);
    adapter->setRequestHeader(&networkRequest, m_currentModelInfo);

    m_lastRequestContent = adapter->createRequest(
        reqData,
        m_currentModelInfo,
        m_toolRegistry->enabledToolEntries(),
        m_conversation->turns());

    // TODO: remove. Needed for now for debugging
    QJsonDocument doc = QJsonDocument::fromJson(m_lastRequestContent);
    QJsonObject obj = doc.object();
    obj.remove("tools");
    obj.remove("system_instruction");
    doc.setObject(obj);
    qDebug().noquote()
        << "\x1b[42m \x1b[1m" << __FUNCTION__
        << ", m_lastRequestContent="
        << doc.toJson(QJsonDocument::Indented)
        << "\x1b[m";

    emit logMessage(QString("Sending LLM request (%1 bytes)").arg(m_lastRequestContent.size()));

    m_currentNetworkReply = m_network->post(networkRequest, m_lastRequestContent);
    connect(m_currentNetworkReply, &QNetworkReply::finished,
            this, &AgenticRequestManager::onNetworkReplyFinished);
}

void AgenticRequestManager::onNetworkReplyFinished()
{
    if (!m_currentNetworkReply)
        return;

    QNetworkReply *reply = m_currentNetworkReply;
    m_currentNetworkReply = nullptr;

    const QByteArray responseData = reply->readAll();
    reply->deleteLater();

    if (reply->error() != QNetworkReply::NoError) {
        // Extract error message from the response body if exists
        QString error;
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        if (!doc.isNull() && doc.isObject())
            error = doc.object().value("error").toObject().value("message").toString();

        // Fall back to Qt's generic reply->errorString()
        if (error.isEmpty())
            error = reply->errorString();

        // Retry
        if (m_retryCount < m_maxRetries) {
            ++m_retryCount;
            emit logMessage(QString("Network error, retrying %1/%2: %3")
                          .arg(m_retryCount).arg(m_maxRetries).arg(error));

            // Retry with exponential backoff
            int delay = m_retryDelayMs * (1 << (m_retryCount - 1));
            QTimer::singleShot(delay, this, &AgenticRequestManager::retryLastRequest);
            return;
        }

        finishWithError(error);
        return;
    }

    // Success - reset retry counter
    m_retryCount = 0;

    emit logMessage(QString("Received LLM response (%1 bytes)").arg(responseData.size()));

    handleLlmResponse(responseData);
}

void AgenticRequestManager::handleLlmResponse(const QByteArray &responseData)
{
    // TODO: remove. Needed for now for debugging
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    QJsonObject obj = doc.object();
    obj.remove("tools");
    obj.remove("system_instruction");
    doc.setObject(obj);
    qDebug().noquote() << "\x1b[42m \x1b[1m" << __FUNCTION__
             << ", responseData=" << doc.toJson(QJsonDocument::Indented)
             << "\x1b[m";

    m_lastLlmResponse = responseData;

    AiApiAdapter *adapter = adapterForProvider(m_currentModelInfo.url);
    if (!adapter) {
        finishWithError("No adapter available");
        return;
    }

    // Parse for tool calls
    QList<ToolCall> toolCalls = adapter->parseToolCalls(responseData);

    if (toolCalls.isEmpty()) {
        // No tool calls - check if response is complete
        if (adapter->isResponseComplete(responseData)) {
            // We're done! Parse the final response
            AiResponse finalResponse = adapter->interpretResponse(responseData);
            finishWithResponse(finalResponse);
        } else {
            // Not complete but no tool calls - error state
            finishWithError("LLM response incomplete but no tool calls requested");
        }
        return;
    }

    emit logMessage(QString("LLM requested %1 tool call(s)").arg(toolCalls.size()));

    const QString thinkingText = adapter->extractText(responseData).trimmed();
    if (!thinkingText.isEmpty())
        emit toolCallTextReady(thinkingText);

    // Add assistant message to history
    const QJsonArray assistantItems = adapter->buildAssistantTurn(responseData);
    if (!assistantItems.isEmpty())
        m_conversation->addAssistantMessage(assistantItems);

    // Execute tool calls
    executeToolCalls(toolCalls);
}

void AgenticRequestManager::executeToolCalls(const QList<ToolCall> &calls)
{
    m_pendingToolCalls.clear();

    bool allCompleted = true;

    // Collect all destructive calls first so we can emit one batched signal.
    QStringList confirmServerNames;
    QStringList confirmToolNames;
    QList<QJsonObject> confirmArgs;
    QList<int> confirmIndices;

    for (const ToolCall &call : calls) {
        PendingToolCall pending;
        pending.call = call;
        pending.completed = false;

        // Find server for this tool
        QString serverName = call.serverName;
        if (serverName.isEmpty()) {
            serverName = m_toolRegistry->findServerForTool(call.toolName);
            if (serverName.isEmpty()) {
                emit logMessage(QString("Warning: No server found for tool '%1'")
                              .arg(call.toolName));
                pending.completed = true;
                pending.result.toolCallId = call.id;
                pending.result.toolName = call.toolName;
                pending.result.success = false;
                pending.result.error = QString("No MCP server provides tool '%1'")
                                              .arg(call.toolName);
                m_pendingToolCalls.append(pending);
                continue;
            }
        }

        // Destructive tools are parked until the user explicitly approves them.
        if (Constants::toolsRequiringConfirmation.contains(call.toolName)) {
            pending.awaitingConfirmation = true;
            m_pendingToolCalls.append(pending);
            allCompleted = false;
            confirmServerNames.append(serverName);
            confirmToolNames.append(call.toolName);
            confirmArgs.append(call.arguments);
            confirmIndices.append(m_pendingToolCalls.size() - 1);
            continue;
        }

        emit toolCallStarted(serverName, call.toolName, call.arguments);
        emit logMessage(QString("Calling tool '%1.%2'").arg(serverName, call.toolName));

        // Execute the tool call
        qint64 requestId = m_mcpHost->callTool(serverName, call.toolName, call.arguments);

        if (requestId < 0) {
            // Immediate error
            pending.completed = true;
            pending.result.toolCallId = call.id;
            pending.result.toolName = call.toolName;
            pending.result.success = false;
            pending.result.error = "Failed to initiate tool call";
            emit toolCallFinished(serverName, call.toolName, false);
        } else {
            pending.requestId = requestId;
            allCompleted = false;
        }

        m_pendingToolCalls.append(pending);
    }

    if (!confirmIndices.isEmpty())
        emit confirmationRequired(confirmServerNames, confirmToolNames, confirmArgs, confirmIndices);

    if (allCompleted)
        onAllToolCallsCompleted();
}

void AgenticRequestManager::confirmToolsCall(const QList<int> &pendingIndices, bool confirmed)
{
    for (int idx : pendingIndices)
        confirmToolCall(idx, confirmed);
}

void AgenticRequestManager::confirmToolCall(int pendingIndex, bool confirmed)
{
    if (pendingIndex < 0 || pendingIndex >= m_pendingToolCalls.size()) {
        emit logMessage(QString("confirmToolCall: invalid index %1").arg(pendingIndex));
        return;
    }

    PendingToolCall &pending = m_pendingToolCalls[pendingIndex];

    if (!pending.awaitingConfirmation) {
        emit logMessage(QString("confirmToolCall: index %1 is not awaiting confirmation")
                            .arg(pendingIndex));
        return;
    }

    pending.awaitingConfirmation = false;

    const QString serverName = pending.call.serverName.isEmpty()
                                   ? m_toolRegistry->findServerForTool(pending.call.toolName)
                                   : pending.call.serverName;

    if (!confirmed) {
        pending.completed = true;
        pending.result.toolCallId = pending.call.id;
        pending.result.toolName = pending.call.toolName;
        pending.result.success = false;
        pending.result.error = QString("User declined to execute '%1'").arg(pending.call.toolName);
        emit logMessage(QString("Tool '%1' declined by user").arg(pending.call.toolName));
    } else {
        emit toolCallStarted(serverName, pending.call.toolName, pending.call.arguments);
        emit logMessage(QString("Tool '%1.%2' confirmed, executing")
                            .arg(serverName, pending.call.toolName));

        const qint64 requestId = m_mcpHost->callTool(serverName, pending.call.toolName,
                                                     pending.call.arguments);
        if (requestId < 0) {
            pending.completed = true;
            pending.result.toolCallId = pending.call.id;
            pending.result.toolName = pending.call.toolName;
            pending.result.success = false;
            pending.result.error = QString("Failed to initiate tool call");
            emit toolCallFinished(serverName, pending.call.toolName, false);
        } else {
            pending.requestId = requestId;
            // Tool call ongoing — onToolCallSucceeded/Failed will handle the rest.
            return;
        }
    }

    // Reach here only if the call resolved synchronously (denied or immediate error).
    // Check whether all pending calls are now done.
    const bool allCompleted = std::all_of(m_pendingToolCalls.cbegin(), m_pendingToolCalls.cend(),
                                          [](const PendingToolCall &p) { return p.completed; });
    if (allCompleted)
        onAllToolCallsCompleted();
}

void AgenticRequestManager::onToolCallSucceeded(
    const QString &serverName, const QJsonObject &result, qint64 requestId)
{
    // Find the pending tool call with this request ID
    for (auto &pending : m_pendingToolCalls) {
        if (pending.requestId == requestId && !pending.completed) {
            pending.completed = true;
            pending.result.toolCallId = pending.call.id;
            pending.result.toolName = pending.call.toolName;
            pending.result.success = true;

            // Extract content from result
            // MCP tools return content array with text blocks
            QJsonArray contentArray = result.value("content").toArray();
            if (!contentArray.isEmpty()) {
                QJsonObject firstContent = contentArray.first().toObject();
                pending.result.content = firstContent.value("text").toString();
            } else {
                // Fallback: serialize entire result
                pending.result.content = QString::fromUtf8(
                    QJsonDocument(result).toJson(QJsonDocument::Compact));
            }

            emit toolCallFinished(serverName, pending.call.toolName, true);
            emit logMessage(QString("Tool '%1' succeeded").arg(pending.call.toolName));

            // Check if all are done
            bool allCompleted = true;
            for (const auto &p : std::as_const(m_pendingToolCalls)) {
                if (!p.completed) {
                    allCompleted = false;
                    break;
                }
            }

            if (allCompleted)
                onAllToolCallsCompleted();

            return;
        }
    }
}

void AgenticRequestManager::onToolCallFailed(
    const QString &serverName,
    const QString &message,
    const QJsonObject &,
    qint64 requestId)
{
    // Find the pending tool call with this request ID
    for (auto &pending : m_pendingToolCalls) {
        if (pending.requestId == requestId && !pending.completed) {
            pending.completed = true;
            pending.result.toolCallId = pending.call.id;
            pending.result.toolName = pending.call.toolName;
            pending.result.success = false;

            // Provide detailed error message for LLM
            QString detailedError = QString(
                "Tool '%1' on server '%2' failed: %3\n"
                "Arguments: %4")
                .arg(pending.call.toolName,
                     serverName,
                     message,
                     QString::fromUtf8(QJsonDocument(pending.call.arguments)
                          .toJson(QJsonDocument::Compact)));

            pending.result.error = detailedError;

            emit toolCallFinished(serverName, pending.call.toolName, false);
            emit logMessage(QString("Tool '%1' failed: %2")
                          .arg(pending.call.toolName, message));

            // Check if all are done
            bool allCompleted = true;
            for (const auto &p : std::as_const(m_pendingToolCalls)) {
                if (!p.completed) {
                    allCompleted = false;
                    break;
                }
            }

            if (allCompleted)
                onAllToolCallsCompleted();

            return;
        }
    }
}

void AgenticRequestManager::onAllToolCallsCompleted()
{
    emit logMessage("All tool calls completed, continuing conversation");

    AiApiAdapter *adapter = adapterForProvider(m_currentModelInfo.url);
    if (!adapter) {
        finishWithError("No adapter available when storing tool results");
        return;
    }

    // Collect tool results
    QList<ToolResult> results;
    for (const auto &pending : std::as_const(m_pendingToolCalls))
        results.append(pending.result);

    // Add tool results to conversation history
    const QJsonArray toolItems = adapter->buildToolResultsTurn(results);
    if (!toolItems.isEmpty())
        m_conversation->addToolResultsMessage(toolItems);

    // Continue to next iteration
    executeNextIteration();
}

void AgenticRequestManager::finishWithResponse(const AiResponse &response)
{
    qint64 totalMs = m_requestTimer.elapsed();

    emit logMessage(QString("Agentic request completed successfully after %1 iteration(s), %2ms")
                   .arg(m_currentIteration).arg(totalMs));

    m_isRunning = false;
    emit responseReady(response);
    emit finished();
}

void AgenticRequestManager::finishWithError(const QString &error)
{
    emit logMessage(QString("Agentic request failed: %1").arg(error));

    m_isRunning = false;
    emit errorOccurred(error);
    emit finished();
}

void AgenticRequestManager::retryLastRequest()
{
    if (m_lastRequestContent.isEmpty()) {
        finishWithError("Cannot retry: no saved request");
        return;
    }

    AiApiAdapter *adapter = adapterForProvider(m_currentModelInfo.url);
    if (!adapter) {
        finishWithError("No adapter available for retry");
        return;
    }

    const QUrl resolvedUrl = adapter->resolveUrl(m_currentModelInfo.url, m_currentModelInfo);
    QNetworkRequest networkRequest(resolvedUrl);
    adapter->setRequestHeader(&networkRequest, m_currentModelInfo);

    emit logMessage("Retrying last request...");

    m_currentNetworkReply = m_network->post(networkRequest, m_lastRequestContent);
    connect(m_currentNetworkReply, &QNetworkReply::finished,
            this, &AgenticRequestManager::onNetworkReplyFinished);
}

AiApiAdapter *AgenticRequestManager::adapterForProvider(const QString &url) const
{
    for (AiApiAdapter *adapter : std::as_const(m_adapters)) {
        if (adapter->accepts(url))
            return adapter;
    }

    return nullptr;
}

void AgenticRequestManager::clearAdapters()
{
    for (AiApiAdapter *adapter : std::as_const(m_adapters))
        adapter->clear();
}

bool AgenticRequestManager::hasPendingConfirmations() const
{
    return std::any_of(m_pendingToolCalls.cbegin(), m_pendingToolCalls.cend(),
                       [](const PendingToolCall &p) { return p.awaitingConfirmation; });
}

} // namespace QmlDesigner
