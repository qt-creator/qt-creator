// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mcpmanager.h"

#include "coreconstants.h"
#include "coreplugintr.h"
#include "dialogs/ioptionspage.h"

#include <utils/algorithm.h>
#include <utils/aspects.h>
#include <utils/layoutbuilder.h>

#include <QMetaEnum>
#include <QStandardItemModel>
#include <QUuid>

using namespace Utils;

namespace Core {

class McpServerAspect : public AspectContainer
{
    static const auto &conTypeEnum()
    {
        static auto e = QMetaEnum::fromType<McpManager::ConnectionType>();
        return e;
    };

public:
    McpServerAspect()
    {
        setAutoApply(false);

        id.setSettingsKey("id");
        id.setValue(QUuid::createUuid().toString());

        name.setLabelText(Tr::tr("Name:"));
        name.setSettingsKey("name");
        name.setToolTip(Tr::tr("The display name"));
        name.setDisplayStyle(StringAspect::DisplayStyle::LineEditDisplay);
        name.setDefaultValue(Tr::tr("<New Server>"));

        // name.setValidationFunction([](){});

        launchCommand.setLabelText(Tr::tr("Launch command:"));
        launchCommand.setSettingsKey("launchCommand");
        launchCommand.setToolTip(
            Tr::tr(
                "The command to launch the MCP server process. Only used for standard IO "
                "connection type."));
        launchCommand.setExpectedKind(PathChooser::ExistingCommand);

        launchArguments.setLabelText(Tr::tr("Launch arguments:"));
        launchArguments.setSettingsKey("launchArguments");
        launchArguments.setToolTip(
            Tr::tr(
                "The arguments to launch the MCP server process. Only used for standard IO "
                "connection type."));
        launchArguments.setDisplayStyle(StringAspect::DisplayStyle::LineEditDisplay);

        url.setLabelText("URL:");
        url.setSettingsKey("url");
        url.setToolTip(
            Tr::tr(
                "The URL to connect to the MCP server. Not used for standard IO connection type."));
        url.setDisplayStyle(StringAspect::DisplayStyle::LineEditDisplay);

        connectionType.setLabelText(Tr::tr("Connection type:"));
        connectionType.setSettingsKey("connectionType");
        connectionType.setToolTip(Tr::tr("The type of connection to use for the MCP server."));
        connectionType.setComboBoxEditable(false);
        connectionType.setFillCallback([](StringSelectionAspect::ResultCallback cb) {
            QStandardItem *stdItem = new QStandardItem(Tr::tr("Standard IO"));
            stdItem->setData(conTypeEnum().valueToKey(McpManager::Stdio));
            stdItem->setToolTip("Standard IO");
            QStandardItem *sseItem = new QStandardItem(Tr::tr("SSE"));
            sseItem->setData(conTypeEnum().valueToKey(McpManager::Sse));
            sseItem->setToolTip("SSE");
            QStandardItem *httpItem = new QStandardItem(Tr::tr("Streamable HTTP"));
            httpItem->setData(conTypeEnum().valueToKey(McpManager::Streamable_Http));
            httpItem->setToolTip("Streamable HTTP");
            cb({stdItem, sseItem, httpItem});
        });

        httpHeaders.setLabelText(Tr::tr("HTTP headers:"));
        httpHeaders.setSettingsKey("httpHeaders");
        httpHeaders.setToolTip(
            Tr::tr(
                "HTTP headers to include when connecting to the MCP server. Only used for HTTP "
                "connection types."));

        setLayouter([this]() -> Layouting::Layout {
            using namespace Layouting;

            const auto updateVisible = [this]() {
                const QString type = connectionType.volatileValue();
                const bool isStdio = conTypeEnum().keyToValue(type.toUtf8()) == McpManager::Stdio;

                launchCommand.setVisible(isStdio);
                launchArguments.setVisible(isStdio);
                url.setVisible(!isStdio);
                httpHeaders.setVisible(!isStdio);
            };
            updateVisible();

            connect(
                &connectionType, &StringSelectionAspect::volatileValueChanged, this, updateVisible);

            // clang-format off
            return Form {
                noMargin,
                name, br,
                connectionType, br,
                launchCommand, br,
                launchArguments, br,
                url, br,
                httpHeaders, br,
            };
            // clang-format on
        });
    }

    McpManager::ServerInfo toServerInfo() const
    {
        McpManager::ServerInfo info;
        info.id = id.value();
        info.name = name.value();
        info.connectionType = McpManager::ConnectionType(
            conTypeEnum().keyToValue(connectionType.value().toUtf8()));

        if (info.connectionType == McpManager::ConnectionType::Stdio) {
            info.launchInfo = Utils::CommandLine(
                FilePath::fromUserInput(launchCommand.value()),
                launchArguments.value(),
                Utils::CommandLine::Raw);
        } else {
            info.launchInfo = QUrl(url.value());
        }
        info.httpHeaders = httpHeaders();
        return info;
    }

    StringAspect id{this};
    StringAspect name{this};
    StringSelectionAspect connectionType{this};

    FilePathAspect launchCommand{this};
    StringAspect launchArguments{this};

    StringAspect url{this};
    StringListAspect httpHeaders{this};
};

static QString displayFunc(McpServerAspect *aspect)
{
    return aspect->name.volatileValue();
}

class McpManagerSettings : public Utils::AspectContainer
{
public:
    McpManagerSettings()
    {
        setAutoApply(false);
        mcpServers.setSettingsKey("McpServers");
        mcpServers.setDisplayStyle(AspectList::DisplayStyle::ListViewWithDetails);
        mcpServers.setCreateItemFunction([] {
            auto newItem = std::make_shared<McpServerAspect>();
            return newItem;
        });

        mcpServers.listViewDisplayCallback = displayFunc;

        setLayouter([this]() {
            using namespace Layouting;
            // clang-format off
            return Column{
                Tr::tr("MCP Servers:"), br,
                &mcpServers,
            };
            // clang-format on
        });

        readSettings();
    }

    static McpManagerSettings &instance()
    {
        static McpManagerSettings settings;
        return settings;
    }

    AspectList mcpServers{this};
};

static const char MCPMANAGER_SETTINGS_PAGE_ID[] = "AI.MCPMANAGER";

class McpManagerSettingsPage final : public IOptionsPage
{
public:
    McpManagerSettingsPage()
    {
        setId(MCPMANAGER_SETTINGS_PAGE_ID);
        setDisplayName(Tr::tr("Mcp Servers"));
        setCategory(Core::Constants::SETTINGS_CATEGORY_AI);
        setSettingsProvider([] { return &McpManagerSettings::instance(); });
    }
};

static McpManagerSettingsPage &settingsPage()
{
    static McpManagerSettingsPage page;
    return page;
}

static QList<McpManager::ServerInfo> &builtInMcpServers()
{
    static QList<McpManager::ServerInfo> servers;
    return servers;
}

McpManager::McpManager()
{
    QObject::connect(
        &McpManagerSettings::instance().mcpServers,
        &BaseAspect::changed,
        this,
        &McpManager::mcpServersChanged);
}

McpManager::~McpManager() = default;

McpManager &McpManager::instance()
{
    static McpManager manager;
    return manager;
}

bool McpManager::registerMcpServer(McpManager::ServerInfo info)
{
    if (Utils::anyOf(mcpServers(), [&info](const McpManager::ServerInfo &s) {
            return s.id == info.id;
        }))
        return false;

    builtInMcpServers().append(info);
    emit instance().mcpServersChanged();
    return true;
}

void McpManager::removeMcpServer(const QString &id)
{
    auto &servers = builtInMcpServers();
    auto it = std::remove_if(servers.begin(), servers.end(), [&id](const McpManager::ServerInfo &s) {
        return s.id == id;
    });
    if (it != servers.end()) {
        servers.erase(it, servers.end());
        emit instance().mcpServersChanged();
    }
}

QList<McpManager::ServerInfo> McpManager::mcpServers()
{
    QList<McpManager::ServerInfo> servers = builtInMcpServers();

    McpManagerSettings::instance().mcpServers.forEachItem([&](McpServerAspect *server) {
        McpManager::ServerInfo info = server->toServerInfo();
        servers.push_back(info);
    });

    return servers;
}

void setupMcpManager()
{
    (void) settingsPage();
    (void) McpManager::instance();
}

} // namespace Core
