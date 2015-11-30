/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
#include <projectexplorer/toolchain.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QFormLayout>
#include <QInputDialog>

using namespace ProjectExplorer;

namespace GenericProjectManager {
namespace Internal {

const char GENERIC_BC_ID[] = "GenericProjectManager.GenericBuildConfiguration";

GenericBuildConfiguration::GenericBuildConfiguration(Target *parent)
    : BuildConfiguration(parent, Core::Id(GENERIC_BC_ID))
{
}

GenericBuildConfiguration::GenericBuildConfiguration(Target *parent, Core::Id id)
    : BuildConfiguration(parent, id)
{
}

GenericBuildConfiguration::GenericBuildConfiguration(Target *parent, GenericBuildConfiguration *source) :
    BuildConfiguration(parent, source)
{
    cloneSteps(source);
}

NamedWidget *GenericBuildConfiguration::createConfigWidget()
{
    return new GenericBuildSettingsWidget(this);
}

/*!
  \class GenericBuildConfigurationFactory
*/

GenericBuildConfigurationFactory::GenericBuildConfigurationFactory(QObject *parent) :
    IBuildConfigurationFactory(parent)
{
}

GenericBuildConfigurationFactory::~GenericBuildConfigurationFactory()
{
}

int GenericBuildConfigurationFactory::priority(const Target *parent) const
{
    return canHandle(parent) ? 0 : -1;
}

QList<BuildInfo *> GenericBuildConfigurationFactory::availableBuilds(const Target *parent) const
{
    QList<BuildInfo *> result;
    BuildInfo *info = createBuildInfo(parent->kit(), parent->project()->projectDirectory());
    result << info;
    return result;
}

int GenericBuildConfigurationFactory::priority(const Kit *k, const QString &projectPath) const
{
    Utils::MimeDatabase mdb;
    if (k && mdb.mimeTypeForFile(projectPath).matchesName(QLatin1String(Constants::GENERICMIMETYPE)))
        return 0;
    return -1;
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

BuildConfiguration *GenericBuildConfigurationFactory::create(Target *parent, const BuildInfo *info) const
{
    QTC_ASSERT(info->factory() == this, return 0);
    QTC_ASSERT(info->kitId == parent->kit()->id(), return 0);
    QTC_ASSERT(!info->displayName.isEmpty(), return 0);

    GenericBuildConfiguration *bc = new GenericBuildConfiguration(parent);
    bc->setDisplayName(info->displayName);
    bc->setDefaultDisplayName(info->displayName);
    bc->setBuildDirectory(info->buildDirectory);

    BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    BuildStepList *cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);

    Q_ASSERT(buildSteps);
    GenericMakeStep *makeStep = new GenericMakeStep(buildSteps);
    buildSteps->insertStep(0, makeStep);
    makeStep->setBuildTarget(QLatin1String("all"), /* on = */ true);

    Q_ASSERT(cleanSteps);
    GenericMakeStep *cleanMakeStep = new GenericMakeStep(cleanSteps);
    cleanSteps->insertStep(0, cleanMakeStep);
    cleanMakeStep->setBuildTarget(QLatin1String("clean"), /* on = */ true);
    cleanMakeStep->setClean(true);

    return bc;
}

bool GenericBuildConfigurationFactory::canClone(const Target *parent, BuildConfiguration *source) const
{
    if (!canHandle(parent))
        return false;
    return source->id() == GENERIC_BC_ID;
}

BuildConfiguration *GenericBuildConfigurationFactory::clone(Target *parent, BuildConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    return new GenericBuildConfiguration(parent, qobject_cast<GenericBuildConfiguration *>(source));
}

bool GenericBuildConfigurationFactory::canRestore(const Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    return ProjectExplorer::idFromMap(map) == GENERIC_BC_ID;
}

BuildConfiguration *GenericBuildConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    GenericBuildConfiguration *bc(new GenericBuildConfiguration(parent));
    if (bc->fromMap(map))
        return bc;
    delete bc;
    return 0;
}

bool GenericBuildConfigurationFactory::canHandle(const Target *t) const
{
    if (!t->project()->supportsKit(t->kit()))
        return false;
    return qobject_cast<GenericProject *>(t->project());
}

BuildInfo *GenericBuildConfigurationFactory::createBuildInfo(const Kit *k,
                                                             const Utils::FileName &buildDir) const
{
    BuildInfo *info = new BuildInfo(this);
    info->typeName = tr("Build");
    info->buildDirectory = buildDir;
    info->kitId = k->id();
    return info;
}

BuildConfiguration::BuildType GenericBuildConfiguration::buildType() const
{
    return Unknown;
}

////////////////////////////////////////////////////////////////////////////////////
// GenericBuildSettingsWidget
////////////////////////////////////////////////////////////////////////////////////

GenericBuildSettingsWidget::GenericBuildSettingsWidget(GenericBuildConfiguration *bc)
    : m_buildConfiguration(0)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setContentsMargins(0, -1, 0, -1);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    // build directory
    m_pathChooser = new Utils::PathChooser(this);
    m_pathChooser->setHistoryCompleter(QLatin1String("Generic.BuildDir.History"));
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
