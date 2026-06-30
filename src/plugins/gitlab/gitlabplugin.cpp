// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gitlabplugin.h"

#include "gitlabdialog.h"
#include "gitlaboptionspage.h"
#include "gitlabparameters.h"
#include "gitlabprojectsettings.h"
#include "gitlabquery.h"
#include "gitlabtr.h"
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

#include <QtTaskTree/qconditional.h>
#include <QtTaskTree/qtasktree.h>
#include <QtTaskTree/QSingleTaskTreeRunner>

#include <QAction>
#include <QMessageBox>
#include <QPointer>
#include <QTimer>

#include <optional>

using namespace Core;

namespace GitLab {

// Cross-step state shared by the optional user query and the paginated events loop.
struct EventsData
{
    QString projectName;
    QDateTime timeStamp; // reference time; events newer than this get reported
    std::optional<int> currentPage = 1; // page to request; nullopt once there is nothing more
};

class GitLabPluginPrivate : public QObject
{
public:
    void setupNotificationTimer();
    void fetchEvents();

    GitLabOptionsPage optionsPage;
    QHash<ProjectExplorer::Project *, GitLabProjectSettings *> projectSettings;
    QPointer<GitLabDialog> dialog;

    QtTaskTree::QSingleTaskTreeRunner taskTreeRunner;
    QTimer notificationTimer;
    Utils::Id serverId;
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
            if (!gitLabParameters().isValid()) {
                QMessageBox::warning(Core::ICore::dialogParent(), Tr::tr("Error"),
                                     Tr::tr("Invalid GitLab configuration. For a fully functional "
                                            "configuration, you need to set up host name or address and "
                                            "an access token. Providing the path to curl is mandatory."));
                Core::ICore::showSettings("GitLab");
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

static void handleEvents(const Events &events, EventsData *data)
{
    data->currentPage = std::nullopt; // stop the loop unless we make it to the end successfully

    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    QTC_ASSERT(project, return);

    GitLabProjectSettings *projSettings = GitLab::projectSettings(project);
    QTC_ASSERT(projSettings->currentProject() == data->projectName, return);

    if (!projSettings->isLinked()) // link state has changed meanwhile - ignore the request
        return;

    if (!events.error.message.isEmpty()) {
        const Utils::FilePath workingDirectory = project->projectDirectory();
        VcsBase::VcsOutputWindow::appendError(workingDirectory,
                                              "GitLab: Error while fetching events. " + events.error.message);
        return;
    }

    QDateTime lastTimeStamp;
    for (const Event &event : events.events) {
        const QDateTime eventTimeStamp = QDateTime::fromString(event.timeStamp, Qt::ISODateWithMs);
        if (!data->timeStamp.isValid() || data->timeStamp < eventTimeStamp) {
            const Utils::FilePath workingDirectory = project->projectDirectory();
            VcsBase::VcsOutputWindow::appendMessage(workingDirectory, "GitLab: " + event.toMessage());
            if (!lastTimeStamp.isValid() || lastTimeStamp < eventTimeStamp)
                lastTimeStamp = eventTimeStamp;
        }
    }
    if (lastTimeStamp.isValid()) {
        if (auto outputWindow = VcsBase::VcsOutputWindow::instance())
            outputWindow->flash();
        projSettings->setLastRequest(lastTimeStamp);
    }

    // Queue the next page if there is one; otherwise currentPage stays unset and the loop ends.
    if (events.pageInfo.currentPage < events.pageInfo.totalPages)
        data->currentPage = events.pageInfo.currentPage + 1;
}

void GitLabPluginPrivate::fetchEvents()
{
    using namespace QtTaskTree;

    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    QTC_ASSERT(project, return);

    if (taskTreeRunner.isRunning())
        return;

    const GitLabProjectSettings *projSettings = GitLab::projectSettings(project);
    const QString projectName = projSettings->currentProject();
    serverId = projSettings->currentServer();
    const QDateTime lastRequest = projSettings->lastRequest();

    const Storage<EventsData> storage{projectName, lastRequest};

    // When we haven't queried this project yet, look up the user first to learn the reference
    // timestamp; otherwise this task is skipped and the last request time is used.
    const auto onUserSetup = [this](GitLabQuery &query) {
        query.setServerId(serverId);
        query.setQuery(Query(Query::User));
    };
    const auto onUserDone = [storage](const GitLabQuery &query) {
        const User user = ResultParser::parseUser(query.result());
        if (user.error.message.isEmpty())
            storage->timeStamp = QDateTime::fromString(user.lastLogin, Qt::ISODateWithMs);
        else // give up - without a user we have no reference time to fetch events against
            storage->currentPage = std::nullopt;
    };

    // Fetch events page by page until the last page has been retrieved.
    const auto morePages = [storage](int) { return storage->currentPage.has_value(); };

    const auto onEventsSetup = [this, storage](GitLabQuery &query) {
        Query events(Query::Events, {storage->projectName});
        QStringList additional = {"sort=asc"};
        const QDateTime after = storage->timeStamp.addDays(-1);
        additional.append(QLatin1String("after=%1").arg(after.toString("yyyy-MM-dd")));
        events.setAdditionalParameters(additional);
        if (*storage->currentPage > 1) // the first page is requested without an explicit number
            events.setPageParameter(*storage->currentPage);
        query.setServerId(serverId);
        query.setQuery(events);
    };
    const auto onEventsDone = [storage](const GitLabQuery &query) {
        handleEvents(ResultParser::parseEvents(query.result()), storage.activeStorage());
    };

    const Group recipe {
        storage,
        If ([storage] { return !storage->timeStamp.isValid(); }) >> Then {
            gitLabQuery(onUserSetup, onUserDone),
        },
        For (UntilIterator(morePages)) >> Do {
            gitLabQuery(onEventsSetup, onEventsDone),
        },
    };

    taskTreeRunner.start(recipe);
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

void acceptCertificate(const Utils::Id &serverId)
{
    QTC_ASSERT(dd, return);

    GitLabParameters &params = gitLabParameters();
    GitLabServer server = params.serverForId(serverId);
    const int index = params.gitLabServers.indexOf(server);
    server.validateCert = false;
    params.gitLabServers.replace(index, server);
    if (dd->dialog)
        dd->dialog->updateRemotes();
}

void linkedStateChanged(bool enabled)
{
    QTC_ASSERT(dd, return);

    ProjectExplorer::Project *project = ProjectExplorer::ProjectManager::startupProject();
    if (project) {
        const GitLabProjectSettings *pSettings = projectSettings(project);
        dd->serverId = pSettings->currentServer();
    } else {
        dd->serverId = Utils::Id();
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
