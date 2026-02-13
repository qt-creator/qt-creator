// Copyright (C) 2025 David M. Cotter
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpserver.h"
#include "issuesmanager.h"
#include "utils/qtcassert.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <debugger/debuggerconstants.h>
#include <utils/threadutils.h>

#include <QCoreApplication>
#include <QHostAddress>
#include <QLoggingCategory>
#include <QMetaObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>

// Define logging category for MCP server
Q_LOGGING_CATEGORY(mcpServer, "qtc.mcpserver", QtWarningMsg)

namespace Mcp::Internal {

template<class Callable>
static auto runOnGuiThread(Callable &&c) -> std::invoke_result_t<Callable>
{
    using Ret = std::invoke_result_t<Callable>;

    // Decide which connection type to use.
    Qt::ConnectionType connectionType = Utils::isMainThread() ? Qt::DirectConnection
                                                              : Qt::BlockingQueuedConnection;
    if constexpr (std::is_void_v<Ret>) {
        // No return value – just invoke and wait.
        QMetaObject::invokeMethod(
            QCoreApplication::instance(), std::forward<Callable>(c), connectionType);
    } else {
        // Callable returns something – capture it in a local variable.
        Ret ret{};
        QMetaObject::invokeMethod(QCoreApplication::instance(), [&] { ret = c(); }, connectionType);
        return ret;
    }
}

McpServer::McpServer(QObject *parent)
    : QObject(parent)
    , m_tcpServerP(new QTcpServer(this))
    , m_httpParserP(new HttpParser(this))
    , m_port(3001)
{
    // Set up TCP server connections
    connect(m_tcpServerP, &QTcpServer::newConnection, this, &McpServer::handleNewConnection);

    auto addMethod = [&](const QString &name, MethodHandler handler) {
        m_methods.insert(name, std::move(handler));
    };

    addMethod(
        "initialize", [this](const QJsonObject &, const QJsonValue &id, const Callback &callback) {
            QJsonObject capabilities;
            QJsonObject toolsCapability;
            toolsCapability["listChanged"] = false;
            capabilities["tools"] = toolsCapability;

            QJsonObject serverInfo;
            serverInfo["name"] = "Qt Creator MCP Server";
            serverInfo["version"] = m_commands.getVersion();

            QJsonObject initResult;
            initResult["protocolVersion"] = "2024-11-05";
            initResult["capabilities"] = capabilities;
            initResult["serverInfo"] = serverInfo;
            initResult["instructions"]
                = "Use the listed tools to interact with the current Qt Creator session.";

            // optional meta
            QJsonObject meta;
            meta["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
            initResult["_meta"] = meta;

            callback(createSuccessResponse(initResult, id));
        });

    addMethod(
        "tools/list", [this](const QJsonObject &, const QJsonValue &id, const Callback &callback) {
            callback(createSuccessResponse(QJsonObject{{"tools", m_toolList}}, id));
        });

    addMethod(
        "tools/call",
        [this](const QJsonObject &params, const QJsonValue &id, const Callback &callback) {
            const QString toolName = params.value("name").toString();
            const QJsonObject arguments = params.value("arguments").toObject();

            auto it = m_toolHandlers.find(toolName);
            if (it == m_toolHandlers.end()) {
                callback(createErrorResponse(
                    -32601,
                    QStringLiteral("Unknown tool: %1").arg(toolName),
                    id));
            }

            it.value()(arguments, [callback, id](const QJsonObject &rawResult) {
                // Build the CallToolResult payload as required by the spec.
                QJsonObject result;
                result.insert("structuredContent", rawResult);
                QJsonObject textBlock{
                                      {"type", "text"},
                                      {"text",
                                       QString::fromUtf8(QJsonDocument(rawResult).toJson(QJsonDocument::Compact))}};
                result.insert("content", QJsonArray{textBlock});
                result.insert("isError", false);
                callback(createSuccessResponse(result, id));
            });

        });

    addMethod(
        "resources/list",
        [](const QJsonObject &, const QJsonValue &id, const Callback &callback) {
            QJsonArray emptyResources;
            QJsonObject result;
            result.insert("resources", emptyResources);

            callback(createSuccessResponse(result, id));
        });

    addTool(
        {{"name", "get_build_status"},
         {"title", "Get current build status"},
         {"description", "Get current build progress and status"},
         {"inputSchema", QJsonObject{{"type", "object"}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties", QJsonObject{{"result", QJsonObject{{"type", "string"}}}}},
              {"required", QJsonArray{"result"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", true}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            Q_UNUSED(p);
            QString status = runOnGuiThread([this] { return m_commands.getBuildStatus(); });
            callback(QJsonObject{{"result", status}});
        });

    addTool(
        {{"name", "open_file"},
         {"title", "Open a file in Qt Creator"},
         {"description", "Open a file in Qt Creator"},
         {"inputSchema",
          QJsonObject{
                      {"type", "object"},
                      {"properties",
                       QJsonObject{
                                   {"path",
                                    QJsonObject{
                                                {"type", "string"},
                                                {"format", "uri"},
                                                {"description", "Absolute path of the file to open"}}}}},
                      {"required", QJsonArray{"path"}}}},
         {"outputSchema",
          QJsonObject{
                      {"type", "object"},
                      {"properties", QJsonObject{{"success", QJsonObject{{"type", "boolean"}}}}},
                      {"required", QJsonArray{"success"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", false}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            const QString path = p.value("path").toString();
            bool ok = runOnGuiThread([this, path] { return m_commands.openFile(path); });
            callback(QJsonObject{{"success", ok}});
        });

    addTool(
        {{"name", "file_plain_text"},
         {"title", "file plain text"},
         {"description", "Returns the content of the file as plain text"},
         {"inputSchema",
          QJsonObject{
                      {"type", "object"},
                      {"properties",
                       QJsonObject{
                                   {"path",
                                    QJsonObject{
                                                {"type", "string"},
                                                {"format", "uri"},
                                                {"description", "Absolute path of the file"}}}}},
                      {"required", QJsonArray{"path"}}}},
         {"outputSchema",
          QJsonObject{
                      {"type", "object"},
                      {"properties", QJsonObject{{"text", QJsonObject{{"type", "string"}}}}},
                      {"required", QJsonArray{"text"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", false}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            const QString path = p.value("path").toString();
            const QString text = runOnGuiThread([this, path] { return m_commands.getFilePlainText(path); });
            callback(QJsonObject{{"text", text}});
        });

    addTool(
        {{"name", "set_file_plain_text"},
         {"title", "set file plain text"},
         {"description",
          "overrided the content of the file with the provided plain text. If the file is "
          "currently open it is not saved!"},
         {"inputSchema",
          QJsonObject{
                      {"type", "object"},
                      {"properties",
                       QJsonObject{
                                   {"path",
                                    QJsonObject{
                                                {"type", "string"},
                                                {"format", "uri"},
                                                {"description", "Absolute path of the file"}}},
                                   {"plainText",
                                    QJsonObject{{"type", "string"}, {"description", "text to write into the file"}}}}},
                      {"required", QJsonArray{"path"}}}},
         {"outputSchema",
          QJsonObject{
                      {"type", "object"},
                      {"properties", QJsonObject{{"success", QJsonObject{{"type", "boolean"}}}}},
                      {"required", QJsonArray{"success"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", false}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            const QString path = p.value("path").toString();
            const QString text = p.value("plainText").toString();
            bool ok = runOnGuiThread(
                [this, path, text] { return m_commands.setFilePlainText(path, text); });
            callback(QJsonObject{{"success", ok}});
        });

    addTool(
        {{"name", "save_file"},
         {"title", "Save a file in Qt Creator"},
         {"description", "Save a file in Qt Creator"},
         {"inputSchema",
          QJsonObject{
                      {"type", "object"},
                      {"properties",
                       QJsonObject{
                                   {"path",
                                    QJsonObject{
                                                {"type", "string"},
                                                {"format", "uri"},
                                                {"description", "Absolute path of the file to save"}}}}},
                      {"required", QJsonArray{"path"}}}},
         {"outputSchema",
          QJsonObject{
                      {"type", "object"},
              {"properties", QJsonObject{{"success", QJsonObject{{"type", "boolean"}}}}},
              {"required", QJsonArray{"success"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", false}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            const QString path = p.value("path").toString();
            bool ok = runOnGuiThread([this, path] { return m_commands.saveFile(path); });
            callback(QJsonObject{{"success", ok}});
        });

    addTool(
        {{"name", "close_file"},
         {"title", "Close a file in Qt Creator"},
         {"description", "Close a file in Qt Creator"},
         {"inputSchema",
          QJsonObject{
                      {"type", "object"},
                      {"properties",
                       QJsonObject{
                                   {"path",
                                    QJsonObject{
                                                {"type", "string"},
                                                {"format", "uri"},
                                                {"description", "Absolute path of the file to close"}}}}},
                      {"required", QJsonArray{"path"}}}},
         {"outputSchema",
          QJsonObject{
                      {"type", "object"},
                      {"properties", QJsonObject{{"success", QJsonObject{{"type", "boolean"}}}}},
                      {"required", QJsonArray{"success"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", false}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            const QString path = p.value("path").toString();
            bool ok = runOnGuiThread([this, path] { return m_commands.closeFile(path); });
            callback(QJsonObject{{"success", ok}});
        });

    addTool(
        {{"name", "find_files_in_projects"},
         {"title", "Find files in projects"},
         {"description", "Find all files matching the pattern in all open projects"},
         {"inputSchema",
          QJsonObject{
                      {"type", "object"},
              {"properties",
               QJsonObject{
                           {"pattern",
                    QJsonObject{{"type", "string"}, {"description", "Pattern for finding the file"}}},
                   {"regex",
                    QJsonObject{
                                {"type", "bool"}, {"description", "Whether the pattern is a regex"}}}}},
              {"required", QJsonArray{"pattern"}}}},

         {"outputSchema",
          QJsonObject{
                      {"type", "object"},
              {"properties",
               QJsonObject{
                           {"files",
                    QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}}}}},
              {"required", QJsonArray{"files"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", true}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            const QString pattern = p.value("pattern").toString();
            const bool isRegex = p.value("regex").toBool();
            const QStringList files = runOnGuiThread([this, pattern, isRegex] {
                return m_commands.findFilesInProjects(pattern, isRegex);
            });
            callback(QJsonObject{{"files", QJsonArray::fromStringList(files)}});
        });

    addTool(
        {{"name", "find_files_in_project"},
         {"title", "Find files in project"},
         {"description", "Find all files matching the pattern in a given project"},
         {"inputSchema",
          QJsonObject{
                      {"type", "object"},
              {"properties",
               QJsonObject{
                           {"name", QJsonObject{{"type", "string"}, {"description", "name of the project"}}},
                   {"pattern",
                    QJsonObject{{"type", "string"}, {"description", "Pattern for finding the file"}}},
                   {"regex",
                    QJsonObject{
                                {"type", "bool"}, {"description", "Whether the pattern is a regex"}}}}},
              {"required", QJsonArray{"name, pattern"}}}},

         {"outputSchema",
          QJsonObject{
                      {"type", "object"},
              {"properties",
               QJsonObject{
                           {"files",
                    QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}}}}},
              {"required", QJsonArray{"files"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", true}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            const QString name = p.value("name").toString();
            const QString pattern = p.value("pattern").toString();
            const bool isRegex = p.value("regex").toBool();
            const QStringList files = runOnGuiThread([this, name, pattern, isRegex] {
                return m_commands.findFilesInProject(name, pattern, isRegex);
            });
            callback(QJsonObject{{"files", QJsonArray::fromStringList(files)}});
        });

    addTool(
        {{"name", "search_in_file"},
         {"title", "Search for pattern in a single file"},
         {"description", "Search for a text pattern in a single file and return all matches with line, column, and matched text"},
         {"inputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties",
               QJsonObject{
                   {"path",
                    QJsonObject{
                        {"type", "string"},
                        {"format", "uri"},
                        {"description", "Absolute path of the file to search"}}},
                   {"pattern",
                    QJsonObject{
                        {"type", "string"},
                        {"description", "Text pattern to search for"}}},
                   {"regex",
                    QJsonObject{
                        {"type", "boolean"},
                        {"description", "Whether the pattern is a regular expression"}}},
                   {"caseSensitive",
                    QJsonObject{
                        {"type", "boolean"},
                        {"description", "Whether the search should be case sensitive"}}}}},
              {"required", QJsonArray{"path", "pattern"}}}},
         {"outputSchema", McpCommands::searchResultsSchema()},
         {"annotations", QJsonObject{{"readOnlyHint", true}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            const QString path = p.value("path").toString();
            const QString pattern = p.value("pattern").toString();
            const bool isRegex = p.value("regex").toBool(false);
            const bool caseSensitive = p.value("caseSensitive").toBool(false);
            m_commands.searchInFile(path, pattern, isRegex, caseSensitive, callback);
        });

    addTool(
        {{"name", "search_in_files"},
         {"title", "Search for pattern in project files"},
         {"description", "Search for a text pattern in files matching a file pattern within a project (or all projects) and return all matches"},
         {"inputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties",
               QJsonObject{
                   {"filePattern",
                    QJsonObject{
                        {"type", "string"},
                        {"description", "File pattern to filter which files to search (e.g., '*.cpp', '*.h')"}}},
                   {"projectName",
                    QJsonObject{
                        {"type", "string"},
                        {"description", "Optional: name of the project to search in (searches all projects if not specified)"}}},
                   {"pattern",
                    QJsonObject{
                        {"type", "string"},
                        {"description", "Text pattern to search for"}}},
                   {"regex",
                    QJsonObject{
                        {"type", "boolean"},
                        {"description", "Whether the pattern is a regular expression"}}},
                   {"caseSensitive",
                    QJsonObject{
                        {"type", "boolean"},
                        {"description", "Whether the search should be case sensitive"}}}}},
              {"required", QJsonArray{"filePattern", "pattern"}}}},
         {"outputSchema", McpCommands::searchResultsSchema()},
         {"annotations", QJsonObject{{"readOnlyHint", true}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            const QString filePattern = p.value("filePattern").toString();
            const QString pattern = p.value("pattern").toString();
            const bool isRegex = p.value("regex").toBool(false);
            const bool caseSensitive = p.value("caseSensitive").toBool(false);
            const std::optional<QString> projectName = p.contains("projectName")
                ? std::optional<QString>(p.value("projectName").toString())
                : std::nullopt;
            m_commands.searchInFiles(
                filePattern, projectName, "", pattern, isRegex, caseSensitive, callback);
        });

    addTool(
        {{"name", "search_in_directory"},
         {"title", "Search for pattern in a directory"},
         {"description", "Search for a text pattern recursively in all files within a directory and return all matches"},
         {"inputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties",
               QJsonObject{
                   {"directory",
                    QJsonObject{
                        {"type", "string"},
                        {"format", "uri"},
                        {"description", "Absolute path of the directory to search in"}}},
                   {"pattern",
                    QJsonObject{
                        {"type", "string"},
                        {"description", "Text pattern to search for"}}},
                   {"regex",
                    QJsonObject{
                        {"type", "boolean"},
                        {"description", "Whether the pattern is a regular expression"}}},
                   {"caseSensitive",
                    QJsonObject{
                        {"type", "boolean"},
                        {"description", "Whether the search should be case sensitive"}}}}},
              {"required", QJsonArray{"directory", "pattern"}}}},
         {"outputSchema", McpCommands::searchResultsSchema()},
         {"annotations", QJsonObject{{"readOnlyHint", true}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            const QString directory = p.value("directory").toString();
            const QString pattern = p.value("pattern").toString();
            const bool isRegex = p.value("regex").toBool(false);
            const bool caseSensitive = p.value("caseSensitive").toBool(false);
            m_commands.searchInDirectory(directory, pattern, isRegex, caseSensitive, callback);
        });

    addTool(
        {{"name", "replace_in_file"},
         {"title", "Replace pattern in a single file"},
         {"description", "Replace all matches of a text pattern in a single file with replacement text"},
         {"inputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties",
               QJsonObject{
                   {"path",
                    QJsonObject{
                        {"type", "string"},
                        {"format", "uri"},
                        {"description", "Absolute path of the file to modify"}}},
                   {"pattern",
                    QJsonObject{
                        {"type", "string"},
                        {"description", "Text pattern to search for"}}},
                   {"replacement",
                    QJsonObject{
                        {"type", "string"},
                        {"description", "Replacement text"}}},
                   {"regex",
                    QJsonObject{
                        {"type", "boolean"},
                        {"description", "Whether the pattern is a regular expression"}}},
                   {"caseSensitive",
                    QJsonObject{
                        {"type", "boolean"},
                        {"description", "Whether the search should be case sensitive"}}}}},
              {"required", QJsonArray{"path", "pattern", "replacement"}}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties", QJsonObject{{"ok", QJsonObject{{"type", "boolean"}}}}},
              {"required", QJsonArray{"ok"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", false}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            const QString path = p.value("path").toString();
            const QString pattern = p.value("pattern").toString();
            const QString replacement = p.value("replacement").toString();
            const bool isRegex = p.value("regex").toBool(false);
            const bool caseSensitive = p.value("caseSensitive").toBool(false);
            m_commands.replaceInFile(path, pattern, replacement, isRegex, caseSensitive, callback);
        });

    addTool(
        {{"name", "replace_in_files"},
         {"title", "Replace pattern in project files"},
         {"description", "Replace all matches of a text pattern in files matching a file pattern within a project (or all projects) with replacement text"},
         {"inputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties",
               QJsonObject{
                   {"filePattern",
                    QJsonObject{
                        {"type", "string"},
                        {"description", "File pattern to filter which files to modify (e.g., '*.cpp', '*.h')"}}},
                   {"projectName",
                    QJsonObject{
                        {"type", "string"},
                        {"description", "Optional: name of the project to search in (searches all projects if not specified)"}}},
                   {"pattern",
                    QJsonObject{
                        {"type", "string"},
                        {"description", "Text pattern to search for"}}},
                   {"replacement",
                    QJsonObject{
                        {"type", "string"},
                        {"description", "Replacement text"}}},
                   {"regex",
                    QJsonObject{
                        {"type", "boolean"},
                        {"description", "Whether the pattern is a regular expression"}}},
                   {"caseSensitive",
                    QJsonObject{
                        {"type", "boolean"},
                        {"description", "Whether the search should be case sensitive"}}}}},
              {"required", QJsonArray{"filePattern", "pattern", "replacement"}}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties", QJsonObject{{"ok", QJsonObject{{"type", "boolean"}}}}},
              {"required", QJsonArray{"ok"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", false}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            const QString filePattern = p.value("filePattern").toString();
            const QString pattern = p.value("pattern").toString();
            const QString replacement = p.value("replacement").toString();
            const bool isRegex = p.value("regex").toBool(false);
            const bool caseSensitive = p.value("caseSensitive").toBool(false);
            const std::optional<QString> projectName = p.contains("projectName")
                ? std::optional<QString>(p.value("projectName").toString())
                : std::nullopt;
            m_commands.replaceInFiles(
                filePattern, projectName, "", pattern, replacement, isRegex, caseSensitive, callback);
        });

    addTool(
        {{"name", "replace_in_directory"},
         {"title", "Replace pattern in a directory"},
         {"description", "Replace all matches of a text pattern recursively in all files within a directory with replacement text"},
         {"inputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties",
               QJsonObject{
                   {"directory",
                    QJsonObject{
                        {"type", "string"},
                        {"format", "uri"},
                        {"description", "Absolute path of the directory to search in"}}},
                   {"pattern",
                    QJsonObject{
                        {"type", "string"},
                        {"description", "Text pattern to search for"}}},
                   {"replacement",
                    QJsonObject{
                        {"type", "string"},
                        {"description", "Replacement text"}}},
                   {"regex",
                    QJsonObject{
                        {"type", "boolean"},
                        {"description", "Whether the pattern is a regular expression"}}},
                   {"caseSensitive",
                    QJsonObject{
                        {"type", "boolean"},
                        {"description", "Whether the search should be case sensitive"}}}}},
              {"required", QJsonArray{"directory", "pattern", "replacement"}}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties", QJsonObject{{"ok", QJsonObject{{"type", "boolean"}}}}},
              {"required", QJsonArray{"ok"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", false}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            const QString directory = p.value("directory").toString();
            const QString pattern = p.value("pattern").toString();
            const QString replacement = p.value("replacement").toString();
            const bool isRegex = p.value("regex").toBool(false);
            const bool caseSensitive = p.value("caseSensitive").toBool(false);
            m_commands.replaceInDirectory(directory, pattern, replacement, isRegex, caseSensitive, callback);
        });

    addTool(
        {{"name", "list_projects"},
         {"title", "List all available projects"},
         {"description", "List all available projects"},
         {"inputSchema", QJsonObject{{"type", "object"}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties",
               QJsonObject{
                   {"projects",
                    QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}}}}},
              {"required", QJsonArray{"projects"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", true}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            Q_UNUSED(p);
            const QStringList projects = runOnGuiThread(
                [this] { return m_commands.listProjects(); });
            QJsonArray arr;
            for (const QString &pr : projects)
                arr.append(pr);
            callback(QJsonObject{{"projects", arr}});
        });

    addTool(
        {{"name", "project_dependencies"},
         {"title", "List project dependencies for a project"},
         {"description", "List project dependencies for a project"},
         {"inputSchema",
          QJsonObject{
                      {"type", "object"},
                      {"properties",
                       QJsonObject{
                                   {"name",
                                    QJsonObject{
                                                {"type", "string"},
                                                {"description", "Name of the project to query dependencies for"}}}}},
                      {"required", QJsonArray{"name"}}}},
         {"outputSchema",
          QJsonObject{
                      {"type", "object"},
                      {"properties",
                       QJsonObject{
                                   {"dependencies",
                                    QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}}}}},
                      {"required", QJsonArray{"dependencies"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", true}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            const Utils::Result<QStringList> projects = runOnGuiThread(
                [this, p] { return m_commands.projectDependencies(p["name"].toString()); });
            QJsonArray arr;
            for (const QString &pr : projects.value_or(QStringList{})) // TODO: proper error handling
                arr.append(pr);
            callback(QJsonObject{{"dependencies", arr}});
        });

    addTool(
        {{"name", "list_build_configs"},
         {"title", "List available build configurations"},
         {"description", "List available build configurations"},
         {"inputSchema", QJsonObject{{"type", "object"}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties",
               QJsonObject{
                   {"buildConfigs",
                    QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}}}}},
              {"required", QJsonArray{"buildConfigs"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", true}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            Q_UNUSED(p);
            const QStringList configs = runOnGuiThread(
                [this] { return m_commands.listBuildConfigs(); });
            QJsonArray arr;
            for (const QString &c : configs)
                arr.append(c);
            callback(QJsonObject{{"buildConfigs", arr}});
        });

    addTool(
        {{"name", "switch_build_config"},
         {"title", "Switch to a specific build configuration"},
         {"description", "Switch to a specific build configuration"},
         {"inputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties",
               QJsonObject{
                   {"name",
                    QJsonObject{
                        {"type", "string"},
                        {"description", "Name of the build configuration to switch to"}}}}},
              {"required", QJsonArray{"name"}}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties", QJsonObject{{"success", QJsonObject{{"type", "boolean"}}}}},
              {"required", QJsonArray{"success"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", false}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            const QString name = p.value("name").toString();
            bool ok = runOnGuiThread(
                [this, name] { return m_commands.switchToBuildConfig(name); });
            callback(QJsonObject{{"success", ok}});
        });

    addTool(
        {{"name", "list_open_files"},
         {"title", "List currently open files"},
         {"description", "List currently open files"},
         {"inputSchema", QJsonObject{{"type", "object"}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties",
               QJsonObject{
                   {"openFiles",
                    QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}}}}},
              {"required", QJsonArray{"openFiles"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", true}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            Q_UNUSED(p);
            const QStringList files = runOnGuiThread(
                [this] { return m_commands.listOpenFiles(); });
            QJsonArray arr;
            for (const QString &f : files)
                arr.append(f);
            callback(QJsonObject{{"openFiles", arr}});
        });

    addTool(
        {{"name", "list_sessions"},
         {"title", "List available sessions"},
         {"description", "List available sessions"},
         {"inputSchema", QJsonObject{{"type", "object"}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties",
               QJsonObject{
                   {"sessions",
                    QJsonObject{{"type", "array"}, {"items", QJsonObject{{"type", "string"}}}}}}},
              {"required", QJsonArray{"sessions"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", true}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            Q_UNUSED(p);
            const QStringList sessions = runOnGuiThread(
                [this] { return m_commands.listSessions(); });
            QJsonArray arr;
            for (const QString &s : sessions)
                arr.append(s);
            callback(QJsonObject{{"sessions", arr}});
        });

    addTool(
        {{"name", "load_session"},
         {"title", "Load a specific session"},
         {"description", "Load a specific session"},
         {"inputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties",
               QJsonObject{
                   {"sessionName",
                    QJsonObject{{"type", "string"}, {"description", "Name of the session to load"}}}}},
              {"required", QJsonArray{"sessionName"}}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties", QJsonObject{{"success", QJsonObject{{"type", "boolean"}}}}},
              {"required", QJsonArray{"success"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", false}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            const QString name = p.value("sessionName").toString();
            bool ok = runOnGuiThread([this, name] { return m_commands.loadSession(name); });
            callback(QJsonObject{{"success", ok}});
        });



    addTool(
        {{"name", "list_issues"},
         {"title", "List current issues (warnings and errors)"},
         {"description", "List current issues (warnings and errors)"},
         {"inputSchema", QJsonObject{{"type", "object"}}},
         {"outputSchema", IssuesManager::issuesSchema()},
         {"annotations", QJsonObject{{"readOnlyHint", true}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            Q_UNUSED(p);
            callback(runOnGuiThread([this] { return m_commands.listIssues(); }));
        });

    addTool(
        {{"name", "list_file_issues"},
         {"title", "List current issues for file (warnings and errors)"},
         {"description", "List current issues for file (warnings and errors)"},
         {"inputSchema",
          QJsonObject{
                      {"type", "object"},
                      {"properties",
                       QJsonObject{
                                   {"path",
                                    QJsonObject{
                                                {"type", "string"},
                                                {"format", "uri"},
                                                {"description", "Absolute path of the file to open"}}}}},
                      {"required", QJsonArray{"path"}}}},
         {"outputSchema", IssuesManager::issuesSchema()},
         {"annotations", QJsonObject{{"readOnlyHint", true}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            callback(runOnGuiThread(
                [this, path = p["path"].toString()] {return m_commands.listIssues(path);}));
        });

    addTool(
        {{"name", "quit"},
         {"title", "Quit Qt Creator"},
         {"description", "Quit Qt Creator"},
         {"inputSchema", QJsonObject{{"type", "object"}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties", QJsonObject{{"success", QJsonObject{{"type", "boolean"}}}}},
              {"required", QJsonArray{"success"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", false}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            Q_UNUSED(p);
            bool ok = runOnGuiThread([this] { return m_commands.quit(); });
            callback(QJsonObject{{"success", ok}});
        });

    addTool(
        {{"name", "get_current_project"},
         {"title", "Get the currently active project"},
         {"description", "Get the currently active project"},
         {"inputSchema", QJsonObject{{"type", "object"}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties", QJsonObject{{"project", QJsonObject{{"type", "string"}}}}},
              {"required", QJsonArray{"project"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", true}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            Q_UNUSED(p);
            QString proj = runOnGuiThread([this] { return m_commands.getCurrentProject(); });
            callback(QJsonObject{{"project", proj}});
        });

    addTool(
        {{"name", "get_current_build_config"},
         {"title", "Get the currently active build configuration"},
         {"description", "Get the currently active build configuration"},
         {"inputSchema", QJsonObject{{"type", "object"}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties", QJsonObject{{"buildConfig", QJsonObject{{"type", "string"}}}}},
              {"required", QJsonArray{"buildConfig"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", true}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            Q_UNUSED(p);
            QString cfg = runOnGuiThread([this] { return m_commands.getCurrentBuildConfig(); });
            callback(QJsonObject{{"buildConfig", cfg}});
        });

    addTool(
        {{"name", "get_current_session"},
         {"title", "Get the currently active session"},
         {"description", "Get the currently active session"},
         {"inputSchema", QJsonObject{{"type", "object"}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties", QJsonObject{{"session", QJsonObject{{"type", "string"}}}}},
              {"required", QJsonArray{"session"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", true}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            Q_UNUSED(p);
            QString sess = runOnGuiThread([this] { return m_commands.getCurrentSession(); });
            callback(QJsonObject{{"session", sess}});
        });

    addTool(
        {{"name", "save_session"},
         {"title", "Save the current session"},
         {"description", "Save the current session"},
         {"inputSchema", QJsonObject{{"type", "object"}}},
         {"outputSchema",
          QJsonObject{
              {"type", "object"},
              {"properties", QJsonObject{{"success", QJsonObject{{"type", "boolean"}}}}},
              {"required", QJsonArray{"success"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", false}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            Q_UNUSED(p);
            bool ok = runOnGuiThread([this] { return m_commands.saveSession(); });
            callback(QJsonObject{{"success", ok}});
        });

    addTool(
        {{"name", "save_session"},
         {"title", "Save the current session"},
         {"description", "Save the current session"},
         {"inputSchema", QJsonObject{{"type", "object"}}},
         {"outputSchema",
          QJsonObject{
                      {"type", "object"},
                      {"properties", QJsonObject{{"success", QJsonObject{{"type", "boolean"}}}}},
                      {"required", QJsonArray{"success"}}}},
         {"annotations", QJsonObject{{"readOnlyHint", false}}}},
        [this](const QJsonObject &p, const Callback &callback) {
            Q_UNUSED(p);
            bool ok = runOnGuiThread([this] { return m_commands.saveSession(); });
            callback(QJsonObject{{"success", ok}});
        });

    const QJsonObject commandInputSchema = QJsonObject{
        {"type", "object"},
        {"properties",
         QJsonObject{
             {"command", QJsonObject{{"type", "string"}, {"description", "Command to execute"}}},
             {"arguments", QJsonObject{{"type", "string"}, {"description", "arguments passed to the command"}}},
             {"workingDir", QJsonObject{{"type", "string"}, {"description", "directory in which the command is executed"}}}}},
        {"required", QJsonArray{"command"}}};

    const QJsonObject commandOutputSchema = QJsonObject{
        {"type", "object"},
        {"properties",
         QJsonObject{
             {"exitCode", QJsonObject{{"type", "integer"}}},
             {"stdout", QJsonObject{{"type", "string"}}},
             {"stderr", QJsonObject{{"type", "string"}}}}},
        {"required", QJsonArray{"exitCode"}}};

    addTool(
        {{"name", "execut_command"},
         {"title", "executes the command"},
         {"description",
          "executes the command and returns the exit code as well as standart output and error"},
         {"inputSchema", commandInputSchema},
         {"outputSchema", commandOutputSchema}},
        [this](const QJsonObject &p, const Callback &callback) {
            m_commands.executeCommand(
                p["command"].toString(),
                p["arguments"].toString(),
                p["workingDir"].toString(),
                callback);
        });
}

McpServer::~McpServer()
{
    stop();
}

bool McpServer::isHttpRequest(const QByteArray &data)
{
    return HttpParser::isHttpRequest(data);
}

void McpServer::handleHttpRequest(QTcpSocket *client, const HttpParser::HttpRequest &request)
{
    qCDebug(mcpServer) << "Handling HTTP request:" << request.method << request.uri;

    // Handle different HTTP methods
    if (request.method == "GET") {
        return onHttpGet(client, request);
    } else if (request.method == "POST") {
        return onHttpPost(client, request);
    } else if (request.method == "OPTIONS") {
        return onHttpOptions(client, request);
    }

    // Method not allowed
    QByteArray errorResponse = HttpResponse::createErrorResponse(
        HttpResponse::METHOD_NOT_ALLOWED, "Method not allowed: " + request.method);
    sendHttpResponse(client, errorResponse);
}

void McpServer::onHttpGet(QTcpSocket *client, const HttpParser::HttpRequest &request)
{
    // If the client asks for the SSE stream, upgrade the connection.
    if (request.uri == QLatin1String("/sse")) {
        qCDebug(mcpServer) << "Upgrading connection to SSE stream";
        handleSseClient(client);
        return;
    }

    // Simple GET request - return server info
    QJsonObject serverInfo;
    serverInfo["name"] = "Qt Creator MCP Server";
    serverInfo["version"] = "1.0.0";
    serverInfo["transport"] = "HTTP";
    serverInfo["protocol"] = "MCP";

    QJsonDocument doc(serverInfo);
    QByteArray response = HttpResponse::createCorsResponse(doc.toJson());
    sendHttpResponse(client, response);
}

void McpServer::onHttpPost(QTcpSocket *client, const HttpParser::HttpRequest &request)
{
    // Handle MCP JSON-RPC requests
    if (request.body.isEmpty()) {
        QByteArray noContent
            = HttpResponse::createCorsResponse(QByteArray(), HttpResponse::NO_CONTENT);
        sendHttpResponse(client, noContent);
        return;
    }

    // Parse JSON-RPC request
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(request.body, &error);

    if (error.error != QJsonParseError::NoError) {
        qCDebug(mcpServer) << "JSON parse error:" << error.errorString();
        QByteArray errorResponse = HttpResponse::createErrorResponse(
            HttpResponse::BAD_REQUEST, "Invalid JSON: " + error.errorString());
        sendHttpResponse(client, errorResponse);
        return;
    }

    if (!doc.isObject()) {
        qCDebug(mcpServer) << "Invalid JSON-RPC message: not an object";
        QByteArray errorResponse = HttpResponse::createErrorResponse(
            HttpResponse::BAD_REQUEST, "Invalid JSON-RPC: not an object");
        sendHttpResponse(client, errorResponse);
        return;
    }

    // Process the MCP request
    const QJsonObject obj = doc.object();
    const QString method = obj.value("method").toString();
    const QJsonValue id = obj.value("id");
    const QJsonObject params = obj.value("params").toObject();

    QString jsonrpc = obj.value("jsonrpc").toString();
    if (jsonrpc != "2.0") {
        qCDebug(mcpServer) << "Invalid JSON-RPC version";
        QJsonObject errorResponse
            = createErrorResponse(-32600, "Invalid Request: jsonrpc must be '2.0'", id);
        sendResponse(client, errorResponse);
    }

    auto responseHandler = [self = QPointer<McpServer>(this), client = QPointer(client)](const QJsonObject &response) {
        // Responses are sent as SSE events.
        if (!self || !client)
            return;

        self->broadcastSseMessage(response);
        static QByteArray noContent
            = HttpResponse::createCorsResponse(QByteArray(), HttpResponse::NO_CONTENT);
        // The HTTP POST itself gets a 204 No Content response (nothing in body).
        sendHttpResponse(client, noContent);
    };
    callMCPMethod(method, responseHandler, params, id);

}

void McpServer::onHttpOptions(QTcpSocket *client, const HttpParser::HttpRequest &request)
{
    Q_UNUSED(request);

    // No body, just CORS headers and a 204 status
    QByteArray response = HttpResponse::createCorsResponse(QByteArray(), HttpResponse::NO_CONTENT);
    sendHttpResponse(client, response);
}

void McpServer::handleSseClient(QTcpSocket *client)
{
    // Build minimal SSE response headers
    QByteArray header;
    header.append("HTTP/1.1 200 OK\r\n");
    header.append("Content-Type: text/event-stream\r\n");
    header.append("Cache-Control: no-cache\r\n");
    header.append("Connection: keep-alive\r\n");
    // CORS – required for browsers / remote clients
    header.append("Access-Control-Allow-Origin: *\r\n");
    header.append("Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n");
    header.append("Access-Control-Allow-Headers: Content-Type, Authorization\r\n");
    header.append("\r\n"); // end of header

    client->write(header);
    client->flush();

    // Send the mandatory `endpoint` event that tells the client where to POST.
    QByteArray event;
    event.append("event: endpoint\n");
    event.append("data: /sse\n\n");
    client->write(event);
    client->flush();

    // Remember this socket so we can push future messages.
    m_sseClients.append(client);
}

void McpServer::broadcastSseMessage(const QJsonObject &msg)
{
    // Encode the JSON object in the compact form required by the spec.
    const QByteArray json = QJsonDocument(msg).toJson(QJsonDocument::Compact);

    // Build the SSE event payload.
    QByteArray event;
    event.append("event: message\n");
    event.append("data: ");
    event.append(json);
    event.append("\n\n");

    // Send to each live SSE client.
    for (QTcpSocket *sseClient : std::as_const(m_sseClients)) {
        if (sseClient && sseClient->state() == QAbstractSocket::ConnectedState) {
            sseClient->write(event);
            sseClient->flush();
        }
    }
}

void McpServer::sendHttpResponse(QTcpSocket *client, const QByteArray &httpResponse)
{
    if (!client)
        return;

    qCDebug(mcpServer) << "Sending HTTP response, size:" << httpResponse.size();
    client->write(httpResponse);
    client->flush();

    // Close connection after sending response (HTTP/1.1 behavior)
    client->disconnectFromHost();
}

bool McpServer::start(quint16 port)
{
    m_port = port;
    qCDebug(mcpServer) << "Starting MCP HTTP server on port" << m_port;
    qCDebug(mcpServer) << "Qt version:" << QT_VERSION_STR;
    qCDebug(mcpServer) << "Build type:" <<
#ifdef QT_NO_DEBUG
        "Release"
#else
        "Debug"
#endif
        ;

    // Try to start TCP server on the requested port first
    if (!m_tcpServerP->listen(QHostAddress::LocalHost, m_port)) {
        qCWarning(mcpServer) << "Failed to start MCP TCP server on port" << m_port << ":"
                              << m_tcpServerP->errorString();
        qCWarning(mcpServer) << "Port" << m_port
                             << "is in use, trying to find an available port...";

        // Try ports from 3001 to 3010
        for (quint16 tryPort = 3001; tryPort <= 3010; ++tryPort) {
            if (m_tcpServerP->listen(QHostAddress::LocalHost, tryPort)) {
                m_port = tryPort;
                qCInfo(mcpServer) << "MCP TCP Server started on port" << m_port << "(port" << port
                                  << "was busy)";
                break;
            }
        }

        if (!m_tcpServerP->isListening()) {
            qCWarning(mcpServer) << "Failed to start MCP TCP server on any port from 3001-3010";
            return false;
        }
    }

    qCDebug(mcpServer) << "MCP server startup complete - TCP listening:"
                       << m_tcpServerP->isListening();

    qCInfo(mcpServer) << "MCP HTTP Server started successfully on port" << m_port;
    return true;
}

void McpServer::stop()
{
    if (m_tcpServerP && m_tcpServerP->isListening()) {
        m_tcpServerP->close();
        qCDebug(mcpServer) << "MCP HTTP Server stopped";
    }
}

bool McpServer::isRunning() const
{
    return m_tcpServerP && m_tcpServerP->isListening();
}

quint16 McpServer::getPort() const
{
    return m_port;
}

void McpServer::callMCPMethod(
    const QString &method, const Callback &callback, const QJsonObject &params, const QJsonValue &id)
{
    if (method.isEmpty()) {
        callback(createErrorResponse(-32600, "Invalid Request: method is required", id));
        return;
    }

    // Find the handler
    auto it = m_methods.find(method);
    if (it == m_methods.end()) {
        callback(createErrorResponse(-32601, QStringLiteral("Method not found: %1").arg(method), id));
    } else {
        try {
            it.value()(params, id, callback);
        } catch (const std::exception &e) {
            callback(createErrorResponse(-32603, QString::fromStdString(e.what()), id));
        }
    }
}

void McpServer::addTool(const QJsonObject &tool, ToolHandler handler)
{
    m_toolList.append(tool);
    const QString name = tool.value("name").toString();
    if (QTC_GUARD(!name.isEmpty()))
        m_toolHandlers.insert(name, std::move(handler));
}

static bool triggerCommand(const QString toolName, const Utils::Id commandId)
{
    auto error = [toolName](const QString &msg) {
        qCDebug(mcpServer) << "Cannot run tool " << toolName << ": " << msg;
        return false;
    };
    Core::Command *command = Core::ActionManager::command(commandId);
    if (!command)
        return error("Cannot find command with id" + commandId.toString());
    QAction *action = command->action();
    if (!action)
        return error("Command with id" + commandId.toString() + "has no action assigned");
    if (!action->isEnabled())
        return error("Action for Command with id" + commandId.toString() + "is not enabled");
    action->trigger();
    return true;
}

void McpServer::initializeToolsForCommands()
{
    auto addToolForCommand = [this](const QString &name, const Utils::Id commandId) {
        Core::Command *command = Core::ActionManager::command(commandId);
        if (!command)
            return;
        QAction *action = command->action();
        QTC_ASSERT(action, return);

        const QString title = action->text();
        const QString description = command->description();

        addTool(
            {{"name", name},
             {"title", title},
             {"description", description},
             {"inputSchema", QJsonObject{{"type", "object"}}},
             {"outputSchema",
              QJsonObject{
                          {"type", "object"},
                          {"properties", QJsonObject{{"success", QJsonObject{{"type", "boolean"}}}}},
                          {"required", QJsonArray{"success"}}}},
             {"annotations", QJsonObject{{"readOnlyHint", false}}}},
            [commandId, name](const QJsonObject &p, const Callback &callback) {
                Q_UNUSED(p);
                const bool ok = triggerCommand(name, commandId);
                callback(QJsonObject{{"success", ok}});
            });
    };

    addToolForCommand("run_project", ProjectExplorer::Constants::RUN);
    addToolForCommand("build", ProjectExplorer::Constants::BUILD);
    addToolForCommand("clean_project", ProjectExplorer::Constants::CLEAN);
    addToolForCommand("debug", Debugger::Constants::DEBUGGER_START);
}

QJsonObject McpServer::createErrorResponse(int code, const QString &message, const QJsonValue &id)
{
    QJsonObject response;
    response["jsonrpc"] = "2.0";
    response["id"] = id;

    QJsonObject error;
    error["code"] = code;
    error["message"] = message;
    response["error"] = error;

    return response;
}

QJsonObject McpServer::createSuccessResponse(const QJsonValue &result, const QJsonValue &id)
{
    QJsonObject response;
    response["jsonrpc"] = "2.0";
    response["id"] = id;
    response["result"] = result;

    return response;
}

void McpServer::handleNewConnection()
{
    QTcpSocket *client = m_tcpServerP->nextPendingConnection();
    if (!client)
        return;

    m_clients.append(client);
    connect(client, &QTcpSocket::readyRead, this, &McpServer::handleClientData);
    connect(client, &QTcpSocket::disconnected, this, &McpServer::handleClientDisconnected);

    qCDebug(mcpServer) << "New TCP client connected, total clients:" << m_clients.size();
}

void McpServer::handleClientData()
{
    QTcpSocket *client = qobject_cast<QTcpSocket *>(sender());
    if (!client)
        return;

    // Append the newly-received bytes to any data we already have for this socket
    m_partialData[client] += client->readAll();
    bool needMoreData = false;
    const QScopeGuard cleanUpPartialData([&] {
        if (!needMoreData)
            m_partialData.remove(client);
    });

    const QByteArray &data = m_partialData[client];
    qCDebug(mcpServer) << "Received data, total buffered size:" << data.size();

    // Check if this is an HTTP request
    if (isHttpRequest(data)) {
        qCDebug(mcpServer) << "Detected HTTP request, parsing...";

        // Parse HTTP request
        HttpParser::HttpRequest httpRequest = m_httpParserP->parseRequest(data);

        if (httpRequest.needMoreData) {
            // We don't have the whole request yet – just wait for more data.
            needMoreData = true;
            qCDebug(mcpServer) << "HTTP request incomplete, waiting for more bytes";
            return;
        }

        if (!httpRequest.isValid) {
            qCDebug(mcpServer) << "Invalid HTTP request:" << httpRequest.errorMessage;
            QByteArray errorResponse = HttpResponse::createErrorResponse(
                HttpResponse::BAD_REQUEST, httpRequest.errorMessage);
            sendHttpResponse(client, errorResponse);
            return;
        }

        // Handle HTTP request
        handleHttpRequest(client, httpRequest);
        return;
    }

    // Handle as TCP/JSON-RPC request (original behavior)
    qCDebug(mcpServer) << "Handling as TCP/JSON-RPC request";

    // Parse JSON-RPC request
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError) {
        qCDebug(mcpServer) << "JSON parse error:" << error.errorString();
        QJsonObject errorResponse = createErrorResponse(-32700, "Parse error");
        sendResponse(client, errorResponse);
        return;
    }

    if (!doc.isObject()) {
        qCDebug(mcpServer) << "Invalid JSON-RPC message: not an object";
        QJsonObject errorResponse = createErrorResponse(-32600, "Invalid Request");
        sendResponse(client, errorResponse);
        return;
    }

    const QJsonObject obj = doc.object();

    // The JSON‑RPC 2.0 request
    const QString method = obj.value("method").toString();
    const QJsonValue id = obj.value("id");
    const QJsonObject params = obj.value("params").toObject();

    const QString jsonrpc = obj.value("jsonrpc").toString();
    if (jsonrpc != "2.0") {
        qCDebug(mcpServer) << "Invalid JSON-RPC version";
        QJsonObject errorResponse
            = createErrorResponse(-32600, "Invalid Request: jsonrpc must be '2.0'", id);
        sendResponse(client, errorResponse);
        return;
    }

    auto responseHandler = [self = QPointer(this),
                            client = QPointer(client)](const QJsonObject &response) {
        if (self && client)
            self->sendResponse(client, response);
    };
    callMCPMethod(method, responseHandler, params, id);
}

void McpServer::handleClientDisconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket *>(sender());
    if (!client)
        return;

    m_clients.removeAll(client);
    m_sseClients.removeAll(client);
    client->deleteLater();

    qCDebug(mcpServer) << "TCP client disconnected, remaining clients:" << m_clients.size();
}

void McpServer::sendResponse(QTcpSocket *client, const QJsonObject &response)
{
    if (!client)
        return;

    QJsonDocument doc(response);
    QByteArray data = doc.toJson(QJsonDocument::Compact) + "\n";

    qCDebug(mcpServer) << "Sending TCP response:" << data;
    client->write(data);
    client->flush();
}

} // namespace Mcp::Internal
