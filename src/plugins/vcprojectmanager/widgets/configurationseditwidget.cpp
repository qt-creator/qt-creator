/**************************************************************************
**
** Copyright (c) 2014 Bojan Petrovic
** Copyright (c) 2014 Radovan Zivkovic
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
#include "configurationseditwidget.h"

#include <QVBoxLayout>

#include "../vcprojectmodel/configurationcontainer.h"
#include "../interfaces/iconfigurationbuildtools.h"
#include "../interfaces/iconfigurationbuildtool.h"
#include "../interfaces/itools.h"
#include "../interfaces/iconfiguration.h"
#include "../interfaces/iconfigurations.h"
#include "../interfaces/iattributecontainer.h"
#include "../interfaces/iplatform.h"
#include "../interfaces/iplatforms.h"
#include "../interfaces/ifile.h"
#include "../interfaces/ifiles.h"
#include "../interfaces/ifilecontainer.h"
#include "../interfaces/ivisualstudioproject.h"
#include "../interfaces/itooldescription.h"
#include "../vcprojectmodel/configuration.h"
#include "../vcprojectmodel/tools/tool_constants.h"

#include "configurationswidget.h"
#include "../vcprojectmodel/tools/toolattributes/tooldescriptiondatamanager.h"

namespace VcProjectManager {
namespace Internal {

ConfigurationsEditWidget::ConfigurationsEditWidget(VcProjectManager::Internal::IVisualStudioProject *vsProj, ConfigurationContainer *configContainer)
    : m_vsProject(vsProj)
{
    m_configsWidget = new ConfigurationsWidget;
    m_buildConfigurations = new ConfigurationContainer(*(m_vsProject->configurations()->configurationContainer()));

    if (configContainer == m_vsProject->configurations()->configurationContainer()) {
        connect(m_buildConfigurations, SIGNAL(configurationAdded(IConfiguration*)), this, SLOT(addConfigWidget(IConfiguration*)));

        for (int i = 0; i < m_buildConfigurations->configurationCount(); ++i) {
            IConfiguration *config = m_buildConfigurations->configuration(i);
            if (config)
                m_configsWidget->addConfiguration(config->fullName(), config->createSettingsWidget());
        }
    }

    readFileBuildConfigurations();

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->addWidget(m_configsWidget);
    setLayout(layout);

    connect(m_configsWidget, SIGNAL(addNewConfigSignal(QString, QString)), this, SLOT(onAddNewConfig(QString, QString)));
    connect(m_configsWidget, SIGNAL(renameConfigSignal(QString,QString)), this, SLOT(onRenameConfig(QString, QString)));
    connect(m_configsWidget, SIGNAL(removeConfigSignal(QString)), this, SLOT(onRemoveConfig(QString)));
}

ConfigurationsEditWidget::~ConfigurationsEditWidget()
{
    delete m_buildConfigurations;

    QList<ConfigurationContainer *> fileConfigurations = m_fileConfigurations.values();
    qDeleteAll(fileConfigurations);
}

void ConfigurationsEditWidget::saveData()
{
    ConfigurationContainer *configContainer = m_vsProject->configurations()->configurationContainer();
    *configContainer = *m_buildConfigurations;

    QMapIterator<IFile *, ConfigurationContainer *> it(m_fileConfigurations);

    while (it.hasNext()) {
        it.next();
        IFile *file = it.key();
        ConfigurationContainer *newConfigCont = it.value();

        if (file && newConfigCont) {
            ConfigurationContainer *oldConfigContainer = file->configurationContainer();
            *oldConfigContainer = *newConfigCont;
        }
    }
}

void ConfigurationsEditWidget::onAddNewConfig(QString newConfigName, QString copyFrom)
{
    QTC_ASSERT(m_vsProject->platforms(), return);

    IPlatforms *platforms = m_vsProject->platforms();
    QString copyFromConfigName = copyFrom.split(QLatin1Char('|')).at(0);

    for (int i = 0; i < platforms->platformCount(); ++i) {
        IPlatform *platform = platforms->platform(i);

        if (platform) {
            QString newFullConfigName = newConfigName + QLatin1Char('|') + platform->displayName();
            QString copyFromFullConfigName;

            if (!copyFromConfigName.isEmpty())
                copyFromFullConfigName = copyFromConfigName + QLatin1Char('|') + platform->displayName();

            addConfigToProjectBuild(newFullConfigName, copyFromFullConfigName);
            addConfigToFiles(newFullConfigName, copyFromFullConfigName);
        }
    }
}

void ConfigurationsEditWidget::onNewConfigAdded(IConfiguration *config)
{
    QTC_ASSERT(config, return);
    m_configsWidget->addConfiguration(config->fullName(), config->createSettingsWidget());
}

void ConfigurationsEditWidget::onRenameConfig(QString newConfigName, QString oldConfigNameWithPlatform)
{
    QTC_ASSERT(m_vsProject->platforms(), return);

    IPlatforms *platforms = m_vsProject->platforms();
    QString copyFromConfigName = oldConfigNameWithPlatform.split(QLatin1Char('|')).at(0);

    for (int i = 0; i < platforms->platformCount(); ++i) {
        IPlatform *platform = platforms->platform(i);

        if (platform) {
            QString oldConfigName = copyFromConfigName + QLatin1Char('|') + platform->displayName();
            QString newConfigNamePl = newConfigName + QLatin1Char('|') + platform->displayName();
            IConfiguration *config = m_buildConfigurations->configuration(oldConfigName);

            if (config)
                config->setFullName(newConfigNamePl);

            QMapIterator<IFile*, ConfigurationContainer*> it(m_fileConfigurations);

            while (it.hasNext()) {
                it.next();
                config = it.value()->configuration(oldConfigName);
                if (config)
                    config->setFullName(newConfigNamePl);
            }
        }
    }
}

void ConfigurationsEditWidget::onRemoveConfig(QString configNameWithPlatform)
{
    QTC_ASSERT(m_vsProject->platforms(), return);

    IPlatforms *platforms = m_vsProject->platforms();
    QString copyFromConfigName = configNameWithPlatform.split(QLatin1Char('|')).at(0);

    for (int i = 0; i < platforms->platformCount(); ++i) {
        IPlatform *platform = platforms->platform(i);
        if (platform) {
            QString configName = copyFromConfigName + QLatin1Char('|') + platform->displayName();
            m_buildConfigurations->removeConfiguration(configName);
            m_configsWidget->removeConfiguration(configName);

            QMapIterator<IFile*, ConfigurationContainer*> it(m_fileConfigurations);

            while (it.hasNext()) {
                it.next();
                it.value()->removeConfiguration(configName);
            }
        }
    }
}

void ConfigurationsEditWidget::addConfigWidget(IConfiguration *config)
{
    QTC_ASSERT(config, return);
    m_configsWidget->addConfiguration(config->fullName(), config->createSettingsWidget());
}

void ConfigurationsEditWidget::readFileBuildConfigurations()
{
    QTC_ASSERT(m_vsProject, return);
    QTC_ASSERT(m_vsProject->files(), return);

    IFiles *files = m_vsProject->files();

    for (int i = 0; i < files->fileContainerCount(); ++i) {
        IFileContainer *fileContainer = files->fileContainer(i);
        readFileBuildConfigurations(fileContainer);
    }

    for (int i = 0; i < files->fileCount(); ++i) {
        IFile *file = files->file(i);
        readFileBuildConfigurations(file);
    }
}

void ConfigurationsEditWidget::readFileBuildConfigurations(IFileContainer *container)
{
    QTC_ASSERT(container, return);

    for (int i = 0; i < container->childCount(); ++i) {
        IFileContainer *fileContainer = container->fileContainer(i);
        readFileBuildConfigurations(fileContainer);
    }

    for (int i = 0; i < container->fileCount(); ++i) {
        IFile *file = container->file(i);
        readFileBuildConfigurations(file);
    }
}

void ConfigurationsEditWidget::readFileBuildConfigurations(IFile *file)
{
    QTC_ASSERT(file, return);
    QTC_ASSERT(file->configurationContainer(), return);

    ConfigurationContainer *configCont = new ConfigurationContainer(*file->configurationContainer());
    m_fileConfigurations[file] = configCont;
}

void ConfigurationsEditWidget::addConfigToProjectBuild(const QString &newConfigName, const QString &copyFrom)
{
    if (copyFrom.isEmpty()) {
        IConfiguration *newConfig = m_vsProject->createDefaultBuildConfiguration(newConfigName);
        if (newConfig)
            m_buildConfigurations->addConfiguration(newConfig);
    } else {
        IConfiguration *config = m_buildConfigurations->configuration(copyFrom);

        if (config) {
            IConfiguration *newConfig = config->clone();
            newConfig->setFullName(newConfigName);
            m_buildConfigurations->addConfiguration(newConfig);
        }
    }
}

void ConfigurationsEditWidget::addConfigToFiles(const QString &newConfigName, const QString &copyFrom)
{
    QMapIterator<IFile *, ConfigurationContainer *> it(m_fileConfigurations);

    while (it.hasNext()) {
        it.next();
        ConfigurationContainer *container = it.value();

        if (container) {
            IConfiguration *config = container->configuration(copyFrom);

            if (config) {
                IConfiguration *newConfig = config->clone();
                if (newConfig) {
                    newConfig->setFullName(newConfigName);
                    container->addConfiguration(newConfig);
                }
            }
        }
    }
}


} // Internal
} // VcProjectManager
