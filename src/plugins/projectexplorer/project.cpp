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

bool Project::hasBuildSettings() const
{
    return true;
}

void Project::saveSettings()
{
    PersistentSettingsWriter writer;
    saveSettingsImpl(writer);
    writer.save(file()->fileName() + QLatin1String(PROJECT_FILE_POSTFIX), "QtCreatorProject");
}

bool Project::restoreSettings()
{
    PersistentSettingsReader reader;
    reader.load(file()->fileName() + QLatin1String(PROJECT_FILE_POSTFIX));
    if (!restoreSettingsImpl(reader))
        return false;

    if (!m_activeBuildConfiguration && !m_buildConfigurations.isEmpty())
        setActiveBuildConfiguration(m_buildConfigurations.at(0));

    if (!m_activeRunConfiguration && !m_runConfigurations.isEmpty())
        setActiveRunConfiguration(m_runConfigurations.at(0));
    return true;
}

QList<BuildConfigWidget*> Project::subConfigWidgets()
{
    return QList<BuildConfigWidget*>();
}

void Project::saveSettingsImpl(PersistentSettingsWriter &writer)
{
    const QList<BuildConfiguration *> bcs = buildConfigurations();

    // For compatibility with older versions the "name" is saved as a string instead of a number
    writer.saveValue("activebuildconfiguration", QString::number(bcs.indexOf(m_activeBuildConfiguration)));

    //save buildsettings
    QStringList buildConfigurationNames;
    for(int i=0; i < bcs.size(); ++i) {
        writer.saveValue("buildConfiguration-" + QString::number(i), bcs.at(i)->toMap());
        buildConfigurationNames << QString::number(i);
    }
    writer.saveValue("buildconfigurations", buildConfigurationNames);

    // Running
    int i = 0;
    int activeId = 0;
    foreach (RunConfiguration* rc, m_runConfigurations) {
        writer.saveValue("RunConfiguration" + QString().setNum(i), rc->toMap());
        if (rc == m_activeRunConfiguration)
            activeId = i;
        ++i;
    }
    writer.setPrefix(QString::null);
    writer.saveValue("activeRunConfiguration", activeId);

    writer.saveValue(QLatin1String(EDITOR_SETTINGS_KEY), m_editorConfiguration->toMap());
}

bool Project::restoreSettingsImpl(PersistentSettingsReader &reader)
{
    // restoring BuldConfigurations from settings
    const QStringList buildConfigurationNames = reader.restoreValue("buildconfigurations").toStringList();

    foreach (const QString &buildConfigurationName, buildConfigurationNames) {
        QVariantMap temp(reader.restoreValue("buildConfiguration-" + buildConfigurationName).toMap());
        BuildConfiguration *bc = buildConfigurationFactory()->restore(this, temp);
        addBuildConfiguration(bc);
    }

    { // Try restoring the active configuration
        QString activeConfigurationName = reader.restoreValue("activebuildconfiguration").toString();
        int index = buildConfigurationNames.indexOf(activeConfigurationName);
        if (index != -1)
            m_activeBuildConfiguration = buildConfigurations().at(index);
        else if (!buildConfigurations().isEmpty())
            m_activeBuildConfiguration = buildConfigurations().at(0);
        else
            m_activeBuildConfiguration = 0;
    }

    // Running
    const int activeId = reader.restoreValue("activeRunConfiguration").toInt();
    int i = 0;
    const QList<IRunConfigurationFactory *> factories =
        ExtensionSystem::PluginManager::instance()->getObjects<IRunConfigurationFactory>();
    forever {
        QVariantMap values(reader.restoreValue("RunConfiguration" + QString().setNum(i)).toMap());
        if (values.isEmpty())
            break;
        foreach (IRunConfigurationFactory *factory, factories) {
            if (factory->canRestore(this, values)) {
                RunConfiguration* rc = factory->restore(this, values);
                if (!rc)
                    continue;
                addRunConfiguration(rc);
                if (i == activeId)
                    setActiveRunConfiguration(rc);
            }
        }
        ++i;
    }
    if (!activeRunConfiguration() && !m_runConfigurations.isEmpty())
        setActiveRunConfiguration(m_runConfigurations.at(0));

    QVariantMap tmp = reader.restoreValue(QLatin1String(EDITOR_SETTINGS_KEY)).toMap();
    return m_editorConfiguration->fromMap(tmp);
}

BuildConfiguration *Project::activeBuildConfiguration() const
{
    return m_activeBuildConfiguration;
}

void Project::setActiveBuildConfiguration(BuildConfiguration *configuration)
{
    if (m_activeBuildConfiguration != configuration && m_buildConfigurations.contains(configuration)) {
        m_activeBuildConfiguration = configuration;
        emit activeBuildConfigurationChanged();
    }
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

