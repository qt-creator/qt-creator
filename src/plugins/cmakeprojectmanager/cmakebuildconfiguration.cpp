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

CMakeBuildConfiguration::CMakeBuildConfiguration(CMakeProject *pro)
    : BuildConfiguration(pro),
    m_toolChain(0),
    m_clearSystemEnvironment(false)
{

}

CMakeBuildConfiguration::CMakeBuildConfiguration(CMakeProject *pro, const QMap<QString, QVariant> &map)
    : BuildConfiguration(pro, map),
    m_toolChain(0)
{

    QMap<QString, QVariant>::const_iterator it = map.constFind("clearSystemEnvironment");
    m_clearSystemEnvironment = (it != map.constEnd() && it.value().toBool());
    m_userEnvironmentChanges =
            ProjectExplorer::EnvironmentItem::fromStringList(
                    map.value("userEnvironmentChanges").toStringList());
    m_msvcVersion = map.value("msvcVersion").toString();
    m_buildDirectory = map.value("buildDirectory").toString();

}

CMakeBuildConfiguration::CMakeBuildConfiguration(CMakeBuildConfiguration *source)
    : BuildConfiguration(source),
    m_toolChain(0),
    m_clearSystemEnvironment(source->m_clearSystemEnvironment),
    m_userEnvironmentChanges(source->m_userEnvironmentChanges),
    m_buildDirectory(source->m_buildDirectory),
    m_msvcVersion(source->m_msvcVersion)
{

}

void CMakeBuildConfiguration::toMap(QMap<QString, QVariant> &map) const
{
    map.insert("userEnvironmentChanges",
               ProjectExplorer::EnvironmentItem::toStringList(m_userEnvironmentChanges));
    map.insert("msvcVersion", m_msvcVersion);
    map.insert("buildDirectory", m_buildDirectory);
    BuildConfiguration::toMap(map);
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

CMakeBuildConfigurationFactory::CMakeBuildConfigurationFactory(CMakeProject *project)
    : IBuildConfigurationFactory(project),
    m_project(project)
{
}

CMakeBuildConfigurationFactory::~CMakeBuildConfigurationFactory()
{
}

QStringList CMakeBuildConfigurationFactory::availableCreationIds() const
{
    return QStringList() << "Create";
}

QString CMakeBuildConfigurationFactory::displayNameForId(const QString &id) const
{
    QTC_ASSERT(id == "Create", return QString());
    return tr("Create");
}

ProjectExplorer::BuildConfiguration *CMakeBuildConfigurationFactory::create(const QString &id) const
{
    QTC_ASSERT(id == "Create", return 0);

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
    CMakeBuildConfiguration *bc = new CMakeBuildConfiguration(m_project);
    bc->setDisplayName(buildConfigurationName);

    MakeStep *makeStep = new MakeStep(bc);
    bc->insertBuildStep(0, makeStep);

    MakeStep *cleanMakeStep = new MakeStep(bc);
    bc->insertCleanStep(0, cleanMakeStep);
    cleanMakeStep->setClean(true);

    CMakeOpenProjectWizard copw(m_project->projectManager(),
                                m_project->sourceDirectory(),
                                bc->buildDirectory(),
                                bc->environment());
    if (copw.exec() != QDialog::Accepted) {
        delete bc;
        return false;
    }
    m_project->addBuildConfiguration(bc); // this also makes the name unique

    bc->setBuildDirectory(copw.buildDirectory());
    bc->setMsvcVersion(copw.msvcVersion());
    m_project->parseCMakeLists();

    // Default to all
    if (m_project->targets().contains("all"))
        makeStep->setBuildTarget("all", true);
    return bc;
}

ProjectExplorer::BuildConfiguration *CMakeBuildConfigurationFactory::clone(ProjectExplorer::BuildConfiguration *source) const
{
    CMakeBuildConfiguration *old = static_cast<CMakeBuildConfiguration *>(source);
    CMakeBuildConfiguration *bc = new CMakeBuildConfiguration(old);
    return bc;
}

ProjectExplorer::BuildConfiguration *CMakeBuildConfigurationFactory::restore(const QVariantMap &map) const
{
    CMakeBuildConfiguration *bc = new CMakeBuildConfiguration(m_project, map);
    return bc;
}
