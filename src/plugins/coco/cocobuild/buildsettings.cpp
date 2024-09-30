// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "buildsettings.h"

#include "cocobuildstep.h"
#include "cococmakesettings.h"
#include "cocoqmakesettings.h"
#include "modificationfile.h"

#include <projectexplorer/target.h>
#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

namespace Coco::Internal {

bool BuildSettings::supportsBuildConfig(const ProjectExplorer::BuildConfiguration &config)
{
    return config.id() == QmakeProjectManager::Constants::QMAKE_BC_ID
           || config.id() == CMakeProjectManager::Constants::CMAKE_BUILDCONFIGURATION_ID;
}

BuildSettings *BuildSettings::createdFor(const ProjectExplorer::BuildConfiguration &config)
{
    if (config.id() == QmakeProjectManager::Constants::QMAKE_BC_ID)
        return new CocoQMakeSettings{config.project()};
    else if (config.id() == CMakeProjectManager::Constants::CMAKE_BUILDCONFIGURATION_ID)
        return new CocoCMakeSettings{config.project()};
    else
        return nullptr;
}

BuildSettings::BuildSettings(ModificationFile &featureFile, ProjectExplorer::Project *project)
    : m_featureFile{featureFile}
    , m_project{*project}
{
    // Do not use m_featureFile in the constructor; it may not yet be valid.
}

void BuildSettings::connectToBuildStep(CocoBuildStep *step) const
{
    connect(
        activeTarget(),
        &ProjectExplorer::Target::buildSystemUpdated,
        step,
        &CocoBuildStep::buildSystemUpdated);
}

bool BuildSettings::enabled() const
{
    return m_enabled;
}

const QStringList &BuildSettings::options() const
{
    return m_featureFile.options();
}

const QStringList &BuildSettings::tweaks() const
{
    return m_featureFile.tweaks();
}

bool BuildSettings::hasTweaks() const
{
    return !m_featureFile.tweaks().isEmpty();
}

QString BuildSettings::featureFilenName() const
{
    return m_featureFile.fileName();
}

QString BuildSettings::featureFilePath() const
{
    return m_featureFile.nativePath();
}

void BuildSettings::provideFile()
{
    if (!m_featureFile.exists())
        write("", "");
}

QString BuildSettings::tableRow(const QString &name, const QString &value) const
{
    return QString("<tr><td><b>%1</b></td><td>%2</td></tr>").arg(name, value);
}

void BuildSettings::setEnabled(bool enabled)
{
    m_enabled = enabled;
}

ProjectExplorer::Target *BuildSettings::activeTarget() const
{
    return m_project.activeTarget();
}

} // namespace Coco::Internal
