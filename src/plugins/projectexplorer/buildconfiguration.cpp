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

IBuildStepFactory *findFactory(const QString &name)
{
    QList<IBuildStepFactory *> factories = ExtensionSystem::PluginManager::instance()->getObjects<IBuildStepFactory>();
    foreach(IBuildStepFactory *factory, factories)
        if (factory->canCreate(name))
            return factory;
    return 0;
}

BuildConfiguration::BuildConfiguration(const QString &name)
    : m_name(name)
{
    setDisplayName(name);
}

BuildConfiguration::BuildConfiguration(const QString &name, BuildConfiguration *source)
    : m_values(source->m_values), m_name(name)
{
    foreach(BuildStep *originalbs, source->buildSteps()) {
        IBuildStepFactory *factory = findFactory(originalbs->name());
        BuildStep *clonebs = factory->clone(originalbs, this);
        m_buildSteps.append(clonebs);
    }
    foreach(BuildStep *originalcs, source->cleanSteps()) {
        IBuildStepFactory *factory = findFactory(originalcs->name());
        BuildStep *clonecs = factory->clone(originalcs, this);
        m_cleanSteps.append(clonecs);
    }
}

BuildConfiguration::~BuildConfiguration()
{
    qDeleteAll(m_buildSteps);
    qDeleteAll(m_cleanSteps);
}

void BuildConfiguration::setName(const QString &name)
{
    m_name = name;
}

QString BuildConfiguration::name() const
{
    return m_name;
}

QString BuildConfiguration::displayName() const
{
    QVariant v = value("ProjectExplorer.BuildConfiguration.DisplayName");
    QTC_ASSERT(v.isValid(), return QString());
    return v.toString();
}

void BuildConfiguration::setDisplayName(const QString &name)
{
    setValue("ProjectExplorer.BuildConfiguration.DisplayName", name);
}

QVariant BuildConfiguration::value(const QString & key) const
{
    QHash<QString, QVariant>::const_iterator it = m_values.find(key);
    if (it != m_values.constEnd())
        return *it;
    else
        return QVariant();
}

void BuildConfiguration::setValue(const QString & key, QVariant value)
{
    m_values[key] = value;
}

void BuildConfiguration::setValuesFromMap(QMap<QString, QVariant> map)
{
    QMap<QString, QVariant>::const_iterator it, end;
    end = map.constEnd();
    for (it = map.constBegin(); it != end; ++it)
        setValue(it.key(), it.value());
}

QMap<QString, QVariant> BuildConfiguration::toMap() const
{
    QMap<QString, QVariant> result;
    QHash<QString, QVariant>::const_iterator it, end;
    end = m_values.constEnd();
    for (it = m_values.constBegin(); it != end; ++it)
        result.insert(it.key(), it.value());
    return result;
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

///
// IBuildConfigurationFactory
///

IBuildConfigurationFactory::IBuildConfigurationFactory(QObject *parent)
    : QObject(parent)
{
}

IBuildConfigurationFactory::~IBuildConfigurationFactory()
{

}
