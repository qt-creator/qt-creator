/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010-2011 Openismus GmbH.
**   Authors: Peter Penz (ppenz@openismus.com)
**            Patricia Santana Cruz (patriciasantanacruz@gmail.com)
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "autotoolsbuildconfiguration.h"
#include "makestep.h"
#include "autotoolsproject.h"
#include "autotoolstarget.h"
#include "autotoolsprojectconstants.h"
#include "autogenstep.h"
#include "autoreconfstep.h"
#include "configurestep.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/customexecutablerunconfiguration.h>
#include <utils/qtcassert.h>

#include <QtGui/QInputDialog>

using namespace AutotoolsProjectManager;
using namespace Internal;

//////////////////////////////////////
// AutotoolsBuildConfiguration class
//////////////////////////////////////
AutotoolsBuildConfiguration::AutotoolsBuildConfiguration(AutotoolsTarget *parent)
    : BuildConfiguration(parent, QLatin1String(Constants::AUTOTOOLS_BC_ID))
{
    m_buildDirectory = autotoolsTarget()->defaultBuildDirectory();
}

AutotoolsBuildConfiguration::AutotoolsBuildConfiguration(AutotoolsTarget *parent, const QString &id)
    : BuildConfiguration(parent, id)
{
}

AutotoolsBuildConfiguration::AutotoolsBuildConfiguration(AutotoolsTarget *parent, AutotoolsBuildConfiguration *source)
    : BuildConfiguration(parent, source),
      m_buildDirectory(source->m_buildDirectory)
{
    cloneSteps(source);
}

QVariantMap AutotoolsBuildConfiguration::toMap() const
{
    QVariantMap map = ProjectExplorer::BuildConfiguration::toMap();
    map.insert(QLatin1String(Constants::BUILD_DIRECTORY_KEY), m_buildDirectory);
    return map;
}

bool AutotoolsBuildConfiguration::fromMap(const QVariantMap &map)
{
    if (!BuildConfiguration::fromMap(map))
        return false;

    m_buildDirectory = map.value(QLatin1String(Constants::BUILD_DIRECTORY_KEY), autotoolsTarget()->defaultBuildDirectory()).toString();
    return true;
}

QString AutotoolsBuildConfiguration::buildDirectory() const
{
    return m_buildDirectory;
}

void AutotoolsBuildConfiguration::setBuildDirectory(const QString &buildDirectory)
{
    if (m_buildDirectory == buildDirectory)
        return;
    m_buildDirectory = buildDirectory;
    emit buildDirectoryChanged();
}

AutotoolsTarget *AutotoolsBuildConfiguration::autotoolsTarget() const
{
    return static_cast<AutotoolsTarget *>(target());
}

ProjectExplorer::IOutputParser *AutotoolsBuildConfiguration::createOutputParser() const
{
    ProjectExplorer::ToolChain *tc = autotoolsTarget()->autotoolsProject()->toolChain();
    if (tc)
        return tc->outputParser();
    return 0;
}

//////////////////////////////////////
// AutotoolsBuildConfiguration class
//////////////////////////////////////
AutotoolsBuildConfigurationFactory::AutotoolsBuildConfigurationFactory(QObject *parent) :
    ProjectExplorer::IBuildConfigurationFactory(parent)
{
}

QStringList AutotoolsBuildConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    if (!qobject_cast<AutotoolsTarget *>(parent))
        return QStringList();
    return QStringList() << QLatin1String(Constants::AUTOTOOLS_BC_ID);
}

QString AutotoolsBuildConfigurationFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(Constants::AUTOTOOLS_BC_ID))
        return tr("Build");
    return QString();
}

bool AutotoolsBuildConfigurationFactory::canCreate(ProjectExplorer::Target *parent, const QString &id) const
{
    if (!qobject_cast<AutotoolsTarget *>(parent))
        return false;
    if (id == QLatin1String(Constants::AUTOTOOLS_BC_ID))
        return true;
    return false;
}

AutotoolsBuildConfiguration *AutotoolsBuildConfigurationFactory::create(ProjectExplorer::Target *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;

    AutotoolsTarget *t = static_cast<AutotoolsTarget *>(parent);
    AutotoolsBuildConfiguration *bc = createDefaultConfiguration(t);

    bool ok;
    QString buildConfigurationName = QInputDialog::getText(0,
                          tr("New Configuration"),
                          tr("New configuration name:"),
                          QLineEdit::Normal,
                          QString(),
                          &ok);

    if (!ok || buildConfigurationName.isEmpty())
        return 0;
    bc->setDisplayName(buildConfigurationName);

    t->addBuildConfiguration(bc);
    t->addDeployConfiguration(t->createDeployConfiguration(ProjectExplorer::Constants::DEFAULT_DEPLOYCONFIGURATION_ID));
    // User needs to choose where the executable file is.
    // TODO: Parse the file in *Anjuta style* to be able to add custom RunConfigurations.
    t->addRunConfiguration(new ProjectExplorer::CustomExecutableRunConfiguration(t));

    return bc;
}

AutotoolsBuildConfiguration *AutotoolsBuildConfigurationFactory::createDefaultConfiguration(AutotoolsTarget *target) const
{
    AutotoolsBuildConfiguration *bc = new AutotoolsBuildConfiguration(target);
    ProjectExplorer::BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);

    // ### Build Steps Build ###
    // autogen.sh or autoreconf
    QFile autogenFile(target->autotoolsProject()->sourceDirectory() + QLatin1String("/autogen.sh"));
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
    ProjectExplorer::BuildStepList *cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
    MakeStep *cleanMakeStep = new MakeStep(cleanSteps);
    cleanMakeStep->setAdditionalArguments(QLatin1String("clean"));
    cleanMakeStep->setClean(true);
    cleanSteps->insertStep(0, cleanMakeStep);

    return bc;
}

bool AutotoolsBuildConfigurationFactory::canClone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const
{
    return canCreate(parent, source->id());
}

AutotoolsBuildConfiguration *AutotoolsBuildConfigurationFactory::clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;

    AutotoolsBuildConfiguration *origin = static_cast<AutotoolsBuildConfiguration *>(source);
    AutotoolsTarget *target(static_cast<AutotoolsTarget *>(parent));
    return new AutotoolsBuildConfiguration(target, origin);
}

bool AutotoolsBuildConfigurationFactory::canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    QString id(ProjectExplorer::idFromMap(map));
    return canCreate(parent, id);
}

AutotoolsBuildConfiguration *AutotoolsBuildConfigurationFactory::restore(ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    AutotoolsTarget *target(static_cast<AutotoolsTarget *>(parent));
    AutotoolsBuildConfiguration *bc = new AutotoolsBuildConfiguration(target);
    if (bc->fromMap(map))
        return bc;
    delete bc;
    return 0;
}

ProjectExplorer::BuildConfiguration::BuildType AutotoolsBuildConfiguration::buildType() const
{
    // TODO: Should I return something different from Unknown?
    return Unknown;
}
