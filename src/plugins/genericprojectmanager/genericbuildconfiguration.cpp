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

#include "genericbuildconfiguration.h"

#include "genericmakestep.h"
#include "genericproject.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QFormLayout>
#include <QInputDialog>

using namespace ProjectExplorer;

namespace GenericProjectManager {
namespace Internal {

const char GENERIC_BC_ID[] = "GenericProjectManager.GenericBuildConfiguration";
const char BUILD_DIRECTORY_KEY[] = "GenericProjectManager.GenericBuildConfiguration.BuildDirectory";

GenericBuildConfiguration::GenericBuildConfiguration(Target *parent)
    : BuildConfiguration(parent, Core::Id(GENERIC_BC_ID))
{
}

GenericBuildConfiguration::GenericBuildConfiguration(Target *parent, const Core::Id id)
    : BuildConfiguration(parent, id)
{
}

GenericBuildConfiguration::GenericBuildConfiguration(Target *parent, GenericBuildConfiguration *source) :
    BuildConfiguration(parent, source),
    m_buildDirectory(source->m_buildDirectory)
{
    cloneSteps(source);
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

NamedWidget *GenericBuildConfiguration::createConfigWidget()
{
    return new GenericBuildSettingsWidget(this);
}

/*!
  \class GenericBuildConfigurationFactory
*/

GenericBuildConfigurationFactory::GenericBuildConfigurationFactory(QObject *parent) :
    IBuildConfigurationFactory(parent)
{
}

GenericBuildConfigurationFactory::~GenericBuildConfigurationFactory()
{
}

QList<Core::Id> GenericBuildConfigurationFactory::availableCreationIds(const Target *parent) const
{
    if (!canHandle(parent))
        return QList<Core::Id>();
    return QList<Core::Id>() << Core::Id(GENERIC_BC_ID);
}

QString GenericBuildConfigurationFactory::displayNameForId(const Core::Id id) const
{
    if (id == GENERIC_BC_ID)
        return tr("Build");
    return QString();
}

bool GenericBuildConfigurationFactory::canCreate(const Target *parent, const Core::Id id) const
{
    if (!canHandle(parent))
        return false;
    if (id == GENERIC_BC_ID)
        return true;
    return false;
}

BuildConfiguration *GenericBuildConfigurationFactory::create(Target *parent, const Core::Id id, const QString &name)
{
    if (!canCreate(parent, id))
        return 0;

    //TODO asking for name is duplicated everywhere, but maybe more
    // wizards will show up, that incorporate choosing the nam
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

    GenericBuildConfiguration *bc = new GenericBuildConfiguration(parent);
    bc->setDisplayName(buildConfigurationName);

    BuildStepList *buildSteps = bc->stepList(Constants::BUILDSTEPS_BUILD);
    BuildStepList *cleanSteps = bc->stepList(Constants::BUILDSTEPS_CLEAN);

    Q_ASSERT(buildSteps);
    GenericMakeStep *makeStep = new GenericMakeStep(buildSteps);
    buildSteps->insertStep(0, makeStep);
    makeStep->setBuildTarget(QLatin1String("all"), /* on = */ true);

    Q_ASSERT(cleanSteps);
    GenericMakeStep *cleanMakeStep = new GenericMakeStep(cleanSteps);
    cleanSteps->insertStep(0, cleanMakeStep);
    cleanMakeStep->setBuildTarget(QLatin1String("clean"), /* on = */ true);
    cleanMakeStep->setClean(true);

    return bc;
}

bool GenericBuildConfigurationFactory::canClone(const Target *parent, BuildConfiguration *source) const
{
    return canCreate(parent, source->id());
}

BuildConfiguration *GenericBuildConfigurationFactory::clone(Target *parent, BuildConfiguration *source)
{
    if (!canClone(parent, source))
        return 0;
    return new GenericBuildConfiguration(parent, qobject_cast<GenericBuildConfiguration *>(source));
}

bool GenericBuildConfigurationFactory::canRestore(const Target *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
}

BuildConfiguration *GenericBuildConfigurationFactory::restore(Target *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    GenericBuildConfiguration *bc(new GenericBuildConfiguration(parent));
    if (bc->fromMap(map))
        return bc;
    delete bc;
    return 0;
}

bool GenericBuildConfigurationFactory::canHandle(const Target *t) const
{
    if (!t->project()->supportsKit(t->kit()))
        return false;
    return qobject_cast<GenericProject *>(t->project());
}

BuildConfiguration::BuildType GenericBuildConfiguration::buildType() const
{
    return Unknown;
}

////////////////////////////////////////////////////////////////////////////////////
// GenericBuildSettingsWidget
////////////////////////////////////////////////////////////////////////////////////

GenericBuildSettingsWidget::GenericBuildSettingsWidget(GenericBuildConfiguration *bc)
    : m_buildConfiguration(0)
{
    QFormLayout *fl = new QFormLayout(this);
    fl->setContentsMargins(0, -1, 0, -1);
    fl->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    // build directory
    m_pathChooser = new Utils::PathChooser(this);
    m_pathChooser->setEnabled(true);
    fl->addRow(tr("Build directory:"), m_pathChooser);
    connect(m_pathChooser, SIGNAL(changed(QString)), this, SLOT(buildDirectoryChanged()));

    m_buildConfiguration = bc;
    m_pathChooser->setBaseDirectory(bc->target()->project()->projectDirectory());
    m_pathChooser->setPath(m_buildConfiguration->rawBuildDirectory());
    setDisplayName(tr("Generic Manager"));
}

void GenericBuildSettingsWidget::buildDirectoryChanged()
{
    m_buildConfiguration->setBuildDirectory(m_pathChooser->rawPath());
}

} // namespace Internal
} // namespace GenericProjectManager
