/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "clangprojectsettings.h"

namespace ClangCodeModel {
namespace Internal {

static QString useGlobalWarningConfigKey()
{ return QStringLiteral("ClangCodeModel.UseGlobalWarningConfig"); }

static QString warningConfigIdKey()
{ return QStringLiteral("ClangCodeModel.WarningConfigId"); }

ClangProjectSettings::ClangProjectSettings(ProjectExplorer::Project *project)
    : m_project(project)
{
    load();

    connect(project, &ProjectExplorer::Project::settingsLoaded,
            this, &ClangProjectSettings::load);
    connect(project, &ProjectExplorer::Project::aboutToSaveSettings,
            this, &ClangProjectSettings::store);
}

Core::Id ClangProjectSettings::warningConfigId() const
{
    return m_warningConfigId;
}

void ClangProjectSettings::setWarningConfigId(const Core::Id &customConfigId)
{
    m_warningConfigId = customConfigId;
}

bool ClangProjectSettings::useGlobalWarningConfig() const
{
    return m_useGlobalWarningConfig;
}

void ClangProjectSettings::setUseGlobalWarningConfig(bool useGlobalWarningConfig)
{
    m_useGlobalWarningConfig = useGlobalWarningConfig;
}

void ClangProjectSettings::load()
{
    const QVariant useGlobalConfigVariant = m_project->namedSettings(useGlobalWarningConfigKey());
    const bool useGlobalConfig = useGlobalConfigVariant.isValid()
            ? useGlobalConfigVariant.toBool()
            : true;

    setUseGlobalWarningConfig(useGlobalConfig);
    setWarningConfigId(Core::Id::fromSetting(m_project->namedSettings(warningConfigIdKey())));
}

void ClangProjectSettings::store()
{
    m_project->setNamedSettings(useGlobalWarningConfigKey(), useGlobalWarningConfig());
    m_project->setNamedSettings(warningConfigIdKey(), warningConfigId().toSetting());
}

} // namespace Internal
} // namespace ClangCodeModel
