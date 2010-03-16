/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "cmaketarget.h"

#include "cmakeopenprojectwizard.h"
#include "cmakeproject.h"
#include "cmakerunconfiguration.h"
#include "cmakebuildconfiguration.h"

#include <QtGui/QApplication>
#include <QtGui/QStyle>

using namespace CMakeProjectManager;
using namespace CMakeProjectManager::Internal;

namespace {

QString displayNameForId(const QString &id) {
    if (id == QLatin1String(DEFAULT_CMAKE_TARGET_ID))
        return QApplication::translate("CMakeProjectManager::Internal::CMakeTarget", "Desktop", "CMake Default target display name");
    return QString();
}

} // namespace

// -------------------------------------------------------------------------
// CMakeTarget
// -------------------------------------------------------------------------

CMakeTarget::CMakeTarget(CMakeProject *parent) :
    ProjectExplorer::Target(parent, QLatin1String(DEFAULT_CMAKE_TARGET_ID)),
    m_buildConfigurationFactory(new CMakeBuildConfigurationFactory(this))
{
    setDisplayName(displayNameForId(id()));
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

CMakeBuildConfiguration *CMakeTarget::activeBuildConfiguration() const
{
    return static_cast<CMakeBuildConfiguration *>(Target::activeBuildConfiguration());
}

CMakeBuildConfigurationFactory *CMakeTarget::buildConfigurationFactory() const
{
        return m_buildConfigurationFactory;
}

bool CMakeTarget::fromMap(const QVariantMap &map)
{
    if (!Target::fromMap(map))
        return false;

    setDisplayName(displayNameForId(id()));
    return true;
}

void CMakeTarget::updateRunConfigurations()
{
    // *Update* runconfigurations:
    QMultiMap<QString, CMakeRunConfiguration*> existingRunConfigurations;
    foreach(ProjectExplorer::RunConfiguration* cmakeRunConfiguration, runConfigurations()) {
        if (CMakeRunConfiguration* rc = qobject_cast<CMakeRunConfiguration *>(cmakeRunConfiguration))
            existingRunConfigurations.insert(rc->title(), rc);
    }

    foreach(const CMakeBuildTarget &ct, cmakeProject()->buildTargets()) {
        if (ct.executable.isEmpty())
            continue;
        if (ct.title.endsWith("/fast"))
            continue;
        QList<CMakeRunConfiguration *> list = existingRunConfigurations.values(ct.title);
        if (!list.isEmpty()) {
            // Already exists, so override the settings...
            foreach (CMakeRunConfiguration *rc, list) {
                rc->setExecutable(ct.executable);
                rc->setWorkingDirectory(ct.workingDirectory);
            }
            existingRunConfigurations.remove(ct.title);
        } else {
            // Does not exist yet
            addRunConfiguration(new CMakeRunConfiguration(this, ct.executable, ct.workingDirectory, ct.title));
        }
    }
    QMultiMap<QString, CMakeRunConfiguration *>::const_iterator it =
            existingRunConfigurations.constBegin();
    for( ; it != existingRunConfigurations.constEnd(); ++it) {
        CMakeRunConfiguration *rc = it.value();
        removeRunConfiguration(rc);
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

QStringList CMakeTargetFactory::availableCreationIds(ProjectExplorer::Project *parent) const
{
    if (!qobject_cast<CMakeProject *>(parent))
        return QStringList();
    return QStringList() << QLatin1String(DEFAULT_CMAKE_TARGET_ID);
}
QString CMakeTargetFactory::displayNameForId(const QString &id) const
{
    return ::displayNameForId(id);
}

bool CMakeTargetFactory::canCreate(ProjectExplorer::Project *parent, const QString &id) const
{
    if (!qobject_cast<CMakeProject *>(parent))
        return false;
    return id == QLatin1String(DEFAULT_CMAKE_TARGET_ID);
}

CMakeTarget *CMakeTargetFactory::create(ProjectExplorer::Project *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;
    CMakeProject *cmakeparent(static_cast<CMakeProject *>(parent));
    CMakeTarget *t(new CMakeTarget(cmakeparent));

    // Add default build configuration:
    CMakeBuildConfiguration *bc(new CMakeBuildConfiguration(t));
    bc->setDisplayName("all");

    // Now create a standard build configuration
    bc->insertStep(ProjectExplorer::Build, 0, new MakeStep(bc));

    MakeStep *cleanMakeStep = new MakeStep(bc);
    bc->insertStep(ProjectExplorer::Clean, 0, cleanMakeStep);
    cleanMakeStep->setAdditionalArguments(QStringList() << "clean");
    cleanMakeStep->setClean(true);

    t->addBuildConfiguration(bc);

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
