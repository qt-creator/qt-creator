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

#include "genericbuildconfiguration.h"

#include "genericmakestep.h"
#include "genericproject.h"
#include "genericprojectconstants.h"

#include <coreplugin/icore.h>

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>

#include <utils/mimetypes/mimedatabase.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QFormLayout>
#include <QInputDialog>

using namespace ProjectExplorer;

namespace GenericProjectManager {
namespace Internal {

GenericBuildConfiguration::GenericBuildConfiguration(Target *parent, Core::Id id)
    : BuildConfiguration(parent, id)
{
    updateCacheAndEmitEnvironmentChanged();
}

void GenericBuildConfiguration::initialize(const BuildInfo *info)
{
    BuildConfiguration::initialize(info);

    BuildStepList *buildSteps = stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    buildSteps->appendStep(new GenericMakeStep(buildSteps, "all"));

    BuildStepList *cleanSteps = stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
    cleanSteps->appendStep(new GenericMakeStep(cleanSteps, "clean"));

    updateCacheAndEmitEnvironmentChanged();
}

NamedWidget *GenericBuildConfiguration::createConfigWidget()
{
    return new GenericBuildSettingsWidget(this);
}

/*!
  \class GenericBuildConfigurationFactory
*/

GenericBuildConfigurationFactory::GenericBuildConfigurationFactory()
{
    registerBuildConfiguration<GenericBuildConfiguration>
        ("GenericProjectManager.GenericBuildConfiguration");

    setSupportedProjectType(Constants::GENERICPROJECT_ID);
    setSupportedProjectMimeTypeName(Constants::GENERICMIMETYPE);
}

GenericBuildConfigurationFactory::~GenericBuildConfigurationFactory()
{
}

QList<BuildInfo *> GenericBuildConfigurationFactory::availableBuilds(const Target *parent) const
{
    return {createBuildInfo(parent->kit(), parent->project()->projectDirectory())};
}

QList<BuildInfo *> GenericBuildConfigurationFactory::availableSetups(const Kit *k, const QString &projectPath) const
{
    QList<BuildInfo *> result;
    BuildInfo *info = createBuildInfo(k, Project::projectDirectory(Utils::FileName::fromString(projectPath)));
    //: The name of the build configuration created by default for a generic project.
    info->displayName = tr("Default");
    result << info;
    return result;
}

BuildInfo *GenericBuildConfigurationFactory::createBuildInfo(const Kit *k,
                                                             const Utils::FileName &buildDir) const
{
    auto info = new BuildInfo(this);
    info->typeName = tr("Build");
    info->buildDirectory = buildDir;
    info->kitId = k->id();
    return info;
}

BuildConfiguration::BuildType GenericBuildConfiguration::buildType() const
{
    return Unknown;
}

void GenericBuildConfiguration::addToEnvironment(Utils::Environment &env) const
{
    prependCompilerPathToEnvironment(env);
    const QtSupport::BaseQtVersion *qt = QtSupport::QtKitInformation::qtVersion(target()->kit());
    if (qt)
        env.prependOrSetPath(qt->binPath().toString());
}

////////////////////////////////////////////////////////////////////////////////////
// GenericBuildSettingsWidget
////////////////////////////////////////////////////////////////////////////////////

GenericBuildSettingsWidget::GenericBuildSettingsWidget(GenericBuildConfiguration *bc)
    : m_buildConfiguration(0)
{
    auto fl = new QFormLayout(this);
    fl->setContentsMargins(0, -1, 0, -1);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    // build directory
    m_pathChooser = new Utils::PathChooser(this);
    m_pathChooser->setHistoryCompleter("Generic.BuildDir.History");
    m_pathChooser->setEnabled(true);
    fl->addRow(tr("Build directory:"), m_pathChooser);
    connect(m_pathChooser, &Utils::PathChooser::rawPathChanged,
            this, &GenericBuildSettingsWidget::buildDirectoryChanged);

    m_buildConfiguration = bc;
    m_pathChooser->setBaseFileName(bc->target()->project()->projectDirectory());
    m_pathChooser->setEnvironment(bc->environment());
    m_pathChooser->setPath(m_buildConfiguration->rawBuildDirectory().toString());
    setDisplayName(tr("Generic Manager"));

    connect(bc, &GenericBuildConfiguration::environmentChanged,
            this, &GenericBuildSettingsWidget::environmentHasChanged);
}

void GenericBuildSettingsWidget::buildDirectoryChanged()
{
    m_buildConfiguration->setBuildDirectory(Utils::FileName::fromString(m_pathChooser->rawPath()));
}

void GenericBuildSettingsWidget::environmentHasChanged()
{
    m_pathChooser->setEnvironment(m_buildConfiguration->environment());
}

} // namespace Internal
} // namespace GenericProjectManager
