/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "gitlabprojectsettings.h"

#include "gitlaboptionspage.h"
#include "gitlabplugin.h"
#include "queryrunner.h"
#include "resultparser.h"

#include <git/gitclient.h>
#include <projectexplorer/project.h>
#include <utils/infolabel.h>
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

static QString accessLevelString(int accessLevel)
{
    const char trContext[] = "GitLab::GitLabProjectSettingsWidget";
    switch (accessLevel) {
    case 10: return QCoreApplication::translate(trContext, "Guest");
    case 20: return QCoreApplication::translate(trContext, "Reporter");
    case 30: return QCoreApplication::translate(trContext, "Developer");
    case 40: return QCoreApplication::translate(trContext, "Maintainer");
    case 50: return QCoreApplication::translate(trContext, "Owner");
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

    // may still be wrong, but we avoid an additional request by just doing sanity check here
    if (!m_id.isValid() || m_host.isEmpty())
        m_linked = false;
    else
        m_linked = GitLabPlugin::globalParameters()->serverForId(m_id).id.isValid();
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
}

GitLabProjectSettingsWidget::GitLabProjectSettingsWidget(ProjectExplorer::Project *project,
                                                         QWidget *parent)
    : ProjectExplorer::ProjectSettingsWidget(parent)
    , m_projectSettings(GitLabPlugin::projectSettings(project))
{
    setUseGlobalSettingsCheckBoxVisible(false);
    // setup ui
    auto verticalLayout = new QVBoxLayout(this);
    verticalLayout->setContentsMargins(0, 0, 0, 0);
    auto formLayout = new QFormLayout;
    m_hostCB = new QComboBox;
    formLayout->addRow(tr("Host:"), m_hostCB);
    m_linkedGitLabServer = new QComboBox;
    formLayout->addRow(tr("Linked GitLab Configuration:"), m_linkedGitLabServer);
    verticalLayout->addLayout(formLayout);
    m_infoLabel = new Utils::InfoLabel;
    m_infoLabel->setVisible(false);
    verticalLayout->addWidget(m_infoLabel);
    auto horizontalLayout = new QHBoxLayout;
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    m_linkWithGitLab = new QPushButton(tr("Link with GitLab"));
    horizontalLayout->addWidget(m_linkWithGitLab);
    m_unlink = new QPushButton(tr("Unlink from GitLab"));
    m_unlink->setEnabled(false);
    horizontalLayout->addWidget(m_unlink);
    m_checkConnection = new QPushButton(tr("Test Connection"));
    m_checkConnection->setEnabled(false);
    horizontalLayout->addWidget(m_checkConnection);
    horizontalLayout->addStretch(1);
    verticalLayout->addLayout(horizontalLayout);

    connect(m_linkWithGitLab, &QPushButton::clicked, this, [this]() {
        checkConnection(Link);
    });
    connect(m_unlink, &QPushButton::clicked,
            this, &GitLabProjectSettingsWidget::unlink);
    connect(m_checkConnection, &QPushButton::clicked, this, [this]() {
        checkConnection(Connection);
    });
    connect(m_linkedGitLabServer, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() {
        m_infoLabel->setVisible(false);
    });
    connect(m_hostCB, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() {
        m_infoLabel->setVisible(false);
    });
    connect(GitLabPlugin::optionsPage(), &GitLabOptionsPage::settingsChanged,
            this, &GitLabProjectSettingsWidget::updateUi);
    updateUi();
}

void GitLabProjectSettingsWidget::unlink()
{
    QTC_ASSERT(m_projectSettings->isLinked(), return);
    m_projectSettings->setLinked(false);
    m_projectSettings->setCurrentProject({});
    updateEnabledStates();
}

void GitLabProjectSettingsWidget::checkConnection(CheckMode mode)
{
    const GitLabServer server = m_linkedGitLabServer->currentData().value<GitLabServer>();
    const QString remote = m_hostCB->currentData().toString();

    const auto [remoteHost, projName, port] = GitLabProjectSettings::remotePartsFromRemote(remote);
    if (remoteHost != server.host) { // port check as well
        m_infoLabel->setType(Utils::InfoLabel::NotOk);
        m_infoLabel->setText(tr("Remote host does not match chosen GitLab configuration."));
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
        m_infoLabel->setText(project.error.message);
    } else {
        if (project.accessLevel != -1) {
            m_infoLabel->setType(Utils::InfoLabel::Ok);
            m_infoLabel->setText(tr("Accessible (%1)")
                                 .arg(accessLevelString(project.accessLevel)));
            linkable = true;
        } else {
            m_infoLabel->setType(Utils::InfoLabel::Warning);
            m_infoLabel->setText(tr("Read only access"));
        }
    }
    m_infoLabel->setVisible(true);

    if (m_checkMode == Link && linkable) {
        m_projectSettings->setCurrentServer(serverId);
        m_projectSettings->setCurrentServerHost(remote);
        m_projectSettings->setLinked(true);
        m_projectSettings->setCurrentProject(projectName);
    }
    updateEnabledStates();
}

void GitLabProjectSettingsWidget::updateUi()
{
    m_linkedGitLabServer->clear();
    const QList<GitLabServer> allServers = GitLabPlugin::allGitLabServers();
    for (const GitLabServer &server : allServers) {
        const QString display = server.host + " (" + server.description + ')';
        m_linkedGitLabServer->addItem(display, QVariant::fromValue(server));
    }

    const Utils::FilePath projectDirectory = m_projectSettings->project()->projectDirectory();
    const auto *gitClient = Git::Internal::GitClient::instance();
    const Utils::FilePath repository = gitClient
            ? gitClient->findRepositoryForDirectory(projectDirectory) : Utils::FilePath();

    m_hostCB->clear();
    if (!repository.isEmpty()) {
        const QMap<QString, QString> remotes = gitClient->synchronousRemotesList(repository);
        for (auto it = remotes.begin(), end = remotes.end(); it != end; ++it) {
            const QString display = it.key() + " (" + it.value() + ')';
            m_hostCB->addItem(display, QVariant::fromValue(it.value()));
        }
    }

    const Utils::Id id = m_projectSettings->currentServer();
    const QString serverHost = m_projectSettings->currentServerHost();
    if (id.isValid()) {
        const GitLabServer server = GitLabPlugin::gitLabServerForId(id);
        auto [remoteHost, projName, port] = GitLabProjectSettings::remotePartsFromRemote(serverHost);
        if (server.id.isValid() && server.host == remoteHost) { // found config
            m_projectSettings->setLinked(true);
            m_hostCB->setCurrentIndex(m_hostCB->findData(QVariant::fromValue(serverHost)));
            m_linkedGitLabServer->setCurrentIndex(
                        m_linkedGitLabServer->findData(QVariant::fromValue(server)));
        } else {
            m_projectSettings->setLinked(false);
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
        const auto *gitClient = Git::Internal::GitClient::instance();
        const Utils::FilePath repository = gitClient
                ? gitClient->findRepositoryForDirectory(projectDirectory) : Utils::FilePath();
        if (repository.isEmpty())
            m_infoLabel->setText(tr("Not a git repository."));
        else
            m_infoLabel->setText(tr("Local git repository without remotes."));
        m_infoLabel->setType(Utils::InfoLabel::None);
        m_infoLabel->setVisible(true);
    }
}

} // namespace GitLab
