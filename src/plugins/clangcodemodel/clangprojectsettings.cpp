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

#include <utils/qtcassert.h>
#include <utils/hostosinfo.h>

#include <QDebug>

namespace ClangCodeModel {
namespace Internal {

static QString useGlobalConfigKey()
{ return QStringLiteral("ClangCodeModel.UseGlobalConfig"); }

static QString warningConfigIdKey()
{ return QStringLiteral("ClangCodeModel.WarningConfigId"); }

static QString customCommandLineKey()
{ return QLatin1String("ClangCodeModel.CustomCommandLineKey"); }

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

bool ClangProjectSettings::useGlobalConfig() const
{
    return m_useGlobalConfig;
}

void ClangProjectSettings::setUseGlobalConfig(bool useGlobalConfig)
{
    m_useGlobalConfig = useGlobalConfig;
}

QStringList ClangProjectSettings::commandLineOptions() const
{
    return m_useGlobalConfig ? globalCommandLineOptions()
                             : m_customCommandLineOptions;
}

void ClangProjectSettings::setCommandLineOptions(const QStringList &options)
{
    QTC_ASSERT(!m_useGlobalConfig, qDebug()
               << "setCommandLineOptions was called while using global project config");
    m_customCommandLineOptions = options;
}

void ClangProjectSettings::load()
{
    const QVariant useGlobalConfigVariant = m_project->namedSettings(useGlobalConfigKey());
    const bool useGlobalConfig = useGlobalConfigVariant.isValid()
            ? useGlobalConfigVariant.toBool()
            : true;

    setUseGlobalConfig(useGlobalConfig);
    setWarningConfigId(Core::Id::fromSetting(m_project->namedSettings(warningConfigIdKey())));
    m_customCommandLineOptions = m_project->namedSettings(customCommandLineKey()).toStringList();
    if (m_customCommandLineOptions.empty())
        m_customCommandLineOptions = globalCommandLineOptions();
}

void ClangProjectSettings::store()
{
    m_project->setNamedSettings(useGlobalConfigKey(), useGlobalConfig());
    m_project->setNamedSettings(warningConfigIdKey(), warningConfigId().toSetting());
    m_project->setNamedSettings(customCommandLineKey(), m_customCommandLineOptions);
}

QStringList ClangProjectSettings::globalCommandLineOptions()
{
    if (Utils::HostOsInfo::isWindowsHost())
        return {QLatin1String{GlobalWindowsCmdOptions}};
    return {};
}

} // namespace Internal
} // namespace ClangCodeModel
