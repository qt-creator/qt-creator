// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpsettings.h"
#include "acpclienttr.h"

#include <acp/acpregistry.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <utils/fileutils.h>
#include <utils/appinfo.h>
#include <utils/aspectlist.h>
#include <utils/aspects.h>
#include <utils/co_result.h>
#include <utils/environmentchangesaspect.h>
#include <utils/async.h>
#include <utils/environmentdialog.h>
#include <utils/filestreamer.h>
#include <utils/globaltasktree.h>
#include <utils/layoutbuilder.h>
#include <utils/networkaccessmanager.h>
#include <utils/pathchooser.h>
#include <utils/temporarydirectory.h>
#include <utils/temporaryfile.h>
#include <utils/infolabel.h>
#include <utils/theme/theme.h>

#include <QCryptographicHash>
#include <QDir>
#include <QPromise>
#include <QPushButton>
#include <QStandardItemModel>
#include <QUuid>
#include <QtTaskTree/QNetworkReplyWrapper>
#include <QtTaskTree/QParallelTaskTreeRunner>

using namespace Utils;

namespace AcpClient::Internal {

using namespace QtTaskTree;

using SharedTempFile = std::shared_ptr<TemporaryFilePath>;

// Persistent icon cache: stores themed SVGs under userResourcePath("acpclient/icons")
// keyed by a hash of the URL. Survives restarts.

static Result<> createSvgFile(
    QByteArray data,
    const FilePath &destPath,
    const QString themeColor,
    const std::shared_ptr<QPromise<QIcon>> &promise)
{
    data.replace("currentColor", themeColor.toUtf8());
    co_await destPath.parentDir().ensureWritableDir();
    co_await destPath.writeFileContents(data);

    promise->addResult(QIcon(destPath.toFSPathString()));
    co_return ResultOk;
};

static void fetchIconToPersistentCache(const QString &url, const std::shared_ptr<QPromise<QIcon>> &promise)
{
    if (url.isEmpty()) {
        promise->finish();
        return;
    }

    static QString themeColor = creatorColor(Theme::PanelTextColorDark).toRgb().name();

    const QByteArray hash = QCryptographicHash::hash((url + themeColor).toUtf8(), QCryptographicHash::Sha1);
    const FilePath destPath = Core::ICore::userResourcePath("acpclient/icons")
        / QString::fromLatin1(hash.toHex() + ".svg");

    if (destPath.isFile()) {
        promise->addResult(QIcon(destPath.toFSPathString()));
        promise->finish();
        return;
    }

    const auto setupFetch = [url](FileStreamer &task) {
        task.setSource(FilePath::fromUserInput(url));
        task.setStreamMode(StreamMode::Reader);
    };

    const auto fetchDone = [url, promise, destPath](const FileStreamer &task, DoneWith doneWith) {
        if (doneWith == DoneWith::Success) {
            Result<> res = createSvgFile(task.readData(), destPath, themeColor, promise);
            QTC_CHECK_RESULT(res);
        }
        promise->finish();
    };

    GlobalTaskTree::start({FileStreamerTask(setupFetch, fetchDone)});
}

static std::optional<Acp::Registry::ACPAgentRegistry> s_registry;
static bool s_fetchingRegistry = false;

class AcpRegistryBrowser : public StringSelectionAspect
{
public:
    AcpRegistryBrowser(AspectContainer *parent = nullptr)
        : StringSelectionAspect(parent)
    {
        setComboBoxEditable(false);

        auto fillCallback = [this](ResultCallback resultCb) {
            QList<QStandardItem *> items = registryItems();
            // Until the registry is there, the selected template is kept as an
            // item of its own, or the combo box would fall back to the first
            // item and the selection would be lost.
            const QString selectedId = volatileValue();
            if (!s_registry && !selectedId.isEmpty()) {
                auto selectedItem = new QStandardItem(selectedId);
                selectedItem->setData(selectedId);
                selectedItem->setToolTip(Tr::tr("The agent registry has not been fetched yet."));
                items.append(selectedItem);
            }
            resultCb(items);
        };
        setFillCallback(fillCallback);
    }

    void fixupComboBox(QComboBox *comboBox) override
    {
        comboBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    }

    QList<QStandardItem *> registryItems()
    {
        QList<QStandardItem *> items;
        auto customItem = new QStandardItem(Tr::tr("<Custom>"));
        customItem->setData(QString());
        customItem->setToolTip(
            Tr::tr("Manually specify an agent not listed in the registry."));
        items.append(customItem);

        if (s_registry) {
            for (const auto &agent : s_registry->agents()) {
                auto item = new QStandardItem(agent.name() + " (" + agent.version() + ")");
                item->setToolTip(agent.description());
                item->setData(agent.id());
                Utils::onResultReady(
                    AcpSettings::iconForUrl(agent.icon().value_or(QString())),
                    this,
                    [id = agent.id(), this](const QIcon &icon) {
                        if (auto item = itemById(id))
                            item->setData(icon, Qt::DecorationRole);
                    });
                items.append(item);
            }
        }
        return items;
    }

    static void prefetch(std::function<void(bool success)> onDone = {})
    {
        if (s_registry) {
            if (onDone)
                onDone(true);
            return;
        }

        // A fetch on its way reports through the same signals; a second one would
        // download the registry twice.
        if (s_fetchingRegistry)
            return;
        s_fetchingRegistry = true;

        const auto setupFetch = [](QNetworkReplyWrapper &wrapper) {
            wrapper.setNetworkAccessManager(Utils::NetworkAccessManager::instance());
            wrapper.setOperation(QNetworkAccessManager::Operation::GetOperation);
            QNetworkRequest request(
                QUrl("https://cdn.agentclientprotocol.com/registry/v1/latest/registry.json"));
            wrapper.setRequest(request);
        };
        // Every way out answers, so that a fetch which did not arrive can be
        // told apart from one that is still on its way.
        const auto fetchDone = [onDone](const QNetworkReplyWrapper &wrapper, DoneWith doneWith) {
            const auto answer = [onDone](bool success) {
                s_fetchingRegistry = false;
                if (onDone)
                    onDone(success);
            };

            if (doneWith != DoneWith::Success) {
                answer(false);
                return;
            }

            const QByteArray data = wrapper.reply()->readAll();
            QJsonParseError error;
            const QJsonDocument doc = QJsonDocument::fromJson(data, &error);
            if (error.error != QJsonParseError::NoError) {
                qWarning() << "Failed to parse registry JSON:" << error.errorString();
                answer(false);
                return;
            }

            auto registry = Acp::Registry::fromJson<Acp::Registry::ACPAgentRegistry>(
                doc.object());
            if (!registry) {
                qWarning() << "Failed to parse registry:" << registry.error();
                answer(false);
                return;
            }
            s_registry = std::move(*registry);
            answer(true);
        };

        GlobalTaskTree::start({QNetworkReplyWrapperTask(setupFetch, fetchDone)});
    }

};

class AcpServerAspect : public AspectContainer
{
public:
    AcpServerAspect()
    {
        setAutoApply(false);

        id.setSettingsKey("id");
        id.setValue(QUuid::createUuid().toString());

        iconUrl.setSettingsKey("iconUrl");
        iconUrl.setVisible(false);

        registryBrowser.setLabelText(Tr::tr("Template:"));
        registryBrowser.setSettingsKey("registryTemplate");
        registryBrowser.setToolTip(
            Tr::tr("Select a template from the registry to pre-fill the settings."));

        name.setLabelText(Tr::tr("Name:"));
        name.setSettingsKey("name");
        name.setToolTip(Tr::tr("The display name."));
        name.setDisplayStyle(StringAspect::DisplayStyle::LineEditDisplay);
        name.setDefaultValue(Tr::tr("<New Server>"));

        launchCommand.setLabelText(Tr::tr("Executable:"));
        launchCommand.setSettingsKey("launchCommand");
        launchCommand.setToolTip(Tr::tr("The executable to launch the ACP server process."));
        launchCommand.setExpectedKind(PathChooserKind::ExistingCommand);

        launchArguments.setLabelText(Tr::tr("Arguments:"));
        launchArguments.setSettingsKey("launchArguments");
        launchArguments.setToolTip(
            Tr::tr("The arguments to launch the ACP server process."));
        launchArguments.setDisplayStyle(StringAspect::DisplayStyle::LineEditDisplay);

        environment.setSettingsKey("environment");
        environment.setLabelText(Tr::tr("Environment changes:"));
        connect(&registryBrowser, &AcpRegistryBrowser::volatileValueChanged, this, [this]() {
            applyRegistryTemplate();
        });

        setLayouter([this]() -> Layouting::Layout {
            using namespace Layouting;

            InfoLabel *templateCmdInfo = new InfoLabel();
            templateCmdInfo->setWordWrap(true);
            templateCmdInfo->setElideMode(Qt::ElideNone);
            templateCmdInfo->setToolTip(Tr::tr("The command that will spawn the ACP server."));
            templateCmdInfo->setType(InfoLabelType::Information);
            templateCmdInfo->setTextInteractionFlags(Qt::TextSelectableByMouse);

            InfoLabel *cmdNotFoundLabel = new InfoLabel();
            cmdNotFoundLabel->setWordWrap(true);
            cmdNotFoundLabel->setElideMode(Qt::ElideNone);
            cmdNotFoundLabel->setType(InfoLabelType::Error);
            cmdNotFoundLabel->setTextFormat(Qt::RichText);
            cmdNotFoundLabel->setOpenExternalLinks(true);
            cmdNotFoundLabel->setTextInteractionFlags(
                Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);

            const auto updateCmdInfo = [templateCmdInfo, cmdNotFoundLabel, this]() {
                const bool isCustom = registryBrowser.volatileValue().isEmpty();
                const FilePath executable = launchCommand.expandedValue();
                const QString info = QString("%1 %2")
                                         .arg(executable.toUserOutput())
                                         .arg(launchArguments());
                templateCmdInfo->setText(info);
                templateCmdInfo->setVisible(!isCustom);
                const bool executableCanBeFound
                    = executable.searchInPath(FileUtils::usefulExtraSearchPaths()).exists();
                cmdNotFoundLabel->setVisible(!isCustom && !executableCanBeFound);
                QString warningText = Tr::tr("\"%1\" is needed for this Agent, but was not found.")
                                          .arg(executable.toUserOutput().toHtmlEscaped());
                if (executable == "npx") {
                    const QString npmDocsUrl
                        = "https://docs.npmjs.com/downloading-and-installing-node-js-and-npm";
                    warningText += " "
                        + Tr::tr("Make sure Node.js is installed and npx is available in PATH. "
                                 "See <a href=\"%1\">%1</a> for installation instructions.")
                              .arg(npmDocsUrl);
                } else if (executable == "uvx") {
                    const QString uvDocsUrl
                        = "https://docs.astral.sh/uv/getting-started/installation/";
                    warningText += " "
                        + Tr::tr("Make sure UV is installed and uvx is available in PATH. "
                                 "See <a href=\"%1\">%1</a> for installation instructions.")
                              .arg(uvDocsUrl);
                }
                cmdNotFoundLabel->setText(warningText);
            };

            const auto updateVisible = [this]() {
                const bool isCustom = registryBrowser.volatileValue().isEmpty();
                name.setVisible(isCustom);
                launchCommand.setVisible(isCustom);
                launchArguments.setVisible(isCustom);
                environment.setVisible(isCustom);
            };
            // avoid popping up before getting parented
            QMetaObject::invokeMethod(this, [updateVisible, updateCmdInfo]{
                updateVisible();
                updateCmdInfo();
            }, Qt::QueuedConnection);
            connect(
                &registryBrowser,
                &AcpRegistryBrowser::volatileValueChanged,
                this,
                updateVisible);
            connect(
                &registryBrowser,
                &AcpRegistryBrowser::volatileValueChanged,
                templateCmdInfo,
                updateCmdInfo);

            connect(
                &environment,
                &EnvironmentChangesAspect::volatileValueChanged,
                templateCmdInfo,
                updateCmdInfo);

            // clang-format off
            return Form {
                noMargin,
                registryBrowser, br,
                name, br,
                launchCommand, br,
                launchArguments, br,
                environment, br,
                templateCmdInfo, br,
                cmdNotFoundLabel, br,
            };
            // clang-format on
        });
    }

    void applyRegistryTemplate()
    {
        const QString selectedId = registryBrowser.volatileValue();
        if (selectedId.isEmpty())
            return;

        if (!s_registry) {
            qWarning() << "No registry data available";
            return;
        }

        const auto agent = std::find_if(
            s_registry->agents().begin(),
            s_registry->agents().end(),
            [&selectedId](const Acp::Registry::ACPAgent &a) {
                return a.id() == selectedId;
            });
        if (agent == s_registry->agents().end()) {
            qWarning() << "Selected agent not found in registry:" << selectedId;
            return;
        }
        const Acp::Registry::ACPAgent &selectedAgent = *agent;
        name.setValue(selectedAgent.name());
        iconUrl.setValue(selectedAgent.icon().value_or(QString()));

        if (selectedAgent.distribution().binary()) {
            const FilePath stubPath = (appInfo().libexec / "dlwrapper").withExecutableSuffix();
            launchCommand.setValue(stubPath);

            const QMap<QPair<OsType, OsArch>, std::optional<Acp::Registry::binaryTarget>>
                platformToBinary{
                    {qMakePair(OsTypeMac, OsArchArm64),
                     selectedAgent.distribution().binary()->darwinminusaarch64()},
                    {qMakePair(OsTypeMac, OsArchAMD64),
                     selectedAgent.distribution().binary()->darwinminusx86_64()},
                    {qMakePair(OsTypeLinux, OsArchArm64),
                     selectedAgent.distribution().binary()->linuxminusaarch64()},
                    {qMakePair(OsTypeLinux, OsArchAMD64),
                     selectedAgent.distribution().binary()->linuxminusx86_64()},
                    {qMakePair(OsTypeWindows, OsArchArm64),
                     selectedAgent.distribution().binary()->windowsminusaarch64()},
                    {qMakePair(OsTypeWindows, OsArchAMD64),
                     selectedAgent.distribution().binary()->windowsminusx86_64()},
                };
            const auto it = platformToBinary.find(
                qMakePair(HostOsInfo::hostOs(), HostOsInfo::hostArchitecture()));
            if (it == platformToBinary.end() || !it.value().has_value()) {
                qWarning() << "No suitable binary found for current platform";
                return;
            }
            const auto binary = it.value().value();

            QStringList envChanges;
            const QMap<QString, QString> env = binary.env().value_or(QMap<QString, QString>{});
            for (const auto &[key, value] : env.asKeyValueRange())
                envChanges.append("--env " + key + "=" + value);

            const QString cmdLine
                = (QStringList{binary.cmd()} + binary.args().value_or(QStringList{})).join(" ");

            launchArguments.setValue(QString("--download %1 --version %2 %3 %4")
                                         .arg(binary.archive())
                                         .arg(selectedAgent.version())
                                         .arg(envChanges.join(" "))
                                         .arg(cmdLine));
        } else if (selectedAgent.distribution().npx()) {
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
    }

    AcpSettings::ServerInfo toServerInfo() const
    {
        AcpSettings::ServerInfo info;
        info.id = id.value();
        info.name = name.value();
        info.iconUrl = iconUrl.value();
        info.launchCommand = CommandLine(
            FilePath::fromUserInput(launchCommand.value()),
            launchArguments.value(),
            CommandLine::Raw);
        info.envChanges = environment.value();
        return info;
    }

    AcpRegistryBrowser registryBrowser{this};

    StringAspect id{this};
    StringAspect name{this};
    StringAspect iconUrl{this};

    FilePathAspect launchCommand{this};
    StringAspect launchArguments{this};
    EnvironmentChangesAspect environment{this};
};

class AcpManagerSettings : public AspectContainer
{
public:
    AcpManagerSettings()
    {
        setSettingsGroup("AcpClient");

        setAutoApply(false);
        acpServers.setSettingsKey("AcpServers");
        acpServers.setDisplayStyle(AspectList::DisplayStyle::ListViewWithDetails);
        acpServers.setCreateItemFunction([] { return std::make_shared<AcpServerAspect>(); });

        acpServers.listViewDataCallback = [](AcpServerAspect *aspect, int role) -> QVariant {
            if (role == Qt::DisplayRole)
                return aspect->name.volatileValue();

            if (role == Qt::DecorationRole) {
                const QString iconUrl = aspect->iconUrl.volatileValue();
                if (iconUrl.isEmpty())
                    return QVariant();

                return QVariant::fromValue(
                    AcpSettings::iconForUrl(iconUrl).then(
                        [](const QIcon &icon) -> QVariant { return icon; }));
            }
            return {};
        };

        setLayouter([this]() {
            using namespace Layouting;
            return Column{
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

bool AcpSettings::hasServers()
{
    return AcpManagerSettings::instance().acpServers.size() > 0;
}

bool AcpSettings::isRegistryAvailable()
{
    return s_registry.has_value();
}

QList<AcpSettings::RegistryAgent> AcpSettings::unconfiguredRegistryAgents()
{
    if (!s_registry)
        return {};

    QSet<QString> configured;
    AcpManagerSettings::instance().acpServers.forEachItem(
        [&configured](const std::shared_ptr<AcpServerAspect> &server) {
            const QString templateId = server->registryBrowser.value();
            if (!templateId.isEmpty())
                configured.insert(templateId);
        });

    QList<RegistryAgent> result;
    for (const Acp::Registry::ACPAgent &agent : s_registry->agents()) {
        if (configured.contains(agent.id()))
            continue;
        result.append({agent.id(),
                       agent.name(),
                       agent.description(),
                       agent.icon().value_or(QString())});
    }
    return result;
}

void AcpSettings::addServerFromRegistry(const QString &registryId)
{
    auto server = std::make_shared<AcpServerAspect>();
    server->registryBrowser.setValue(registryId);

    AspectList &servers = AcpManagerSettings::instance().acpServers;
    servers.addItem(server);
    servers.apply();
    AcpManagerSettings::instance().writeSettings();
}

QFuture<QIcon> AcpSettings::iconForUrl(const QString &url)
{
    auto promise = std::make_shared<QPromise<QIcon>>();
    promise->start();
    fetchIconToPersistentCache(url, promise);
    return promise->future();
}

const int AcpTermsVersion = 1;

// The terms and conditions only apply to commercial users. The licensechecker plugin is
// only shipped to them, so its presence is what decides whether they are asked at all.
const char LicenseCheckerId[] = "licensechecker";

class AcpTermsSettings : public AspectContainer
{
public:
    AcpTermsSettings()
    {
        setSettingsGroup("AcpClient");

        acceptedTermsVersion.setSettingsKey("AcceptedTermsVersion");
        acceptedTermsVersion.setDefaultValue(0);

        readSettings();
        migrateAcceptanceFromPluginSettings();
    }

    static AcpTermsSettings &instance()
    {
        static AcpTermsSettings settings;
        return settings;
    }

    IntegerAspect acceptedTermsVersion{this};

private:
    // The extension system used to ask for the terms while the plugin was loaded, and
    // recorded the acceptance under the plugin id. Carry that over once, so that a user who
    // accepted them already is not asked again.
    void migrateAcceptanceFromPluginSettings()
    {
        if (Utils::userSettings().contains("AcpClient/AcceptedTermsVersion"))
            return;

        const QStringList acceptedPlugins
            = Utils::userSettings().value("Plugins/TermsAndConditionsAccepted").toStringList();
        if (!acceptedPlugins.contains("acpclient"))
            return;

        acceptedTermsVersion.setValue(AcpTermsVersion);
        writeSettings();
    }
};

bool acpTermsPending()
{
    const ExtensionSystem::PluginSpec *licenseChecker
        = ExtensionSystem::PluginManager::specById(LicenseCheckerId);
    if (!licenseChecker || !licenseChecker->isEffectivelyEnabled())
        return false;

    return !acpTermsAccepted();
}

bool acpTermsAccepted()
{
    return AcpTermsSettings::instance().acceptedTermsVersion() >= AcpTermsVersion;
}

void setAcpTermsAccepted(bool accepted)
{
    AcpTermsSettings &settings = AcpTermsSettings::instance();
    settings.acceptedTermsVersion.setValue(accepted ? AcpTermsVersion : 0);
    settings.writeSettings();
}

void setupAcpSettings()
{
    (void) settingsPage();
    (void) AcpSettings::instance();
}

void prefetchAcpRegistry()
{
    AcpRegistryBrowser::prefetch(
        [](bool success) {
            if (success) {
                AcpManagerSettings::instance().acpServers.forEachItem(
                    [](const std::shared_ptr<AcpServerAspect> &server) {
                        // A settings page opened before the registry arrived
                        // lists the registry now, too.
                        server->registryBrowser.refill();
                        server->applyRegistryTemplate();
                    });
                AcpManagerSettings::instance().writeSettings();
                emit AcpSettings::instance().serversChanged();
            }
            emit AcpSettings::instance().registryFetched(success);
        });
}

void AcpSettings::fetchRegistry()
{
    prefetchAcpRegistry();
}

} // namespace AcpClient::Internal
