// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gitlabprojectsettings.h"

#include "gitlaboptionspage.h"
#include "gitlabparameters.h"
#include "gitlabplugin.h"
#include "gitlabtr.h"
#include "queryrunner.h"
#include "resultparser.h"

#include <git/gitclient.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/projectsettingswidget.h>

#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>

#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QVariant>

namespace GitLab {

const char PSK_LINKED_ID[]  = "GitLab.LinkedId";
const char PSK_SERVER[]     = "GitLab.Server";
const char PSK_PROJECT[]    = "GitLab.Project";
const char PSK_LAST_REQ[]   = "GitLab.LastRequest";

static QString accessLevelString(int accessLevel)
{
    switch (accessLevel) {
    case 10: return Tr::tr("Guest");
    case 20: return Tr::tr("Reporter");
    case 30: return Tr::tr("Developer");
    case 40: return Tr::tr("Maintainer");
    case 50: return Tr::tr("Owner");
    }
    return {};
}

std::tuple<QString, QString, int>
GitLabProjectSettings::remotePartsFromRemote(const QString &remote)
{
    QString host;
    QString path;
    int port = -1;
    if (remote.startsWith("git@")) {
            int colon = remote.indexOf(':');
            host = remote.mid(4, colon - 4);
            path = remote.mid(colon + 1);
    } else {
        const QUrl url(remote);
        host = url.host();
        path = url.path().mid(1); // ignore leading slash
        port = url.port();
    }
    if (path.endsWith(".git"))
        path.chop(4);

    return std::make_tuple(host, path, port);
}

GitLabProjectSettings::GitLabProjectSettings(ProjectExplorer::Project *project)
    : m_project(project)
{
    load();
    connect(project, &ProjectExplorer::Project::settingsLoaded,
            this, &GitLabProjectSettings::load);
    connect(project, &ProjectExplorer::Project::aboutToSaveSettings,
            this, &GitLabProjectSettings::save);
}

void GitLabProjectSettings::setLinked(bool linked)
{
    m_linked = linked;
    save();
}

void GitLabProjectSettings::load()
{
    m_id = Utils::Id::fromSetting(m_project->namedSettings(PSK_LINKED_ID));
    m_host = m_project->namedSettings(PSK_SERVER).toString();
    m_currentProject = m_project->namedSettings(PSK_PROJECT).toString();
    m_lastRequest = m_project->namedSettings(PSK_LAST_REQ).toDateTime();

    // may still be wrong, but we avoid an additional request by just doing sanity check here
    if (!m_id.isValid() || m_host.isEmpty())
        m_linked = false;
    else
        m_linked = gitLabParameters().serverForId(m_id).id.isValid();
}

void GitLabProjectSettings::save()
{
    if (m_linked) {
        m_project->setNamedSettings(PSK_LINKED_ID, m_id.toSetting());
        m_project->setNamedSettings(PSK_SERVER, m_host);
    } else {
        m_project->setNamedSettings(PSK_LINKED_ID, Utils::Id().toSetting());
        m_project->setNamedSettings(PSK_SERVER, QString());
    }
    m_project->setNamedSettings(PSK_PROJECT, m_currentProject);
    m_project->setNamedSettings(PSK_LAST_REQ, m_lastRequest);
}

class GitLabProjectSettingsWidget : public ProjectExplorer::ProjectSettingsWidget
{
public:
    explicit GitLabProjectSettingsWidget(ProjectExplorer::Project *project);

private:
    enum CheckMode { Connection, Link };

    void unlink();
    void checkConnection(CheckMode mode);
    void onConnectionChecked(const Project &project, const Utils::Id &serverId,
                             const QString &remote, const QString &projName);
    void updateUi();
    void updateEnabledStates();

    GitLabProjectSettings *m_projectSettings = nullptr;
    QComboBox *m_linkedGitLabServer = nullptr;
    QComboBox *m_hostCB = nullptr;
    QPushButton *m_linkWithGitLab = nullptr;
    QPushButton *m_unlink = nullptr;
    QPushButton *m_checkConnection = nullptr;
    Utils::InfoLabel *m_infoLabel = nullptr;
    CheckMode m_checkMode = Connection;
};

GitLabProjectSettingsWidget::GitLabProjectSettingsWidget(ProjectExplorer::Project *project)
    : m_projectSettings(projectSettings(project))
{
    setUseGlobalSettingsCheckBoxVisible(false);
    setUseGlobalSettingsLabelVisible(true);
    setGlobalSettingsId(Constants::GITLAB_SETTINGS);

    // setup ui
    m_hostCB = new QComboBox(this);
    m_linkedGitLabServer = new QComboBox(this);
    m_infoLabel = new Utils::InfoLabel(this);
    m_infoLabel->setVisible(false);
    m_linkWithGitLab = new QPushButton(Tr::tr("Link with GitLab"), this);
    m_unlink = new QPushButton(Tr::tr("Unlink from GitLab"), this);
    m_unlink->setEnabled(false);
    m_checkConnection = new QPushButton(Tr::tr("Test Connection"), this);
    m_checkConnection->setEnabled(false);

    using namespace Layouting;

    Column {
        noMargin,
        Form {
            Tr::tr("Host:"), m_hostCB, br,
            Tr::tr("Linked GitLab Configuration"), m_linkedGitLabServer, br,
        },
        m_infoLabel,
        Row { noMargin, m_linkWithGitLab, m_unlink, m_checkConnection, st },
        Tr::tr("Projects linked with GitLab receive event notifications in the Version Control "
               "output pane."),
        st,
    }.attachTo(this);

    connect(m_linkWithGitLab, &QPushButton::clicked, this, [this] {
        checkConnection(Link);
    });
    connect(m_unlink, &QPushButton::clicked,
            this, &GitLabProjectSettingsWidget::unlink);
    connect(m_checkConnection, &QPushButton::clicked, this, [this] {
        checkConnection(Connection);
    });
    connect(m_linkedGitLabServer, &QComboBox::currentIndexChanged, this, [this] {
        m_infoLabel->setVisible(false);
    });
    connect(m_hostCB, &QComboBox::currentIndexChanged, this, [this] {
        m_infoLabel->setVisible(false);
    });
    connect(&gitLabParameters(), &GitLabParameters::changed,
            this, &GitLabProjectSettingsWidget::updateUi);
    updateUi();
}

void GitLabProjectSettingsWidget::unlink()
{
    QTC_ASSERT(m_projectSettings->isLinked(), return);
    m_projectSettings->setLinked(false);
    m_projectSettings->setCurrentProject({});
    updateEnabledStates();
    linkedStateChanged(false);
}

void GitLabProjectSettingsWidget::checkConnection(CheckMode mode)
{
    const GitLabServer server = m_linkedGitLabServer->currentData().value<GitLabServer>();
    const QString remote = m_hostCB->currentData().toString();

    const auto [remoteHost, projName, port] = GitLabProjectSettings::remotePartsFromRemote(remote);
    if (remoteHost != server.host) { // port check as well
        m_infoLabel->setType(Utils::InfoLabel::NotOk);
        m_infoLabel->setText(Tr::tr("Remote host does not match chosen GitLab configuration."));
        m_infoLabel->setVisible(true);
        return;
    }

    // temporarily disable ui
    m_linkedGitLabServer->setEnabled(false);
    m_hostCB->setEnabled(false);
    m_checkConnection->setEnabled(false);

    m_checkMode = mode;
    const Query query(Query::Project, {projName});
    QueryRunner *runner = new QueryRunner(query, server.id, this);
    // can't use server, projName as captures inside the lambda below (bindings vs. local vars) :/
    const Utils::Id id = server.id;
    const QString projectName = projName;
    connect(runner, &QueryRunner::resultRetrieved, this,
            [this, id, remote, projectName](const QByteArray &result) {
        onConnectionChecked(ResultParser::parseProject(result), id, remote, projectName);
    });
    connect(runner, &QueryRunner::finished, this, [runner]() { runner->deleteLater(); });
    runner->start();
}

void GitLabProjectSettingsWidget::onConnectionChecked(const Project &project,
                                                      const Utils::Id &serverId,
                                                      const QString &remote,
                                                      const QString &projectName)
{
    bool linkable = false;
    if (!project.error.message.isEmpty()) {
        m_infoLabel->setType(Utils::InfoLabel::Error);
        m_infoLabel->setText(Tr::tr("Check settings for misconfiguration.")
                             + " (" + project.error.message + ')');
    } else {
        if (project.accessLevel != -1) {
            m_infoLabel->setType(Utils::InfoLabel::Ok);
            m_infoLabel->setText(Tr::tr("Accessible (%1).")
                                 .arg(accessLevelString(project.accessLevel)));
            linkable = true;
        } else {
            m_infoLabel->setType(Utils::InfoLabel::Warning);
            m_infoLabel->setText(Tr::tr("Read only access."));
        }
    }
    m_infoLabel->setVisible(true);

    if (m_checkMode == Link && linkable) {
        m_projectSettings->setCurrentServer(serverId);
        m_projectSettings->setCurrentServerHost(remote);
        m_projectSettings->setLinked(true);
        m_projectSettings->setCurrentProject(projectName);
        linkedStateChanged(true);
    }
    updateEnabledStates();
}

void GitLabProjectSettingsWidget::updateUi()
{
    m_linkedGitLabServer->clear();
    const QList<GitLabServer> allServers = gitLabParameters().gitLabServers;
    for (const GitLabServer &server : allServers) {
        const QString display = server.host + " (" + server.description + ')';
        m_linkedGitLabServer->addItem(display, QVariant::fromValue(server));
    }

    const Utils::FilePath projectDirectory = m_projectSettings->project()->projectDirectory();
    const Utils::FilePath repository =
        Git::Internal::gitClient().findRepositoryForDirectory(projectDirectory);

    m_hostCB->clear();
    if (!repository.isEmpty()) {
        const QMap<QString, QString> remotes =
            Git::Internal::gitClient().synchronousRemotesList(repository);
        for (auto it = remotes.begin(), end = remotes.end(); it != end; ++it) {
            const QString display = it.key() + " (" + it.value() + ')';
            m_hostCB->addItem(display, QVariant::fromValue(it.value()));
        }
    }

    const Utils::Id id = m_projectSettings->currentServer();
    const QString serverHost = m_projectSettings->currentServerHost();
    if (id.isValid()) {
        const GitLabServer server = gitLabParameters().serverForId(id);
        auto [remoteHost, projName, port] = GitLabProjectSettings::remotePartsFromRemote(serverHost);
        if (server.id.isValid() && server.host == remoteHost) { // found config
            m_projectSettings->setLinked(true);
            m_hostCB->setCurrentIndex(m_hostCB->findData(QVariant::fromValue(serverHost)));
            m_linkedGitLabServer->setCurrentIndex(
                m_linkedGitLabServer->findData(QVariant::fromValue(server)));
            linkedStateChanged(true);
        } else {
            m_projectSettings->setLinked(false);
            linkedStateChanged(false);
        }
    }
    updateEnabledStates();
}

void GitLabProjectSettingsWidget::updateEnabledStates()
{
    const bool isGitRepository = m_hostCB->count() > 0;
    const bool hasGitLabServers = m_linkedGitLabServer->count();
    const bool linked = m_projectSettings->isLinked();

    m_linkedGitLabServer->setEnabled(isGitRepository && !linked);
    m_hostCB->setEnabled(isGitRepository && !linked);
    m_linkWithGitLab->setEnabled(isGitRepository && !linked && hasGitLabServers);
    m_unlink->setEnabled(isGitRepository && linked);
    m_checkConnection->setEnabled(isGitRepository && hasGitLabServers);
    if (!isGitRepository) {
        const Utils::FilePath projectDirectory = m_projectSettings->project()->projectDirectory();
        const Utils::FilePath repository =
            Git::Internal::gitClient().findRepositoryForDirectory(projectDirectory);
        if (repository.isEmpty())
            m_infoLabel->setText(Tr::tr("Not a git repository."));
        else
            m_infoLabel->setText(Tr::tr("Local git repository without remotes."));
        m_infoLabel->setType(Utils::InfoLabel::None);
        m_infoLabel->setVisible(true);
    }
}

class GitlabProjectPanelFactory final : public ProjectExplorer::ProjectPanelFactory
{
public:
    GitlabProjectPanelFactory()
    {
        setPriority(999);
        setDisplayName(Tr::tr("GitLab"));
        setCreateWidgetFunction([](ProjectExplorer::Project *project) {
            return new GitLabProjectSettingsWidget(project);
        });
    }
};

void setupGitlabProjectPanel()
{
    static GitlabProjectPanelFactory theGitlabProjectPanelFactory;
}

} // namespace GitLab
