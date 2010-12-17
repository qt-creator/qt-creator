/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "target.h"

#include "buildconfiguration.h"
#include "deployconfiguration.h"
#include "project.h"
#include "projectexplorerconstants.h"
#include "runconfiguration.h"

#include <limits>
#include <utils/qtcassert.h>

#include <QtGui/QIcon>

namespace {
const char * const ACTIVE_BC_KEY("ProjectExplorer.Target.ActiveBuildConfiguration");
const char * const BC_KEY_PREFIX("ProjectExplorer.Target.BuildConfiguration.");
const char * const BC_COUNT_KEY("ProjectExplorer.Target.BuildConfigurationCount");

const char * const ACTIVE_DC_KEY("ProjectExplorer.Target.ActiveDeployConfiguration");
const char * const DC_KEY_PREFIX("ProjectExplorer.Target.DeployConfiguration.");
const char * const DC_COUNT_KEY("ProjectExplorer.Target.DeployConfigurationCount");

const char * const ACTIVE_RC_KEY("ProjectExplorer.Target.ActiveRunConfiguration");
const char * const RC_KEY_PREFIX("ProjectExplorer.Target.RunConfiguration.");
const char * const RC_COUNT_KEY("ProjectExplorer.Target.RunConfigurationCount");

} // namespace

// -------------------------------------------------------------------------
// Target
// -------------------------------------------------------------------------

namespace ProjectExplorer {

class TargetPrivate {
public:
    TargetPrivate();
    bool m_isEnabled;
    QIcon m_icon;
    QIcon m_overlayIcon;
    QString m_toolTip;

    QList<BuildConfiguration *> m_buildConfigurations;
    BuildConfiguration *m_activeBuildConfiguration;
    QList<DeployConfiguration *> m_deployConfigurations;
    DeployConfiguration *m_activeDeployConfiguration;
    QList<RunConfiguration *> m_runConfigurations;
    RunConfiguration* m_activeRunConfiguration;
};

TargetPrivate::TargetPrivate() :
    m_isEnabled(true),
    m_activeBuildConfiguration(0),
    m_activeDeployConfiguration(0),
    m_activeRunConfiguration(0)
{
}


Target::Target(Project *project, const QString &id) :
    ProjectConfiguration(project, id), d(new TargetPrivate)
{
}

Target::~Target()
{
    qDeleteAll(d->m_buildConfigurations);
    qDeleteAll(d->m_runConfigurations);
}

void Target::changeEnvironment()
{
    ProjectExplorer::BuildConfiguration *bc(qobject_cast<ProjectExplorer::BuildConfiguration *>(sender()));
    if (bc == activeBuildConfiguration())
        emit environmentChanged();
}

Project *Target::project() const
{
    return static_cast<Project *>(parent());
}

void Target::addBuildConfiguration(BuildConfiguration *configuration)
{
    QTC_ASSERT(configuration && !d->m_buildConfigurations.contains(configuration), return);
    Q_ASSERT(configuration->target() == this);

    if (!buildConfigurationFactory())
        return;

    // Check that we don't have a configuration with the same displayName
    QString configurationDisplayName = configuration->displayName();
    QStringList displayNames;
    foreach (const BuildConfiguration *bc, d->m_buildConfigurations)
        displayNames << bc->displayName();
    configurationDisplayName = Project::makeUnique(configurationDisplayName, displayNames);
    configuration->setDisplayName(configurationDisplayName);

    // add it
    d->m_buildConfigurations.push_back(configuration);

    emit addedBuildConfiguration(configuration);

    connect(configuration, SIGNAL(environmentChanged()),
            SLOT(changeEnvironment()));

    if (!activeBuildConfiguration())
        setActiveBuildConfiguration(configuration);
}

void Target::removeBuildConfiguration(BuildConfiguration *configuration)
{
    //todo: this might be error prone
    if (!d->m_buildConfigurations.contains(configuration))
        return;

    d->m_buildConfigurations.removeOne(configuration);

    emit removedBuildConfiguration(configuration);

    if (activeBuildConfiguration() == configuration) {
        if (d->m_buildConfigurations.isEmpty())
            setActiveBuildConfiguration(0);
        else
            setActiveBuildConfiguration(d->m_buildConfigurations.at(0));
    }

    delete configuration;
}

QList<BuildConfiguration *> Target::buildConfigurations() const
{
    return d->m_buildConfigurations;
}

BuildConfiguration *Target::activeBuildConfiguration() const
{
    return d->m_activeBuildConfiguration;
}

void Target::setActiveBuildConfiguration(BuildConfiguration *configuration)
{
    if ((!configuration && d->m_buildConfigurations.isEmpty()) ||
        (configuration && d->m_buildConfigurations.contains(configuration) &&
         configuration != d->m_activeBuildConfiguration)) {
        d->m_activeBuildConfiguration = configuration;
        emit activeBuildConfigurationChanged(d->m_activeBuildConfiguration);
        emit environmentChanged();
    }
}

void Target::addDeployConfiguration(DeployConfiguration *dc)
{
    QTC_ASSERT(dc && !d->m_deployConfigurations.contains(dc), return);
    Q_ASSERT(dc->target() == this);

    if (!deployConfigurationFactory())
        return;

    // Check that we don't have a configuration with the same displayName
    QString configurationDisplayName = dc->displayName();
    QStringList displayNames;
    foreach (const DeployConfiguration *current, d->m_deployConfigurations)
        displayNames << current->displayName();
    configurationDisplayName = Project::makeUnique(configurationDisplayName, displayNames);
    dc->setDisplayName(configurationDisplayName);

    // add it
    d->m_deployConfigurations.push_back(dc);

    emit addedDeployConfiguration(dc);

    if (!d->m_activeDeployConfiguration)
        setActiveDeployConfiguration(dc);
    Q_ASSERT(activeDeployConfiguration());
}

void Target::removeDeployConfiguration(DeployConfiguration *dc)
{
    //todo: this might be error prone
    if (!d->m_deployConfigurations.contains(dc))
        return;

    d->m_deployConfigurations.removeOne(dc);

    emit removedDeployConfiguration(dc);

    if (activeDeployConfiguration() == dc) {
        if (d->m_deployConfigurations.isEmpty())
            setActiveDeployConfiguration(0);
        else
            setActiveDeployConfiguration(d->m_deployConfigurations.at(0));
    }

    delete dc;
}

QList<DeployConfiguration *> Target::deployConfigurations() const
{
    return d->m_deployConfigurations;
}

DeployConfiguration *Target::activeDeployConfiguration() const
{
    return d->m_activeDeployConfiguration;
}

void Target::setActiveDeployConfiguration(DeployConfiguration *dc)
{
    if ((!dc && d->m_deployConfigurations.isEmpty()) ||
        (dc && d->m_deployConfigurations.contains(dc) &&
         dc != d->m_activeDeployConfiguration)) {
        d->m_activeDeployConfiguration = dc;
        emit activeDeployConfigurationChanged(d->m_activeDeployConfiguration);
    }
}

QList<RunConfiguration *> Target::runConfigurations() const
{
    return d->m_runConfigurations;
}

void Target::addRunConfiguration(RunConfiguration* runConfiguration)
{
    QTC_ASSERT(runConfiguration && !d->m_runConfigurations.contains(runConfiguration), return);
    Q_ASSERT(runConfiguration->target() == this);

    // Check that we don't have a configuration with the same displayName
    QString configurationDisplayName = runConfiguration->displayName();
    QStringList displayNames;
    foreach (const RunConfiguration *rc, d->m_runConfigurations)
        displayNames << rc->displayName();
    configurationDisplayName = Project::makeUnique(configurationDisplayName, displayNames);
    runConfiguration->setDisplayName(configurationDisplayName);

    d->m_runConfigurations.push_back(runConfiguration);
    emit addedRunConfiguration(runConfiguration);

    if (!activeRunConfiguration())
        setActiveRunConfiguration(runConfiguration);
}

void Target::removeRunConfiguration(RunConfiguration* runConfiguration)
{
    QTC_ASSERT(runConfiguration && d->m_runConfigurations.contains(runConfiguration), return);

    d->m_runConfigurations.removeOne(runConfiguration);

    if (activeRunConfiguration() == runConfiguration) {
        if (d->m_runConfigurations.isEmpty())
            setActiveRunConfiguration(0);
        else
            setActiveRunConfiguration(d->m_runConfigurations.at(0));
    }

    emit removedRunConfiguration(runConfiguration);
    delete runConfiguration;
}

RunConfiguration* Target::activeRunConfiguration() const
{
    return d->m_activeRunConfiguration;
}

void Target::setActiveRunConfiguration(RunConfiguration* configuration)
{
    if ((!configuration && d->m_runConfigurations.isEmpty()) ||
        (configuration && d->m_runConfigurations.contains(configuration) &&
         configuration != d->m_activeRunConfiguration)) {
        d->m_activeRunConfiguration = configuration;
        emit activeRunConfigurationChanged(d->m_activeRunConfiguration);
    }
}

bool Target::isEnabled() const
{
    return d->m_isEnabled;
}

QIcon Target::icon() const
{
    return d->m_icon;
}

void Target::setIcon(QIcon icon)
{
    d->m_icon = icon;
    emit iconChanged();
}

QIcon Target::overlayIcon() const
{
    return d->m_overlayIcon;
}

void Target::setOverlayIcon(QIcon icon)
{
    d->m_overlayIcon = icon;
    emit overlayIconChanged();
}

QString Target::toolTip() const
{
    return d->m_toolTip;
}

void Target::setToolTip(const QString &text)
{
    d->m_toolTip = text;
    emit toolTipChanged();
}

QVariantMap Target::toMap() const
{
    const QList<BuildConfiguration *> bcs = buildConfigurations();

    QVariantMap map(ProjectConfiguration::toMap());
    map.insert(QLatin1String(ACTIVE_BC_KEY), bcs.indexOf(d->m_activeBuildConfiguration));
    map.insert(QLatin1String(BC_COUNT_KEY), bcs.size());
    for (int i = 0; i < bcs.size(); ++i)
        map.insert(QString::fromLatin1(BC_KEY_PREFIX) + QString::number(i), bcs.at(i)->toMap());

    const QList<DeployConfiguration *> dcs = deployConfigurations();
    map.insert(QLatin1String(ACTIVE_DC_KEY), dcs.indexOf(d->m_activeDeployConfiguration));
    map.insert(QLatin1String(DC_COUNT_KEY), dcs.size());
    for (int i = 0; i < dcs.size(); ++i)
        map.insert(QString::fromLatin1(DC_KEY_PREFIX) + QString::number(i), dcs.at(i)->toMap());

    const QList<RunConfiguration *> rcs = runConfigurations();
    map.insert(QLatin1String(ACTIVE_RC_KEY), rcs.indexOf(d->m_activeRunConfiguration));
    map.insert(QLatin1String(RC_COUNT_KEY), rcs.size());
    for (int i = 0; i < rcs.size(); ++i)
        map.insert(QString::fromLatin1(RC_KEY_PREFIX) + QString::number(i), rcs.at(i)->toMap());

    return map;
}

void Target::setEnabled(bool enabled)
{
    if (enabled == d->m_isEnabled)
        return;

    d->m_isEnabled = enabled;
    emit targetEnabled(d->m_isEnabled);
}

bool Target::fromMap(const QVariantMap &map)
{
    if (!ProjectConfiguration::fromMap(map))
        return false;

    bool ok;
    int bcCount(map.value(QLatin1String(BC_COUNT_KEY), 0).toInt(&ok));
    if (!ok || bcCount < 0)
        bcCount = 0;
    int activeConfiguration(map.value(QLatin1String(ACTIVE_BC_KEY), 0).toInt(&ok));
    if (!ok || activeConfiguration < 0)
        activeConfiguration = 0;
    if (0 > activeConfiguration || bcCount < activeConfiguration)
        activeConfiguration = 0;

    for (int i = 0; i < bcCount; ++i) {
        const QString key(QString::fromLatin1(BC_KEY_PREFIX) + QString::number(i));
        if (!map.contains(key))
            return false;
        BuildConfiguration *bc(buildConfigurationFactory()->restore(this, map.value(key).toMap()));
        if (!bc)
            continue;
        addBuildConfiguration(bc);
        if (i == activeConfiguration)
            setActiveBuildConfiguration(bc);
    }
    if (buildConfigurations().isEmpty() && buildConfigurationFactory())
        return false;

    int dcCount(map.value(QLatin1String(DC_COUNT_KEY), 0).toInt(&ok));
    if (!ok || dcCount < 0)
        dcCount = 0;
    activeConfiguration = map.value(QLatin1String(ACTIVE_DC_KEY), 0).toInt(&ok);
    if (!ok || activeConfiguration < 0)
        activeConfiguration = 0;
    if (0 > activeConfiguration || dcCount < activeConfiguration)
        activeConfiguration = 0;

    for (int i = 0; i < dcCount; ++i) {
        const QString key(QString::fromLatin1(DC_KEY_PREFIX) + QString::number(i));
        if (!map.contains(key))
            return false;
        DeployConfiguration *dc = 0;
        if (deployConfigurationFactory())
            dc = deployConfigurationFactory()->restore(this, map.value(key).toMap());
        if (!dc)
            continue;
        addDeployConfiguration(dc);
        if (i == activeConfiguration)
            setActiveDeployConfiguration(dc);
    }
    if (deployConfigurations().isEmpty() && deployConfigurationFactory())
        return false;

    int rcCount(map.value(QLatin1String(RC_COUNT_KEY), 0).toInt(&ok));
    if (!ok || rcCount < 0)
        rcCount = 0;
    activeConfiguration = map.value(QLatin1String(ACTIVE_RC_KEY), 0).toInt(&ok);
    if (!ok || activeConfiguration < 0)
        activeConfiguration = 0;
    if (0 > activeConfiguration || rcCount < activeConfiguration)
        activeConfiguration = 0;

    for (int i = 0; i < rcCount; ++i) {
        const QString key(QString::fromLatin1(RC_KEY_PREFIX) + QString::number(i));
        if (!map.contains(key))
            return false;

        QVariantMap valueMap(map.value(key).toMap());
        IRunConfigurationFactory *factory(IRunConfigurationFactory::restoreFactory(this, valueMap));
        if (!factory)
            continue; // Skip RCs we do not know about.)

        RunConfiguration *rc(factory->restore(this, valueMap));
        if (!rc)
            continue;
        addRunConfiguration(rc);
        if (i == activeConfiguration)
            setActiveRunConfiguration(rc);
    }
    // Ignore missing RCs: We will just populate them using the default ones.

    return true;
}


// -------------------------------------------------------------------------
// ITargetFactory
// -------------------------------------------------------------------------

ITargetFactory::ITargetFactory(QObject *parent) :
    QObject(parent)
{ }

ITargetFactory::~ITargetFactory()
{ }

} // namespace ProjectExplorer
