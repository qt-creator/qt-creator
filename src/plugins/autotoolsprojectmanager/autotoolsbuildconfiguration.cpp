/**************************************************************************
**
** Copyright (C) 2015 Openismus GmbH.
** Authors: Peter Penz (ppenz@openismus.com)
**          Patricia Santana Cruz (patriciasantanacruz@gmail.com)
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
#include <qtsupport/customexecutablerunconfiguration.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QInputDialog>

using namespace AutotoolsProjectManager;
using namespace AutotoolsProjectManager::Constants;
using namespace Internal;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Constants;

//////////////////////////////////////
// AutotoolsBuildConfiguration class
//////////////////////////////////////
AutotoolsBuildConfiguration::AutotoolsBuildConfiguration(Target *parent)
    : BuildConfiguration(parent, Core::Id(AUTOTOOLS_BC_ID))
{
  // /<foobar> is used so the un-changed check in setBuildDirectory() works correctly.
  // The leading / is to avoid the relative the path expansion in BuildConfiguration::buildDirectory.
  BuildConfiguration::setBuildDirectory(Utils::FileName::fromString(QString::fromLatin1("/<foobar>")));
}

NamedWidget *AutotoolsBuildConfiguration::createConfigWidget()
{
    return new AutotoolsBuildSettingsWidget(this);
}

AutotoolsBuildConfiguration::AutotoolsBuildConfiguration(Target *parent, Core::Id id)
    : BuildConfiguration(parent, id)
{ }

AutotoolsBuildConfiguration::AutotoolsBuildConfiguration(Target *parent,
                                                         AutotoolsBuildConfiguration *source)
    : BuildConfiguration(parent, source)
{
    cloneSteps(source);
}

//////////////////////////////////////
// AutotoolsBuildConfiguration class
//////////////////////////////////////
AutotoolsBuildConfigurationFactory::AutotoolsBuildConfigurationFactory(QObject *parent) :
    IBuildConfigurationFactory(parent)
{
}

int AutotoolsBuildConfigurationFactory::priority(const Target *parent) const
{
    return canHandle(parent) ? 0 : -1;
}

QList<BuildInfo *> AutotoolsBuildConfigurationFactory::availableBuilds(const Target *parent) const
{
    QList<BuildInfo *> result;
    result << createBuildInfo(parent->kit(), parent->project()->projectDirectory());
    return result;
}

int AutotoolsBuildConfigurationFactory::priority(const Kit *k, const QString &projectPath) const
{
    Utils::MimeDatabase mdb;
    if (k && mdb.mimeTypeForFile(projectPath).matchesName(QLatin1String(Constants::MAKEFILE_MIMETYPE)))
        return 0;
    return -1;
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

BuildConfiguration *AutotoolsBuildConfigurationFactory::create(Target *parent, const BuildInfo *info) const
{
    QTC_ASSERT(parent, return 0);
    QTC_ASSERT(info->factory() == this, return 0);
    QTC_ASSERT(info->kitId == parent->kit()->id(), return 0);
    QTC_ASSERT(!info->displayName.isEmpty(), return 0);

    AutotoolsBuildConfiguration *bc = new AutotoolsBuildConfiguration(parent);
    bc->setDisplayName(info->displayName);
    bc->setDefaultDisplayName(info->displayName);
    bc->setBuildDirectory(info->buildDirectory);

    BuildStepList *buildSteps = bc->stepList(Core::Id(BUILDSTEPS_BUILD));

    // ### Build Steps Build ###
    // autogen.sh or autoreconf
    QFile autogenFile(parent->project()->projectDirectory().toString() + QLatin1String("/autogen.sh"));
    if (autogenFile.exists()) {
        AutogenStep *autogenStep = new AutogenStep(buildSteps);
        buildSteps->insertStep(0, autogenStep);
    } else {
        AutoreconfStep *autoreconfStep = new AutoreconfStep(buildSteps);
        autoreconfStep->setAdditionalArguments(QLatin1String("--force --install"));
        buildSteps->insertStep(0, autoreconfStep);
    }

    // ./configure.
    ConfigureStep *configureStep = new ConfigureStep(buildSteps);
    buildSteps->insertStep(1, configureStep);

    // make
    MakeStep *makeStep = new MakeStep(buildSteps);
    buildSteps->insertStep(2, makeStep);
    makeStep->setBuildTarget(QLatin1String("all"),  /*on =*/ true);

    // ### Build Steps Clean ###
    BuildStepList *cleanSteps = bc->stepList(Core::Id(BUILDSTEPS_CLEAN));
    MakeStep *cleanMakeStep = new MakeStep(cleanSteps);
    cleanMakeStep->setAdditionalArguments(QLatin1String("clean"));
    cleanMakeStep->setClean(true);
    cleanSteps->insertStep(0, cleanMakeStep);

    return bc;
}

bool AutotoolsBuildConfigurationFactory::canHandle(const Target *t) const
{
    QTC_ASSERT(t, return false);

    if (!t->project()->supportsKit(t->kit()))
        return false;
    return t->project()->id() == Constants::AUTOTOOLS_PROJECT_ID;
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

bool AutotoolsBuildConfigurationFactory::canClone(const Target *parent, BuildConfiguration *source) const
{
    if (!canHandle(parent))
        return false;
    return source->id() == AUTOTOOLS_BC_ID;
}

AutotoolsBuildConfiguration *AutotoolsBuildConfigurationFactory::clone(Target *parent, BuildConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;

    AutotoolsBuildConfiguration *origin = static_cast<AutotoolsBuildConfiguration *>(source);
    return new AutotoolsBuildConfiguration(parent, origin);
}

bool AutotoolsBuildConfigurationFactory::canRestore(const Target *parent, const QVariantMap &map) const
{
    if (!canHandle(parent))
        return false;
    return idFromMap(map) == AUTOTOOLS_BC_ID;
}

AutotoolsBuildConfiguration *AutotoolsBuildConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    AutotoolsBuildConfiguration *bc = new AutotoolsBuildConfiguration(parent);
    if (bc->fromMap(map))
        return bc;
    delete bc;
    return 0;
}

BuildConfiguration::BuildType AutotoolsBuildConfiguration::buildType() const
{
    // TODO: Should I return something different from Unknown?
    return Unknown;
}

void AutotoolsBuildConfiguration::setBuildDirectory(const Utils::FileName &directory)
{
    if (directory == buildDirectory())
        return;
    BuildConfiguration::setBuildDirectory(directory);
    BuildStepList *bsl = stepList(BUILDSTEPS_BUILD);
    foreach (BuildStep *bs, bsl->steps()) {
        ConfigureStep *cs = qobject_cast<ConfigureStep *>(bs);
        if (cs) {
            cs->notifyBuildDirectoryChanged();
        }
    }
}
