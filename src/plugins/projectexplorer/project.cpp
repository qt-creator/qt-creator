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
        QMap<QString, QVariant> temp;
        bcs.at(i)->toMap(temp);
        writer.saveValue("buildConfiguration-" + QString::number(i), temp);
        buildConfigurationNames << QString::number(i);
    }
    writer.saveValue("buildconfigurations", buildConfigurationNames);

    // save each buildstep/buildConfiguration combination
    for(int i=0; i < bcs.size(); ++i) {
        QStringList buildStepNames;
        foreach (BuildStep *buildStep, bcs.at(i)->buildSteps())
            buildStepNames << buildStep->id();
        writer.saveValue("buildconfiguration-" + QString::number(i) + "-buildsteps", buildStepNames);

        int buildstepnr = 0;
        foreach (BuildStep *buildStep, bcs.at(i)->buildSteps()) {
            QMap<QString, QVariant> temp;
            buildStep->storeIntoLocalMap(temp);
            writer.saveValue("buildconfiguration-" + QString::number(i) + "-buildstep" + QString().setNum(buildstepnr), temp);
            ++buildstepnr;
        }
    }

    // save each cleanstep/buildConfiguration combination
    for(int i=0; i < bcs.size(); ++i) {
        QStringList cleanStepNames;
        foreach (BuildStep *cleanStep, bcs.at(i)->cleanSteps())
            cleanStepNames << cleanStep->id();
        writer.saveValue("buildconfiguration-" + QString::number(i) + "-cleansteps", cleanStepNames);

        int cleanstepnr = 0;
        foreach (BuildStep *cleanStep, bcs.at(i)->cleanSteps()) {
            QMap<QString, QVariant> temp;
            cleanStep->storeIntoLocalMap(temp);
            writer.saveValue("buildconfiguration-" + QString::number(i) + "-cleanstep" + QString().setNum(cleanstepnr), temp);
            ++cleanstepnr;
        }
    }

    // Running
    int i = 0;
    int activeId = 0;
    foreach (RunConfiguration* rc, m_runConfigurations) {
        writer.setPrefix("RunConfiguration" + QString().setNum(i) + "-");
        writer.saveValue("Id", rc->id());
        rc->save(writer);
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
    const QList<IBuildStepFactory *> buildStepFactories =
          ExtensionSystem::PluginManager::instance()->getObjects<IBuildStepFactory>();

    // restoring BuldConfigurations from settings
    const QStringList buildConfigurationNames = reader.restoreValue("buildconfigurations").toStringList();

    foreach (const QString &buildConfigurationName, buildConfigurationNames) {
        QMap<QString, QVariant> temp =
            reader.restoreValue("buildConfiguration-" + buildConfigurationName).toMap();

        BuildConfiguration *bc = buildConfigurationFactory()->restore(temp);
        // Restore build steps
        QVariant buildStepsValueVariant = reader.restoreValue("buildconfiguration-" + buildConfigurationName + "-buildsteps");
        if(buildStepsValueVariant.isValid()) {
            int pos = 0;
            QStringList buildStepNames = buildStepsValueVariant.toStringList();
            for (int buildstepnr = 0; buildstepnr < buildStepNames.size(); ++buildstepnr) {
                const QString &buildStepName = buildStepNames.at(buildstepnr);
                BuildStep *buildStep  = 0;
                foreach (IBuildStepFactory *factory, buildStepFactories) {
                    if (factory->canCreate(buildStepName)) {
                        buildStep = factory->create(bc, buildStepName);
                        break;
                    }
                }
                // Restoring settings
                if (buildStep) {
                    // TODO remove restoreFromGlobalMap after 2.0
                    QMap<QString, QVariant> buildStepValues = reader.restoreValue("buildstep" + QString().setNum(buildstepnr)).toMap();
                    buildStep->restoreFromGlobalMap(buildStepValues);
                    buildStepValues =
                            reader.restoreValue("buildconfiguration-" + buildConfigurationName + "-buildstep" + QString().setNum(buildstepnr)).toMap();
                    buildStep->restoreFromLocalMap(buildStepValues);
                    bc->insertBuildStep(pos, buildStep);
                    ++pos;
                }
            }
        }
        // Restore clean steps
        QVariant cleanStepsValueVariant = reader.restoreValue("buildconfiguration-" + buildConfigurationName + "-cleansteps");
        if(cleanStepsValueVariant.isValid()) {
            int pos = 0;
            QStringList cleanStepNames = cleanStepsValueVariant.toStringList();
            for (int cleanstepnr = 0; cleanstepnr < cleanStepNames.size(); ++cleanstepnr) {
                const QString &cleanStepName = cleanStepNames.at(cleanstepnr);
                BuildStep *cleanStep = 0;
                foreach (IBuildStepFactory *factory, buildStepFactories) {
                    if (factory->canCreate(cleanStepName)) {
                        cleanStep = factory->create(bc, cleanStepName);
                        break;
                    }
                }
                // Restoring settings
                if (cleanStep) {
                    // TODO remove restoreFromGlobalMap after 2.0
                    QMap<QString, QVariant> buildStepValues = reader.restoreValue("cleanstep" + QString().setNum(cleanstepnr)).toMap();
                    cleanStep->restoreFromGlobalMap(buildStepValues);
                    buildStepValues =
                            reader.restoreValue("buildconfiguration-" + buildConfigurationName + "-cleanstep" + QString().setNum(cleanstepnr)).toMap();
                    cleanStep->restoreFromLocalMap(buildStepValues);
                    bc->insertCleanStep(pos, cleanStep);
                    ++pos;
                }
            }
        }
        addBuildConfiguration(bc);
    }

    // Set Active Configuration
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

    //Build Settings
    QVariant buildStepsVariant = reader.restoreValue("buildsteps");
    if (buildStepsVariant.isValid()) {
        // Old code path for 1.3 compatibility
        // restoring BuildSteps from settings
        int pos = 0;
        QStringList buildStepNames = buildStepsVariant.toStringList();
        for (int buildstepnr = 0; buildstepnr < buildStepNames.size(); ++buildstepnr) {
            const QString &buildStepName = buildStepNames.at(buildstepnr);
            BuildStep *buildStep  = 0;
            IBuildStepFactory *factory = 0;
            foreach (IBuildStepFactory *fac, buildStepFactories) {
                if (fac->canCreate(buildStepName)) {
                    factory = fac;
                    break;
                }
            }
            if (factory) {
                const QList<BuildConfiguration *> &bcs = buildConfigurations();
                for(int i = 0; i < bcs.size(); ++i) {
                    buildStep = factory->create(bcs.at(i), buildStepName);
                    bcs.at(i)->insertBuildStep(pos, buildStep);
                    QMap<QString, QVariant> buildStepValues = reader.restoreValue("buildstep" + QString().setNum(buildstepnr)).toMap();
                    buildStep->restoreFromGlobalMap(buildStepValues);
                    buildStepValues =
                            reader.restoreValue("buildconfiguration-" + QString::number(i) + "-buildstep" + QString().setNum(buildstepnr)).toMap();
                    buildStep->restoreFromLocalMap(buildStepValues);
                }
                ++pos;
            }
        }
    }

    QVariant cleanStepsVariant = reader.restoreValue("cleansteps");
    if (cleanStepsVariant.isValid()) {
        // Old code path for 1.3 compatibility
        QStringList cleanStepNames = cleanStepsVariant.toStringList();
        // restoring BuildSteps from settings
        int pos = 0;
        for (int cleanstepnr = 0; cleanstepnr < cleanStepNames.size(); ++cleanstepnr) {
            const QString &cleanStepName = cleanStepNames.at(cleanstepnr);
            BuildStep *cleanStep = 0;
            IBuildStepFactory *factory = 0;
            foreach (IBuildStepFactory *fac, buildStepFactories) {
                if (fac->canCreate(cleanStepName)) {
                    factory = fac;
                    break;
                }
            }

            if (factory) {
                const QList<BuildConfiguration *> &bcs = buildConfigurations();
                for (int i = 0; i < bcs.size(); ++i) {
                    cleanStep = factory->create(bcs.at(i), cleanStepName);
                    bcs.at(i)->insertCleanStep(pos, cleanStep);
                    QMap<QString, QVariant> cleanStepValues = reader.restoreValue("cleanstep" + QString().setNum(cleanstepnr)).toMap();
                    cleanStep->restoreFromGlobalMap(cleanStepValues);
                    QMap<QString, QVariant> buildStepValues =
                            reader.restoreValue("buildconfiguration-" + QString::number(i) + "-cleanstep" + QString().setNum(cleanstepnr)).toMap();
                    cleanStep->restoreFromLocalMap(buildStepValues);
                }
                ++pos;
            }
        }
    }

    // Running
    const int activeId = reader.restoreValue("activeRunConfiguration").toInt();
    int i = 0;
    const QList<IRunConfigurationFactory *> factories =
        ExtensionSystem::PluginManager::instance()->getObjects<IRunConfigurationFactory>();
    forever {
        reader.setPrefix("RunConfiguration" + QString().setNum(i) + "-");
        const QVariant &idVariant = reader.restoreValue("Id");
        if (!idVariant.isValid())
            break;
        const QString &id = idVariant.toString();
        foreach (IRunConfigurationFactory *factory, factories) {
            if (factory->canRestore(id)) {
                RunConfiguration* rc = factory->create(this, id);
                rc->restore(reader);
                addRunConfiguration(rc);
                if (i == activeId)
                    setActiveRunConfiguration(rc);
            }
        }
        ++i;
    }
    reader.setPrefix(QString::null);

    if (!m_activeRunConfiguration && !m_runConfigurations.isEmpty())
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

