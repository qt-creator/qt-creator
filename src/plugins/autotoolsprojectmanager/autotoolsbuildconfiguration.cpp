/****************************************************************************
**
** Copyright (C) 2016 Openismus GmbH.
** Author: Peter Penz (ppenz@openismus.com)
** Author: Patricia Santana Cruz (patriciasantanacruz@gmail.com)
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

#include "autotoolsbuildconfiguration.h"
#include "autotoolsbuildsettingswidget.h"
#include "makestep.h"
#include "autotoolsproject.h"
#include "autotoolsprojectconstants.h"
#include "autogenstep.h"
#include "autoreconfstep.h"
#include "configurestep.h"

#include <coreplugin/icore.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QInputDialog>

using namespace AutotoolsProjectManager;
using namespace AutotoolsProjectManager::Constants;
using namespace Internal;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Constants;


// AutotoolsBuildConfiguration

AutotoolsBuildConfiguration::AutotoolsBuildConfiguration(Target *parent, Core::Id id)
    : BuildConfiguration(parent, id)
{
    // /<foobar> is used so the un-changed check in setBuildDirectory() works correctly.
    // The leading / is to avoid the relative the path expansion in BuildConfiguration::buildDirectory.
    setBuildDirectory(Utils::FileName::fromString("/<foobar>"));
}

void AutotoolsBuildConfiguration::initialize(const BuildInfo *info)
{
    BuildConfiguration::initialize(info);

    BuildStepList *buildSteps = stepList(BUILDSTEPS_BUILD);

    // ### Build Steps Build ###
    // autogen.sh or autoreconf
    QFile autogenFile(target()->project()->projectDirectory().toString() + "/autogen.sh");
    if (autogenFile.exists()) {
        AutogenStep *autogenStep = new AutogenStep(buildSteps);
        buildSteps->insertStep(0, autogenStep);
    } else {
        AutoreconfStep *autoreconfStep = new AutoreconfStep(buildSteps);
        autoreconfStep->setAdditionalArguments("--force --install");
        buildSteps->insertStep(0, autoreconfStep);
    }

    // ./configure.
    ConfigureStep *configureStep = new ConfigureStep(buildSteps);
    buildSteps->insertStep(1, configureStep);
    connect(this, &BuildConfiguration::buildDirectoryChanged,
            configureStep, &ConfigureStep::notifyBuildDirectoryChanged);

    // make
    MakeStep *makeStep = new MakeStep(buildSteps, "all");
    buildSteps->insertStep(2, makeStep);

    // ### Build Steps Clean ###
    BuildStepList *cleanSteps = stepList(BUILDSTEPS_CLEAN);
    MakeStep *cleanMakeStep = new MakeStep(cleanSteps, "clean");
    cleanMakeStep->setClean(true);
    cleanSteps->insertStep(0, cleanMakeStep);
}

NamedWidget *AutotoolsBuildConfiguration::createConfigWidget()
{
    return new AutotoolsBuildSettingsWidget(this);
}


// AutotoolsBuildConfiguration class

AutotoolsBuildConfigurationFactory::AutotoolsBuildConfigurationFactory()
{
    registerBuildConfiguration<AutotoolsBuildConfiguration>
            ("AutotoolsProjectManager.AutotoolsBuildConfiguration");

    setSupportedProjectType(Constants::AUTOTOOLS_PROJECT_ID);
    setSupportedProjectMimeTypeName(Constants::MAKEFILE_MIMETYPE);
}

QList<BuildInfo *> AutotoolsBuildConfigurationFactory::availableBuilds(const Target *parent) const
{
    return {createBuildInfo(parent->kit(), parent->project()->projectDirectory())};
}

QList<BuildInfo *> AutotoolsBuildConfigurationFactory::availableSetups(const Kit *k, const QString &projectPath) const
{
    QList<BuildInfo *> result;
    BuildInfo *info = createBuildInfo(k,
                                      Utils::FileName::fromString(AutotoolsProject::defaultBuildDirectory(projectPath)));
    //: The name of the build configuration created by default for a autotools project.
    info->displayName = tr("Default");
    result << info;
    return result;
}

BuildInfo *AutotoolsBuildConfigurationFactory::createBuildInfo(const Kit *k,
                                                               const Utils::FileName &buildDir) const
{
    BuildInfo *info = new BuildInfo(this);
    info->typeName = tr("Build");
    info->buildDirectory = buildDir;
    info->kitId = k->id();

    return info;
}

BuildConfiguration::BuildType AutotoolsBuildConfiguration::buildType() const
{
    // TODO: Should I return something different from Unknown?
    return Unknown;
}
