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

#include "project.h"

#include "persistentsettings.h"
#include "buildconfiguration.h"
#include "environment.h"
#include "projectnodes.h"
#include "buildstep.h"
#include "projectexplorer.h"
#include "runconfiguration.h"
#include "editorconfiguration.h"

#include <coreplugin/ifile.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QTextCodec>

using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;

namespace {
const char * const PROJECT_FILE_POSTFIX(".user");

const char * const ACTIVE_BC_KEY("ProjectExplorer.Project.ActiveBuildConfiguration");
const char * const BC_KEY_PREFIX("ProjectExplorer.Project.BuildConfiguration.");
const char * const BC_COUNT_KEY("ProjectExplorer.Project.BuildConfigurationCount");

const char * const ACTIVE_RC_KEY("ProjectExplorer.Project.ActiveRunConfiguration");
const char * const RC_KEY_PREFIX("ProjectExplorer.Project.RunConfiguration.");
const char * const RC_COUNT_KEY("ProjectExplorer.Project.RunConfigurationCount");

const char * const EDITOR_SETTINGS_KEY("ProjectExplorer.Project.EditorSettings");
} // namespace

// -------------------------------------------------------------------------
// Project
// -------------------------------------------------------------------------

Project::Project() :
    m_activeBuildConfiguration(0),
    m_activeRunConfiguration(0),
    m_editorConfiguration(new EditorConfiguration())
{
}

Project::~Project()
{
    qDeleteAll(m_buildConfigurations);
    qDeleteAll(m_runConfigurations);
    delete m_editorConfiguration;
}

QString Project::makeUnique(const QString &preferredName, const QStringList &usedNames)
{
    if (!usedNames.contains(preferredName))
        return preferredName;
    int i = 2;
    QString tryName = preferredName + QString::number(i);
    while (usedNames.contains(tryName))
        tryName = preferredName + QString::number(++i);
    return tryName;
}

void Project::addBuildConfiguration(BuildConfiguration *configuration)
{
    QTC_ASSERT(configuration && !m_buildConfigurations.contains(configuration), return);

    // Check that we don't have a configuration with the same displayName
    QString configurationDisplayName = configuration->displayName();
    QStringList displayNames;
    foreach (const BuildConfiguration *bc, m_buildConfigurations)
        displayNames << bc->displayName();
    configurationDisplayName = makeUnique(configurationDisplayName, displayNames);
    configuration->setDisplayName(configurationDisplayName);

    // add it
    m_buildConfigurations.push_back(configuration);

    emit addedBuildConfiguration(configuration);
}

void Project::removeBuildConfiguration(BuildConfiguration *configuration)
{
    //todo: this might be error prone
    if (!m_buildConfigurations.contains(configuration))
        return;

    m_buildConfigurations.removeOne(configuration);

    emit removedBuildConfiguration(configuration);
    delete configuration;
}

QList<BuildConfiguration *> Project::buildConfigurations() const
{
    return m_buildConfigurations;
}

BuildConfiguration *Project::activeBuildConfiguration() const
{
    return m_activeBuildConfiguration;
}

void Project::setActiveBuildConfiguration(BuildConfiguration *configuration)
{
    if (!configuration ||
        m_activeBuildConfiguration == configuration ||
        !m_buildConfigurations.contains(configuration))
        return;
    m_activeBuildConfiguration = configuration;
    emit activeBuildConfigurationChanged();
}

bool Project::hasBuildSettings() const
{
    return true;
}

void Project::saveSettings()
{
    PersistentSettingsWriter writer;
    QVariantMap map(toMap());
    for (QVariantMap::const_iterator i = map.constBegin(); i != map.constEnd(); ++i)
        writer.saveValue(i.key(), i.value());

    writer.save(file()->fileName() + QLatin1String(PROJECT_FILE_POSTFIX), "QtCreatorProject");
}

bool Project::restoreSettings()
{
    PersistentSettingsReader reader;
    reader.load(file()->fileName() + QLatin1String(PROJECT_FILE_POSTFIX));

    QVariantMap map(reader.restoreValues());

    return fromMap(map);
}

QList<BuildConfigWidget*> Project::subConfigWidgets()
{
    return QList<BuildConfigWidget*>();
}

QVariantMap Project::toMap() const
{
    const QList<BuildConfiguration *> bcs = buildConfigurations();

    QVariantMap map;
    map.insert(QLatin1String(ACTIVE_BC_KEY), bcs.indexOf(m_activeBuildConfiguration));
    map.insert(QLatin1String(BC_COUNT_KEY), bcs.size());
    for (int i = 0; i < bcs.size(); ++i)
        map.insert(QString::fromLatin1(BC_KEY_PREFIX) + QString::number(i), bcs.at(i)->toMap());

    const QList<RunConfiguration *> rcs = runConfigurations();
    map.insert(QLatin1String(ACTIVE_RC_KEY), rcs.indexOf(m_activeRunConfiguration));
    map.insert(QLatin1String(RC_COUNT_KEY), rcs.size());
    for (int i = 0; i < rcs.size(); ++i)
        map.insert(QString::fromLatin1(RC_KEY_PREFIX) + QString::number(i), rcs.at(i)->toMap());

    map.insert(QLatin1String(EDITOR_SETTINGS_KEY), m_editorConfiguration->toMap());

    return map;
}

bool Project::fromMap(const QVariantMap &map)
{
    if (map.contains(QLatin1String(EDITOR_SETTINGS_KEY))) {
        QVariantMap values(map.value(QLatin1String(EDITOR_SETTINGS_KEY)).toMap());
        if (!m_editorConfiguration->fromMap(values))
            return false;
    }

    bool ok;
    int maxI(map.value(QLatin1String(BC_COUNT_KEY), 0).toInt(&ok));
    if (!ok || maxI < 0)
        maxI = 0;
    int activeConfiguration(map.value(QLatin1String(ACTIVE_BC_KEY), 0).toInt(&ok));
    if (!ok || activeConfiguration < 0)
        activeConfiguration = 0;
    if (0 > activeConfiguration || maxI < activeConfiguration)
        activeConfiguration = 0;

    for (int i = 0; i < maxI; ++i) {
        const QString key(QString::fromLatin1(BC_KEY_PREFIX) + QString::number(i));
        if (!map.contains(key))
            return false;
        if (!buildConfigurationFactory()->canRestore(this, map.value(key).toMap()))
            return false;
        BuildConfiguration *bc(buildConfigurationFactory()->restore(this, map.value(key).toMap()));
        if (!bc)
            continue;
        addBuildConfiguration(bc);
        if (i == activeConfiguration)
            setActiveBuildConfiguration(bc);
    }
    if (!activeBuildConfiguration() && !m_buildConfigurations.isEmpty())
        setActiveBuildConfiguration(m_buildConfigurations.at(0));

    maxI = map.value(QLatin1String(RC_COUNT_KEY), 0).toInt(&ok);
    if (!ok || maxI < 0)
        maxI = 0;
    activeConfiguration = map.value(QLatin1String(ACTIVE_RC_KEY), 0).toInt(&ok);
    if (!ok || activeConfiguration < 0)
        activeConfiguration = 0;
    if (0 > activeConfiguration || maxI < activeConfiguration)
        activeConfiguration = 0;

    QList<IRunConfigurationFactory *>
        factories(ExtensionSystem::PluginManager::instance()->
            getObjects<IRunConfigurationFactory>());

    for (int i = 0; i < maxI; ++i) {
        const QString key(QString::fromLatin1(RC_KEY_PREFIX) + QString::number(i));
        if (!map.contains(key))
            return false;

        QVariantMap valueMap(map.value(key).toMap());
        IRunConfigurationFactory *factory(0);
        foreach (IRunConfigurationFactory *f, factories) {
            if (!f->canRestore(this, valueMap))
                continue;
            factory = f;
            break;
        }
        if (!factory)
            return false;

        RunConfiguration *rc(factory->restore(this, valueMap));
        if (!rc)
            return false;
        addRunConfiguration(rc);
        if (i == activeConfiguration)
            setActiveRunConfiguration(rc);
    }

    if (!activeRunConfiguration() && !m_runConfigurations.isEmpty())
        setActiveRunConfiguration(m_runConfigurations.at(0));

    return true;
}

QList<RunConfiguration *> Project::runConfigurations() const
{
    return m_runConfigurations;
}

void Project::addRunConfiguration(RunConfiguration* runConfiguration)
{
    QTC_ASSERT(runConfiguration && !m_runConfigurations.contains(runConfiguration), return);

    m_runConfigurations.push_back(runConfiguration);
    emit addedRunConfiguration(runConfiguration);
}

void Project::removeRunConfiguration(RunConfiguration* runConfiguration)
{
    if(!m_runConfigurations.contains(runConfiguration)) {
        qWarning()<<"Not removing runConfiguration"<<runConfiguration->displayName()<<"becasue it doesn't exist";
        return;
    }

    if (m_activeRunConfiguration == runConfiguration) {
        if (m_runConfigurations.size() <= 1)
            setActiveRunConfiguration(0);
        else if (m_runConfigurations.at(0) == m_activeRunConfiguration)
            setActiveRunConfiguration(m_runConfigurations.at(1));
        else
            setActiveRunConfiguration(m_runConfigurations.at(0));
    }

    m_runConfigurations.removeOne(runConfiguration);
    emit removedRunConfiguration(runConfiguration);
    delete runConfiguration;
}

RunConfiguration* Project::activeRunConfiguration() const
{
    return m_activeRunConfiguration;
}

void Project::setActiveRunConfiguration(RunConfiguration* runConfiguration)
{
    if (runConfiguration == m_activeRunConfiguration)
        return;
    Q_ASSERT(m_runConfigurations.contains(runConfiguration) || runConfiguration == 0);
    m_activeRunConfiguration = runConfiguration;
    emit activeRunConfigurationChanged();
}

EditorConfiguration *Project::editorConfiguration() const
{
    return m_editorConfiguration;
}

QByteArray Project::predefinedMacros(const QString &) const
{
    return QByteArray();
}

QStringList Project::includePaths(const QString &) const
{
    return QStringList();
}

QStringList Project::frameworkPaths(const QString &) const
{
    return QStringList();
}

QString Project::generatedUiHeader(const QString & /* formFile */) const
{
    return QString();
}

