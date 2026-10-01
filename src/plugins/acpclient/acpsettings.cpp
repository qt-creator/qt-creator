// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "acpsettings.h"
#include "acpclienttr.h"

#include <acp/acpregistry.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

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
#include <utils/guiutils.h>
#include <utils/layoutbuilder.h>
#include <utils/networkaccessmanager.h>
#include <utils/pathchooser.h>
#include <utils/qtcsettings.h>
#include <utils/temporarydirectory.h>
#include <utils/temporaryfile.h>
#include <utils/infolabel.h>
#include <utils/stylehelper.h>
#include <utils/theme/theme.h>

#include <QCryptographicHash>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QLabel>
#include <QPromise>
#include <QPushButton>
#include <QRadioButton>
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

static const char kRegistryUrl[]
    = "https://cdn.agentclientprotocol.com/registry/v1/latest/registry.json";
static const char kRegistryAccessKey[] = "AcpClient/AllowRegistryDownload";

enum class RegistryUpdate { Updated, Denied, Failed };

// OnDemand makes do with a registry that is loaded already. Only ManualUpdate
// asks again when a decision about the download is remembered.
enum class RegistryRequest { OnDemand, AutomaticUpdate, ManualUpdate };

static void refillRegistryBrowsers();
static void applyRegistryToServers();
static void emitRegistryResult(RegistryUpdate result);

static QList<std::function<void(RegistryUpdate)>> s_pendingCallbacks;
static bool s_fetching = false;
// Lasts for the session, so that only an explicit request asks again.
static bool s_registryDeclined = false;

static std::optional<bool> rememberedRegistryAccess()
{
    const QVariant remembered = Core::ICore::settings()->value(kRegistryAccessKey);
    if (!remembered.isValid())
        return std::nullopt;
    return remembered.toBool();
}

static void requestRegistryAccess(
    const std::function<void(bool allowed)> &callback, RegistryRequest request)
{
    const std::optional<bool> remembered = rememberedRegistryAccess();
    if (remembered && request != RegistryRequest::ManualUpdate) {
        callback(*remembered);
        return;
    }

    auto dialog = new QDialog(Utils::dialogParent());
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(Tr::tr("Download Agent Registry"));

    auto description = new QLabel(
        Tr::tr("To list the available agents, Qt Creator downloads the agent registry from "
               "<a href=\"%1\">%1</a>.")
            .arg(QString::fromLatin1(kRegistryUrl)));
    description->setTextFormat(Qt::RichText);
    description->setOpenExternalLinks(true);
    description->setWordWrap(true);

    auto alwaysAllow = new QRadioButton(
        Tr::tr("Always allow, including automatic updates of the configured agents"));
    auto allowOnce = new QRadioButton(Tr::tr("Allow once"));
    auto blockOnce = new QRadioButton(Tr::tr("Block once"));
    auto alwaysBlock = new QRadioButton(Tr::tr("Always block"));
    if (!remembered)
        allowOnce->setChecked(true);
    else if (*remembered)
        alwaysAllow->setChecked(true);
    else
        alwaysBlock->setChecked(true);

    auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QObject::connect(buttons, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

    using namespace Layouting;
    // clang-format off
    Column {
        spacing(StyleHelper::SpacingTokens::GapVL),
        description,
        Column {
            spacing(StyleHelper::SpacingTokens::GapVXs),
            alwaysAllow,
            allowOnce,
            blockOnce,
            alwaysBlock,
        },
        buttons,
    }.attachTo(dialog);
    // clang-format on

    QObject::connect(
        dialog,
        &QDialog::finished,
        dialog,
        [alwaysAllow, allowOnce, alwaysBlock, callback](int result) {
            if (result != QDialog::Accepted) {
                callback(false);
                return;
            }
            // A decision taken once replaces a remembered one, so the next
            // download asks again.
            if (alwaysAllow->isChecked() || alwaysBlock->isChecked())
                Core::ICore::settings()->setValue(kRegistryAccessKey, alwaysAllow->isChecked());
            else
                Core::ICore::settings()->remove(kRegistryAccessKey);
            callback(alwaysAllow->isChecked() || allowOnce->isChecked());
        });

    dialog->show();
}

class AcpRegistryBrowser : public StringSelectionAspect
{
public:
    AcpRegistryBrowser(AspectContainer *parent = nullptr)
        : StringSelectionAspect(parent)
    {
        setComboBoxEditable(false);

        auto fillCallback = [this](ResultCallback resultCb) {
            if (!s_registry && !s_registryDeclined) {
                // The list is what the registry is needed for, so this is where
                // the download is asked for.
                ensureRegistry([](RegistryUpdate result) {
                    if (result == RegistryUpdate::Failed)
                        Core::MessageManager::writeFlashing(
                            Tr::tr("Failed to download the agent registry."));
                });
            }
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
        QList<QStandardItem *> items{customItem()};

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

    static void ensureRegistry(const std::function<void(RegistryUpdate)> &onDone = {},
                               RegistryRequest request = RegistryRequest::OnDemand)
    {
        if (s_registry && request == RegistryRequest::OnDemand) {
            if (onDone)
                onDone(RegistryUpdate::Updated);
            return;
        }

        // A fetch on its way answers every caller that joined it; a second one
        // would download the registry twice.
        if (onDone)
            s_pendingCallbacks.append(onDone);

        if (s_fetching)
            return;
        s_fetching = true;

        requestRegistryAccess(
            [](bool allowed) {
                s_registryDeclined = !allowed;
                if (!allowed) {
                    s_fetching = false;
                    notify(RegistryUpdate::Denied);
                    return;
                }
                fetch();
            },
            request);
    }

private:
    static QStandardItem *customItem()
    {
        auto item = new QStandardItem(Tr::tr("<Custom>"));
        item->setData(QString());
        item->setToolTip(Tr::tr("Manually specify an agent not listed in the registry."));
        return item;
    }

    static void notify(RegistryUpdate result)
    {
        const QList<std::function<void(RegistryUpdate)>> callbacks = s_pendingCallbacks;
        s_pendingCallbacks.clear();
        for (const std::function<void(RegistryUpdate)> &callback : callbacks)
            callback(result);
    }

    static void fetch()
    {
        const auto setupFetch = [](QNetworkReplyWrapper &wrapper) {
            wrapper.setNetworkAccessManager(Utils::NetworkAccessManager::instance());
            wrapper.setOperation(QNetworkAccessManager::Operation::GetOperation);
            QNetworkRequest request(QUrl(QString::fromLatin1(kRegistryUrl)));
            request.setTransferTimeout();
            wrapper.setRequest(request);
        };
        // Every way out answers, so that a fetch which did not arrive can be
        // told apart from one that is still on its way.
        const auto fetchDone = [](const QNetworkReplyWrapper &wrapper, DoneWith doneWith) {
            s_fetching = false;

            if (doneWith != DoneWith::Success) {
                notify(RegistryUpdate::Failed);
                return;
            }

            const QByteArray data = wrapper.reply()->readAll();
            QJsonParseError error;
            const QJsonDocument doc = QJsonDocument::fromJson(data, &error);
            if (error.error != QJsonParseError::NoError) {
                qWarning() << "Failed to parse registry JSON:" << error.errorString();
                notify(RegistryUpdate::Failed);
                return;
            }

            auto registry = Acp::Registry::fromJson<Acp::Registry::ACPAgentRegistry>(
                doc.object());
            if (!registry) {
                qWarning() << "Failed to parse registry:" << registry.error();
                notify(RegistryUpdate::Failed);
                return;
            }
            s_registry = std::move(*registry);
            refillRegistryBrowsers();

            notify(RegistryUpdate::Updated);
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
                Row{
                    st,
                    PushButton{
                        text(Tr::tr("Update Agent Registry")),
                        Layouting::toolTip(Tr::tr("Download the list of available agents again.")),
                        onClicked(this, [this] { updateRegistry(); }),
                    },
                },
            };
        });

        readSettings();
    }

    static AcpManagerSettings &instance()
    {
        static AcpManagerSettings settings;
        return settings;
    }

    // Always asks, with the remembered decision preselected.
    void updateRegistry()
    {
        AcpRegistryBrowser::ensureRegistry(
            [](RegistryUpdate result) {
                if (result == RegistryUpdate::Failed) {
                    Core::MessageManager::writeDisrupting(
                        Tr::tr("Failed to download the agent registry."));
                }
                emitRegistryResult(result);
            },
            RegistryRequest::ManualUpdate);
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

static void emitRegistryResult(RegistryUpdate result)
{
    if (result == RegistryUpdate::Denied)
        emit AcpSettings::instance().registryDenied();
    else
        emit AcpSettings::instance().registryFetched(result == RegistryUpdate::Updated);
}

#ifdef WITH_TESTS
void updateAcpRegistry()
{
    AcpManagerSettings::instance().updateRegistry();
}

QWidget *createRegistryBrowserWidget()
{
    auto browser = new AcpRegistryBrowser;
    QWidget *widget = Layouting::Column{*browser}.emerge();
    browser->setParent(widget);
    return widget;
}
#endif

void updateAcpRegistryIfAllowed()
{
    if (!rememberedRegistryAccess().value_or(false))
        return;

    AcpRegistryBrowser::ensureRegistry(
        [](RegistryUpdate result) {
            if (result == RegistryUpdate::Updated)
                applyRegistryToServers();
            emitRegistryResult(result);
        },
        RegistryRequest::AutomaticUpdate);
}

// A settings page opened before the registry arrived lists the registry now,
// too. Refilling keeps the selection, and unlike applying the templates it
// leaves unapplied edits on the page alone.
static void refillRegistryBrowsers()
{
    AcpManagerSettings::instance().acpServers.forEachItem(
        [](const std::shared_ptr<AcpServerAspect> &server) {
            server->registryBrowser.refill();
        });
}

static void applyRegistryToServers()
{
    AcpManagerSettings::instance().acpServers.forEachItem(
        [](const std::shared_ptr<AcpServerAspect> &server) {
            server->applyRegistryTemplate();
        });
    AcpManagerSettings::instance().writeSettings();
    emit AcpSettings::instance().serversChanged();
}

void AcpSettings::fetchRegistry()
{
    // On demand, so the templates are left alone: applying them writes the
    // aspects, which would take unapplied edits on an open preferences page
    // with it.
    AcpRegistryBrowser::ensureRegistry(
        [](RegistryUpdate result) { emitRegistryResult(result); });
}

} // namespace AcpClient::Internal
