/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at http://www.qt.io/contact-us
**
** This file is part of the Qt Enterprise Qt Quick Profiler Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/
#include "clangstaticanalyzerprojectsettingsmanager.h"

#include "clangstaticanalyzerprojectsettings.h"

#include <projectexplorer/session.h>

namespace ClangStaticAnalyzer {
namespace Internal {

ProjectSettingsManager::ProjectSettingsManager()
{
    QObject::connect(ProjectExplorer::SessionManager::instance(),
            &ProjectExplorer::SessionManager::aboutToRemoveProject,
            &ProjectSettingsManager::handleProjectToBeRemoved);
}

ProjectSettings *ProjectSettingsManager::getSettings(ProjectExplorer::Project *project)
{
    auto &settings = m_settings[project];
    if (!settings)
        settings.reset(new ProjectSettings(project));
    return settings.data();
}

void ProjectSettingsManager::handleProjectToBeRemoved(ProjectExplorer::Project *project)
{
    m_settings.remove(project);
}

ProjectSettingsManager::SettingsMap ProjectSettingsManager::m_settings;

} // namespace Internal
} // namespace ClangStaticAnalyzer
