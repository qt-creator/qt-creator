/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "generictarget.h"

#include "genericbuildconfiguration.h"
#include "genericproject.h"
#include "genericmakestep.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qtsupport/customexecutablerunconfiguration.h>

#include <QApplication>
#include <QStyle>

const char GENERIC_DESKTOP_TARGET_DISPLAY_NAME[] = "Desktop";

using namespace GenericProjectManager;
using namespace GenericProjectManager::Internal;

////////////////////////////////////////////////////////////////////////////////////
// GenericTarget
////////////////////////////////////////////////////////////////////////////////////

GenericTarget::GenericTarget(GenericProject *parent) :
    ProjectExplorer::Target(parent, Core::Id(GENERIC_DESKTOP_TARGET_ID)),
    m_buildConfigurationFactory(new GenericBuildConfigurationFactory(this))
{
    setDefaultDisplayName(QApplication::translate("GenericProjectManager::GenericTarget",
                                                  GENERIC_DESKTOP_TARGET_DISPLAY_NAME));
    setIcon(qApp->style()->standardIcon(QStyle::SP_ComputerIcon));
}

ProjectExplorer::BuildConfigWidget *GenericTarget::createConfigWidget()
{
    return new GenericBuildSettingsWidget(this);
}

GenericProject *GenericTarget::genericProject() const
{
    return static_cast<GenericProject *>(project());
}

GenericBuildConfigurationFactory *GenericTarget::buildConfigurationFactory() const
{
    return m_buildConfigurationFactory;
}

GenericBuildConfiguration *GenericTarget::activeBuildConfiguration() const
{
    return static_cast<GenericBuildConfiguration *>(Target::activeBuildConfiguration());
}

bool GenericTarget::fromMap(const QVariantMap &map)
{
    return Target::fromMap(map);
}

////////////////////////////////////////////////////////////////////////////////////
// GenericTargetFactory
////////////////////////////////////////////////////////////////////////////////////

GenericTargetFactory::GenericTargetFactory(QObject *parent) :
    ITargetFactory(parent)
{
}

bool GenericTargetFactory::supportsTargetId(const Core::Id id) const
{
    return id == Core::Id(GENERIC_DESKTOP_TARGET_ID);
}

QList<Core::Id> GenericTargetFactory::supportedTargetIds() const
{
    return QList<Core::Id>() << Core::Id(GENERIC_DESKTOP_TARGET_ID);
}

QString GenericTargetFactory::displayNameForId(const Core::Id id) const
{
    if (id == Core::Id(GENERIC_DESKTOP_TARGET_ID))
        return QCoreApplication::translate("GenericProjectManager::GenericTarget",
                                           GENERIC_DESKTOP_TARGET_DISPLAY_NAME,
                                           "Generic desktop target display name");
    return QString();
}

bool GenericTargetFactory::canCreate(ProjectExplorer::Project *parent, const Core::Id id) const
{
    if (!qobject_cast<GenericProject *>(parent))
        return false;
    return id == Core::Id(GENERIC_DESKTOP_TARGET_ID);
}

GenericTarget *GenericTargetFactory::create(ProjectExplorer::Project *parent, const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    GenericProject *genericproject = static_cast<GenericProject *>(parent);
    GenericTarget *t = new GenericTarget(genericproject);

    // Set up BuildConfiguration:
    GenericBuildConfiguration *bc = new GenericBuildConfiguration(t);
    bc->setDisplayName(QLatin1String("all"));

    ProjectExplorer::BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    ProjectExplorer::BuildStepList *cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);

    GenericMakeStep *makeStep = new GenericMakeStep(buildSteps);
    buildSteps->insertStep(0, makeStep);
    makeStep->setBuildTarget(QLatin1String("all"), /* on = */ true);

    GenericMakeStep *cleanMakeStep = new GenericMakeStep(cleanSteps);
    cleanSteps->insertStep(0, cleanMakeStep);
    cleanMakeStep->setBuildTarget(QLatin1String("clean"), /* on = */ true);
    cleanMakeStep->setClean(true);

    bc->setBuildDirectory(genericproject->projectDirectory());

    t->addBuildConfiguration(bc);

    t->addDeployConfiguration(t->createDeployConfiguration(ProjectExplorer::Constants::DEFAULT_DEPLOYCONFIGURATION_ID));

    // Add a runconfiguration. The CustomExecutableRC one will query the user
    // for its settings, so it is a good choice here.
    t->addRunConfiguration(new QtSupport::CustomExecutableRunConfiguration(t));

    return t;
}

bool GenericTargetFactory::canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

GenericTarget *GenericTargetFactory::restore(ProjectExplorer::Project *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    GenericProject *genericproject = static_cast<GenericProject *>(parent);
    GenericTarget *target = new GenericTarget(genericproject);
    if (target->fromMap(map))
        return target;
    delete target;
    return 0;
}
