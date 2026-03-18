// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "mcpserver_global.h"

#include "../schemas/schema_2025_11_25.h"

class QTcpServer;

namespace Mcp {

namespace Schema = Generated::Schema::_2025_11_25;

class ServerPrivate;
struct Responder;

template<typename T>
concept IsDuration = requires(T duration)
{
    std::chrono::duration_cast<std::chrono::milliseconds>(duration);
};

template<IsDuration DurationType>
void letTaskDieIn(Schema::Task &task, DurationType dieIn)
{
    using namespace std::chrono_literals;
    const QDateTime createdAt = QDateTime::fromString(task.createdAt(), Qt::ISODate);
    const QDateTime now = QDateTime::currentDateTime();
    const std::chrono::milliseconds ttl = (now - createdAt) + dieIn;
    task.ttl(ttl.count());
}

class ToolInterface;

/*!
    \class Server
    \inmodule McpServer
    \brief Implements the MCP server for tool, prompt, and resource handling.

    The Server class provides the main entry point for hosting an MCP (Model Context Protocol)
    server. It manages incoming client connections, tool registration and invocation,
    prompt and resource handling, and notification delivery. Tools and prompts can be
    registered dynamically, and resources can be served or generated on demand.

    The server supports both HTTP and custom IO streams, and can be configured to allow
    cross-origin requests for browser-based clients. Notifications and completions can be
    sent to connected clients at any time.

    \sa ToolInterface, addTool(), addPrompt(), addResource()
*/
class MCPSERVER_EXPORT Server
{
public:
    /*! \brief Constructs a new MCP server instance.
        \param serverInfo The implementation metadata to advertise to clients.
    */
    Server(Schema::Implementation serverInfo);

    /*! \brief Destroys the server and releases all resources. */
    ~Server();

    /*! \brief Binds the server to the given QTcpServer for incoming connections.
        \param server The QTcpServer to bind to.
        \return true if binding succeeded, false otherwise.

        \note If successful, the QTcpServer will be parented to this server.
    */
    bool bind(QTcpServer *server);

    /*! \brief Returns the list of all currently bound QTcpServer instances. */
    QList<QTcpServer *> boundTcpServers() const;

    /*! \brief Enables or disables Cross-Origin Resource Sharing (CORS).
        \param enabled If true, allows browser-based clients to connect from other origins.
    */
    void setCorsEnabled(bool enabled);

    /*! \brief Binds the server to custom IO streams for manual JSONRPC message handling.
        \param outputHandler Function to handle outgoing data.
        \return A function to handle incoming data, or an error.
    */
    Utils::Result<std::function<void(QByteArray)>> bindIO(
        std::function<void(QByteArray)> outputHandler);

    /*! \brief Sends a notification to the specified client session.
        \param notification The notification to send.
        \param sessionId The session ID to target, or empty for broadcast.
    */
    void sendNotification(
        const Schema::ServerNotification &notification, const QString &sessionId = {});

    /*! \brief Registers a tool with a callback that receives the ToolInterface.
        \param tool The tool metadata.
        \param callback The callback to invoke for tool calls.
    */
    using ToolInterfaceCallback = std::function<
        Utils::Result<>(const Schema::CallToolRequestParams &, const ToolInterface &)>;
    void addTool(const Schema::Tool &tool, const ToolInterfaceCallback &callback);

    /*! \brief Registers a tool with a simple callback.
        \param tool The tool metadata.
        \param callback The callback to invoke for tool calls.
    */
    using ToolCallback
        = std::function<Utils::Result<Schema::CallToolResult>(const Schema::CallToolRequestParams &)>;
    void addTool(const Schema::Tool &tool, const ToolCallback &callback);

    /*! \brief Removes a previously registered tool by name.
        \param toolName The name of the tool to remove.
    */
    void removeTool(const QString &toolName);

    using PromptArguments = QMap<QString, QString>;
    using PromptMessageList = QList<Schema::PromptMessage>;
    using PromptCallback = std::function<PromptMessageList(PromptArguments)>;

    /*! \brief Registers a prompt with a callback.
        \param prompt The prompt metadata.
        \param callback The callback to invoke for prompt requests.
    */
    void addPrompt(const Schema::Prompt &prompt, const PromptCallback &callback);

    /*! \brief Removes a previously registered prompt by name.
        \param promptName The name of the prompt to remove.
    */
    void removePrompt(const QString &promptName);

    using ResourceCallback = std::function<Utils::Result<Schema::ReadResourceResult>(
        const Schema::ReadResourceRequestParams &)>;

    /*! \brief Registers a resource with a callback.
        \param resource The resource metadata.
        \param callback The callback to invoke for resource requests.
    */
    void addResource(const Schema::Resource &resource, const ResourceCallback &callback);

    /*! \brief Removes a previously registered resource by URI.
        \param uri The URI of the resource to remove.
    */
    void removeResource(const QString &uri);

    /*! \brief Sets a fallback callback for resource requests not matched by any registered resource.
        \param callback The fallback callback.
    */
    void setResourceFallbackCallback(const ResourceCallback &callback);

    /*! \brief Registers a resource template.
        \param resourceTemplate The resource template metadata.
    */
    void addResourceTemplate(const Schema::ResourceTemplate &resourceTemplate);

    /*! \brief Removes a previously registered resource template by name.
        \param name The name of the resource template to remove.
    */
    void removeResourceTemplate(const QString &name);

    using CompletionResultCallback = std::function<void(Utils::Result<Schema::CompleteResult>)>;
    using CompletionCallback = std::function<
        void(const Schema::CompleteRequestParams &, const CompletionResultCallback &)>;

    /*! \brief Sets the callback for completion requests.
        \param callback The callback to invoke for completions.
    */
    void setCompletionCallback(const CompletionCallback &callback);

private:
    std::shared_ptr<ServerPrivate> d;
};

struct ToolInterfacePrivate;

/*!
    \class ToolInterface
    \inmodule McpServer
    \brief Provides the interface available to tool implementations during a tool call.

    ToolInterface is passed to tool callbacks registered via Server::addTool() and
    exposes the full set of operations a tool may perform while handling a request.
    This includes sending the final result back to the client, upgrading the
    connection to a long-running task, issuing notifications, and making
    client-directed requests such as elicitation and sampling.

    A ToolInterface instance is valid until either finish() is called or a
    started task is reported as complete. It cannot be default-constructed;
    instances are created internally by the server.

    \sa Server::addTool()
*/
class MCPSERVER_EXPORT ToolInterface
{
    friend class ServerPrivate;

public:
    using UpdateTaskCallback = std::function<Schema::Task(Schema::Task)>;
    using TaskResultCallback = std::function<Utils::Result<Schema::CallToolResult>()>;
    using CancelTaskCallback = std::function<void()>;
    using TaskProgressNotify = std::function<void(
        Schema::TaskStatus status,
        const std::optional<QString> &statusMessage,
        const std::optional<int> &ttl)>;
    using SampleResultCallback
        = std::function<void(const Utils::Result<Schema::CreateMessageResult> &)>;
    using ElicitResultCallback = std::function<void(const Utils::Result<Schema::ElicitResult> &)>;

    ToolInterface() = delete;
    ~ToolInterface();

    /*!
        \brief Returns the capabilities reported by the connected client.
    */
    const Schema::ClientCapabilities &clientCapabilities() const;

    /*!
        \brief Sends the tool call result to the client and closes the connection.

        Use this to return the final result of a synchronous (non-task-based)
        tool invocation.

        \param result The result of the tool call, or an error.
    */
    void finish(const Utils::Result<Schema::CallToolResult> &result) const;

    /*!
        \brief Upgrades the connection to a long-running task with a time-to-live.

        Starts a long-running task by sending a \c CreateTaskResult to the client and
        closing the initial connection. The task can then be polled, updated, and
        cancelled independently. The \a ttl parameter specifies how long the task
        should be kept alive after creation before being automatically deleted; it is
        converted to milliseconds internally, so any \c std::chrono duration type
        satisfying the \c IsDuration concept may be used.

        \param pollingInterval The interval at which the client should poll the
               server for task updates, expressed as any \c std::chrono duration
               type satisfying the \c IsDuration concept. It is converted to
               milliseconds internally.
        \param onUpdateTask Callback invoked when the client requests an update on
               the task. It receives the current \c Schema::Task and must return an
               updated \c Schema::Task reflecting the latest progress.
        \param onResultCallback Callback invoked when the client requests the final
               result of the task. It must return the \c Schema::CallToolResult
               representing the completed work, or an error.
        \param onCancelTaskCallback Optional callback invoked when the client
               requests cancellation of the task. If \c std::nullopt, the task
               cannot be cancelled by the client.
        \param ttl The time-to-live for the task, expressed as any
               \c std::chrono duration type. The task will be automatically deleted
               after this duration elapses from its creation time.

        \return A \c Utils::Result containing a \c TaskProgressNotify functor on
                success, which can be called to push intermediate progress
                notifications to the client, or an error on failure.
    */
    Utils::Result<TaskProgressNotify> startTask(
        IsDuration auto pollingInterval,
        const UpdateTaskCallback &onUpdateTask,
        const TaskResultCallback &onResultCallback,
        const std::optional<CancelTaskCallback> &onCancelTaskCallback,
        IsDuration auto ttl) const
    {
        return startTask(
            std::chrono::duration_cast<std::chrono::milliseconds>(pollingInterval).count(),
            onUpdateTask,
            onResultCallback,
            onCancelTaskCallback,
            std::chrono::duration_cast<std::chrono::milliseconds>(ttl).count());
    }

    /*!
        \overload

        Starts a long-running task without a time-to-live, so the task will not
        be automatically deleted.

        \sa startTask(IsDuration, const UpdateTaskCallback &, const TaskResultCallback &,
                     const std::optional<CancelTaskCallback> &, IsDuration)
    */
    Utils::Result<TaskProgressNotify> startTask(
        IsDuration auto pollingInterval,
        const UpdateTaskCallback &onUpdateTask,
        const TaskResultCallback &onResultCallback,
        const std::optional<CancelTaskCallback> &onCancelTaskCallback) const
    {
        return startTask(
            std::chrono::duration_cast<std::chrono::milliseconds>(pollingInterval).count(),
            onUpdateTask,
            onResultCallback,
            onCancelTaskCallback,
            std::nullopt);
    }

    /*!
        \overload

        Starts a long-running task without a time-to-live and without a polling interval.
        The client will not poll for updates, and it is the responsibility of the server to push
        updates to the client using the returned \c TaskProgressNotify functor.

        \sa startTask(IsDuration, const UpdateTaskCallback &, const TaskResultCallback &,
                     const std::optional<CancelTaskCallback> &, IsDuration)
    */
    Utils::Result<TaskProgressNotify> startTask(
        const UpdateTaskCallback &onUpdateTask,
        const TaskResultCallback &onResultCallback,
        const std::optional<CancelTaskCallback> &onCancelTaskCallback) const
    {
        return startTask(
            std::nullopt, onUpdateTask, onResultCallback, onCancelTaskCallback, std::nullopt);
    }

    /*!
        \overload

        Starts a long-running task with a time-to-live and without a polling interval.
        The client will not poll for updates, and it is the responsibility of the server to push
        updates to the client using the returned \c TaskProgressNotify functor.

        \sa startTask(IsDuration, const UpdateTaskCallback &, const TaskResultCallback &,
                     const std::optional<CancelTaskCallback> &, IsDuration)
    */
    Utils::Result<TaskProgressNotify> startTask(
        const UpdateTaskCallback &onUpdateTask,
        const TaskResultCallback &onResultCallback,
        const std::optional<CancelTaskCallback> &onCancelTaskCallback,
        IsDuration auto ttl) const
    {
        return startTask(
            std::nullopt,
            onUpdateTask,
            onResultCallback,
            onCancelTaskCallback,
            std::chrono::duration_cast<std::chrono::milliseconds>(ttl).count());
    }

    /*!
        \brief Sends an elicitation request to the client.

        Asks the client to elicit additional information from the user according
        to the given \a params. The result is delivered asynchronously via \a cb.

        \note This function will fail if the tool is already finished (i.e., if finish() has been called or a started task is complete).

        \param params The elicitation request parameters describing what
               information to gather from the user.
        \param cb Callback invoked with the elicitation result or an error.
    */
    void elicit(const Schema::ElicitRequestParams &params, const ElicitResultCallback &cb) const;

    /*!
        \brief Sends a sampling request to the client.

        Asks the client to create a message using its language model according
        to the given \a params. The result is delivered asynchronously via \a cb.

        \note This function will fail if the tool is already finished (i.e., if finish() has been called or a started task is complete).

        \param params The sampling request parameters describing the message to
               create.
        \param cb Callback invoked with the sampling result or an error.
    */
    void sample(
        const Schema::CreateMessageRequestParams &params, const SampleResultCallback &cb) const;

    /*!
        \brief Sends a server notification to the client.

        \note This function will fail if the tool is already finished (i.e., if finish() has been called or a started task is complete).

        \param notification The notification to send to the client.
    */
    void notify(const Schema::ServerNotification &notification) const;

protected:
    ToolInterface(
        std::weak_ptr<ServerPrivate> serverPrivate,
        const Schema::ClientCapabilities &clientCaps,
        const Schema::CallToolRequest &request,
        const QString &sessionId,
        const Responder &responder);

    Utils::Result<TaskProgressNotify> startTask(
        std::optional<int> pollingIntervalMs,
        const UpdateTaskCallback &onUpdateTask,
        const TaskResultCallback &onResultCallback,
        const std::optional<CancelTaskCallback> &onCancelTaskCallback,
        std::optional<int> ttlMs) const;

    std::shared_ptr<ToolInterfacePrivate> d;
};

} // namespace Mcp
