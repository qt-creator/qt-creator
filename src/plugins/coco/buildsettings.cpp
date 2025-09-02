// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "buildsettings.h"

#include "cocobuildstep.h"
#include "cococmakesettings.h"
#include "cocoqmakesettings.h"
#include "modificationfile.h"

#include <projectexplorer/buildsystem.h>
#include <projectexplorer/target.h>
#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

using namespace ProjectExplorer;

namespace Coco::Internal {

bool BuildSettings::supportsBuildConfig(const ProjectExplorer::BuildConfiguration &config)
{
    return config.id() == QmakeProjectManager::Constants::QMAKE_BC_ID
           || config.id() == CMakeProjectManager::Constants::CMAKE_BUILDCONFIGURATION_ID;
}

BuildSettings *BuildSettings::createdFor(BuildConfiguration *buildConfig)
{
    if (buildConfig->id() == QmakeProjectManager::Constants::QMAKE_BC_ID)
        return createCocoQMakeSettings(buildConfig);
    if (buildConfig->id() == CMakeProjectManager::Constants::CMAKE_BUILDCONFIGURATION_ID)
        return createCocoCMakeSettings(buildConfig);
    return nullptr;
}

BuildSettings::BuildSettings(ModificationFile &featureFile, BuildConfiguration *buildConfig)
    : m_featureFile{featureFile}
    , m_buildConfig{buildConfig}
{
    // Do not use m_featureFile in the constructor; it may not yet be valid.
}

void BuildSettings::connectToBuildStep(CocoBuildStep *step) const
{
    connect(buildConfig()->buildSystem(), &BuildSystem::updated,
            step, &CocoBuildStep::buildSystemUpdated);
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

BuildConfiguration *BuildSettings::buildConfig() const
{
    return m_buildConfig;
}

} // namespace Coco::Internal
