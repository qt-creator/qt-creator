/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "cmakebuildconfiguration.h"

#include "cmakeopenprojectwizard.h"
#include "cmakeproject.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <utils/qtcassert.h>

#include <QtGui/QInputDialog>

using namespace CMakeProjectManager;
using namespace Internal;

namespace {
const char * const CMAKE_BC_ID("CMakeProjectManager.CMakeBuildConfiguration");

const char * const USER_ENVIRONMENT_CHANGES_KEY("CMakeProjectManager.CMakeBuildConfiguration.UserEnvironmentChanges");
const char * const MSVC_VERSION_KEY("CMakeProjectManager.CMakeBuildConfiguration.MsvcVersion");
const char * const BUILD_DIRECTORY_KEY("CMakeProjectManager.CMakeBuildConfiguration.BuildDirectory");

}

CMakeBuildConfiguration::CMakeBuildConfiguration(CMakeProject *project) :
    BuildConfiguration(project, QLatin1String(CMAKE_BC_ID)),
    m_toolChain(0),
    m_clearSystemEnvironment(false)
{
}

CMakeBuildConfiguration::CMakeBuildConfiguration(CMakeProject *project, const QString &id) :
    BuildConfiguration(project, id),
    m_toolChain(0),
    m_clearSystemEnvironment(false)
{
}

CMakeBuildConfiguration::CMakeBuildConfiguration(CMakeProject *pro, CMakeBuildConfiguration *source) :
    BuildConfiguration(pro, source),
    m_toolChain(0),
    m_clearSystemEnvironment(source->m_clearSystemEnvironment),
    m_userEnvironmentChanges(source->m_userEnvironmentChanges),
    m_buildDirectory(source->m_buildDirectory),
    m_msvcVersion(source->m_msvcVersion)
{
}

QVariantMap CMakeBuildConfiguration::toMap() const
{
    QVariantMap map(ProjectExplorer::BuildConfiguration::toMap());
    map.insert(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY),
               ProjectExplorer::EnvironmentItem::toStringList(m_userEnvironmentChanges));
    map.insert(QLatin1String(MSVC_VERSION_KEY), m_msvcVersion);
    map.insert(QLatin1String(BUILD_DIRECTORY_KEY), m_buildDirectory);
    return map;
}

bool CMakeBuildConfiguration::fromMap(const QVariantMap &map)
{
    m_userEnvironmentChanges = ProjectExplorer::EnvironmentItem::fromStringList(map.value(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY)).toStringList());
    m_msvcVersion = map.value(QLatin1String(MSVC_VERSION_KEY)).toString();
    m_buildDirectory = map.value(QLatin1String(BUILD_DIRECTORY_KEY)).toString();

    return BuildConfiguration::fromMap(map);
}

CMakeBuildConfiguration::~CMakeBuildConfiguration()
{
    delete m_toolChain;
}

CMakeProject *CMakeBuildConfiguration::cmakeProject() const
{
    return static_cast<CMakeProject *>(project());
}

ProjectExplorer::Environment CMakeBuildConfiguration::baseEnvironment() const
{
    ProjectExplorer::Environment env = useSystemEnvironment() ?
                                       ProjectExplorer::Environment(QProcess::systemEnvironment()) :
                                       ProjectExplorer::Environment();
    return env;
}

QString CMakeBuildConfiguration::baseEnvironmentText() const
{
    if (useSystemEnvironment())
        return tr("System Environment");
    else
        return tr("Clear Environment");
}

ProjectExplorer::Environment CMakeBuildConfiguration::environment() const
{
    ProjectExplorer::Environment env = baseEnvironment();
    env.modify(userEnvironmentChanges());
    return env;
}

void CMakeBuildConfiguration::setUseSystemEnvironment(bool b)
{
    if (b == m_clearSystemEnvironment)
        return;
    m_clearSystemEnvironment = !b;
    emit environmentChanged();
}

bool CMakeBuildConfiguration::useSystemEnvironment() const
{
    return !m_clearSystemEnvironment;
}

QList<ProjectExplorer::EnvironmentItem> CMakeBuildConfiguration::userEnvironmentChanges() const
{
    return m_userEnvironmentChanges;
}

void CMakeBuildConfiguration::setUserEnvironmentChanges(const QList<ProjectExplorer::EnvironmentItem> &diff)
{
    if (m_userEnvironmentChanges == diff)
        return;
    m_userEnvironmentChanges = diff;
    emit environmentChanged();
}

QString CMakeBuildConfiguration::buildDirectory() const
{
    QString buildDirectory = m_buildDirectory;
    if (buildDirectory.isEmpty())
        buildDirectory = cmakeProject()->sourceDirectory() + "/qtcreator-build";
    return buildDirectory;
}

ProjectExplorer::ToolChain::ToolChainType CMakeBuildConfiguration::toolChainType() const
{
    if (m_toolChain)
        return m_toolChain->type();
    return ProjectExplorer::ToolChain::UNKNOWN;
}

ProjectExplorer::ToolChain *CMakeBuildConfiguration::toolChain() const
{
    updateToolChain();
    return m_toolChain;
}

void CMakeBuildConfiguration::updateToolChain() const
{
    ProjectExplorer::ToolChain *newToolChain = 0;
    if (msvcVersion().isEmpty()) {
#ifdef Q_OS_WIN
        newToolChain = ProjectExplorer::ToolChain::createMinGWToolChain("gcc", QString());
#else
        newToolChain = ProjectExplorer::ToolChain::createGccToolChain("gcc");
#endif
    } else { // msvc
        newToolChain = ProjectExplorer::ToolChain::createMSVCToolChain(m_msvcVersion, false);
    }

    if (ProjectExplorer::ToolChain::equals(newToolChain, m_toolChain)) {
        delete newToolChain;
        newToolChain = 0;
    } else {
        delete m_toolChain;
        m_toolChain = newToolChain;
    }
}

void CMakeBuildConfiguration::setBuildDirectory(const QString &buildDirectory)
{
    if (m_buildDirectory == buildDirectory)
        return;
    m_buildDirectory = buildDirectory;
    emit buildDirectoryChanged();
}

QString CMakeBuildConfiguration::msvcVersion() const
{
    return m_msvcVersion;
}

void CMakeBuildConfiguration::setMsvcVersion(const QString &msvcVersion)
{
    if (m_msvcVersion == msvcVersion)
        return;
    m_msvcVersion = msvcVersion;
    updateToolChain();

    emit msvcVersionChanged();
}

/*!
  \class CMakeBuildConfigurationFactory
*/

CMakeBuildConfigurationFactory::CMakeBuildConfigurationFactory(QObject *parent) :
    ProjectExplorer::IBuildConfigurationFactory(parent)
{
}

CMakeBuildConfigurationFactory::~CMakeBuildConfigurationFactory()
{
}

QStringList CMakeBuildConfigurationFactory::availableCreationIds(ProjectExplorer::Project *parent) const
{
    if (!qobject_cast<CMakeProject *>(parent))
        return QStringList();
    return QStringList() << QLatin1String(CMAKE_BC_ID);
}

QString CMakeBuildConfigurationFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(CMAKE_BC_ID))
        return tr("Build");
    return QString();
}

bool CMakeBuildConfigurationFactory::canCreate(ProjectExplorer::Project *parent, const QString &id) const
{
    if (!qobject_cast<CMakeProject *>(parent))
        return false;
    if (id == QLatin1String(CMAKE_BC_ID))
        return true;
    return false;
}

ProjectExplorer::BuildConfiguration *CMakeBuildConfigurationFactory::create(ProjectExplorer::Project *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;

    CMakeProject *cmProject = static_cast<CMakeProject *>(parent);
    Q_ASSERT(cmProject);

    //TODO configuration name should be part of the cmakeopenprojectwizard
    bool ok;
    QString buildConfigurationName = QInputDialog::getText(0,
                          tr("New configuration"),
                          tr("New Configuration Name:"),
                          QLineEdit::Normal,
                          QString(),
                          &ok);
    if (!ok || buildConfigurationName.isEmpty())
        return false;
    CMakeBuildConfiguration *bc = new CMakeBuildConfiguration(cmProject);
    bc->setDisplayName(buildConfigurationName);

    MakeStep *makeStep = new MakeStep(bc);
    bc->insertBuildStep(0, makeStep);

    MakeStep *cleanMakeStep = new MakeStep(bc);
    bc->insertCleanStep(0, cleanMakeStep);
    cleanMakeStep->setAdditionalArguments(QStringList() << "clean");
    cleanMakeStep->setClean(true);

    CMakeOpenProjectWizard copw(cmProject->projectManager(),
                                cmProject->sourceDirectory(),
                                bc->buildDirectory(),
                                bc->environment());
    if (copw.exec() != QDialog::Accepted) {
        delete bc;
        return false;
    }
    cmProject->addBuildConfiguration(bc); // this also makes the name unique

    bc->setBuildDirectory(copw.buildDirectory());
    bc->setMsvcVersion(copw.msvcVersion());
    cmProject->parseCMakeLists();

    // Default to all
    if (cmProject->hasTarget("all"))
        makeStep->setBuildTarget("all", true);

    return bc;
}

bool CMakeBuildConfigurationFactory::canClone(ProjectExplorer::Project *parent, ProjectExplorer::BuildConfiguration *source) const
{
    return canCreate(parent, source->id());
}

ProjectExplorer::BuildConfiguration *CMakeBuildConfigurationFactory::clone(ProjectExplorer::Project *parent, ProjectExplorer::BuildConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    CMakeBuildConfiguration *old = static_cast<CMakeBuildConfiguration *>(source);
    CMakeProject *cmProject(static_cast<CMakeProject *>(parent));
    return new CMakeBuildConfiguration(cmProject, old);
}

bool CMakeBuildConfigurationFactory::canRestore(ProjectExplorer::Project *parent, const QVariantMap &map) const
{
    QString id(ProjectExplorer::idFromMap(map));
    return canCreate(parent, id);
}

ProjectExplorer::BuildConfiguration *CMakeBuildConfigurationFactory::restore(ProjectExplorer::Project *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    CMakeProject *cmProject(static_cast<CMakeProject *>(parent));
    CMakeBuildConfiguration *bc = new CMakeBuildConfiguration(cmProject);
    if (bc->fromMap(map))
        return bc;
    delete bc;
    return 0;
}
