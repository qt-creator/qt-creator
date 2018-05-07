/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "clangtoolsprojectsettings.h"

#include <projectexplorer/session.h>

#include <utils/algorithm.h>

namespace ClangTools {
namespace Internal {

static const char SETTINGS_KEY_SELECTED_DIRS[] = "ClangTools.SelectedDirs";
static const char SETTINGS_KEY_SELECTED_FILES[] = "ClangTools.SelectedFiles";

ClangToolsProjectSettings::ClangToolsProjectSettings(ProjectExplorer::Project *project)
    : m_project(project)
{
    load();
    connect(project, &ProjectExplorer::Project::settingsLoaded,
            this, &ClangToolsProjectSettings::load);
    connect(project, &ProjectExplorer::Project::aboutToSaveSettings, this,
            &ClangToolsProjectSettings::store);
}

ClangToolsProjectSettings::~ClangToolsProjectSettings()
{
    store();
}

void ClangToolsProjectSettings::load()
{
    auto toFileName = [](const QString &s) { return Utils::FileName::fromString(s); };

    const QStringList dirs = m_project->namedSettings(SETTINGS_KEY_SELECTED_DIRS).toStringList();
    m_selectedDirs = Utils::transform<QSet>(dirs, toFileName);

    const QStringList files = m_project->namedSettings(SETTINGS_KEY_SELECTED_FILES).toStringList();
    m_selectedFiles = Utils::transform<QSet>(files, toFileName);
}

void ClangToolsProjectSettings::store()
{
    const QStringList dirs = Utils::transform(m_selectedDirs.toList(), &Utils::FileName::toString);
    m_project->setNamedSettings(SETTINGS_KEY_SELECTED_DIRS, dirs);

    const QStringList files = Utils::transform(m_selectedFiles.toList(), &Utils::FileName::toString);
    m_project->setNamedSettings(SETTINGS_KEY_SELECTED_FILES, files);
}

ClangToolsProjectSettingsManager::ClangToolsProjectSettingsManager()
{
    QObject::connect(ProjectExplorer::SessionManager::instance(),
                     &ProjectExplorer::SessionManager::aboutToRemoveProject,
                     &ClangToolsProjectSettingsManager::handleProjectToBeRemoved);
}

ClangToolsProjectSettings *ClangToolsProjectSettingsManager::getSettings(
    ProjectExplorer::Project *project)
{
    auto &settings = m_settings[project];
    if (!settings)
        settings.reset(new ClangToolsProjectSettings(project));
    return settings.data();
}

void ClangToolsProjectSettingsManager::handleProjectToBeRemoved(ProjectExplorer::Project *project)
{
    m_settings.remove(project);
}

ClangToolsProjectSettingsManager::SettingsMap ClangToolsProjectSettingsManager::m_settings;

} // namespace Internal
} // namespace ClangTools
