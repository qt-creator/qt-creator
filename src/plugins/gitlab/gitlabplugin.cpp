// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gitlabplugin.h"

#include "gitlabdialog.h"
#include "gitlaboptionspage.h"
#include "gitlabparameters.h"
#include "gitlabprojectsettings.h"
#include "gitlabtr.h"
#include "queryrunner.h"
#include "resultparser.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>

#include <extensionsystem/iplugin.h>

#include <git/gitplugin.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/projectmanager.h>

#include <utils/qtcassert.h>

#include <vcsbase/vcsoutputwindow.h>

#include <QAction>
#include <QMessageBox>
#include <QPointer>
#include <QTimer>

using namespace Core;

namespace GitLab {

class GitLabPluginPrivate : public QObject
{
public:
    void setupNotificationTimer();
    void fetchEvents();
    void fetchUser();
    void createAndSendEventsRequest(const QDateTime timeStamp, int page = -1);
    void handleUser(const User &user);
    void handleEvents(const Events &events, const QDateTime &timeStamp);

    GitLabOptionsPage optionsPage;
    QHash<ProjectExplorer::Project *, GitLabProjectSettings *> projectSettings;
    QPointer<GitLabDialog> dialog;

    QTimer notificationTimer;
    QString projectName;
    Utils::Id serverId;
    bool runningQuery = false;
};

static GitLabPluginPrivate *dd = nullptr;

class GitLabPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "GitLab.json")

    ~GitLabPlugin() final
    {
        if (!dd->projectSettings.isEmpty()) {
            qDeleteAll(dd->projectSettings);
            dd->projectSettings.clear();
        }
        delete dd;
        dd = nullptr;
    }

    void initialize() final
    {
        dd = new GitLabPluginPrivate;
        gitLabParameters().fromSettings(Core::ICore::settings());

        setupGitlabProjectPanel();

        ActionBuilder(this, "GitLab.OpenView")
            .setText(Tr::tr("GitLab..."))
            .addOnTriggered(this, &GitLabPlugin::openView)
            .addToContainer(Core::Constants::M_TOOLS);

        connect(ProjectExplorer::ProjectManager::instance(),
                &ProjectExplorer::ProjectManager::startupProjectChanged,
                this, &GitLabPlugin::onStartupProjectChanged);
    }

    void openView()
    {
        if (dd->dialog.isNull()) {
            while (!gitLabParameters().isValid()) {
                QMessageBox::warning(Core::ICore::dialogParent(), Tr::tr("Error"),
                                     Tr::tr("Invalid GitLab configuration. For a fully functional "
                                            "configuration, you need to set up host name or address and "
                                            "an access token. Providing the path to curl is mandatory."));
                if (!Core::ICore::showOptionsDialog("GitLab"))
                    return;
            }
            GitLabDialog *gitlabD = new GitLabDialog(Core::ICore::dialogParent());
            gitlabD->setModal(true);
            Core::ICore::registerWindow(gitlabD, Core::Context("Git.GitLab"));
            dd->dialog = gitlabD;
        }
        const Qt::WindowStates state = dd->dialog->windowState();
        if (state & Qt::WindowMinimized)
            dd->dialog->setWindowState(state & ~Qt::WindowMinimized);
        dd->dialog->show();
        dd->dialog->raise();
    }

    void onStartupProjectChanged()
    {
        QTC_ASSERT(dd, return);
        disconnect(&dd->notificationTimer);
        ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
        if (!project) {
            dd->notificationTimer.stop();
            return;
        }

        const GitLabProjectSettings *projSettings = GitLab::projectSettings(project);
        if (!projSettings->isLinked()) {
            dd->notificationTimer.stop();
            return;
        }

        dd->fetchEvents();
        dd->setupNotificationTimer();
    }
};


void GitLabPluginPrivate::setupNotificationTimer()
{
    // make interval configurable?
    notificationTimer.setInterval(15 * 60 * 1000);
    QObject::connect(&notificationTimer, &QTimer::timeout, this, &GitLabPluginPrivate::fetchEvents);
    notificationTimer.start();
}

void GitLabPluginPrivate::fetchEvents()
{
    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    QTC_ASSERT(project, return);

    if (runningQuery)
        return;

    const GitLabProjectSettings *projSettings = GitLab::projectSettings(project);
    projectName = projSettings->currentProject();
    serverId = projSettings->currentServer();

    const QDateTime lastRequest = projSettings->lastRequest();
    if (!lastRequest.isValid()) { // we haven't queried events for this project yet
        fetchUser();
        return;
    }
    createAndSendEventsRequest(lastRequest);
}

void GitLabPluginPrivate::fetchUser()
{
    if (runningQuery)
        return;

    const Query query(Query::User);
    QueryRunner *runner = new QueryRunner(query, serverId, this);
    QObject::connect(runner, &QueryRunner::resultRetrieved, this, [this](const QByteArray &result) {
        handleUser(ResultParser::parseUser(result));
    });
    QObject::connect(runner, &QueryRunner::finished, [runner]() { runner->deleteLater(); });
    runningQuery = true;
    runner->start();
}

void GitLabPluginPrivate::createAndSendEventsRequest(const QDateTime timeStamp, int page)
{
    if (runningQuery)
        return;

    Query query(Query::Events, {projectName});
    QStringList additional = {"sort=asc"};

    QDateTime after = timeStamp.addDays(-1);
    additional.append(QLatin1String("after=%1").arg(after.toString("yyyy-MM-dd")));
    query.setAdditionalParameters(additional);

    if (page > 1)
        query.setPageParameter(page);

    QueryRunner *runner = new QueryRunner(query, serverId, this);
    QObject::connect(runner, &QueryRunner::resultRetrieved, this,
                     [this, timeStamp](const QByteArray &result) {
        handleEvents(ResultParser::parseEvents(result), timeStamp);
    });
    QObject::connect(runner, &QueryRunner::finished, [runner]() { runner->deleteLater(); });
    runningQuery = true;
    runner->start();
}

void GitLabPluginPrivate::handleUser(const User &user)
{
    runningQuery = false;

    QTC_ASSERT(user.error.message.isEmpty(), return);
    const QDateTime timeStamp = QDateTime::fromString(user.lastLogin, Qt::ISODateWithMs);
    createAndSendEventsRequest(timeStamp);
}

GitLabProjectSettings *projectSettings(ProjectExplorer::Project *project)
{
    QTC_ASSERT(project, return nullptr);
    QTC_ASSERT(dd, return nullptr);
    auto &settings = dd->projectSettings[project];
    if (!settings)
        settings = new GitLabProjectSettings(project);
    return settings;
}

void GitLabPluginPrivate::handleEvents(const Events &events, const QDateTime &timeStamp)
{
    runningQuery = false;

    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    QTC_ASSERT(project, return);

    GitLabProjectSettings *projSettings = GitLab::projectSettings(project);
    QTC_ASSERT(projSettings->currentProject() == projectName, return);

    if (!projSettings->isLinked()) // link state has changed meanwhile - ignore the request
        return;

    if (!events.error.message.isEmpty()) {
        VcsBase::VcsOutputWindow::appendError("GitLab: Error while fetching events. "
                                              + events.error.message + '\n');
        return;
    }

    QDateTime lastTimeStamp;
    for (const Event &event : events.events) {
        const QDateTime eventTimeStamp = QDateTime::fromString(event.timeStamp, Qt::ISODateWithMs);
        if (!timeStamp.isValid() || timeStamp < eventTimeStamp) {
            VcsBase::VcsOutputWindow::appendMessage("GitLab: " + event.toMessage());
            if (!lastTimeStamp.isValid() || lastTimeStamp < eventTimeStamp)
                lastTimeStamp = eventTimeStamp;
        }
    }
    if (lastTimeStamp.isValid()) {
        if (auto outputWindow = VcsBase::VcsOutputWindow::instance())
            outputWindow->flash();
        projSettings->setLastRequest(lastTimeStamp);
    }

    if (events.pageInfo.currentPage < events.pageInfo.totalPages)
        createAndSendEventsRequest(timeStamp, events.pageInfo.currentPage + 1);
}

bool handleCertificateIssue(const Utils::Id &serverId)
{
    QTC_ASSERT(dd, return false);

    GitLabParameters &params = gitLabParameters();
    GitLabServer server = params.serverForId(serverId);
    if (QMessageBox::question(Core::ICore::dialogParent(),
                              Tr::tr("Certificate Error"),
                              Tr::tr(
                                  "Server certificate for %1 cannot be authenticated.\n"
                                  "Do you want to disable SSL verification for this server?\n"
                                  "Note: This can expose you to man-in-the-middle attack.")
                              .arg(server.host))
            == QMessageBox::Yes) {
        int index = params.gitLabServers.indexOf(server);
        server.validateCert = false;
        params.gitLabServers.replace(index, server);
        if (dd->dialog)
            dd->dialog->updateRemotes();

        return true;
    }
    return false;
}

void linkedStateChanged(bool enabled)
{
    QTC_ASSERT(dd, return);

    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    if (project) {
        const GitLabProjectSettings *pSettings = projectSettings(project);
        dd->serverId = pSettings->currentServer();
        dd->projectName = pSettings->currentProject();
    } else {
        dd->serverId = Utils::Id();
        dd->projectName = QString();
    }

    if (enabled) {
        dd->fetchEvents();
        dd->setupNotificationTimer();
    } else {
        QObject::disconnect(&dd->notificationTimer, &QTimer::timeout,
                            dd, &GitLabPluginPrivate::fetchEvents);
        dd->notificationTimer.stop();
    }
}

} // namespace GitLab

#include "gitlabplugin.moc"
