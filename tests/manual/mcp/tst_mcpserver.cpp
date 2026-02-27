// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QCoreApplication>
#include <QDeadlineTimer>
#include <QDebug>
#include <QJsonArray>
#include <QSocketNotifier>
#include <QTcpServer>
#include <QTimer>

#include <server/mcpserver.h>

#include <iostream>

using namespace Utils;

Mcp::Server *s_server = nullptr;

static void completion(
    const Mcp::Schema::CompleteRequestParams &request,
    const Mcp::Server::CompletionResultCallback &cb)
{
    if (std::holds_alternative<Mcp::Schema::ResourceTemplateReference>(request.ref())) {
        auto resourceTemplateRef = std::get<Mcp::Schema::ResourceTemplateReference>(request.ref());
        qDebug() << "Received completion request for resource template:"
                 << resourceTemplateRef.uri();

        if (resourceTemplateRef.uri().startsWith("test://")) {
            cb(Mcp::Schema::CompleteResult().completion(
                Mcp::Schema::CompleteResult::Completion().addValue(
                    "Completion result for test resource template: " + resourceTemplateRef.uri())));
            return;
        }
        if (resourceTemplateRef.uri().startsWith("file://")) {
            QString actual = resourceTemplateRef.uri();
            actual.replace("{" + request.argument().name() + "}", request.argument().value());

            if (QString("file:///current_time").startsWith(actual)) {
                cb(Mcp::Schema::CompleteResult().completion(
                    Mcp::Schema::CompleteResult::Completion().addValue("current_time")));
                return;
            }
        }

        cb(Mcp::Schema::CompleteResult());
        return;
    }

    cb(Mcp::Schema::CompleteResult().completion(
        Mcp::Schema::CompleteResult::Completion().addValue(
            "Completion result for non-resource template reference")));
}

static Result<Mcp::Schema::CallToolResult> echoTool(const Mcp::Schema::CallToolRequestParams &params)
{
    if (!params._arguments || !params._arguments->contains("message")
        || !(*params._arguments)["message"].isString()) {
        return ResultError("Invalid arguments for echo tool: missing 'message' string");
    }

    //qDebug() << "Echo tool called with params:" << params._arguments;

    return Mcp::Schema::CallToolResult()
        .isError(false)
        .addStructuredContent("echoedMessage", params.arguments()->value("message"));
}

template<typename T>
Utils::Result<T> as(const auto &variant)
{
    if (!std::holds_alternative<T>(variant))
        return Utils::ResultError("Variant does not hold the expected type");
    return std::get<T>(variant);
}

static Utils::Result<> sampleTool(
    const Mcp::Schema::CallToolRequestParams &, const Mcp::ToolInterface &toolInterface)
{
    toolInterface.sample(
        Mcp::Schema::CreateMessageRequestParams()
            .addMessage(
                Mcp::Schema::SamplingMessage()
                    .content(
                        Mcp::Schema::TextContent().text("Write a function that sums two integers."))
                    .role(Mcp::Schema::Role::assistant))
            .systemPrompt("You are a helpful assistant that writes C++ code.")
            .maxTokens(1000)
            .temperature(7.5),
        [toolInterface](const Utils::Result<Mcp::Schema::CreateMessageResult> &result) {
            if (!result) {
                qDebug().noquote() << "Sample request failed:" << result.error();
                return;
            }

            qDebug() << "Received sample input from client:"
                     << Mcp::Schema::toJsonValue(result->content());

            toolInterface.finish(
                Mcp::Schema::CallToolResult()
                    .isError(false)
                    .addStructuredContent("sampleResponse", QString("Thanks")));
        });

    return ResultOk;
}

static Utils::Result<> elicitTool(
    const Mcp::Schema::CallToolRequestParams &, const Mcp::ToolInterface &toolInterface)
{
    toolInterface.elicit(
        Mcp::Schema::ElicitRequestFormParams()
            .message("Please provide info")
            .requestedSchema(
                Mcp::Schema::ElicitRequestFormParams::RequestedSchema().addProperty(
                    "name",
                    Mcp::Schema::StringSchema().title("String").description(
                        "Your full, legal name"))),
        [toolInterface](const Utils::Result<Mcp::Schema::ElicitResult> &result) {
            if (!result) {
                qDebug() << "Elicit request failed:" << result.error();
                return;
            }
            qDebug() << "Received elicit result from client";

            if (result->action() != Mcp::Schema::ElicitResult::Action::accept) {
                toolInterface.finish(
                    Mcp::Schema::CallToolResult()
                        .isError(true)
                        .addStructuredContent("elicitResponse", "Client rejected the request"));
                return;
            }

            auto nameResult = as<QString>((*result->content())["name"]);
            if (!nameResult) {
                toolInterface.finish(
                    Mcp::Schema::CallToolResult()
                        .isError(true)
                        .addStructuredContent("elicitResponse", "Invalid response from client"));
                return;
            }

            qDebug() << "Received name from elicit result:" << *nameResult;

            toolInterface.finish(
                Mcp::Schema::CallToolResult().isError(false).addStructuredContent(
                    "elicitResponse", QString("Received your response: %1").arg(*nameResult)));
        });

    return ResultOk;
}

static Utils::Result<> asyncEchoTool(
    const Mcp::Schema::CallToolRequestParams &params, const Mcp::ToolInterface &toolInterface)
{
    if (!params.arguments() || !params.arguments()->contains("message")
        || !(*params.arguments())["message"].isString()) {
        return ResultError("Invalid arguments for async echo tool: missing 'message' string");
    }

    //qDebug() << "Async echo tool called with params:" << params._arguments;
    qDebug() << "Progress token:"
             << Mcp::Schema::toJsonValue(params._meta()->progressToken().value_or("null"));

    QJsonValue progressToken;
    if (params._meta() && params._meta()->progressToken()) {
        progressToken = Mcp::Schema::toJsonValue(*params._meta()->progressToken());
    }

    // Simulate some asynchronous work with a single-shot timer
    QTimer *t = new QTimer();
    t->setSingleShot(false);
    t->setInterval(1000);
    t->start();
    QObject::connect(
        t,
        &QTimer::timeout,
        t,
        [progressToken,
         toolInterface,
         msg = params.arguments()->value("message"),
         t,
         count = 0]() mutable {
            if (count == 5) {
                t->stop();
                t->deleteLater();
                qDebug() << "Async echo tool completed, sending final result";
                toolInterface.finish(
                    Mcp::Schema::CallToolResult()
                        .isError(false)
                        .addStructuredContent("echoedMessage", msg));
            } else if (!progressToken.isNull()) {
                qDebug() << "Sending progress notification with token:" << progressToken;
                toolInterface.notify(
                    Mcp::Schema::ProgressNotification().params(
                        Mcp::Schema::ProgressNotificationParams()
                            .progress(count)
                            .total(5)
                            .message("reticulating splines ...")
                            .progressToken(*Mcp::Schema::fromJson<Mcp::Schema::ProgressToken>(
                                progressToken))));
            }
            count++;
        });

    return ResultOk;
}

static Result<> toolTask(
    const Mcp::Schema::CallToolRequestParams &params, const Mcp::ToolInterface &toolInterface)
{
    if (!params.arguments() || !params.arguments()->contains("message")
        || !(*params.arguments())["message"].isString()) {
        return ResultError("Invalid arguments for task echo tool: missing 'message' string");
    }

    QString taskId = QUuid::createUuid().toString();
    QString message = (*params.arguments())["message"].toString();

    using namespace std::literals::chrono_literals;
    struct TaskData
    {
        QDeadlineTimer timer{10s};
        bool cancelled{false};
    };

    std::shared_ptr<TaskData> taskData = std::make_shared<TaskData>();

    auto update = [taskData](Mcp::Schema::Task task) -> Mcp::Schema::Task {
        if (taskData->timer.hasExpired()) {
            task.status(Mcp::Schema::TaskStatus::completed);
            task.statusMessage("Task is done.");
            return task;
        }
        if (taskData->cancelled) {
            task.status(Mcp::Schema::TaskStatus::cancelled);
            task.statusMessage("Task is cancelled");
            return task;
        }

        task.statusMessage(QString("Time remaining: %1 seconds")
                               .arg(
                                   std::chrono::duration_cast<std::chrono::seconds>(
                                       taskData->timer.remainingTimeAsDuration())
                                       .count()));
        return task;
    };
    // Result
    auto result = [message]() -> Result<Mcp::Schema::CallToolResult> {
        return Mcp::Schema::CallToolResult()
            .isError(false)
            .addStructuredContent("echoedMessage", message);
    };
    // Cancel
    auto cancel = [taskData]() { taskData->cancelled = true; };

    toolInterface.startTask(1s, update, result, cancel, 20s);

    return ResultOk;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    Mcp::Server server(
        Mcp::Schema::Implementation()
            .description("A simple Mcp server for testing purposes")
            .name("test-mcp-server")
            .title("Test Mcp Server")
            .version("0.1")
            .websiteUrl("https://www.qt.io"),
        true);

    server.setCorsEnabled(true);

    s_server = &server;

    QTimer *timer = new QTimer();
    QObject::connect(timer, &QTimer::timeout, []() {
        static int count = 0;
        count++;
        s_server->sendNotification(
            Mcp::Schema::LoggingMessageNotification().params(
                Mcp::Schema::LoggingMessageNotificationParams()
                    .level(Mcp::Schema::LoggingLevel::info)
                    .data(QString("Just a ping %1").arg(count))));
    });
    timer->start(5000);

    QTcpServer tcpServer;

    server.setCompletionCallback(completion);

    server.addTool(
        Mcp::Schema::Tool()
            .name("echo")
            .description("A simple tool that echoes back the input message.")
            .title("Echo Tool")
            .inputSchema(
                Mcp::Schema::Tool::InputSchema()
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .required(QStringList{"message"}))
            .outputSchema(
                Mcp::Schema::Tool::OutputSchema()
                    .addProperty("echoedMessage", QJsonObject{{"type", "string"}})
                    .required(QStringList{"echoedMessage"})),
        echoTool);

    server.addTool(
        Mcp::Schema::Tool()
            .name("task_echo")
            .description(
                "A tool that creates a task which echoes back the input message after a delay.")
            .title("Task Echo Tool")
            .execution(
                Mcp::Schema::ToolExecution().taskSupport(
                    Mcp::Schema::ToolExecution::TaskSupport::required))
            .inputSchema(
                Mcp::Schema::Tool::InputSchema()
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .required(QStringList{"message"}))
            .outputSchema(
                Mcp::Schema::Tool::OutputSchema{}
                    .addProperty("echoedMessage", QJsonObject{{"type", "string"}})
                    .required(QStringList{"echoedMessage"})),
        toolTask);

    server.addTool(
        Mcp::Schema::Tool()
            .name("async-echo")
            .description("An asynchronous tool that echoes back the input message after a delay.")
            .title("Async Echo Tool")
            .inputSchema(
                Mcp::Schema::Tool::InputSchema()
                    .addProperty("message", QJsonObject{{"type", "string"}})
                    .required(QStringList{"message"}))
            .outputSchema(
                Mcp::Schema::Tool::OutputSchema()
                    .addProperty("echoedMessage", QJsonObject{{"type", "string"}})
                    .required(QStringList{"echoedMessage"})),
        asyncEchoTool);

    server.addTool(
        Mcp::Schema::Tool()
            .name("elicit_test")
            .description(
                "A tool that elicits additional information from the client before executing.")
            .title("Elicit Test Tool"),
        elicitTool);

    server.addTool(
        Mcp::Schema::Tool()
            .name("sample_test")
            .description("A tool that samples an LLM from the client.")
            .title("Sample Test Tool"),
        sampleTool);

    server.addPrompt(
        Mcp::Schema::Prompt()
            .name("code_review")
            .description("A prompt for reviewing a C++ code snippet and providing feedback.")
            .title("Code Review Prompt")
            .addArgument(
                Mcp::Schema::PromptArgument()
                    .name("codeSnippet")
                    .description("A snippet of code to review")
                    .required(true)
                    .title("Code Snippet")),
        [](const Mcp::Server::PromptArguments &args) -> QList<Mcp::Schema::PromptMessage> {
            return {{
                Mcp::Schema::PromptMessage().content(
                    Mcp::Schema::TextContent().text(
                        QString("Please review this C++ code:\n%1").arg(args["codeSnippet"]))),
            }};
        });

    server.addResourceTemplate(
        Mcp::Schema::ResourceTemplate()
            .name("file")
            .description("A resource template for file contents")
            .uriTemplate("file:///{filename}")
            .mimeType("text/plain"));

    server.addResourceTemplate(
        Mcp::Schema::ResourceTemplate()
            .name("test")
            .description("Test files")
            .uriTemplate("test:///{filename}")
            .mimeType("text/plain"));

    server.addResource(
        Mcp::Schema::Resource()
            .name("current_time")
            .description("A resource that returns the current server time.")
            .uri("file:///current_time")
            .title("Current Time Resource"),
        [](const Mcp::Schema::ReadResourceRequestParams &)
            -> Result<Mcp::Schema::ReadResourceResult> {
            return Mcp::Schema::ReadResourceResult().addContent(
                Mcp::Schema::TextResourceContents()
                    .text(QDateTime::currentDateTime().toString())
                    .uri("file:///current_time")
                    .mimeType("text/plain"));
        });

    if (app.arguments().contains("--stdio")) {
        qDebug() << "Binding to stdio";
        auto bindResult = server.bindIO([](const QByteArray &data) {
            // Write received data to stdout (in a real application, you would likely want to handle this differently)
            std::cout << data.toStdString() << std::endl;
        });
        if (!bindResult) {
            qWarning() << "Failed to bind to stdio:" << bindResult.error();
            return -1;
        }

        qInstallMessageHandler(
            [](QtMsgType type, const QMessageLogContext &ctxt, const QString &msg) {
                Mcp::Schema::LoggingLevel level;
                switch (type) {
                case QtDebugMsg:
                    level = Mcp::Schema::LoggingLevel::debug;
                    break;
                case QtInfoMsg:
                    level = Mcp::Schema::LoggingLevel::info;
                    break;
                case QtWarningMsg:
                    level = Mcp::Schema::LoggingLevel::warning;
                    break;
                case QtCriticalMsg:
                    level = Mcp::Schema::LoggingLevel::error;
                    break;
                case QtFatalMsg:
                    level = Mcp::Schema::LoggingLevel::error;
                    break;
                default:
                    level = Mcp::Schema::LoggingLevel::info;
                }

                s_server->sendNotification(
                    Mcp::Schema::LoggingMessageNotification().params(
                        Mcp::Schema::LoggingMessageNotificationParams()
                            .level(level)
                            .logger(ctxt.category)
                            .data(msg)));
            });

        auto inHandler = bindResult.value();
        QSocketNotifier *stdinNotifier
            = new QSocketNotifier(fileno(stdin), QSocketNotifier::Read, &app);

        QObject::connect(stdinNotifier, &QSocketNotifier::activated, [&stdinNotifier, inHandler](int) {
            QTextStream stdinStream(stdin);
            QString line = stdinStream.readLine();
            qDebug() << "Received line on stdin:" << line;
            if (line.isNull()) {
                // EOF reached, stop the server
                qDebug() << "EOF on stdin, stopping server.";
                stdinNotifier->setEnabled(false);
                return;
            }
            inHandler(line.toUtf8());
        });
    } else {
        if (!tcpServer.listen(QHostAddress::LocalHost, 8249) || !server.bind(&tcpServer)) {
            qWarning() << QCoreApplication::translate(
                "QHttpServerExample", "Server failed to listen on a port.");
            return -1;
        }

        qDebug() << "Listening on" << tcpServer.serverAddress() << ":" << tcpServer.serverPort();
    }

    return app.exec();
}
