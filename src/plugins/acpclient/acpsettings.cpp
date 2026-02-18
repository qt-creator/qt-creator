// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpsettings.h"
#include "acpclienttr.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/algorithm.h>
#include <utils/aspects.h>
#include <utils/environmentdialog.h>
#include <utils/layoutbuilder.h>

#include <QMetaEnum>
#include <QPushButton>
#include <QStandardItemModel>
#include <QUuid>

using namespace Utils;

namespace AcpClient::Internal {

class AcpServerAspect : public AspectContainer
{
    static const auto &conTypeEnum()
    {
        static auto e = QMetaEnum::fromType<AcpSettings::ConnectionType>();
        return e;
    }

public:
    AcpServerAspect()
    {
        setAutoApply(false);

        id.setSettingsKey("id");
        id.setValue(QUuid::createUuid().toString());

        name.setLabelText(Tr::tr("Name:"));
        name.setSettingsKey("name");
        name.setToolTip(Tr::tr("The display name"));
        name.setDisplayStyle(StringAspect::DisplayStyle::LineEditDisplay);
        name.setDefaultValue(Tr::tr("<New Server>"));

        connectionType.setLabelText(Tr::tr("Connection type:"));
        connectionType.setSettingsKey("connectionType");
        connectionType.setToolTip(Tr::tr("The type of connection to use for the ACP server."));
        connectionType.setComboBoxEditable(false);
        connectionType.setFillCallback([](StringSelectionAspect::ResultCallback cb) {
            auto *stdioItem = new QStandardItem(Tr::tr("Standard IO"));
            stdioItem->setData(conTypeEnum().valueToKey(AcpSettings::Stdio));
            stdioItem->setToolTip("Standard IO");
            auto *tcpItem = new QStandardItem(Tr::tr("TCP"));
            tcpItem->setData(conTypeEnum().valueToKey(AcpSettings::Tcp));
            tcpItem->setToolTip("TCP");
            cb({stdioItem, tcpItem});
        });

        launchCommand.setLabelText(Tr::tr("Launch command:"));
        launchCommand.setSettingsKey("launchCommand");
        launchCommand.setToolTip(
            Tr::tr("The command to launch the ACP server process. Only used for Standard IO."));
        launchCommand.setExpectedKind(PathChooser::ExistingCommand);

        launchArguments.setLabelText(Tr::tr("Launch arguments:"));
        launchArguments.setSettingsKey("launchArguments");
        launchArguments.setToolTip(
            Tr::tr("The arguments to launch the ACP server process. Only used for Standard IO."));
        launchArguments.setDisplayStyle(StringAspect::DisplayStyle::LineEditDisplay);

        host.setLabelText(Tr::tr("Host:"));
        host.setSettingsKey("host");
        host.setToolTip(Tr::tr("The hostname for the TCP connection."));
        host.setDisplayStyle(StringAspect::DisplayStyle::LineEditDisplay);
        host.setDefaultValue("localhost");

        port.setLabelText(Tr::tr("Port:"));
        port.setSettingsKey("port");
        port.setToolTip(Tr::tr("The port for the TCP connection."));
        port.setRange(1, 65535);
        port.setDefaultValue(8080);

        environment.setSettingsKey("environment");
        environment.setLabelText("Environment Changes");

        setLayouter([this]() -> Layouting::Layout {
            using namespace Layouting;
            const auto updateVisible = [this]() {
                const QString type = connectionType.volatileValue();
                const bool isStdio = conTypeEnum().keyToValue(type.toUtf8()) == AcpSettings::Stdio;

                launchCommand.setVisible(isStdio);
                launchArguments.setVisible(isStdio);
                environment.setVisible(isStdio);
                host.setVisible(!isStdio);
                port.setVisible(!isStdio);
            };
            updateVisible();

            connect(&connectionType,
                    &StringSelectionAspect::volatileValueChanged,
                    this,
                    updateVisible);

            return Form{
                noMargin,
                name, br,
                connectionType, br,
                launchCommand, br,
                launchArguments, br,
                environment, br,
                host, br,
                port, br,
            };
        });
    }

    AcpSettings::ServerInfo toServerInfo() const
    {
        AcpSettings::ServerInfo info;
        info.id = id.value();
        info.name = name.value();
        info.connectionType = AcpSettings::ConnectionType(
            conTypeEnum().keyToValue(connectionType.value().toUtf8()));

        if (info.connectionType == AcpSettings::Stdio) {
            info.launchInfo = CommandLine(
                FilePath::fromUserInput(launchCommand.value()),
                launchArguments.value(),
                CommandLine::Raw);
            info.envChanges = environment.value();
        } else {
            info.launchInfo = QPair<QString, quint16>(host.value(),
                                                      static_cast<quint16>(port.value()));
        }
        return info;
    }

    StringAspect id{this};
    StringAspect name{this};
    StringSelectionAspect connectionType{this};

    FilePathAspect launchCommand{this};
    StringAspect launchArguments{this};
    EnvironmentChangesAspect environment{this};

    StringAspect host{this};
    IntegerAspect port{this};
};

static QString displayFunc(AcpServerAspect *aspect)
{
    return aspect->name.volatileValue();
}

class AcpManagerSettings : public AspectContainer
{
public:
    AcpManagerSettings()
    {
        setAutoApply(false);
        acpServers.setSettingsKey("AcpServers");
        acpServers.setDisplayStyle(AspectList::DisplayStyle::ListViewWithDetails);
        acpServers.setCreateItemFunction([] {
            return std::make_shared<AcpServerAspect>();
        });

        acpServers.listViewDisplayCallback = displayFunc;

        setLayouter([this]() {
            using namespace Layouting;
            return Column{
                Tr::tr("ACP Servers:"), br,
                &acpServers,
            };
        });

        readSettings();
    }

    static AcpManagerSettings &instance()
    {
        static AcpManagerSettings settings;
        return settings;
    }

    AspectList acpServers{this};
};

static const char ACP_SETTINGS_PAGE_ID[] = "AI.ACPSERVERS";

class AcpSettingsPage final : public Core::IOptionsPage
{
public:
    AcpSettingsPage()
    {
        setId(ACP_SETTINGS_PAGE_ID);
        setDisplayName(Tr::tr("ACP Servers"));
        setCategory(Core::Constants::SETTINGS_CATEGORY_AI);
        setSettingsProvider([] { return &AcpManagerSettings::instance(); });
    }
};

static AcpSettingsPage &settingsPage()
{
    static AcpSettingsPage page;
    return page;
}

// --- AcpSettings implementation ---

AcpSettings::AcpSettings()
{
    QObject::connect(&AcpManagerSettings::instance().acpServers,
                     &BaseAspect::changed,
                     this,
                     &AcpSettings::serversChanged);
}

AcpSettings::~AcpSettings() = default;

AcpSettings &AcpSettings::instance()
{
    static AcpSettings settings;
    return settings;
}

QList<AcpSettings::ServerInfo> AcpSettings::servers()
{
    QList<ServerInfo> result;
    AcpManagerSettings::instance().acpServers.forEachItem(
        [&](const std::shared_ptr<AcpServerAspect> &server) {
            result.push_back(server->toServerInfo());
        });
    return result;
}

void setupAcpSettings()
{
    (void) settingsPage();
    (void) AcpSettings::instance();
}

} // namespace AcpClient::Internal
