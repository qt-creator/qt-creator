/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "cmakebuildconfiguration.h"

#include "cmakeopenprojectwizard.h"
#include "cmakeproject.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/profileinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtparser.h>
#include <qtsupport/qtprofileinformation.h>
#include <utils/qtcassert.h>

#include <QInputDialog>

using namespace CMakeProjectManager;
using namespace Internal;

namespace {
const char CMAKE_BC_ID[] = "CMakeProjectManager.CMakeBuildConfiguration";
const char BUILD_DIRECTORY_KEY[] = "CMakeProjectManager.CMakeBuildConfiguration.BuildDirectory";
} // namespace

CMakeBuildConfiguration::CMakeBuildConfiguration(ProjectExplorer::Target *parent) :
    BuildConfiguration(parent, Core::Id(CMAKE_BC_ID))
{
    m_buildDirectory = static_cast<CMakeProject *>(parent->project())->defaultBuildDirectory();
}

CMakeBuildConfiguration::CMakeBuildConfiguration(ProjectExplorer::Target *parent,
                                                 CMakeBuildConfiguration *source) :
    BuildConfiguration(parent, source),
    m_buildDirectory(source->m_buildDirectory),
    m_msvcVersion(source->m_msvcVersion)
{
    Q_ASSERT(parent);
    cloneSteps(source);
}

QVariantMap CMakeBuildConfiguration::toMap() const
{
    QVariantMap map(ProjectExplorer::BuildConfiguration::toMap());
    map.insert(QLatin1String(BUILD_DIRECTORY_KEY), m_buildDirectory);
    return map;
}

bool CMakeBuildConfiguration::fromMap(const QVariantMap &map)
{
    if (!BuildConfiguration::fromMap(map))
        return false;

    m_buildDirectory = map.value(QLatin1String(BUILD_DIRECTORY_KEY)).toString();

    return true;
}

CMakeBuildConfiguration::~CMakeBuildConfiguration()
{ }

ProjectExplorer::BuildConfigWidget *CMakeBuildConfiguration::createConfigWidget()
{
    return new CMakeBuildSettingsWidget;
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

ProjectExplorer::IOutputParser *CMakeBuildConfiguration::createOutputParser() const
{
    ProjectExplorer::IOutputParser *parserchain = new ProjectExplorer::GnuMakeParser;

    int versionId = QtSupport::QtProfileInformation::qtVersionId(target()->profile());
    if (versionId >= 0)
        parserchain->appendOutputParser(new QtSupport::QtParser);

    ProjectExplorer::ToolChain *tc = ProjectExplorer::ToolChainProfileInformation::toolChain(target()->profile());
    if (tc)
        parserchain->appendOutputParser(tc->outputParser());
    return parserchain;
}

Utils::Environment CMakeBuildConfiguration::baseEnvironment() const
{
    Utils::Environment env = BuildConfiguration::baseEnvironment();
    target()->profile()->addToEnvironment(env);
    return env;
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
    return QList<Core::Id>() << Core::Id(CMAKE_BC_ID);
}

QString CMakeBuildConfigurationFactory::displayNameForId(const Core::Id id) const
{
    if (id == CMAKE_BC_ID)
        return tr("Build");
    return QString();
}

bool CMakeBuildConfigurationFactory::canCreate(const ProjectExplorer::Target *parent, const Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    if (id == CMAKE_BC_ID)
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
    if (buildConfigurationName.isEmpty())
        buildConfigurationName = QInputDialog::getText(0,
                                                       tr("New Configuration"),
                                                       tr("New configuration name:"),
                                                       QLineEdit::Normal,
                                                       QString(), &ok);
    buildConfigurationName = buildConfigurationName.trimmed();
    if (!ok || buildConfigurationName.isEmpty())
        return 0;

    CMakeBuildConfiguration *bc = new CMakeBuildConfiguration(parent);
    bc->setDisplayName(buildConfigurationName);

    ProjectExplorer::BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    ProjectExplorer::BuildStepList *cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);

    MakeStep *makeStep = new MakeStep(buildSteps);
    buildSteps->insertStep(0, makeStep);

    MakeStep *cleanMakeStep = new MakeStep(cleanSteps);
    cleanSteps->insertStep(0, cleanMakeStep);
    cleanMakeStep->setAdditionalArguments("clean");
    cleanMakeStep->setClean(true);

    CMakeOpenProjectWizard copw(project->projectManager(),
                                project->projectDirectory(),
                                bc->buildDirectory(),
                                bc->environment());
    if (copw.exec() != QDialog::Accepted) {
        delete bc;
        return 0;
    }

    bc->setBuildDirectory(copw.buildDirectory());

    // Default to all
    if (project->hasBuildTarget("all"))
        makeStep->setBuildTarget("all", true);

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
    if (!t->project()->supportsProfile(t->profile()))
        return false;
    return qobject_cast<CMakeProject *>(t->project());
}

ProjectExplorer::BuildConfiguration::BuildType CMakeBuildConfiguration::buildType() const
{
    QString cmakeBuildType;
    QFile cmakeCache(buildDirectory() + "/CMakeCache.txt");
    if (cmakeCache.open(QIODevice::ReadOnly)) {
        while (!cmakeCache.atEnd()) {
            QString line = cmakeCache.readLine();
            if (line.startsWith("CMAKE_BUILD_TYPE")) {
                if (int pos = line.indexOf('=')) {
                    cmakeBuildType = line.mid(pos + 1).trimmed();
                }
                break;
            }
        }
        cmakeCache.close();
    }

    // Cover all common CMake build types
    if (cmakeBuildType.compare("Release", Qt::CaseInsensitive) == 0
        || cmakeBuildType.compare("MinSizeRel", Qt::CaseInsensitive) == 0)
    {
        return Release;
    } else if (cmakeBuildType.compare("Debug", Qt::CaseInsensitive) == 0
               || cmakeBuildType.compare("debugfull", Qt::CaseInsensitive) == 0
               || cmakeBuildType.compare("RelWithDebInfo", Qt::CaseInsensitive) == 0)
    {
        return Debug;
    }

    return Unknown;
}
