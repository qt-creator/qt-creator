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

Project::Project()
    : m_activeRunConfiguration(0),
      m_editorConfiguration(new EditorConfiguration())
{
}

Project::~Project()
{
    qDeleteAll(m_buildSteps);
    qDeleteAll(m_cleanSteps);
    qDeleteAll(m_buildConfigurationValues);
    delete m_editorConfiguration;
}

void Project::insertBuildStep(int position, BuildStep *step)
{
    m_buildSteps.insert(position, step);
    // check that the step has all the configurations
    foreach (const BuildConfiguration *bc, buildConfigurations())
        if (!step->getBuildConfiguration(bc->name()))
            step->addBuildConfiguration(bc->name());
}

void Project::removeBuildStep(int position)
{
    delete m_buildSteps.at(position);
    m_buildSteps.removeAt(position);
}

void Project::moveBuildStepUp(int position)
{
    BuildStep *bs = m_buildSteps.takeAt(position);
    m_buildSteps.insert(position - 1, bs);
}

void Project::insertCleanStep(int position, BuildStep *step)
{
    m_cleanSteps.insert(position, step);
    // check that the step has all the configurations
    foreach (const BuildConfiguration *bc, buildConfigurations())
        if (!step->getBuildConfiguration(bc->name()))
            step->addBuildConfiguration(bc->name());
}

void Project::removeCleanStep(int position)
{
    delete m_cleanSteps.at(position);
    m_cleanSteps.removeAt(position);
}

void Project::moveCleanStepUp(int position)
{
    BuildStep *bs = m_cleanSteps.takeAt(position);
    m_cleanSteps.insert(position - 1, bs);
}

QString Project::makeUnique(const QString &preferedName, const QStringList &usedNames)
{
    if (!usedNames.contains(preferedName))
        return preferedName;
    int i = 2;
    QString tryName = preferedName + QString::number(i);
    while (usedNames.contains(tryName))
        tryName = preferedName + QString::number(++i);
    return tryName;
}


void Project::addBuildConfiguration(BuildConfiguration *configuration)
{
    QStringList buildConfigurationNames;
    foreach (const BuildConfiguration *bc, buildConfigurations())
        buildConfigurationNames << bc->name();

    // Check that the internal name is not taken and use a different one otherwise
    QString configurationName = configuration->name();
    configurationName = makeUnique(configurationName, buildConfigurationNames);
    configuration->setName(configurationName);

    // Check that we don't have a configuration with the same displayName
    QString configurationDisplayName = configuration->displayName();
    QStringList displayNames;
    foreach (const BuildConfiguration *bc, m_buildConfigurationValues)
        displayNames << bc->displayName();
    configurationDisplayName = makeUnique(configurationDisplayName, displayNames);
    configuration->setDisplayName(configurationDisplayName);

    // add it
    m_buildConfigurationValues.push_back(configuration);

    // update build steps
    for (int i = 0; i != m_buildSteps.size(); ++i)
        m_buildSteps.at(i)->addBuildConfiguration(configuration->name());
    for (int i = 0; i != m_cleanSteps.size(); ++i)
        m_cleanSteps.at(i)->addBuildConfiguration(configuration->name());

    emit addedBuildConfiguration(this, configuration->name());
}

void Project::removeBuildConfiguration(BuildConfiguration *configuration)
{
    //todo: this might be error prone
    if (!m_buildConfigurationValues.contains(configuration))
        return;

    m_buildConfigurationValues.removeOne(configuration);

    for (int i = 0; i != m_buildSteps.size(); ++i)
        m_buildSteps.at(i)->removeBuildConfiguration(configuration->name());
    for (int i = 0; i != m_cleanSteps.size(); ++i)
        m_cleanSteps.at(i)->removeBuildConfiguration(configuration->name());

    emit removedBuildConfiguration(this, configuration->name());
    delete configuration;
}

void Project::copyBuildConfiguration(const QString &source, const QString &dest)
{
    BuildConfiguration *sourceConfiguration = buildConfiguration(source);
    if (!sourceConfiguration)
        return;

    m_buildConfigurationValues.push_back(new BuildConfiguration(dest, sourceConfiguration));

    for (int i = 0; i != m_buildSteps.size(); ++i)
        m_buildSteps.at(i)->copyBuildConfiguration(source, dest);

    for (int i = 0; i != m_cleanSteps.size(); ++i)
        m_cleanSteps.at(i)->copyBuildConfiguration(source, dest);
    emit addedBuildConfiguration(this, dest);
}

QList<BuildConfiguration *> Project::buildConfigurations() const
{
    return m_buildConfigurationValues;
}

bool Project::hasBuildSettings() const
{
    return true;
}

QList<BuildStep *> Project::buildSteps() const
{
    return m_buildSteps;
}

QList<BuildStep *> Project::cleanSteps() const
{
    return m_cleanSteps;
}

void Project::saveSettings()
{
    PersistentSettingsWriter writer;
    saveSettingsImpl(writer);
    writer.save(file()->fileName() + QLatin1String(".user"), "QtCreatorProject");
}

bool Project::restoreSettings()
{
    PersistentSettingsReader reader;
    reader.load(file()->fileName() + QLatin1String(".user"));
    if (!restoreSettingsImpl(reader))
        return false;

    if (m_activeBuildConfiguration.isEmpty() && !m_buildConfigurationValues.isEmpty())
        setActiveBuildConfiguration(m_buildConfigurationValues.at(0));

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
    writer.saveValue("activebuildconfiguration", m_activeBuildConfiguration);
    //save m_values
    writer.saveValue("project", m_values);

    //save buildsettings
    QStringList buildConfigurationNames;
    foreach (const BuildConfiguration *bc, buildConfigurations()) {
        QMap<QString, QVariant> temp = bc->toMap();
        writer.saveValue("buildConfiguration-" + bc->name(), temp);
        buildConfigurationNames << bc->name();
    }

    QStringList buildStepNames;
    foreach (BuildStep *buildStep, buildSteps())
        buildStepNames << buildStep->name();
    writer.saveValue("buildsteps", buildStepNames);

    QStringList cleanStepNames;
    foreach (BuildStep *cleanStep, cleanSteps())
        cleanStepNames << cleanStep->name();
    writer.saveValue("cleansteps", cleanStepNames);
    writer.saveValue("buildconfigurations", buildConfigurationNames );

    //save buildstep configuration
    int buildstepnr = 0;
    foreach (BuildStep *buildStep, buildSteps()) {
        QMap<QString, QVariant> buildConfiguration = buildStep->valuesToMap();
        writer.saveValue("buildstep" + QString().setNum(buildstepnr), buildConfiguration);
        ++buildstepnr;
    }

    // save each buildstep/buildConfiguration combination
    foreach (const QString &buildConfigurationName, buildConfigurationNames) {
        buildstepnr = 0;
        foreach (BuildStep *buildStep, buildSteps()) {
            QMap<QString, QVariant> temp =
                buildStep->valuesToMap(buildConfigurationName);
            writer.saveValue("buildconfiguration-" + buildConfigurationName + "-buildstep" + QString().setNum(buildstepnr), temp);
            ++buildstepnr;
        }
    }

    //save cleansteps buildconfiguration
    int cleanstepnr = 0;
    foreach (BuildStep *cleanStep, cleanSteps()) {
        QMap<QString, QVariant> buildConfiguration = cleanStep->valuesToMap();
        writer.saveValue("cleanstep" + QString().setNum(cleanstepnr), buildConfiguration);
        ++cleanstepnr;
    }

    // save each cleanstep/buildConfiguration combination
    foreach (const QString &buildConfigurationName, buildConfigurationNames) {
        cleanstepnr = 0;
        foreach (BuildStep *cleanStep, cleanSteps()) {
            QMap<QString, QVariant> temp = cleanStep->valuesToMap(buildConfigurationName);
            writer.saveValue("buildconfiguration-" + buildConfigurationName + "-cleanstep" + QString().setNum(cleanstepnr), temp);
            ++cleanstepnr;
        }
    }

    // Running
    int i = 0;
    int activeId = 0;
    foreach (QSharedPointer<RunConfiguration> rc, m_runConfigurations) {
        writer.setPrefix("RunConfiguration" + QString().setNum(i) + "-");
        writer.saveValue("type", rc->type());
        rc->save(writer);
        if (rc == m_activeRunConfiguration)
            activeId = i;
        ++i;
    }
    writer.setPrefix(QString::null);
    writer.saveValue("activeRunConfiguration", activeId);

    writer.saveValue("defaultFileEncoding", m_editorConfiguration->defaultTextCodec()->name());
}

bool Project::restoreSettingsImpl(PersistentSettingsReader &reader)
{
    m_activeBuildConfiguration = reader.restoreValue("activebuildconfiguration").toString();

    m_values = reader.restoreValue("project").toMap();

    // restoring BuldConfigurations from settings
    const QStringList buildConfigurationNames = reader.restoreValue("buildconfigurations").toStringList();
    foreach (const QString &buildConfigurationName, buildConfigurationNames) {
        BuildConfiguration *bc = new BuildConfiguration(buildConfigurationName);
        addBuildConfiguration(bc);
        QMap<QString, QVariant> temp =
            reader.restoreValue("buildConfiguration-" + buildConfigurationName).toMap();
        bc->setValuesFromMap(temp);
    }

    const QList<IBuildStepFactory *> buildStepFactories =
          ExtensionSystem::PluginManager::instance()->getObjects<IBuildStepFactory>();
    //Build Settings
    QVariant buildStepsVariant = reader.restoreValue("buildsteps");
    if (buildStepsVariant.isValid()) {
        // restoring BuildSteps from settings
        int pos = 0;
        QStringList buildStepNames = buildStepsVariant.toStringList();
        for (int buildstepnr = 0; buildstepnr < buildStepNames.size(); ++buildstepnr) {
            const QString &buildStepName = buildStepNames.at(buildstepnr);
            BuildStep *buildStep  = 0;
            foreach (IBuildStepFactory *factory, buildStepFactories) {
                if (factory->canCreate(buildStepName)) {
                    buildStep = factory->create(this, buildStepName);
                    insertBuildStep(pos, buildStep);
                    ++pos;
                    break;
                }
            }
            // Restoring settings
            if (buildStep) {
                QMap<QString, QVariant> buildConfiguration = reader.restoreValue("buildstep" + QString().setNum(buildstepnr)).toMap();
                buildStep->setValuesFromMap(buildConfiguration);
                foreach (const QString &buildConfigurationName, buildConfigurationNames) {
                    //get the buildconfiguration for this build step
                    QMap<QString, QVariant> buildConfiguration =
                        reader.restoreValue("buildconfiguration-" + buildConfigurationName + "-buildstep" + QString().setNum(buildstepnr)).toMap();
                    buildStep->setValuesFromMap(buildConfigurationName, buildConfiguration);
                }
            }
        }
    }

    QVariant cleanStepsVariant = reader.restoreValue("cleansteps");
    if (cleanStepsVariant.isValid()) {
        QStringList cleanStepNames = cleanStepsVariant.toStringList();
        // restoring BuildSteps from settings
        int pos = 0;
        for (int cleanstepnr = 0; cleanstepnr < cleanStepNames.size(); ++cleanstepnr) {
            const QString &cleanStepName = cleanStepNames.at(cleanstepnr);
            BuildStep *cleanStep = 0;
            foreach (IBuildStepFactory *factory, buildStepFactories) {
                if (factory->canCreate(cleanStepName)) {
                    cleanStep = factory->create(this, cleanStepName);
                    insertCleanStep(pos, cleanStep);
                    ++pos;
                    break;
                }
            }
            // Restoring settings
            if (cleanStep) {
                QMap<QString, QVariant> buildConfiguration = reader.restoreValue("cleanstep" + QString().setNum(cleanstepnr)).toMap();
                cleanStep->setValuesFromMap(buildConfiguration);
                foreach (const QString &buildConfigurationName, buildConfigurationNames) {
                    QMap<QString, QVariant> buildConfiguration =
                            reader.restoreValue("buildconfiguration-" + buildConfigurationName + "-cleanstep" + QString().setNum(cleanstepnr)).toMap();
                    cleanStep->setValuesFromMap(buildConfigurationName, buildConfiguration);
                }
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
        const QVariant &typeVariant = reader.restoreValue("type");
        if (!typeVariant.isValid())
            break;
        const QString &type = typeVariant.toString();
        foreach (IRunConfigurationFactory *factory, factories) {
            if (factory->canRestore(type)) {
                QSharedPointer<RunConfiguration> rc = factory->create(this, type);
                rc->restore(reader);
                addRunConfiguration(rc);
                if (i == activeId)
                    setActiveRunConfiguration(rc);
            }
        }
        ++i;
    }
    reader.setPrefix(QString::null);

    QTextCodec *codec = QTextCodec::codecForName(reader.restoreValue("defaultFileEncoding").toByteArray());
    if (codec)
        m_editorConfiguration->setDefaultTextCodec(codec);

    if (!m_activeRunConfiguration && !m_runConfigurations.isEmpty())
        setActiveRunConfiguration(m_runConfigurations.at(0));
    return true;
}

void Project::setValue(const QString &name, const QVariant & value)
{
    m_values.insert(name, value);
}

QVariant Project::value(const QString &name) const
{
    QMap<QString, QVariant>::const_iterator it =
        m_values.find(name);
    if (it != m_values.constEnd())
        return it.value();
    else
        return QVariant();
}

BuildConfiguration *Project::buildConfiguration(const QString &name) const
{
    for (int i = 0; i != m_buildConfigurationValues.size(); ++i)
        if (m_buildConfigurationValues.at(i)->name() == name)
            return m_buildConfigurationValues.at(i);
    return 0;
}

BuildConfiguration *Project::activeBuildConfiguration() const
{
    return buildConfiguration(m_activeBuildConfiguration); //TODO
}

void Project::setActiveBuildConfiguration(BuildConfiguration *configuration)
{
    if (m_activeBuildConfiguration != configuration->name() && m_buildConfigurationValues.contains(configuration)) {
        m_activeBuildConfiguration = configuration->name();
        emit activeBuildConfigurationChanged();
    }
}

QList<QSharedPointer<RunConfiguration> > Project::runConfigurations() const
{
    return m_runConfigurations;
}

void Project::addRunConfiguration(QSharedPointer<RunConfiguration> runConfiguration)
{
    if (m_runConfigurations.contains(runConfiguration)) {
        qWarning()<<"Not adding already existing runConfiguration"<<runConfiguration->name();
        return;
    }
    m_runConfigurations.push_back(runConfiguration);
    emit addedRunConfiguration(this, runConfiguration->name());
}

void Project::removeRunConfiguration(QSharedPointer<RunConfiguration> runConfiguration)
{
    if(!m_runConfigurations.contains(runConfiguration)) {
        qWarning()<<"Not removing runConfiguration"<<runConfiguration->name()<<"becasue it doesn't exist";
        return;
    }

    if (m_activeRunConfiguration == runConfiguration) {
        if (m_runConfigurations.size() <= 1)
            setActiveRunConfiguration(QSharedPointer<RunConfiguration>(0));
        else if (m_runConfigurations.at(0) == m_activeRunConfiguration)
            setActiveRunConfiguration(m_runConfigurations.at(1));
        else
            setActiveRunConfiguration(m_runConfigurations.at(0));
    }

    m_runConfigurations.removeOne(runConfiguration);
    emit removedRunConfiguration(this, runConfiguration->name());
}

QSharedPointer<RunConfiguration> Project::activeRunConfiguration() const
{
    return m_activeRunConfiguration;
}

void Project::setActiveRunConfiguration(QSharedPointer<RunConfiguration> runConfiguration)
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

void Project::setDisplayNameFor(BuildConfiguration *configuration, const QString &displayName)
{
    if (configuration->displayName() == displayName)
        return;
    QString dn = displayName;
    QStringList displayNames;
    foreach (BuildConfiguration *bc, m_buildConfigurationValues) {
        if (bc != configuration)
            displayNames << bc->displayName();
    }
    dn = makeUnique(displayName, displayNames);

    configuration->setDisplayName(displayName);

    emit buildConfigurationDisplayNameChanged(configuration->name());
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
