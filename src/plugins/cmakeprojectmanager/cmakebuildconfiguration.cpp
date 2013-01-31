/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cmakebuildconfiguration.h"

#include "cmakeopenprojectwizard.h"
#include "cmakeproject.h"
#include "cmakeprojectconstants.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtparser.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/qtcassert.h>

#include <QInputDialog>

using namespace CMakeProjectManager;
using namespace Internal;

namespace {
const char BUILD_DIRECTORY_KEY[] = "CMakeProjectManager.CMakeBuildConfiguration.BuildDirectory";
const char USE_NINJA_KEY[] = "CMakeProjectManager.CMakeBuildConfiguration.UseNinja";
} // namespace

CMakeBuildConfiguration::CMakeBuildConfiguration(ProjectExplorer::Target *parent) :
    BuildConfiguration(parent, Core::Id(Constants::CMAKE_BC_ID)), m_useNinja(false)
{
    CMakeProject *project = static_cast<CMakeProject *>(parent->project());
    m_buildDirectory = project->shadowBuildDirectory(project->document()->fileName(),
                                                     parent->kit(),
                                                     displayName());
}

CMakeBuildConfiguration::CMakeBuildConfiguration(ProjectExplorer::Target *parent,
                                                 CMakeBuildConfiguration *source) :
    BuildConfiguration(parent, source),
    m_buildDirectory(source->m_buildDirectory),
    m_msvcVersion(source->m_msvcVersion),
    m_useNinja(false)
{
    Q_ASSERT(parent);
    cloneSteps(source);
}

QVariantMap CMakeBuildConfiguration::toMap() const
{
    QVariantMap map(ProjectExplorer::BuildConfiguration::toMap());
    map.insert(QLatin1String(BUILD_DIRECTORY_KEY), m_buildDirectory);
    map.insert(QLatin1String(USE_NINJA_KEY), m_useNinja);
    return map;
}

bool CMakeBuildConfiguration::fromMap(const QVariantMap &map)
{
    if (!BuildConfiguration::fromMap(map))
        return false;

    m_buildDirectory = map.value(QLatin1String(BUILD_DIRECTORY_KEY)).toString();
    m_useNinja = map.value(QLatin1String(USE_NINJA_KEY), false).toBool();

    return true;
}

bool CMakeBuildConfiguration::useNinja() const
{
    return m_useNinja;
}

void CMakeBuildConfiguration::setUseNinja(bool useNninja)
{
    if (m_useNinja != useNninja) {
        m_useNinja = useNninja;
        emit useNinjaChanged(m_useNinja);
    }
}

CMakeBuildConfiguration::~CMakeBuildConfiguration()
{ }

ProjectExplorer::NamedWidget *CMakeBuildConfiguration::createConfigWidget()
{
    return new CMakeBuildSettingsWidget(this);
}

QString CMakeBuildConfiguration::buildDirectory() const
{
    return m_buildDirectory;
}

void CMakeBuildConfiguration::setBuildDirectory(const QString &buildDirectory)
{
    if (m_buildDirectory == buildDirectory)
        return;
    m_buildDirectory = buildDirectory;
    emit buildDirectoryChanged();
    emit environmentChanged();
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

QList<Core::Id> CMakeBuildConfigurationFactory::availableCreationIds(const ProjectExplorer::Target *parent) const
{
    if (!canHandle(parent))
        return QList<Core::Id>();
    return QList<Core::Id>() << Core::Id(Constants::CMAKE_BC_ID);
}

QString CMakeBuildConfigurationFactory::displayNameForId(const Core::Id id) const
{
    if (id == Constants::CMAKE_BC_ID)
        return tr("Build");
    return QString();
}

bool CMakeBuildConfigurationFactory::canCreate(const ProjectExplorer::Target *parent, const Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    if (id == Constants::CMAKE_BC_ID)
        return true;
    return false;
}

CMakeBuildConfiguration *CMakeBuildConfigurationFactory::create(ProjectExplorer::Target *parent, const Core::Id id, const QString &name)
{
    if (!canCreate(parent, id))
        return 0;

    CMakeProject *project = static_cast<CMakeProject *>(parent->project());

    bool ok = true;
    QString buildConfigurationName = name;
    if (buildConfigurationName.isNull())
        buildConfigurationName = QInputDialog::getText(0,
                                                       tr("New Configuration"),
                                                       tr("New configuration name:"),
                                                       QLineEdit::Normal,
                                                       QString(), &ok);
    buildConfigurationName = buildConfigurationName.trimmed();
    if (!ok || buildConfigurationName.isEmpty())
        return 0;

    CMakeOpenProjectWizard::BuildInfo info;
    info.sourceDirectory = project->projectDirectory();
    info.environment = Utils::Environment::systemEnvironment();
    parent->kit()->addToEnvironment(info.environment);
    info.buildDirectory = project->shadowBuildDirectory(project->document()->fileName(),
                                                        parent->kit(),
                                                        buildConfigurationName);
    info.kit = parent->kit();
    info.useNinja = false; // This is ignored anyway

    CMakeOpenProjectWizard copw(project->projectManager(), CMakeOpenProjectWizard::ChangeDirectory, info);
    if (copw.exec() != QDialog::Accepted)
        return 0;

    CMakeBuildConfiguration *bc = new CMakeBuildConfiguration(parent);
    bc->setDisplayName(buildConfigurationName);

    ProjectExplorer::BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    ProjectExplorer::BuildStepList *cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);

    MakeStep *makeStep = new MakeStep(buildSteps);
    buildSteps->insertStep(0, makeStep);

    MakeStep *cleanMakeStep = new MakeStep(cleanSteps);
    cleanSteps->insertStep(0, cleanMakeStep);
    cleanMakeStep->setAdditionalArguments(QLatin1String("clean"));
    cleanMakeStep->setClean(true);

    bc->setBuildDirectory(copw.buildDirectory());
    bc->setUseNinja(copw.useNinja());

    // Default to all
    if (project->hasBuildTarget(QLatin1String("all")))
        makeStep->setBuildTarget(QLatin1String("all"), true);

    return bc;
}

bool CMakeBuildConfigurationFactory::canClone(const ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const
{
    return canCreate(parent, source->id());
}

CMakeBuildConfiguration *CMakeBuildConfigurationFactory::clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    CMakeBuildConfiguration *old = static_cast<CMakeBuildConfiguration *>(source);
    return new CMakeBuildConfiguration(parent, old);
}

bool CMakeBuildConfigurationFactory::canRestore(const ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

CMakeBuildConfiguration *CMakeBuildConfigurationFactory::restore(ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    CMakeBuildConfiguration *bc = new CMakeBuildConfiguration(parent);
    if (bc->fromMap(map))
        return bc;
    delete bc;
    return 0;
}

bool CMakeBuildConfigurationFactory::canHandle(const ProjectExplorer::Target *t) const
{
    if (!t->project()->supportsKit(t->kit()))
        return false;
    return qobject_cast<CMakeProject *>(t->project());
}

ProjectExplorer::BuildConfiguration::BuildType CMakeBuildConfiguration::buildType() const
{
    QString cmakeBuildType;
    QFile cmakeCache(buildDirectory() + QLatin1String("/CMakeCache.txt"));
    if (cmakeCache.open(QIODevice::ReadOnly)) {
        while (!cmakeCache.atEnd()) {
            QByteArray line = cmakeCache.readLine();
            if (line.startsWith("CMAKE_BUILD_TYPE")) {
                if (int pos = line.indexOf('='))
                    cmakeBuildType = QString::fromLocal8Bit(line.mid(pos + 1).trimmed());
                break;
            }
        }
        cmakeCache.close();
    }

    // Cover all common CMake build types
    if (cmakeBuildType.compare(QLatin1String("Release"), Qt::CaseInsensitive) == 0
        || cmakeBuildType.compare(QLatin1String("MinSizeRel"), Qt::CaseInsensitive) == 0)
    {
        return Release;
    } else if (cmakeBuildType.compare(QLatin1String("Debug"), Qt::CaseInsensitive) == 0
               || cmakeBuildType.compare(QLatin1String("DebugFull"), Qt::CaseInsensitive) == 0
               || cmakeBuildType.compare(QLatin1String("RelWithDebInfo"), Qt::CaseInsensitive) == 0)
    {
        return Debug;
    }

    return Unknown;
}

