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

#include "buildconfiguration.h"

#include <utils/qtcassert.h>
#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>

using namespace ProjectExplorer;

namespace {

IBuildStepFactory *findCloneFactory(BuildConfiguration *parent, BuildStep *source)
{
    QList<IBuildStepFactory *> factories = ExtensionSystem::PluginManager::instance()->getObjects<IBuildStepFactory>();
    foreach(IBuildStepFactory *factory, factories)
        if (factory->canClone(parent, source))
            return factory;
    return 0;
}

IBuildStepFactory *findRestoreFactory(BuildConfiguration *parent, const QVariantMap &map)
{
    QList<IBuildStepFactory *> factories = ExtensionSystem::PluginManager::instance()->getObjects<IBuildStepFactory>();
    foreach(IBuildStepFactory *factory, factories)
        if (factory->canRestore(parent, map))
            return factory;
    return 0;
}

const char * const BUILD_STEPS_COUNT_KEY("ProjectExplorer.BuildConfiguration.BuildStepsCount");
const char * const BUILD_STEPS_PREFIX("ProjectExplorer.BuildConfiguration.BuildStep.");
const char * const CLEAN_STEPS_COUNT_KEY("ProjectExplorer.BuildConfiguration.CleanStepsCount");
const char * const CLEAN_STEPS_PREFIX("ProjectExplorer.BuildConfiguration.CleanStep.");

} // namespace

BuildConfiguration::BuildConfiguration(Target *target, const QString &id) :
    ProjectConfiguration(id),
    m_target(target)
{
    Q_ASSERT(m_target);
}

BuildConfiguration::BuildConfiguration(Target *target, BuildConfiguration *source) :
    ProjectConfiguration(source),
    m_target(target)
{
    Q_ASSERT(m_target);
}

BuildConfiguration::~BuildConfiguration()
{
    qDeleteAll(m_buildSteps);
    qDeleteAll(m_cleanSteps);
}

QVariantMap BuildConfiguration::toMap() const
{
    QVariantMap map(ProjectConfiguration::toMap());
    map.insert(QLatin1String(BUILD_STEPS_COUNT_KEY), m_buildSteps.count());
    for (int i = 0; i < m_buildSteps.count(); ++i)
        map.insert(QString::fromLatin1(BUILD_STEPS_PREFIX) + QString::number(i), m_buildSteps.at(i)->toMap());
    map.insert(QLatin1String(CLEAN_STEPS_COUNT_KEY), m_cleanSteps.count());
    for (int i = 0; i < m_cleanSteps.count(); ++i)
        map.insert(QString::fromLatin1(CLEAN_STEPS_PREFIX) + QString::number(i), m_cleanSteps.at(i)->toMap());

    return map;
}

void BuildConfiguration::cloneSteps(BuildConfiguration *source)
{
    Q_ASSERT(source);
    foreach(BuildStep *originalbs, source->buildSteps()) {
        IBuildStepFactory *factory(findCloneFactory(this, originalbs));
        if (!factory)
            continue;
        BuildStep *clonebs(factory->clone(this, originalbs));
        if (clonebs)
            m_buildSteps.append(clonebs);
    }
    foreach(BuildStep *originalcs, source->cleanSteps()) {
        IBuildStepFactory *factory = findCloneFactory(this, originalcs);
        if (!factory)
            continue;
        BuildStep *clonecs = factory->clone(this, originalcs);
        if (clonecs)
            m_cleanSteps.append(clonecs);
    }
}

bool BuildConfiguration::fromMap(const QVariantMap &map)
{
    if (!ProjectConfiguration::fromMap(map))
        return false;

    int maxI(map.value(QLatin1String(BUILD_STEPS_COUNT_KEY), 0).toInt());
    if (maxI < 0)
        maxI = 0;
    for (int i = 0; i < maxI; ++i) {
        QVariantMap bsData(map.value(QString::fromLatin1(BUILD_STEPS_PREFIX) + QString::number(i)).toMap());
        if (bsData.isEmpty())
            continue;
        IBuildStepFactory *factory(findRestoreFactory(this, bsData));
        if (!factory)
            continue;
        BuildStep *bs(factory->restore(this, bsData));
        if (!bs)
            continue;
        insertBuildStep(m_buildSteps.count(), bs);
    }

    maxI = map.value(QLatin1String(CLEAN_STEPS_COUNT_KEY), 0).toInt();
    if (maxI < 0)
        maxI = 0;
    for (int i = 0; i < maxI; ++i) {
        QVariantMap bsData(map.value(QString::fromLatin1(CLEAN_STEPS_PREFIX) + QString::number(i)).toMap());
        if (bsData.isEmpty())
            continue;
        IBuildStepFactory *factory(findRestoreFactory(this, bsData));
        if (!factory)
            continue;
        BuildStep *bs(factory->restore(this, bsData));
        if (!bs)
            continue;
        insertCleanStep(m_cleanSteps.count(), bs);
    }

    return true;
}

QList<BuildStep *> BuildConfiguration::buildSteps() const
{
    return m_buildSteps;
}

void BuildConfiguration::insertBuildStep(int position, BuildStep *step)
{
    m_buildSteps.insert(position, step);
}

void BuildConfiguration::removeBuildStep(int position)
{
    delete m_buildSteps.at(position);
    m_buildSteps.removeAt(position);
}

void BuildConfiguration::moveBuildStepUp(int position)
{
    if (position <= 0 || m_buildSteps.size() <= 1)
        return;
    m_buildSteps.swap(position - 1, position);

}

QList<BuildStep *> BuildConfiguration::cleanSteps() const
{
    return m_cleanSteps;
}

void BuildConfiguration::insertCleanStep(int position, BuildStep *step)
{
    m_cleanSteps.insert(position, step);
}

void BuildConfiguration::removeCleanStep(int position)
{
    delete m_cleanSteps.at(position);
    m_cleanSteps.removeAt(position);
}

void BuildConfiguration::moveCleanStepUp(int position)
{
    if (position <= 0 || m_cleanSteps.size() <= 1)
        return;
    m_cleanSteps.swap(position - 1, position);
}

Target *BuildConfiguration::target() const
{
    return m_target;
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
