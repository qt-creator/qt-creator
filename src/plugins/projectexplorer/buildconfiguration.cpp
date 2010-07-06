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

#include "buildconfiguration.h"

#include <utils/qtcassert.h>
#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/buildmanager.h>

#include <QtCore/QMetaEnum>
#include <QtCore/QMetaObject>
#include <QtCore/QProcess>

using namespace ProjectExplorer;

namespace {

IBuildStepFactory *findCloneFactory(BuildConfiguration *parent, BuildStep::Type type, BuildStep *source)
{
    QList<IBuildStepFactory *> factories = ExtensionSystem::PluginManager::instance()->getObjects<IBuildStepFactory>();
    foreach(IBuildStepFactory *factory, factories)
        if (factory->canClone(parent, type, source))
            return factory;
    return 0;
}

IBuildStepFactory *findRestoreFactory(BuildConfiguration *parent, BuildStep::Type type, const QVariantMap &map)
{
    QList<IBuildStepFactory *> factories = ExtensionSystem::PluginManager::instance()->getObjects<IBuildStepFactory>();
    foreach(IBuildStepFactory *factory, factories)
        if (factory->canRestore(parent, type, map))
            return factory;
    return 0;
}

const char * const STEPS_COUNT_KEY("ProjectExplorer.BuildConfiguration.%1StepsCount");
const char * const STEPS_PREFIX("ProjectExplorer.BuildConfiguration.%1Step.");
const char * const CLEAR_SYSTEM_ENVIRONMENT_KEY("ProjectExplorer.BuildConfiguration.ClearSystemEnvironment");
const char * const USER_ENVIRONMENT_CHANGES_KEY("ProjectExplorer.BuildConfiguration.UserEnvironmentChanges");

} // namespace

BuildConfiguration::BuildConfiguration(Target *target, const QString &id) :
    ProjectConfiguration(id),
    m_target(target),
    m_clearSystemEnvironment(false)
{
    Q_ASSERT(m_target);
}

BuildConfiguration::BuildConfiguration(Target *target, BuildConfiguration *source) :
    ProjectConfiguration(source),
    m_target(target),
    m_clearSystemEnvironment(source->m_clearSystemEnvironment),
    m_userEnvironmentChanges(source->m_userEnvironmentChanges)
{
    Q_ASSERT(m_target);
}

BuildConfiguration::~BuildConfiguration()
{
    for (int i = 0; i < BuildStep::LastStepType; ++i) {
        qDeleteAll(m_steps[i]);
    }
}

QVariantMap BuildConfiguration::toMap() const
{
    QVariantMap map(ProjectConfiguration::toMap());
    // Save build steps
    QMetaEnum typeEnum = BuildStep::staticMetaObject.enumerator(BuildStep::staticMetaObject.indexOfEnumerator("Type"));
    for (int type = 0; type < BuildStep::LastStepType; ++type) {
        const QString key(typeEnum.key(type));
        map.insert(QString::fromLatin1(STEPS_COUNT_KEY).arg(key), m_steps[type].count());
        for (int step = 0; step < m_steps[type].count(); ++step)
            map.insert(QString::fromLatin1(STEPS_PREFIX).arg(key) + QString::number(step), m_steps[type].at(step)->toMap());
    }

    map.insert(QLatin1String(CLEAR_SYSTEM_ENVIRONMENT_KEY), m_clearSystemEnvironment);
    map.insert(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY), EnvironmentItem::toStringList(m_userEnvironmentChanges));

    return map;
}

void BuildConfiguration::cloneSteps(BuildConfiguration *source)
{
    Q_ASSERT(source);
    for (int i = 0; i < BuildStep::LastStepType; ++i) {
        foreach (BuildStep *originalbs, source->steps(BuildStep::Type(i))) {
            IBuildStepFactory *factory(findCloneFactory(this, BuildStep::Type(i), originalbs));
            if (!factory)
                continue;
            BuildStep *clonebs(factory->clone(this, BuildStep::Type(i), originalbs));
            if (clonebs)
                m_steps[i].append(clonebs);
        }
    }
}

bool BuildConfiguration::fromMap(const QVariantMap &map)
{
    if (!ProjectConfiguration::fromMap(map))
        return false;

    QMetaEnum typeEnum = BuildStep::staticMetaObject.enumerator(BuildStep::staticMetaObject.indexOfEnumerator("Type"));
    for (int type = 0; type < BuildStep::LastStepType; ++type) {
        const QString key(typeEnum.key(type));
        int maxSteps = map.value(QString::fromLatin1(STEPS_COUNT_KEY).arg(key), 0).toInt();
        for (int step = 0; step < maxSteps; ++step) {
            QVariantMap bsData(map.value(QString::fromLatin1(STEPS_PREFIX).arg(key) + QString::number(step)).toMap());
            if (bsData.isEmpty()) {
                qWarning() << "No step data found for" << key << step << "(continuing).";
                continue;
            }
            IBuildStepFactory *factory(findRestoreFactory(this, BuildStep::Type(type), bsData));
            if (!factory) {
                qWarning() << "No factory for step" << key << step << "found (continuing).";
                continue;
            }
            BuildStep *bs(factory->restore(this, BuildStep::Type(type), bsData));
            if (!bs) {
                qWarning() << "Restoration of step" << key << step << "failed (continuing).";
                continue;
            }
            insertStep(BuildStep::Type(type), m_steps[type].count(), bs);
        }
    }

    m_clearSystemEnvironment = map.value(QLatin1String(CLEAR_SYSTEM_ENVIRONMENT_KEY)).toBool();
    m_userEnvironmentChanges = EnvironmentItem::fromStringList(map.value(QLatin1String(USER_ENVIRONMENT_CHANGES_KEY)).toStringList());

    return true;
}

QList<BuildStep *> BuildConfiguration::steps(BuildStep::Type type) const
{
    Q_ASSERT(type >= 0 && type < BuildStep::LastStepType);
    return m_steps[type];
}

void BuildConfiguration::insertStep(BuildStep::Type type, int position, BuildStep *step)
{
    Q_ASSERT(type >= 0 && type < BuildStep::LastStepType);
    m_steps[type].insert(position, step);
}

bool BuildConfiguration::removeStep(BuildStep::Type type, int position)
{
    Q_ASSERT(type >= 0 && type < BuildStep::LastStepType);

    ProjectExplorer::BuildManager *bm =
            ProjectExplorer::ProjectExplorerPlugin::instance()->buildManager();
    if (bm->isBuilding(m_steps[type].at(position)))
        return false;

    delete m_steps[type].at(position);
    m_steps[type].removeAt(position);
    return true;
}

void BuildConfiguration::moveStepUp(BuildStep::Type type, int position)
{
    Q_ASSERT(type >= 0 && type < BuildStep::LastStepType);
    if (position <= 0 || m_steps[type].size() <= 1)
        return;
    m_steps[type].swap(position - 1, position);

}

Target *BuildConfiguration::target() const
{
    return m_target;
}

Environment BuildConfiguration::baseEnvironment() const
{
    if (useSystemEnvironment())
        return Environment(QProcess::systemEnvironment());
    return Environment();
}

QString BuildConfiguration::baseEnvironmentText() const
{
    if (useSystemEnvironment())
        return tr("System Environment");
    else
        return tr("Clean Environment");
}

Environment BuildConfiguration::environment() const
{
    Environment env = baseEnvironment();
    env.modify(userEnvironmentChanges());
    return env;
}

void BuildConfiguration::setUseSystemEnvironment(bool b)
{
    if (useSystemEnvironment() == b)
        return;
    m_clearSystemEnvironment = !b;
    emit environmentChanged();
}

bool BuildConfiguration::useSystemEnvironment() const
{
    return !m_clearSystemEnvironment;
}

QList<EnvironmentItem> BuildConfiguration::userEnvironmentChanges() const
{
    return m_userEnvironmentChanges;
}

void BuildConfiguration::setUserEnvironmentChanges(const QList<ProjectExplorer::EnvironmentItem> &diff)
{
    if (m_userEnvironmentChanges == diff)
        return;
    m_userEnvironmentChanges = diff;
    emit environmentChanged();
}

///
// IBuildConfigurationFactory
///

IBuildConfigurationFactory::IBuildConfigurationFactory(QObject *parent) :
    QObject(parent)
{
}

IBuildConfigurationFactory::~IBuildConfigurationFactory()
{
}
