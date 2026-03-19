// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpsettings.h"
#include "acpclienttr.h"

#include <acp/acpregistry.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/algorithm.h>
#include <utils/aspects.h>
#include <utils/environmentdialog.h>
#include <utils/filestreamer.h>
#include <utils/layoutbuilder.h>
#include <utils/networkaccessmanager.h>
#include <utils/stylehelper.h>
#include <utils/temporarydirectory.h>
#include <utils/temporaryfile.h>
#include <utils/theme/theme.h>

#include <QMetaEnum>
#include <QPushButton>
#include <QStandardItemModel>
#include <QUuid>
#include <QtTaskTree/QNetworkReplyWrapper>
#include <QtTaskTree/QParallelTaskTreeRunner>

using namespace Utils;

namespace AcpClient::Internal {

using namespace QtTaskTree;

using SharedTempFile = std::shared_ptr<TemporaryFilePath>;

class AcpRegistryBrowser : public StringSelectionAspect
{
public:
    AcpRegistryBrowser(AspectContainer *parent = nullptr)
        : StringSelectionAspect(parent)
    {
        setComboBoxEditable(false);

        auto findIconOrFetch = [this](const FilePath &url) -> QIcon {
            if (url.isEmpty())
                return QIcon();

            if (const auto it = s_iconCache.find(url); it != s_iconCache.end()) {
                return QIcon((*it)->filePath().toFSPathString());
            }

            Storage<SharedTempFile> tmpFile;

            const auto setupFetch = [tmpFile, url](FileStreamer &task) {
                auto tmpResult = TemporaryFilePath::create(
                    TemporaryDirectory::masterDirectoryFilePath() / "acpclient_icon_XXXXXX.svg");
                QTC_ASSERT_RESULT(tmpResult, return);
                *tmpFile = SharedTempFile(tmpResult->release());

                task.setSource(url);
                task.setDestination((*tmpFile)->filePath());
            };

            const auto fetchDone = [this, tmpFile, url](DoneWith doneWith) {
                if (doneWith != DoneWith::Success)
                    return;

                auto data = (*tmpFile)->filePath().fileContents();
                QTC_ASSERT_RESULT(data, return);

                data->replace(
                    "currentColor", creatorColor(Theme::IconsBaseColor).toRgb().name().toUtf8());
                QTC_ASSERT_RESULT((*tmpFile)->filePath().writeFileContents(*data), return);

                s_iconCache.insert(url, *tmpFile);
                refill();
            };

            m_runner.start(Group{tmpFile, FileStreamerTask(setupFetch, fetchDone)});

            return QIcon();
        };

        auto fillCallback = [this, findIconOrFetch](ResultCallback resultCb) {
            // Cached ?
            if (s_registry) {
                QList<QStandardItem *> items;
                auto customItem = new QStandardItem(Tr::tr("<Custom>"));
                customItem->setData(QString());
                customItem->setToolTip(
                    Tr::tr("Manually specify an agent not listed in the registry"));
                items.append(customItem);

                for (const auto &agent : s_registry->agents()) {
                    auto item = new QStandardItem(agent.name() + " (" + agent.version() + ")");
                    item->setToolTip(agent.description());
                    item->setData(agent.id());
                    item->setData(
                        findIconOrFetch(FilePath::fromUserInput(agent.icon().value_or(QString()))),
                        Qt::DecorationRole);

                    items.append(item);
                }
                resultCb(items);
                return;
            }

            const auto setupFetch = [](QNetworkReplyWrapper &wrapper) {
                wrapper.setNetworkAccessManager(Utils::NetworkAccessManager::instance());
                wrapper.setOperation(QNetworkAccessManager::Operation::GetOperation);
                QNetworkRequest request(
                    QUrl("https://cdn.agentclientprotocol.com/registry/v1/latest/registry.json"));
                wrapper.setRequest(request);
            };
            const auto fetchDone =
                [resultCb, findIconOrFetch](const QNetworkReplyWrapper &wrapper, DoneWith doneWith) {
                    if (doneWith != DoneWith::Success)
                        return;

                    const QByteArray data = wrapper.reply()->readAll();
                    QJsonParseError error;
                    const QJsonDocument doc = QJsonDocument::fromJson(data, &error);
                    if (error.error != QJsonParseError::NoError) {
                        qWarning() << "Failed to parse registry JSON:" << error.errorString();
                        return;
                    }

                    auto registry = Acp::Registry::fromJson<Acp::Registry::ACPAgentRegistry>(
                        doc.object());

                    if (!registry) {
                        s_registry.reset();
                        qWarning() << "Failed to parse registry:" << registry.error();
                        return;
                    }
                    s_registry = std::move(*registry);

                    QList<QStandardItem *> items;
                    auto customItem = new QStandardItem(Tr::tr("<Custom>"));
                    customItem->setData(QString());
                    customItem->setToolTip(
                        Tr::tr("Manually specify an agent not listed in the registry"));
                    items.append(customItem);

                    for (const auto &agent : s_registry->agents()) {
                        auto item = new QStandardItem(agent.name() + " (" + agent.version() + ")");
                        item->setToolTip(agent.description());
                        item->setData(agent.id());
                        item->setData(
                            findIconOrFetch(
                                FilePath::fromUserInput(agent.icon().value_or(QString()))),
                            Qt::DecorationRole);
                        items.append(item);
                    }

                    resultCb(items);
                };
            m_runner.start(Group{QNetworkReplyWrapperTask(setupFetch, fetchDone)});
        };
        setFillCallback(fillCallback);
    }

    std::optional<Acp::Registry::ACPAgentRegistry> registry() const { return s_registry; }

private:
    QParallelTaskTreeRunner m_runner;
    static QMap<FilePath, SharedTempFile> s_iconCache;
    static std::optional<Acp::Registry::ACPAgentRegistry> s_registry;
};

QMap<FilePath, SharedTempFile> AcpRegistryBrowser::s_iconCache;
std::optional<Acp::Registry::ACPAgentRegistry> AcpRegistryBrowser::s_registry;

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

        registryBrowser.setLabelText(Tr::tr("Template:"));
        registryBrowser.setSettingsKey("registryTemplate");
        registryBrowser.setToolTip(
            Tr::tr("Select a template from the registry to pre-fill the settings."));

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
        connect(&registryBrowser, &AcpRegistryBrowser::volatileValueChanged, this, [this]() {
            const QString selectedId = registryBrowser.volatileValue();
            if (selectedId.isEmpty())
                return;

            const auto &registry = registryBrowser.registry();
            if (!registry) {
                qWarning() << "No registry data available";
                return;
            }

            const auto agent = std::find_if(
                registry->agents().begin(),
                registry->agents().end(),
                [&selectedId](const Acp::Registry::ACPAgent &agent) {
                    return agent.id() == selectedId;
                });
            if (agent == registry->agents().end()) {
                qWarning() << "Selected agent not found in registry:" << selectedId;
                return;
            }
            const Acp::Registry::ACPAgent &selectedAgent = *agent;
            name.setValue(selectedAgent.name());
            if (selectedAgent.distribution().binary()) {
                qWarning()
                    << "Registry entry has distribution with binary, but launching from binary is "
                       "not yet supported. Ignoring binary and falling back to command line.";
                return;
            }
            if (selectedAgent.distribution().npx()) {
                launchCommand.setValue(FilePath("npx"));
                launchArguments.setValue(
                    selectedAgent.distribution().npx()->package() + " "
                    + selectedAgent.distribution().npx()->args().value_or(QStringList{}).join(" "));
            } else if (selectedAgent.distribution().uvx()) {
                launchCommand.setValue(FilePath("uvx"));
                launchArguments.setValue(
                    selectedAgent.distribution().uvx()->package() + " "
                    + selectedAgent.distribution().uvx()->args().value_or(QStringList{}).join(" "));
            }
        });

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

            connect(
                &connectionType, &StringSelectionAspect::volatileValueChanged, this, updateVisible);

            // clang-format off
            return Form{
                noMargin,
                registryBrowser, br,
                name, br,
                connectionType, br,
                launchCommand, br,
                launchArguments, br,
                environment, br,
                host, br,
                port, br,
            };
            // clang-format on
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
            info.launchInfo
                = QPair<QString, quint16>(host.value(), static_cast<quint16>(port.value()));
        }
        return info;
    }

    AcpRegistryBrowser registryBrowser{this};

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
        acpServers.setCreateItemFunction([] { return std::make_shared<AcpServerAspect>(); });

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
    QObject::connect(
        &AcpManagerSettings::instance().acpServers,
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
