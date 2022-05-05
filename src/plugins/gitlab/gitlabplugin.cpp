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

#include "gitlaboptionspage.h"
#include "gitlabparameters.h"
#include "gitlabprojectsettings.h"

#include <coreplugin/icore.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectpanelfactory.h>
#include <utils/qtcassert.h>

namespace GitLab {

class GitLabPluginPrivate
{
public:
    GitLabParameters parameters;
    GitLabOptionsPage optionsPage{&parameters};
    QHash<ProjectExplorer::Project *, GitLabProjectSettings *> projectSettings;
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
    return true;
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
