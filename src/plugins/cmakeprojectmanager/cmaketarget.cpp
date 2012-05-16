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

#include "cmaketarget.h"

#include "cmakeproject.h"
#include "cmakerunconfiguration.h"
#include "cmakebuildconfiguration.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qtsupport/customexecutablerunconfiguration.h>

#include <QApplication>
#include <QStyle>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;

namespace {

QString displayNameForId(const Core::Id id) {
    if (id == Core::Id(DEFAULT_CMAKE_TARGET_ID))
        return QApplication::translate("CMakeProjectManager::Internal::CMakeTarget", "Desktop", "CMake Default target display name");
    return QString();
}

} // namespace

// -------------------------------------------------------------------------
// CMakeTarget
// -------------------------------------------------------------------------

CMakeTarget::CMakeTarget(CMakeProject *parent) :
    ProjectExplorer::Target(parent, Core::Id(DEFAULT_CMAKE_TARGET_ID)),
    m_buildConfigurationFactory(new CMakeBuildConfigurationFactory(this))
{
    setDefaultDisplayName(displayNameForId(id()));
    setIcon(qApp->style()->standardIcon(QStyle::SP_ComputerIcon));
    connect(parent, SIGNAL(buildTargetsChanged()), SLOT(updateRunConfigurations()));
}

CMakeTarget::~CMakeTarget()
{
}

CMakeProject *CMakeTarget::cmakeProject() const
{
    return static_cast<CMakeProject *>(project());
}

ProjectExplorer::BuildConfigWidget *CMakeTarget::createConfigWidget()
{
    return new CMakeBuildSettingsWidget(this);
}

bool CMakeTargetFactory::supportsTargetId(const Core::Id id) const
{
    return id == Core::Id(DEFAULT_CMAKE_TARGET_ID);
}

CMakeBuildConfiguration *CMakeTarget::activeBuildConfiguration() const
{
    return static_cast<CMakeBuildConfiguration *>(Target::activeBuildConfiguration());
}

CMakeBuildConfigurationFactory *CMakeTarget::buildConfigurationFactory() const
{
    return m_buildConfigurationFactory;
}

QString CMakeTarget::defaultBuildDirectory() const
{
    return cmakeProject()->defaultBuildDirectory();
}

bool CMakeTarget::fromMap(const QVariantMap &map)
{
    return Target::fromMap(map);
}

void CMakeTarget::updateRunConfigurations()
{
    // *Update* runconfigurations:
    QMultiMap<QString, CMakeRunConfiguration*> existingRunConfigurations;
    QList<ProjectExplorer::RunConfiguration *> toRemove;
    foreach (ProjectExplorer::RunConfiguration* rc, runConfigurations()) {
        if (CMakeRunConfiguration* cmakeRC = qobject_cast<CMakeRunConfiguration *>(rc))
            existingRunConfigurations.insert(cmakeRC->title(), cmakeRC);
        QtSupport::CustomExecutableRunConfiguration *ceRC =
                qobject_cast<QtSupport::CustomExecutableRunConfiguration *>(rc);
        if (ceRC && !ceRC->isConfigured())
            toRemove << rc;
    }

    foreach (const CMakeBuildTarget &ct, cmakeProject()->buildTargets()) {
        if (ct.library)
            continue;
        if (ct.executable.isEmpty())
            continue;
        if (ct.title.endsWith("/fast"))
            continue;
        QList<CMakeRunConfiguration *> list = existingRunConfigurations.values(ct.title);
        if (!list.isEmpty()) {
            // Already exists, so override the settings...
            foreach (CMakeRunConfiguration *rc, list) {
                rc->setExecutable(ct.executable);
                rc->setBaseWorkingDirectory(ct.workingDirectory);
                rc->setEnabled(true);
            }
            existingRunConfigurations.remove(ct.title);
        } else {
            // Does not exist yet
            addRunConfiguration(new CMakeRunConfiguration(this, ct.executable, ct.workingDirectory, ct.title));
        }
    }
    QMultiMap<QString, CMakeRunConfiguration *>::const_iterator it =
            existingRunConfigurations.constBegin();
    for ( ; it != existingRunConfigurations.constEnd(); ++it) {
        CMakeRunConfiguration *rc = it.value();
        // The executables for those runconfigurations aren't build by the current buildconfiguration
        // We just set a disable flag and show that in the display name
        rc->setEnabled(false);
        // removeRunConfiguration(rc);
    }

    foreach (ProjectExplorer::RunConfiguration *rc, toRemove)
        removeRunConfiguration(rc);

    if (runConfigurations().isEmpty()) {
        // Oh no, no run configuration,
        // create a custom executable run configuration
        addRunConfiguration(new QtSupport::CustomExecutableRunConfiguration(this));
    }
}

// -------------------------------------------------------------------------
// CMakeTargetFactory
// -------------------------------------------------------------------------

CMakeTargetFactory::CMakeTargetFactory(QObject *parent) :
    ITargetFactory(parent)
{
}

CMakeTargetFactory::~CMakeTargetFactory()
{
}

QList<Core::Id> CMakeTargetFactory::supportedTargetIds() const
{
    return QList<Core::Id>() << Core::Id(DEFAULT_CMAKE_TARGET_ID);
}
QString CMakeTargetFactory::displayNameForId(const Core::Id id) const
{
    return ::displayNameForId(id);
}

bool CMakeTargetFactory::canCreate(ProjectExplorer::Project *parent, const Core::Id id) const
{
    if (!qobject_cast<CMakeProject *>(parent))
        return false;
    return id == Core::Id(DEFAULT_CMAKE_TARGET_ID);
}

CMakeTarget *CMakeTargetFactory::create(ProjectExplorer::Project *parent, const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    CMakeProject *cmakeparent(static_cast<CMakeProject *>(parent));
    CMakeTarget *t(new CMakeTarget(cmakeparent));

    // Add default build configuration:
    CMakeBuildConfiguration *bc(new CMakeBuildConfiguration(t));
    bc->setDefaultDisplayName("all");

    ProjectExplorer::BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    ProjectExplorer::BuildStepList *cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);

    // Now create a standard build configuration
    buildSteps->insertStep(0, new MakeStep(buildSteps));

    MakeStep *cleanMakeStep = new MakeStep(cleanSteps);
    cleanSteps->insertStep(0, cleanMakeStep);
    cleanMakeStep->setAdditionalArguments("clean");
    cleanMakeStep->setClean(true);

    t->addBuildConfiguration(bc);

    t->addDeployConfiguration(t->createDeployConfiguration(ProjectExplorer::Constants::DEFAULT_DEPLOYCONFIGURATION_ID));

    t->updateRunConfigurations();

    return t;
}

bool CMakeTargetFactory::canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

CMakeTarget *CMakeTargetFactory::restore(ProjectExplorer::Project *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    CMakeProject *cmakeparent(static_cast<CMakeProject *>(parent));
    CMakeTarget *t(new CMakeTarget(cmakeparent));
    if (t->fromMap(map))
        return t;
    delete t;
    return 0;
}
