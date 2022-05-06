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

#include "gitlabplugin.h"

#include "gitlabdialog.h"
#include "gitlaboptionspage.h"
#include "gitlabparameters.h"
#include "gitlabprojectsettings.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>
#include <git/gitplugin.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectpanelfactory.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QMessageBox>
#include <QPointer>

namespace GitLab {
namespace Constants {
const char GITLAB_OPEN_VIEW[] = "GitLab.OpenView";
} // namespace Constants

class GitLabPluginPrivate
{
public:
    GitLabParameters parameters;
    GitLabOptionsPage optionsPage{&parameters};
    QHash<ProjectExplorer::Project *, GitLabProjectSettings *> projectSettings;
    QPointer<GitLabDialog> dialog;
};

static GitLabPluginPrivate *dd = nullptr;

GitLabPlugin::GitLabPlugin()
{
}

GitLabPlugin::~GitLabPlugin()
{
    if (!dd->projectSettings.isEmpty()) {
        qDeleteAll(dd->projectSettings);
        dd->projectSettings.clear();
    }
    delete dd;
    dd = nullptr;
}

bool GitLabPlugin::initialize(const QStringList & /*arguments*/, QString * /*errorString*/)
{
    dd = new GitLabPluginPrivate;
    dd->parameters.fromSettings(Core::ICore::settings());
    auto panelFactory = new ProjectExplorer::ProjectPanelFactory;
    panelFactory->setPriority(999);
    panelFactory->setDisplayName(tr("GitLab"));
    panelFactory->setCreateWidgetFunction([](ProjectExplorer::Project *project) {
        return new GitLabProjectSettingsWidget(project);
    });
    ProjectExplorer::ProjectPanelFactory::registerFactory(panelFactory);
    QAction *openViewAction = new QAction(tr("GitLab..."), this);
    auto gitlabCommand = Core::ActionManager::registerAction(openViewAction,
                                                             Constants::GITLAB_OPEN_VIEW);
    connect(openViewAction, &QAction::triggered, this, &GitLabPlugin::openView);
    Core::ActionContainer *ac = Core::ActionManager::actionContainer(Core::Constants::M_TOOLS);
    ac->addAction(gitlabCommand);
    connect(&dd->optionsPage, &GitLabOptionsPage::settingsChanged, this, [this] {
        if (dd->dialog)
            dd->dialog->updateRemotes();
    });
    return true;
}

void GitLabPlugin::openView()
{
    if (dd->dialog.isNull()) {
        while (!dd->parameters.isValid()) {
            QMessageBox::warning(Core::ICore::dialogParent(), tr("Error"),
                                 tr("Invalid GitLab configuration. For a fully functional "
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

QList<GitLabServer> GitLabPlugin::allGitLabServers()
{
    QTC_ASSERT(dd, return {});
    return dd->parameters.gitLabServers;
}

GitLabServer GitLabPlugin::gitLabServerForId(const Utils::Id &id)
{
    QTC_ASSERT(dd, return {});
    return dd->parameters.serverForId(id);
}

GitLabParameters *GitLabPlugin::globalParameters()
{
    return &dd->parameters;
}

GitLabProjectSettings *GitLabPlugin::projectSettings(ProjectExplorer::Project *project)
{
    QTC_ASSERT(project, return nullptr);
    QTC_ASSERT(dd, return nullptr);
    auto &settings = dd->projectSettings[project];
    if (!settings)
        settings = new GitLabProjectSettings(project);
    return settings;
}

GitLabOptionsPage *GitLabPlugin::optionsPage()
{
    QTC_ASSERT(dd, return {});
    return &dd->optionsPage;
}

} // namespace GitLab
