// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "axivionsettings.h"

#include "axiviontr.h"
#include "coreplugin/messagemanager.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/credentialquery.h>
#include <coreplugin/icore.h>
#include <solutions/tasking/tasktree.h>

#include <utils/algorithm.h>
#include <utils/id.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/utilsicons.h>

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QTreeWidget>
#include <QVBoxLayout>

using namespace Core;
using namespace Utils;
using namespace Tasking;

namespace Axivion::Internal {

bool AxivionServer::operator==(const AxivionServer &other) const
{
    return id == other.id && dashboard == other.dashboard && username == other.username;
}

bool AxivionServer::operator!=(const AxivionServer &other) const
{
    return !(*this == other);
}

QJsonObject AxivionServer::toJson() const
{
    QJsonObject result;
    result.insert("id", id.toString());
    result.insert("dashboard", dashboard);
    result.insert("username", username);
    return result;
}

static QString fixUrl(const QString &url)
{
    const QString trimmed = Utils::trimBack(url, ' ');
    return trimmed.endsWith('/') ? trimmed : trimmed + '/';
}

AxivionServer AxivionServer::fromJson(const QJsonObject &json)
{
    const AxivionServer invalidServer;
    const QJsonValue id = json.value("id");
    if (id == QJsonValue::Undefined)
        return invalidServer;
    const QJsonValue dashboard = json.value("dashboard");
    if (dashboard == QJsonValue::Undefined)
        return invalidServer;
    const QJsonValue username = json.value("username");
    if (username == QJsonValue::Undefined)
        return invalidServer;
    return {Id::fromString(id.toString()), fixUrl(dashboard.toString()), username.toString()};
}

bool PathMapping::operator==(const PathMapping &other) const
{
    return projectName == other.projectName && analysisPath == other.analysisPath
            && localPath == other.localPath;
}

bool PathMapping::operator!=(const PathMapping &other) const
{
    return !(*this == other);
}

static Result<> analysisPathValid(const FilePath &analysisPath)
{
    if (analysisPath.isEmpty())
        return ResultOk;

    if (!analysisPath.isLocal() || analysisPath.isAbsolutePath())
        return ResultError(Tr::tr("Path must be relative."));

    static const QRegularExpression invalid("^(.*/)?\\.\\.?(/.*)?$");
    if (invalid.match(analysisPath.path()).hasMatch())
        return ResultError(Tr::tr("Invalid path elements (. or ..)."));

    return ResultOk;
}

bool PathMapping::isValid() const
 {
    return !projectName.isEmpty() && !localPath.isEmpty()
            && localPath.isLocal() && localPath.isAbsolutePath()
            && analysisPathValid(analysisPath);
}

static FilePath axivionJsonFilePath()
{
    return FilePath::fromString(ICore::settings()->fileName()).parentDir()
            .pathAppended("qtcreator/axivion.json");
}

static void writeAxivionJson(const FilePath &filePath, const QList<AxivionServer> &servers)
{
    QJsonDocument doc;
    QJsonArray serverArray;
    for (const AxivionServer &server : servers)
        serverArray.append(server.toJson());
    doc.setArray(serverArray);
    // FIXME error handling?
    filePath.writeFileContents(doc.toJson());
    filePath.setPermissions(QFile::ReadUser | QFile::WriteUser);
}

static QList<AxivionServer> readAxivionJson(const FilePath &filePath)
{
    if (!filePath.exists())
        return {};
    Result<QByteArray> contents = filePath.fileContents();
    if (!contents)
        return {};
    const QJsonDocument doc = QJsonDocument::fromJson(*contents);
    if (doc.isObject()) // old approach
        return { AxivionServer::fromJson(doc.object()) };
    if (!doc.isArray())
        return {};

    QList<AxivionServer> result;
    const QJsonArray serverArray = doc.array();
    for (const auto &serverValue : serverArray) {
        if (!serverValue.isObject())
            continue;
        result.append(AxivionServer::fromJson(serverValue.toObject()));
    }
    return result;
}

static QVariant pathMappingToVariant(const PathMapping &pm)
{
    QVariantMap m;
    m.insert("ProjectName", pm.projectName);
    m.insert("AnalysisPath", pm.analysisPath.toSettings());
    m.insert("LocalPath", pm.localPath.toSettings());
    return m;
}

static QVariant pathMappingsToSetting(const QList<PathMapping> &mappings)
{
    return Utils::transform(mappings,
                            [](const PathMapping &m) { return pathMappingToVariant(m); });
}

static PathMapping pathMappingFromVariant(const QVariant &m)
{
    const QVariantMap map = m.toMap();
    return map.isEmpty() ? PathMapping{}
                         : PathMapping{map.value("ProjectName").toString(),
                                       FilePath::fromSettings(map.value("AnalysisPath")),
                                       FilePath::fromSettings(map.value("LocalPath"))};
}

static QList<PathMapping> pathMappingsFromSetting(const QVariant &value)
{
    return Utils::transform(
                Utils::filtered(value.toList(), &QVariant::isValid), &pathMappingFromVariant);
}

// AxivionSettings

class PathMappingSettings final : public BaseAspect
{
public:
    PathMappingSettings()
    {
        setSettingsKey("Axivion/PathMappings");
    }

    void setVariantValue(const QVariant &value, Announcement howToAnnounce = DoEmit) final
    {
        m_pathMapping = pathMappingsFromSetting(value);
        if (howToAnnounce == DoEmit)
            emit changed();
    }

    QVariant variantValue() const final { return pathMappingsToSetting(m_pathMapping); }

    const QList<PathMapping> validPathMappings() const
    {
        return Utils::filtered(m_pathMapping, &PathMapping::isValid);
    }

private:
    QList<PathMapping> m_pathMapping;
};

PathMappingSettings &pathMappingSettings()
{
    static PathMappingSettings thePathMapping;
    return thePathMapping;
}

AxivionSettings &settings()
{
    static AxivionSettings theSettings;
    return theSettings;
}

AxivionSettings::AxivionSettings()
{
    setSettingsGroup("Axivion");
    setAutoApply(false);

    highlightMarks.setSettingsKey("HighlightMarks");
    highlightMarks.setLabelText(Tr::tr("Highlight marks"));
    highlightMarks.setToolTip(Tr::tr("Marks issues on the scroll bar."));
    highlightMarks.setDefaultValue(false);

    axivionSuitePath.setSettingsKey("SuitePath");
    axivionSuitePath.setExpectedKind(PathChooser::ExistingDirectory);
    axivionSuitePath.setAllowPathFromDevice(false);
    axivionSuitePath.setLabelText(Tr::tr("Axivion Suite Path:"));

    saveOpenFiles.setSettingsKey("SaveOpenFiles");
    saveOpenFiles.setLabelText(Tr::tr("Save all open files before starting an analysis"));

    bauhausPython.setSettingsKey("BauhausPython");
    bauhausPython.setExpectedKind(PathChooser::ExistingCommand);
    bauhausPython.setAllowPathFromDevice(false);
    bauhausPython.setLabelText("BAUHAUS_PYTHON:");
    bauhausPython.setToolTip(Tr::tr("Path to python executable.\nSet it to overwrite global "
                                    "environment or if Axivion fails to find python in PATH."));

    javaHome.setSettingsKey("JavaHome");
    javaHome.setExpectedKind(PathChooser::ExistingDirectory);
    javaHome.setAllowPathFromDevice(false);
    javaHome.setLabelText("JAVA_HOME:");
    javaHome.setToolTip(Tr::tr("Set it to overwrite global environment or if Axivion fails to "
                               "find java in PATH."));

    m_defaultServerId.setSettingsKey("DefaultDashboardId");
    pathMappingSettings().readSettings();
    AspectContainer::readSettings();
    if (!axivionSuitePath().isEmpty())
        validatePath();

    m_allServers = readAxivionJson(axivionJsonFilePath());

    if (m_allServers.size() == 1 && m_defaultServerId().isEmpty()) // handle settings transition
        m_defaultServerId.setValue(m_allServers.first().id.toString());

    connect(&axivionSuitePath, &BaseAspect::changed, this, [this] { m_versionInfo.reset(); });
}

void AxivionSettings::toSettings() const
{
    writeAxivionJson(axivionJsonFilePath(), m_allServers);
    AspectContainer::writeSettings();
}

Id AxivionSettings::defaultDashboardId() const
{
    return Id::fromString(m_defaultServerId());
}

const AxivionServer AxivionSettings::defaultServer() const
{
    return serverForId(defaultDashboardId());
}

const AxivionServer AxivionSettings::serverForId(const Utils::Id &id) const
{
    return Utils::findOrDefault(m_allServers, [&id](const AxivionServer &server) {
        return id == server.id;
    });
}

void AxivionSettings::disableCertificateValidation(const Utils::Id &id)
{
    const int index = Utils::indexOf(m_allServers, [&id](const AxivionServer &server) {
        return id == server.id;
    });
    if (index == -1)
        return;

    m_allServers[index].validateCert = false;
}

bool AxivionSettings::updateDashboardServers(const QList<AxivionServer> &other,
                                             const Utils::Id &selected)
{
    const Id oldDefault = defaultDashboardId();
    if (selected == oldDefault && m_allServers == other)
        return false;


    // collect dashserver items that have been removed,
    // so we can delete the api tokens from the credentials store
    const QStringList previousKeys = Utils::transform(m_allServers, &credentialKey);
    const QStringList updatedKeys = Utils::transform(other, &credentialKey);
    const QStringList keysToRemove = Utils::filtered(previousKeys, [updatedKeys](const QString &key) {
        return !updatedKeys.contains(key);
    });

    m_defaultServerId.setValue(selected.toString(), BeQuiet);
    m_allServers = other;
    emit serversChanged(); // should we be more detailed? (id)

    const LoopList iterator(keysToRemove);

    const auto onDeleteKeySetup = [iterator](CredentialQuery &query) {
        MessageManager::writeSilently(Tr::tr("Axivion: Deleting API token for %1 as respective "
                                             "dashboard server was removed.")
                                          .arg(*iterator));
        query.setOperation(CredentialOperation::Delete);
        query.setService(s_axivionKeychainService);
        query.setKey(*iterator);
    };

    const Group recipe {
        For (iterator) >> Do {
            CredentialQueryTask(onDeleteKeySetup)
        }
    };

    m_taskTreeRunner.start(recipe);

    return true;
}

const QList<PathMapping> AxivionSettings::validPathMappings() const
{
    return pathMappingSettings().validPathMappings();
}

void AxivionSettings::validatePath()
{
    if (m_versionInfo) {
        emit suitePathValidated();
        return;
    }

    const FilePath &suitePath = axivionSuitePath();
    const FilePath info = (suitePath.isEmpty() ? FilePath{"axivion_suite_info"}
                                               : suitePath.pathAppended("bin/axivion_suite_info"))
            .withExecutableSuffix();

    const auto onSetup = [info](Process &process) {
        process.setCommand({info, {"--json"}});
    };
    const auto onDone = [this](const Process &process) {
        const auto onFinish = qScopeGuard([this] { emit suitePathValidated(); });
        m_versionInfo.reset();
        if (process.result() != ProcessResult::FinishedWithSuccess)
            return;

        const QString output = process.allOutput();
        QJsonParseError error;
        const QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError)
            return;
        if (!doc.isObject())
            return;
        const QJsonObject obj = doc.object();
        const QString version = obj.value("versionNumber").toString();
        const QString date = obj.value("dateTime").toString();
        m_versionInfo.emplace(AxivionVersionInfo{version, date});
    };

    m_taskTreeRunner.start({ProcessTask(onSetup, onDone)});
}

static QString escapeKey(const QString &string)
{
    QString escaped = string;
    return escaped.replace('\\', "\\\\").replace('@', "\\@");
}

QString credentialKey(const AxivionServer &server)
{
    return escapeKey(server.username) + '@' + escapeKey(server.dashboard);
}

// AxivionSettingsPage

// may allow some invalid, but does some minimal check for legality
static bool hostValid(const QString &host)
{
    static const QRegularExpression ip(R"(^(\d+).(\d+).(\d+).(\d+)$)");
    static const QRegularExpression dn(R"(^([a-zA-Z0-9][a-zA-Z0-9-]+\.)*[a-zA-Z0-9][a-zA-Z0-9-]+$)");
    const QRegularExpressionMatch match = ip.match(host);
    if (match.hasMatch()) {
        for (int i = 1; i < 5; ++i) {
            int val = match.captured(i).toInt();
            if (val < 0 || val > 255)
                return false;
        }
        return true;
    }
    return dn.match(host).hasMatch();
}

static bool isUrlValid(const QString &in)
{
    const QUrl url(in);
    return hostValid(url.host()) && (url.scheme() == "https" || url.scheme() == "http");
}

class DashboardSettingsWidget : public QWidget
{
public:
    explicit DashboardSettingsWidget(QWidget *parent, QPushButton *ok = nullptr);

    AxivionServer dashboardServer() const;
    void setDashboardServer(const AxivionServer &server);

    bool isValid() const;

private:
    Id m_id;
    StringAspect m_dashboardUrl;
    StringAspect m_username;
    BoolAspect m_valid;
};

DashboardSettingsWidget::DashboardSettingsWidget(QWidget *parent, QPushButton *ok)
    : QWidget(parent)
{
    m_dashboardUrl.setLabelText(Tr::tr("Dashboard URL:"));
    m_dashboardUrl.setDisplayStyle(StringAspect::LineEditDisplay);
    m_dashboardUrl.setValidationFunction([](FancyLineEdit *edit) -> Result<> {
        if (isUrlValid(edit->text()))
            return ResultOk;
        return ResultError(QString());
    });

    m_username.setLabelText(Tr::tr("Username:"));
    m_username.setDisplayStyle(StringAspect::LineEditDisplay);
    m_username.setPlaceHolderText(Tr::tr("User name"));

    using namespace Layouting;

    Form {
        m_dashboardUrl, br,
        m_username, br,
        noMargin
    }.attachTo(this);

    QTC_ASSERT(ok, return);
    auto checkValidity = [this, ok] {
        m_valid.setValue(isValid());
        ok->setEnabled(m_valid());
    };
    m_dashboardUrl.addOnChanged(this, checkValidity);
    m_username.addOnChanged(this, checkValidity);
}

AxivionServer DashboardSettingsWidget::dashboardServer() const
{
    AxivionServer result;
    if (m_id.isValid())
        result.id = m_id;
    else
        result.id = Id::generate();
    result.dashboard = fixUrl(m_dashboardUrl());
    result.username = m_username();
    return result;
}

void DashboardSettingsWidget::setDashboardServer(const AxivionServer &server)
{
    m_id = server.id;
    m_dashboardUrl.setValue(server.dashboard);
    m_username.setValue(server.username);
}

bool DashboardSettingsWidget::isValid() const
{
    return isUrlValid(m_dashboardUrl());
}

// PathMappingSettingsWidget

class PathMappingDetails : public AspectContainer
{
public:
    PathMappingDetails()
    {
        m_projectName.setLabelText(Tr::tr("Project name:"));
        m_projectName.setDisplayStyle(StringAspect::LineEditDisplay);
        m_projectName.setValidationFunction([](FancyLineEdit *edit) -> Result<> {
            QTC_ASSERT(edit, return ResultError(ResultAssert));
            if (edit->text().isEmpty())
                return ResultError(Tr::tr("Project name must be non-empty."));
            return ResultOk;
        });
        m_analysisPath.setLabelText(Tr::tr("Analysis path:"));
        m_analysisPath.setDisplayStyle(StringAspect::LineEditDisplay);
        m_analysisPath.setValidationFunction([](FancyLineEdit *edit) -> Result<> {
            QTC_ASSERT(edit, return ResultError(ResultAssert));
            // do NOT use fromUserInput() as this also cleans the path
            const FilePath fp = FilePath::fromString(edit->text().replace('\\', '/'));
            return analysisPathValid(fp);
        });
        m_localPath.setLabelText(Tr::tr("Local path:"));
        m_localPath.setExpectedKind(PathChooser::ExistingDirectory);
        m_localPath.setAllowPathFromDevice(false);

        using namespace Layouting;
        setLayouter([this] {
            return Form {
                        &m_projectName, br,
                        &m_analysisPath, br,
                        &m_localPath,
                        noMargin};
        });
    }

    void updateContent(const PathMapping &mapping)
    {
        m_projectName.setValue(mapping.projectName, BaseAspect::BeQuiet);
        m_analysisPath.setValue(mapping.analysisPath.toUserOutput(), BaseAspect::BeQuiet);
        m_localPath.setValue(mapping.localPath, BaseAspect::BeQuiet);
    }

    PathMapping toPathMapping() const
    {
        return PathMapping{
            m_projectName(), FilePath::fromUserInput(m_analysisPath()), m_localPath()
        };
    }

private:
    StringAspect m_projectName{this};
    StringAspect m_analysisPath{this};
    FilePathAspect m_localPath{this};
};

class AxivionSettingsWidget : public IOptionsPageWidget
{
public:
    AxivionSettingsWidget();

    void apply() override;

private:
    void showServerDialog(bool add);
    void removeCurrentServerConfig();
    void updateDashboardServers();
    void updateEnabledStates();
    void addMapping();
    void deleteMapping();
    void mappingChanged();
    void currentChanged(const QModelIndex &index, const QModelIndex &previous);
    void moveCurrentMapping(bool up);
    void updateVersionAndBuildDate(QLabel *version, QLabel *buildDate);

    QComboBox *m_dashboardServers = nullptr;
    QPushButton *m_editServerButton = nullptr;
    QPushButton *m_removeServerButton = nullptr;
    QTreeWidget m_mappingTree;
    PathMappingDetails m_details;
    QWidget *m_detailsWidget = nullptr;
    QPushButton *m_deleteMappingButton = nullptr;
    QPushButton *m_moveUpMappingButton = nullptr;
    QPushButton *m_moveDownMappingButton = nullptr;
};

AxivionSettingsWidget::AxivionSettingsWidget()
{
    using namespace Layouting;

    m_dashboardServers = new QComboBox(this);
    m_dashboardServers->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    updateDashboardServers();

    auto addServerButton = new QPushButton(Tr::tr("Add..."), this);
    m_editServerButton = new QPushButton(Tr::tr("Edit..."), this);
    m_removeServerButton = new QPushButton(Tr::tr("Remove"), this);

    m_detailsWidget = new QWidget(this);
    m_details.layouter()().attachTo(m_detailsWidget);

    m_mappingTree.setSelectionMode(QAbstractItemView::SingleSelection);
    m_mappingTree.setSelectionBehavior(QAbstractItemView::SelectRows);
    m_mappingTree.setHeaderLabels({Tr::tr("Project Name"), Tr::tr("Analysis Path"),
                                   Tr::tr("Local Path")});

    auto addMappingButton = new QPushButton(Tr::tr("Add"), this);
    m_deleteMappingButton = new QPushButton(Tr::tr("Delete"), this);
    m_moveUpMappingButton = new QPushButton(Tr::tr("Move Up"), this);
    m_moveDownMappingButton = new QPushButton(Tr::tr("Move Down"), this);

    auto version = new QLabel(this);
    auto build = new QLabel(this);

    Column buttons { addMappingButton, m_deleteMappingButton, empty, m_moveUpMappingButton, m_moveDownMappingButton, st };

    Column {
        Layouting::Group {
            title(Tr::tr("Dashboard Servers")),
            Row {
                Form { Tr::tr("Default dashboard server:"), m_dashboardServers, br },
                st,
                Column { addServerButton, m_editServerButton, m_removeServerButton },
            },
        },
        Layouting::Group {
            title(Tr::tr("Path Mapping")),
            Column {
                Row { &m_mappingTree, buttons },
                m_detailsWidget
            }
        },
        Layouting::Group {
            title(Tr::tr("Local Analyses")),
            Column {
                Row { settings().axivionSuitePath },
                Row { settings().saveOpenFiles },
                Form {
                    Tr::tr("Version:"), version, br,
                    Tr::tr("Build date:"), build, br,
                },
                Row { Tr::tr("Contact support@axivion.com if you need assistance.") },
                Space(20),
                Form {
                    settings().bauhausPython, br,
                    settings().javaHome, br
                },
            }
        },
        Layouting::Group {
            title(Tr::tr("Misc Options")),
            Row {settings().highlightMarks },
        },
        st
    }.attachTo(this);

    connect(addServerButton, &QPushButton::clicked, this, [this] {
        // add an empty item unconditionally
        m_dashboardServers->addItem(Tr::tr("unset"), QVariant::fromValue(AxivionServer()));
        m_dashboardServers->setCurrentIndex(m_dashboardServers->count() - 1);
        showServerDialog(true);
    });
    connect(m_editServerButton, &QPushButton::clicked, this, [this] { showServerDialog(false); });
    connect(m_removeServerButton, &QPushButton::clicked,
            this, &AxivionSettingsWidget::removeCurrentServerConfig);

    connect(&settings().axivionSuitePath, &BaseAspect::changed,
            &settings(), &AxivionSettings::validatePath);
    connect(&settings(), &AxivionSettings::suitePathValidated, this,
            [this, version, build] { updateVersionAndBuildDate(version, build); });
    const QList<QTreeWidgetItem *> items = Utils::transform(pathMappingSettings().validPathMappings(),
                                                            [this](const PathMapping &m) {
        QTreeWidgetItem *item = new QTreeWidgetItem(&m_mappingTree,
                                                    {m.projectName,
                                                     m.analysisPath.toUserOutput(),
                                                     m.localPath.toUserOutput()});
        if (!m.isValid())
            item->setIcon(0, Icons::CRITICAL.icon());
        return item;
    });
    m_mappingTree.addTopLevelItems(items);

    m_deleteMappingButton->setEnabled(false);
    m_moveUpMappingButton->setEnabled(false);
    m_moveDownMappingButton->setEnabled(false);

    m_detailsWidget->setVisible(false);

    connect(addMappingButton, &QPushButton::clicked, this, &AxivionSettingsWidget::addMapping);
    connect(m_deleteMappingButton, &QPushButton::clicked, this, &AxivionSettingsWidget::deleteMapping);
    connect(m_moveUpMappingButton, &QPushButton::clicked, this, [this]{ moveCurrentMapping(true); });
    connect(m_moveDownMappingButton, &QPushButton::clicked, this, [this]{ moveCurrentMapping(false); });
    connect(m_mappingTree.selectionModel(), &QItemSelectionModel::currentChanged,
            this, &AxivionSettingsWidget::currentChanged);
    connect(&m_details, &AspectContainer::changed, this,
            &AxivionSettingsWidget::mappingChanged);

    updateEnabledStates();
    settings().validatePath();
}

void AxivionSettingsWidget::apply()
{
    QList<AxivionServer> servers;
    for (int i = 0, end = m_dashboardServers->count(); i < end; ++i)
        servers.append(m_dashboardServers->itemData(i).value<AxivionServer>());
    const Id selected = servers.isEmpty() ? Id{}
                                          : servers.at(m_dashboardServers->currentIndex()).id;
    const bool dirty = settings().isDirty();
    const bool dashboardServersChanged = settings().updateDashboardServers(servers, selected);
    settings().apply();
    if (dirty || dashboardServersChanged)
        settings().toSettings();

    const QList<PathMapping> oldMappings = settings().validPathMappings();
    QList<PathMapping> newMappings;
    for (int row = 0, count = m_mappingTree.topLevelItemCount(); row < count; ++row) {
        const QTreeWidgetItem * const item = m_mappingTree.topLevelItem(row);
        newMappings.append({item->text(0),
                            FilePath::fromUserInput(item->text(1)),
                            FilePath::fromUserInput(item->text(2))});
    }
    if (oldMappings == newMappings)
        return;

    pathMappingSettings().setVariantValue(pathMappingsToSetting(newMappings));
    pathMappingSettings().writeSettings();
}

void AxivionSettingsWidget::updateDashboardServers()
{
    m_dashboardServers->clear();
    const QList<AxivionServer> servers = settings().allAvailableServers();
    for (const AxivionServer &server : servers)
        m_dashboardServers->addItem(server.displayString(), QVariant::fromValue(server));
    int index = Utils::indexOf(servers,
                               [id = settings().defaultDashboardId()](const AxivionServer &s) {
        return id == s.id;
    });
    if (index != -1)
        m_dashboardServers->setCurrentIndex(index);
}

void AxivionSettingsWidget::updateEnabledStates()
{
    const bool enabled = m_dashboardServers->count();
    m_editServerButton->setEnabled(enabled);
    m_removeServerButton->setEnabled(enabled);
}

void AxivionSettingsWidget::removeCurrentServerConfig()
{
    const QString config = m_dashboardServers->currentData().value<AxivionServer>().displayString();
    if (QMessageBox::question(
            ICore::dialogParent(),
            Tr::tr("Remove Server Configuration"),
            Tr::tr("Remove the server configuration \"%1\"?").arg(config))
        != QMessageBox::Yes) {
        return;
    }
    m_dashboardServers->removeItem(m_dashboardServers->currentIndex());
    updateEnabledStates();
}

void AxivionSettingsWidget::showServerDialog(bool add)
{
    const AxivionServer old = m_dashboardServers->currentData().value<AxivionServer>();
    QDialog d;
    d.setWindowTitle(add ? Tr::tr("Add Dashboard Configuration")
                         : Tr::tr("Edit Dashboard Configuration"));
    QVBoxLayout *layout = new QVBoxLayout;
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Ok, this);
    auto ok = buttons->button(QDialogButtonBox::Ok);
    auto dashboardWidget = new DashboardSettingsWidget(this, ok);
    dashboardWidget->setDashboardServer(old);
    layout->addWidget(dashboardWidget);
    ok->setEnabled(dashboardWidget->isValid());
    connect(buttons->button(QDialogButtonBox::Cancel), &QPushButton::clicked, &d, &QDialog::reject);
    connect(ok, &QPushButton::clicked, &d, &QDialog::accept);
    layout->addWidget(buttons);
    d.setLayout(layout);
    d.resize(500, 200);

    if (d.exec() != QDialog::Accepted) {
        if (add) { // if we canceled an add, remove the canceled item
            m_dashboardServers->removeItem(m_dashboardServers->currentIndex());
            updateEnabledStates();
        }
        return;
    }
    if (dashboardWidget->isValid()) {
        const AxivionServer server = dashboardWidget->dashboardServer();
        if (server != old) {
            m_dashboardServers->setItemData(m_dashboardServers->currentIndex(),
                                            QVariant::fromValue(server));
            m_dashboardServers->setItemData(m_dashboardServers->currentIndex(),
                                            server.displayString(), Qt::DisplayRole);
        }
    }
    updateEnabledStates();
}

void AxivionSettingsWidget::addMapping()
{
    QTreeWidgetItem *item = new QTreeWidgetItem(&m_mappingTree, {"", "", ""});
    m_mappingTree.setCurrentItem(item);
    item->setIcon(0, Icons::CRITICAL.icon());
}

void AxivionSettingsWidget::deleteMapping()
{
    QTreeWidgetItem *item = m_mappingTree.currentItem();
    QTC_ASSERT(item, return);
    const QModelIndex index = m_mappingTree.indexFromItem(item);
    if (!index.isValid())
        return;
    m_mappingTree.model()->removeRow(index.row());
}

void AxivionSettingsWidget::mappingChanged()
{
    QTreeWidgetItem *item = m_mappingTree.currentItem();
    QTC_ASSERT(item, return);
    PathMapping modified = m_details.toPathMapping();
    item->setText(0, modified.projectName);
    item->setText(1, modified.analysisPath.toUserOutput());
    item->setText(2, modified.localPath.toUserOutput());
    item->setIcon(0, modified.isValid() ? QIcon{} : Icons::CRITICAL.icon());
}

void AxivionSettingsWidget::currentChanged(const QModelIndex &index,
                                               const QModelIndex &/*previous*/)
{
    const bool indexValid = index.isValid();
    const int row = index.row();
    m_deleteMappingButton->setEnabled(indexValid);
    m_moveUpMappingButton->setEnabled(indexValid && row > 0);
    m_moveDownMappingButton->setEnabled(indexValid && row < m_mappingTree.topLevelItemCount() - 1);
    m_detailsWidget->setVisible(indexValid);
    if (indexValid) {
        const QTreeWidgetItem * const item = m_mappingTree.itemFromIndex(index);
        m_details.updateContent({item->text(0),
                                 FilePath::fromUserInput(item->text(1)),
                                 FilePath::fromUserInput(item->text(2))});
    }
}

void AxivionSettingsWidget::moveCurrentMapping(bool up)
{
    const int itemCount = m_mappingTree.topLevelItemCount();
    const QModelIndexList indexes = m_mappingTree.selectionModel()->selectedRows();
    QTC_ASSERT(indexes.size() == 1, return);
    const QModelIndex index = indexes.first();
    QTC_ASSERT(index.isValid(), return);
    const int row = index.row();
    QTC_ASSERT(up ? row > 0 : row < itemCount - 1, return);

    QTreeWidgetItem *item = m_mappingTree.takeTopLevelItem(row);
    m_mappingTree.insertTopLevelItem(up ? row - 1 : row + 1, item);
    m_mappingTree.setCurrentItem(item);
}

void AxivionSettingsWidget::updateVersionAndBuildDate(QLabel *version, QLabel *buildDate)
{
    QTC_ASSERT(version && buildDate, return);
    std::optional<AxivionVersionInfo> info = settings().versionInfo();
    version->setText(info ? info->versionNumber : QString{});
    buildDate->setText(info ? info->dateTime : QString{});
}

// settings pages

class AxivionSettingsPage : public IOptionsPage
{
public:
    AxivionSettingsPage()
    {
        setId("Analyzer.Axivion.Settings");
        setDisplayName(Tr::tr("Axivion"));
        setCategory("T.Analyzer");
        setWidgetCreator([] { return new AxivionSettingsWidget; });
    }
};

const AxivionSettingsPage generalSettingsPage;

} // Axivion::Internal
