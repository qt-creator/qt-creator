/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "project.h"

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
    foreach (const QString &name, buildConfigurations())
        if (!step->getBuildConfiguration(name))
            step->addBuildConfiguration(name);
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
    foreach (const QString &name, buildConfigurations())
        if (!step->getBuildConfiguration(name))
            step->addBuildConfiguration(name);
}

void Project::removeCleanStep(int position)
{
    delete m_cleanSteps.at(position);
    m_cleanSteps.removeAt(position);
}

void Project::addBuildConfiguration(const QString &name)
{
    if (buildConfigurations().contains(name) )
        return;

    m_buildConfigurationValues.push_back(new BuildConfiguration(name));

    for (int i = 0; i != m_buildSteps.size(); ++i)
        m_buildSteps.at(i)->addBuildConfiguration(name);

    for (int i = 0; i != m_cleanSteps.size(); ++i)
        m_cleanSteps.at(i)->addBuildConfiguration(name);
}

void Project::removeBuildConfiguration(const QString &name)
{
    if (!buildConfigurations().contains(name))
        return;

    for (int i = 0; i != m_buildConfigurationValues.size(); ++i)
        if (m_buildConfigurationValues.at(i)->name() == name) {
            delete m_buildConfigurationValues.at(i);
            m_buildConfigurationValues.removeAt(i);
            break;
        }

    for (int i = 0; i != m_buildSteps.size(); ++i)
        m_buildSteps.at(i)->removeBuildConfiguration(name);
    for (int i = 0; i != m_cleanSteps.size(); ++i)
        m_cleanSteps.at(i)->removeBuildConfiguration(name);

}

void Project::copyBuildConfiguration(const QString &source, const QString &dest)
{
    if (!buildConfigurations().contains(source))
        return;

    for (int i = 0; i != m_buildConfigurationValues.size(); ++i)
        if (m_buildConfigurationValues.at(i)->name() == source)
            m_buildConfigurationValues.push_back(new BuildConfiguration(dest, m_buildConfigurationValues.at(i)));

    for (int i = 0; i != m_buildSteps.size(); ++i)
        m_buildSteps.at(i)->copyBuildConfiguration(source, dest);

    for (int i = 0; i != m_cleanSteps.size(); ++i)
        m_cleanSteps.at(i)->copyBuildConfiguration(source, dest);
}

QStringList Project::buildConfigurations() const
{
    QStringList result;
    foreach (BuildConfiguration *bc, m_buildConfigurationValues)
        result << bc->name();
    return result;
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

void Project::restoreSettings()
{
    PersistentSettingsReader reader;
    reader.load(file()->fileName() + QLatin1String(".user"));
    restoreSettingsImpl(reader);

    if (m_activeBuildConfiguration.isEmpty() && !m_buildConfigurations.isEmpty())
        setActiveBuildConfiguration(m_buildConfigurations.at(0));

    if (!m_activeRunConfiguration && !m_runConfigurations.isEmpty())
        setActiveRunConfiguration(m_runConfigurations.at(0));
}

QList<BuildStepConfigWidget*> Project::subConfigWidgets()
{
    return QList<BuildStepConfigWidget*>();
}

void Project::saveSettingsImpl(PersistentSettingsWriter &writer)
{
    writer.saveValue("activebuildconfiguration", m_activeBuildConfiguration);
    //save m_values
    writer.saveValue("project", m_values);

    //save buildsettings
    foreach (const QString &buildConfigurationName, buildConfigurations()) {
        QMap<QString, QVariant> temp =
            getBuildConfiguration(buildConfigurationName)->toMap();
        writer.saveValue("buildConfiguration-" + buildConfigurationName, temp);
    }

    QStringList buildStepNames;
    foreach (BuildStep *buildStep, buildSteps())
        buildStepNames << buildStep->name();
    writer.saveValue("buildsteps", buildStepNames);

    QStringList cleanStepNames;
    foreach (BuildStep *cleanStep, cleanSteps())
        cleanStepNames << cleanStep->name();
    writer.saveValue("cleansteps", cleanStepNames);
    QStringList buildConfigurationNames = buildConfigurations();
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

void Project::restoreSettingsImpl(PersistentSettingsReader &reader)
{
    m_activeBuildConfiguration = reader.restoreValue("activebuildconfiguration").toString();

    m_values = reader.restoreValue("project").toMap();

    //Build Settings
    const QStringList buildConfigurationNames = reader.restoreValue("buildconfigurations").toStringList();
    foreach (const QString &buildConfigurationName, buildConfigurationNames) {
        addBuildConfiguration(buildConfigurationName);
        QMap<QString, QVariant> temp =
            reader.restoreValue("buildConfiguration-" + buildConfigurationName).toMap();
        getBuildConfiguration(buildConfigurationName)->setValuesFromMap(temp);
    }

    QVariant buildStepsVariant = reader.restoreValue("buildsteps");
    if (buildStepsVariant.isValid()) {
        // restoring BuildSteps from settings
        int pos = 0;
        const QList<IBuildStepFactory *> buildStepFactories =
            ExtensionSystem::PluginManager::instance()->getObjects<IBuildStepFactory>();
        QStringList buildStepNames = buildStepsVariant.toStringList();
        foreach (const QString &buildStepName, buildStepNames) {
            foreach (IBuildStepFactory *factory, buildStepFactories) {
                if (factory->canCreate(buildStepName)) {
                    BuildStep *buildStep = factory->create(this, buildStepName);
                    insertBuildStep(pos, buildStep);
                    ++pos;
                    break;
                }
            }
        }

        QStringList cleanStepNames = reader.restoreValue("cleansteps").toStringList();
        // restoring BuildSteps from settings
        pos = 0;
        foreach (const QString &cleanStepName, cleanStepNames) {
            foreach (IBuildStepFactory *factory, buildStepFactories) {
                if (factory->canCreate(cleanStepName)) {
                    BuildStep *cleanStep = factory->create(this, cleanStepName);
                    insertCleanStep(pos, cleanStep);
                    ++pos;
                    break;
                }
            }
        }

        // restoring BuldConfigurations from settings



        // restore BuildSteps configuration
        int buildstepnr = 0;
        foreach (BuildStep *buildStep, buildSteps()) {
            QMap<QString, QVariant> buildConfiguration = reader.restoreValue("buildstep" + QString().setNum(buildstepnr)).toMap();
            buildStep->setValuesFromMap(buildConfiguration);
            ++buildstepnr;
        }

        foreach (const QString &buildConfigurationName, buildConfigurationNames) {
            buildstepnr = 0;
            foreach (BuildStep *buildStep, buildSteps()) {
                //get the buildconfiguration for this build step
                QMap<QString, QVariant> buildConfiguration =
                    reader.restoreValue("buildconfiguration-" + buildConfigurationName + "-buildstep" + QString().setNum(buildstepnr)).toMap();
                buildStep->setValuesFromMap(buildConfigurationName, buildConfiguration);
                ++buildstepnr;
            }
        }

        // restore CleanSteps configuration
        int cleanstepnr = 0;
        foreach (BuildStep *cleanStep, cleanSteps()) {
            QMap<QString, QVariant> buildConfiguration = reader.restoreValue("cleanstep" + QString().setNum(cleanstepnr)).toMap();
            cleanStep->setValuesFromMap(buildConfiguration);
            ++cleanstepnr;
        }

        foreach (const QString &buildConfigurationName, buildConfigurationNames) {
            cleanstepnr = 0;
            foreach (BuildStep *cleanStep, cleanSteps()) {
                //get the buildconfiguration for this clean step
                QMap<QString, QVariant> buildConfiguration =
                    reader.restoreValue("buildconfiguration-" + buildConfigurationName + "-cleanstep" + QString().setNum(cleanstepnr)).toMap();
                cleanStep->setValuesFromMap(buildConfigurationName, buildConfiguration);
                ++cleanstepnr;
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
            if (factory->canCreate(type)) {
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

BuildConfiguration * Project::getBuildConfiguration(const QString &name) const
{
    for (int i = 0; i != m_buildConfigurationValues.size(); ++i)
        if (m_buildConfigurationValues.at(i)->name() == name)
            return m_buildConfigurationValues.at(i);
    return 0;
}

void Project::setValue(const QString &buildConfiguration, const QString &name, const QVariant &value)
{
    BuildConfiguration *bc = getBuildConfiguration(buildConfiguration);
    Q_ASSERT(bc);
    bc->setValue(name, value);
}

QVariant Project::value(const QString &buildConfiguration, const QString &name) const
{
    BuildConfiguration *bc = getBuildConfiguration(buildConfiguration);
    if (bc)
        return bc->getValue(name);
    else
        return QVariant();
}

QString Project::activeBuildConfiguration() const
{
    return m_activeBuildConfiguration;
}

void Project::setActiveBuildConfiguration(const QString &config)
{
    if (m_activeBuildConfiguration != config && buildConfigurations().contains(config)) {
        m_activeBuildConfiguration = config;
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
}

void Project::removeRunConfiguration(QSharedPointer<RunConfiguration> runConfiguration)
{
    if(!m_runConfigurations.contains(runConfiguration)) {
        qWarning()<<"Not removing runConfiguration"<<runConfiguration->name()<<"becasue it doesn't exist";
        return;
    }
    m_runConfigurations.removeOne(runConfiguration);
    if (m_activeRunConfiguration == runConfiguration) {
        if (m_runConfigurations.isEmpty())
            setActiveRunConfiguration(QSharedPointer<RunConfiguration>(0));
        else
            setActiveRunConfiguration(m_runConfigurations.at(0));
    }
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

QString Project::displayNameFor(const QString &buildConfiguration)
{
    return getBuildConfiguration(buildConfiguration)->displayName();
}

void Project::setDisplayNameFor(const QString &buildConfiguration, const QString &displayName)
{
    QStringList displayNames;
    foreach (const QString &bc, buildConfigurations()) {
        if (bc != buildConfiguration)
            displayNames << displayNameFor(bc);
    }
    if (displayNames.contains(displayName)) {
        int i = 2;
        while (displayNames.contains(displayName + QString::number(i)))
            ++i;
        getBuildConfiguration(buildConfiguration)->setDisplayName(displayName + QString::number(i));
    } else {
        getBuildConfiguration(buildConfiguration)->setDisplayName(displayName);
    }
    emit buildConfigurationDisplayNameChanged(buildConfiguration);
}
