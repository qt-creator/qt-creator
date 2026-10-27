// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "toolregistry.h"

#include <utils/algorithm.h>

#include <memory>

namespace Mcp {

namespace Internal {

struct Tool
{
    Schema::Tool metadata;
    std::variant<Server::ToolInterfaceCallback, Server::ToolCallback> callback;
    bool enabled = true;
};

struct Registry
{
    std::vector<Tool> tools;
};

static Registry &registry()
{
    static Registry r;
    return r;
}

} // namespace Internal

static ToolRegistry &toolRegistry()
{
    static ToolRegistry registry;
    return registry;
}

void ToolRegistry::registerTool(
    const Generated::Schema::_2025_11_25::Tool &tool, const Server::ToolInterfaceCallback &callback)
{
    Internal::registry().tools.push_back({tool, callback});
    emit toolRegistry().toolRegistered();
}

void ToolRegistry::registerTool(
    const Generated::Schema::_2025_11_25::Tool &tool, const Server::ToolCallback &callback)
{
    Internal::registry().tools.push_back({tool, callback});
    emit toolRegistry().toolRegistered();
}

const ToolRegistry &ToolRegistry::instance()
{
    return toolRegistry();
}

QList<Schema::Tool> ToolRegistry::registeredTools()
{
    QList<Schema::Tool> tools;
    for (const auto &tool : Internal::registry().tools)
        tools.append(tool.metadata);
    return tools;
}

Utils::Result<Schema::CallToolResult> ToolRegistry::callToolForTests(
    const QString &name, const Schema::CallToolRequestParams &params)
{
    // Owned by the callback too, so a tool that answers late writes into a live slot.
    const auto answer = std::make_shared<std::optional<Utils::Result<Schema::CallToolResult>>>();
    callToolForTests(name, params, [answer](const Utils::Result<Schema::CallToolResult> &result) {
        *answer = result;
    });
    if (!*answer) {
        return Utils::ResultError(
            QString("Tool \"%1\" is asynchronous and did not answer before returning.").arg(name));
    }
    return **answer;
}

void ToolRegistry::callToolForTests(
    const QString &name,
    const Schema::CallToolRequestParams &params,
    const ToolInterface::FinishCallback &onResult)
{
    for (const auto &tool : Internal::registry().tools) {
        if (tool.metadata.name() != name)
            continue;
        if (!tool.enabled)
            return onResult(Utils::ResultError(QString("Tool \"%1\" is disabled.").arg(name)));
        // The same check the server makes, so a test sees what a client sees.
        if (const Utils::Result<> ok = validateToolArguments(tool.metadata, params); !ok)
            return onResult(Utils::ResultError(ok.error()));
        if (auto *callback = std::get_if<Server::ToolCallback>(&tool.callback))
            return onResult((*callback)(params));
        const auto &callback = std::get<Server::ToolInterfaceCallback>(tool.callback);
        // An error returned instead of an answer is what the server reports
        // to the client in place of one.
        const ToolInterface toolInterface = ToolInterface::createForTests(params, onResult);
        if (const Utils::Result<> ok = callback(params, toolInterface); !ok)
            toolInterface.finish(Utils::ResultError(ok.error()));
        return;
    }
    onResult(Utils::ResultError(QString("No registered tool named \"%1\".").arg(name)));
}

bool ToolRegistry::isToolEnabled(const QString &toolName)
{
    return Utils::anyOf(Internal::registry().tools, [&toolName](const Internal::Tool &tool) {
        return tool.metadata.name() == toolName && tool.enabled;
    });
}

void ToolRegistry::enableTool(const QString &toolName, bool enabled)
{
    auto tool = std::find_if(
        Internal::registry().tools.begin(),
        Internal::registry().tools.end(),
        [&toolName](const Internal::Tool &t) { return t.metadata.name() == toolName; });

    if (tool == Internal::registry().tools.end())
        return; // No such tool

    if (tool->enabled == enabled)
        return; // No change

    tool->enabled = enabled;
    emit toolRegistry().toolEnabled(toolName, enabled);
    return;
}

AutoRegisteringServer::AutoRegisteringServer(
    Generated::Schema::_2025_11_25::Implementation serverInfo)
    : Server(serverInfo)
{
    for (const auto &tool : Internal::registry().tools) {
        if (tool.enabled) {
            std::visit(
                [this, &tool](auto &&callback) { addTool(tool.metadata, callback); }, tool.callback);
        }
    }
    m_nTools = Internal::registry().tools.size();

    QObject::connect(&ToolRegistry::instance(), &ToolRegistry::toolRegistered, this, [this]() {
        for (auto i = m_nTools; i < Internal::registry().tools.size(); ++i) {
            const auto &tool = Internal::registry().tools[i];
            if (tool.enabled) {
                std::visit(
                    [this, &tool](auto &&callback) { addTool(tool.metadata, callback); },
                    tool.callback);
            }
        }
        m_nTools = Internal::registry().tools.size();
    });

    QObject::connect(
        &ToolRegistry::instance(),
        &ToolRegistry::toolEnabled,
        this,
        [this](const QString &toolName, bool enabled) {
            if (enabled) {
                for (const auto &tool : Internal::registry().tools) {
                    if (tool.metadata.name() == toolName) {
                        std::visit(
                            [this, &tool](auto &&callback) { addTool(tool.metadata, callback); },
                            tool.callback);
                        return;
                    }
                }
            } else {
                removeTool(toolName);
            }
        });
}

} // namespace Mcp
