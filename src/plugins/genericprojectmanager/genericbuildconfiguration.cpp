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

#include "genericbuildconfiguration.h"

#include "genericmakestep.h"
#include "genericproject.h"
#include "generictarget.h"

#include <utils/qtcassert.h>

#include <QtGui/QInputDialog>

using namespace GenericProjectManager;
using namespace GenericProjectManager::Internal;
using ProjectExplorer::BuildConfiguration;

namespace {
const char * const GENERIC_BC_ID("GenericProjectManager.GenericBuildConfiguration");

const char * const BUILD_DIRECTORY_KEY("GenericProjectManager.GenericBuildConfiguration.BuildDirectory");
}

GenericBuildConfiguration::GenericBuildConfiguration(GenericTarget *parent)
    : BuildConfiguration(parent, QLatin1String(GENERIC_BC_ID))
{
}

GenericBuildConfiguration::GenericBuildConfiguration(GenericTarget *parent, const QString &id)
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

ProjectExplorer::Environment GenericBuildConfiguration::environment() const
{
    return ProjectExplorer::Environment::systemEnvironment();
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

QStringList GenericBuildConfigurationFactory::availableCreationIds(ProjectExplorer::Target *parent) const
{
    if (!qobject_cast<GenericTarget *>(parent))
        return QStringList();
    return QStringList() << QLatin1String(GENERIC_BC_ID);
}

QString GenericBuildConfigurationFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(GENERIC_BC_ID))
        return tr("Build");
    return QString();
}

bool GenericBuildConfigurationFactory::canCreate(ProjectExplorer::Target *parent, const QString &id) const
{
    if (!qobject_cast<GenericTarget *>(parent))
        return false;
    if (id == QLatin1String(GENERIC_BC_ID))
        return true;
    return false;
}

BuildConfiguration *GenericBuildConfigurationFactory::create(ProjectExplorer::Target *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;
    GenericTarget *target(static_cast<GenericTarget *>(parent));

    //TODO asking for name is duplicated everywhere, but maybe more
    // wizards will show up, that incorporate choosing the name
    bool ok;
    QString buildConfigurationName = QInputDialog::getText(0,
                          tr("New configuration"),
                          tr("New Configuration Name:"),
                          QLineEdit::Normal,
                          QString(),
                          &ok);
    if (!ok || buildConfigurationName.isEmpty())
        return false;
    GenericBuildConfiguration *bc = new GenericBuildConfiguration(target);
    bc->setDisplayName(buildConfigurationName);

    GenericMakeStep *makeStep = new GenericMakeStep(bc);
    bc->insertStep(ProjectExplorer::Build, 0, makeStep);
    makeStep->setBuildTarget("all", /* on = */ true);

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
    QString id(ProjectExplorer::idFromMap(map));
    return canCreate(parent, id);
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
