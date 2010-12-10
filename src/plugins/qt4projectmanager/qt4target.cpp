/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qt4target.h"

#include "makestep.h"
#include "qmakestep.h"
#include "qt4project.h"
#include "qt4projectmanagerconstants.h"
#include "qt4projectconfigwidget.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/customexecutablerunconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

// -------------------------------------------------------------------------
// Qt4BaseTargetFactory
// -------------------------------------------------------------------------

Qt4BaseTargetFactory::Qt4BaseTargetFactory(QObject *parent) :
    ITargetFactory(parent)
{

}

Qt4BaseTargetFactory::~Qt4BaseTargetFactory()
{

}

Qt4BaseTargetFactory *Qt4BaseTargetFactory::qt4BaseTargetFactoryForId(const QString &id)
{
    QList<Qt4BaseTargetFactory *> factories = ExtensionSystem::PluginManager::instance()->getObjects<Qt4BaseTargetFactory>();
    foreach (Qt4BaseTargetFactory *fac, factories) {
        if (fac->supportsTargetId(id))
            return fac;
    }
    return 0;
}

// -------------------------------------------------------------------------
// Qt4BaseTarget
// -------------------------------------------------------------------------

Qt4BaseTarget::Qt4BaseTarget(Qt4Project *parent, const QString &id) :
    ProjectExplorer::Target(parent, id)
{
    connect(this, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
            this, SLOT(emitProFileEvaluateNeeded()));
    connect(this, SIGNAL(activeBuildConfigurationChanged(ProjectExplorer::BuildConfiguration*)),
            this, SIGNAL(environmentChanged()));
    connect(this, SIGNAL(addedBuildConfiguration(ProjectExplorer::BuildConfiguration*)),
            this, SLOT(onAddedBuildConfiguration(ProjectExplorer::BuildConfiguration*)));
}

Qt4BaseTarget::~Qt4BaseTarget()
{
}

ProjectExplorer::BuildConfigWidget *Qt4BaseTarget::createConfigWidget()
{
    return new Qt4ProjectConfigWidget(this);
}

Qt4BuildConfiguration *Qt4BaseTarget::activeBuildConfiguration() const
{
    return static_cast<Qt4BuildConfiguration *>(Target::activeBuildConfiguration());
}

Qt4Project *Qt4BaseTarget::qt4Project() const
{
    return static_cast<Qt4Project *>(project());
}

QList<ProjectExplorer::ToolChainType> Qt4BaseTarget::filterToolChainTypes(const QList<ProjectExplorer::ToolChainType> &candidates) const
{
    return candidates;
}

ProjectExplorer::ToolChainType Qt4BaseTarget::preferredToolChainType(const QList<ProjectExplorer::ToolChainType> &candidates) const
{
    ProjectExplorer::ToolChainType preferredType = ProjectExplorer::ToolChain_INVALID;
    if (!candidates.isEmpty())
        preferredType = candidates.at(0);
    return preferredType;
}

QString Qt4BaseTarget::defaultBuildDirectory() const
{
    Qt4BaseTargetFactory *fac = Qt4BaseTargetFactory::qt4BaseTargetFactoryForId(id());
    return fac->defaultShadowBuildDirectory(qt4Project()->defaultTopLevelBuildDirectory(), id());
}

void Qt4BaseTarget::removeUnconfiguredCustomExectutableRunConfigurations()
{
    if (runConfigurations().count()) {
        // Remove all run configurations which the new project wizard created
        QList<ProjectExplorer::RunConfiguration*> toRemove;
        foreach (ProjectExplorer::RunConfiguration * rc, runConfigurations()) {
            ProjectExplorer::CustomExecutableRunConfiguration *cerc
                    = qobject_cast<ProjectExplorer::CustomExecutableRunConfiguration *>(rc);
            if (cerc && !cerc->isConfigured())
                toRemove.append(rc);
        }
        foreach (ProjectExplorer::RunConfiguration *rc, toRemove)
            removeRunConfiguration(rc);
    }
}

Qt4BuildConfiguration *Qt4BaseTarget::addQt4BuildConfiguration(QString displayName, QtVersion *qtversion,
                                                           QtVersion::QmakeBuildConfigs qmakeBuildConfiguration,
                                                           QString additionalArguments,
                                                           QString directory)
{
    Q_ASSERT(qtversion);
    bool debug = qmakeBuildConfiguration & QtVersion::DebugBuild;

    // Add the buildconfiguration
    Qt4BuildConfiguration *bc = new Qt4BuildConfiguration(this);
    bc->setDefaultDisplayName(displayName);

    ProjectExplorer::BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    ProjectExplorer::BuildStepList *cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
    Q_ASSERT(buildSteps);
    Q_ASSERT(cleanSteps);

    QMakeStep *qmakeStep = new QMakeStep(buildSteps);
    buildSteps->insertStep(0, qmakeStep);

    MakeStep *makeStep = new MakeStep(buildSteps);
    buildSteps->insertStep(1, makeStep);

    MakeStep* cleanStep = new MakeStep(cleanSteps);
    cleanStep->setClean(true);
    cleanStep->setUserArguments("clean");
    cleanSteps->insertStep(0, cleanStep);
    if (!additionalArguments.isEmpty())
        qmakeStep->setUserArguments(additionalArguments);

    // set some options for qmake and make
    if (qmakeBuildConfiguration & QtVersion::BuildAll) // debug_and_release => explicit targets
        makeStep->setUserArguments(debug ? "debug" : "release");

    bc->setQMakeBuildConfiguration(qmakeBuildConfiguration);

    // Finally set the qt version & ToolChain
    bc->setQtVersion(qtversion);
    ProjectExplorer::ToolChainType defaultTc = preferredToolChainType(filterToolChainTypes(bc->qtVersion()->possibleToolChainTypes()));
    bc->setToolChainType(defaultTc);
    if (!directory.isEmpty())
        bc->setShadowBuildAndDirectory(directory != project()->projectDirectory(), directory);
    addBuildConfiguration(bc);

    return bc;
}

void Qt4BaseTarget::onAddedBuildConfiguration(ProjectExplorer::BuildConfiguration *bc)
{
    Q_ASSERT(bc);
    Qt4BuildConfiguration *qt4bc = qobject_cast<Qt4BuildConfiguration *>(bc);
    Q_ASSERT(qt4bc);
    connect(qt4bc, SIGNAL(buildDirectoryInitialized()),
            this, SIGNAL(buildDirectoryInitialized()));
    connect(qt4bc, SIGNAL(proFileEvaluateNeeded(Qt4ProjectManager::Qt4BuildConfiguration *)),
            this, SLOT(onProFileEvaluateNeeded(Qt4ProjectManager::Qt4BuildConfiguration *)));
}

void Qt4BaseTarget::onProFileEvaluateNeeded(Qt4ProjectManager::Qt4BuildConfiguration *bc)
{
    if (bc && bc == activeBuildConfiguration())
        emit proFileEvaluateNeeded(this);
}

void Qt4BaseTarget::emitProFileEvaluateNeeded()
{
    emit proFileEvaluateNeeded(this);
}
