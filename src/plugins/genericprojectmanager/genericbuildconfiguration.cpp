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

#include "genericbuildconfiguration.h"

#include "genericmakestep.h"
#include "genericproject.h"
#include "generictarget.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <utils/qtcassert.h>

#include <QInputDialog>

using namespace GenericProjectManager;
using namespace GenericProjectManager::Internal;
using ProjectExplorer::BuildConfiguration;

namespace {
const char * const GENERIC_BC_ID("GenericProjectManager.GenericBuildConfiguration");

const char * const BUILD_DIRECTORY_KEY("GenericProjectManager.GenericBuildConfiguration.BuildDirectory");
}

GenericBuildConfiguration::GenericBuildConfiguration(GenericTarget *parent)
    : BuildConfiguration(parent, Core::Id(GENERIC_BC_ID))
{
}

GenericBuildConfiguration::GenericBuildConfiguration(GenericTarget *parent, const Core::Id id)
    : BuildConfiguration(parent, id)
{
}

GenericBuildConfiguration::GenericBuildConfiguration(GenericTarget *parent, GenericBuildConfiguration *source) :
    BuildConfiguration(parent, source),
    m_buildDirectory(source->m_buildDirectory)
{
    cloneSteps(source);
}

GenericBuildConfiguration::~GenericBuildConfiguration()
{
}

QVariantMap GenericBuildConfiguration::toMap() const
{
    QVariantMap map(BuildConfiguration::toMap());
    map.insert(QLatin1String(BUILD_DIRECTORY_KEY), m_buildDirectory);
    return map;
}

bool GenericBuildConfiguration::fromMap(const QVariantMap &map)
{
    m_buildDirectory = map.value(QLatin1String(BUILD_DIRECTORY_KEY), target()->project()->projectDirectory()).toString();

    return BuildConfiguration::fromMap(map);
}

QString GenericBuildConfiguration::buildDirectory() const
{
    // Convert to absolute path when necessary
    const QDir projectDir(target()->project()->projectDirectory());
    return projectDir.absoluteFilePath(m_buildDirectory);
}

/**
 * Returns the build directory unmodified, instead of making it absolute like
 * buildDirectory() does.
 */
QString GenericBuildConfiguration::rawBuildDirectory() const
{
    return m_buildDirectory;
}

void GenericBuildConfiguration::setBuildDirectory(const QString &buildDirectory)
{
    if (m_buildDirectory == buildDirectory)
        return;
    m_buildDirectory = buildDirectory;
    emit buildDirectoryChanged();
}

GenericTarget *GenericBuildConfiguration::genericTarget() const
{
    return static_cast<GenericTarget *>(target());
}

ProjectExplorer::IOutputParser *GenericBuildConfiguration::createOutputParser() const
{
    ProjectExplorer::ToolChain *tc = genericTarget()->genericProject()->toolChain();
    if (tc)
        return tc->outputParser();
    return 0;
}


/*!
  \class GenericBuildConfigurationFactory
*/

GenericBuildConfigurationFactory::GenericBuildConfigurationFactory(QObject *parent) :
    ProjectExplorer::IBuildConfigurationFactory(parent)
{
}

GenericBuildConfigurationFactory::~GenericBuildConfigurationFactory()
{
}

QList<Core::Id> GenericBuildConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    if (!qobject_cast<GenericTarget *>(parent))
        return QList<Core::Id>();
    return QList<Core::Id>() << Core::Id(GENERIC_BC_ID);
}

QString GenericBuildConfigurationFactory::displayNameForId(const Core::Id id) const
{
    if (id == Core::Id(GENERIC_BC_ID))
        return tr("Build");
    return QString();
}

bool GenericBuildConfigurationFactory::canCreate(ProjectExplorer::Target *parent, const Core::Id id) const
{
    if (!qobject_cast<GenericTarget *>(parent))
        return false;
    if (id == Core::Id(GENERIC_BC_ID))
        return true;
    return false;
}

BuildConfiguration *GenericBuildConfigurationFactory::create(ProjectExplorer::Target *parent, const Core::Id id)
{
    if (!canCreate(parent, id))
        return 0;
    GenericTarget *target(static_cast<GenericTarget *>(parent));

    //TODO asking for name is duplicated everywhere, but maybe more
    // wizards will show up, that incorporate choosing the name
    bool ok;
    QString buildConfigurationName = QInputDialog::getText(0,
                          tr("New Configuration"),
                          tr("New configuration name:"),
                          QLineEdit::Normal,
                          QString(),
                          &ok);
    if (!ok || buildConfigurationName.isEmpty())
        return 0;
    GenericBuildConfiguration *bc = new GenericBuildConfiguration(target);
    bc->setDisplayName(buildConfigurationName);

    ProjectExplorer::BuildStepList *buildSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    ProjectExplorer::BuildStepList *cleanSteps = bc->stepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);

    Q_ASSERT(buildSteps);
    GenericMakeStep *makeStep = new GenericMakeStep(buildSteps);
    buildSteps->insertStep(0, makeStep);
    makeStep->setBuildTarget(QLatin1String("all"), /* on = */ true);

    Q_ASSERT(cleanSteps);
    GenericMakeStep *cleanMakeStep = new GenericMakeStep(cleanSteps);
    cleanSteps->insertStep(0, cleanMakeStep);
    cleanMakeStep->setBuildTarget(QLatin1String("clean"), /* on = */ true);
    cleanMakeStep->setClean(true);

    target->addBuildConfiguration(bc); // also makes the name unique...
    return bc;
}

bool GenericBuildConfigurationFactory::canClone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) const
{
    return canCreate(parent, source->id());
}

BuildConfiguration *GenericBuildConfigurationFactory::clone(ProjectExplorer::Target *parent, BuildConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    GenericTarget *target(static_cast<GenericTarget *>(parent));
    return new GenericBuildConfiguration(target, qobject_cast<GenericBuildConfiguration *>(source));
}

bool GenericBuildConfigurationFactory::canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

BuildConfiguration *GenericBuildConfigurationFactory::restore(ProjectExplorer::Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    GenericTarget *target(static_cast<GenericTarget *>(parent));
    GenericBuildConfiguration *bc(new GenericBuildConfiguration(target));
    if (bc->fromMap(map))
        return bc;
    delete bc;
    return 0;
}

BuildConfiguration::BuildType GenericBuildConfiguration::buildType() const
{
    return Unknown;
}

