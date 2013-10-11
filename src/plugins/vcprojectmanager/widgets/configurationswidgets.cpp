/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkovic
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
#include "configurationswidgets.h"

#include <QVBoxLayout>

#include "configurationswidget.h"
#include "../vcprojectmodel/configurations.h"
#include "../vcprojectmodel/vcprojectdocument.h"
#include "../vcprojectmodel/configurationsfactory.h"
#include "../vcprojectmodel/configurationcontainer.h"
#include "../vcprojectmodel/configuration.h"
#include "../vcprojectmodel/tools/toolattributes/tooldescription.h"
#include "../vcprojectmodel/tools/toolattributes/tooldescriptiondatamanager.h"
#include "configurationwidgets.h"
#include "../vcprojectmodel/files.h"
#include "../vcprojectmodel/file.h"
#include "../interfaces/iattributecontainer.h"
#include "../interfaces/iconfigurationbuildtools.h"
#include "../interfaces/itools.h"

namespace VcProjectManager {
namespace Internal {

ConfigurationsBaseWidget::ConfigurationsBaseWidget(Configurations *configs, VcProjectDocument *vcProjDoc)
    : m_configs(configs),
      m_vcProjDoc(vcProjDoc)
{
    m_configsWidget = new ConfigurationsWidget;

    if (m_configs) {
        for (int i = 0; i < m_configs->configurationContainer()->configurationCount(); ++i) {
            IConfiguration *config = m_configs->configurationContainer()->configuration(i);
            if (config)
                addConfiguration(config);
        }
    }

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->addWidget(m_configsWidget);
    setLayout(layout);

    connect(m_configsWidget, SIGNAL(addNewConfigSignal(QString, QString)), this, SLOT(onAddNewConfig(QString, QString)));
    connect(m_configsWidget, SIGNAL(renameConfigSignal(QString,QString)), this, SLOT(onRenameConfig(QString, QString)));
    connect(m_configsWidget, SIGNAL(removeConfigSignal(QString)), this, SLOT(onRemoveConfig(QString)));
}

ConfigurationsBaseWidget::~ConfigurationsBaseWidget()
{
}

void ConfigurationsBaseWidget::saveData()
{
    // remove deleted configurations
    foreach (const QString &removeConfigName, m_removedConfigurations) {
        IConfiguration *foundConfig = m_configs->configurationContainer()->configuration(removeConfigName);
        if (foundConfig)
            m_configs->configurationContainer()->removeConfiguration(foundConfig->fullName());
    }

    // rename configurations that were renamed
    QMapIterator<IConfiguration*, QString> it(m_renamedConfigurations);

    while (it.hasNext()) {
        it.next();
        IConfiguration *config = it.key();
        config->setFullName(it.value());
    }

    // add new configurations
    foreach (IConfiguration *newConfig, m_newConfigurations)
        m_configs->configurationContainer()->addConfiguration(newConfig);

    QHashIterator<IFile*, QList<IConfiguration*> > fileConfigIt(m_newFilesConfigurations);

    while (fileConfigIt.hasNext()) {
        fileConfigIt.next();

        foreach (IConfiguration *newFileConfig, fileConfigIt.value())
            fileConfigIt.key()->configurationContainer()->addConfiguration(newFileConfig);
    }

    // save data for every configuration
    QList<ConfigurationBaseWidget *> configWidgets = m_configsWidget->configWidgets();
    foreach (ConfigurationBaseWidget *configWidget, configWidgets) {
        if (configWidget)
            configWidget->saveData();
    }
}

void ConfigurationsBaseWidget::onAddNewConfig(QString newConfigName, QString copyFrom)
{
    Platforms::Ptr platforms = m_vcProjDoc->platforms();

    if (platforms && !newConfigName.isEmpty()) {
        if (copyFrom.isEmpty()) {
            for (int i = 0; i < platforms->platformCount(); ++i) {
                IPlatform *platform = platforms->platform(i);
                IConfiguration *newConfig = createConfiguration(newConfigName + QLatin1Char('|') + platform->displayName());

                if (newConfig) {
                    newConfig->attributeContainer()->setAttribute(QLatin1String("OutputDirectory"), QLatin1String("$(SolutionDir)$(ConfigurationName)"));
                    newConfig->attributeContainer()->setAttribute(QLatin1String("IntermediateDirectory"), QLatin1String("$(ConfigurationName)"));
                    newConfig->attributeContainer()->setAttribute(QLatin1String("ConfigurationType"), QLatin1String("1"));
                    m_newConfigurations.append(newConfig);
                    addConfiguration(newConfig);
                }
            }
        } else {
            IConfiguration *config = m_configs->configurationContainer()->configuration(copyFrom);

            if (config) {
                for (int i = 0; i < platforms->platformCount(); ++i) {
                    IPlatform *platform = platforms->platform(i);
                    IConfiguration* newConfig = config->clone();

                    if (newConfig) {
                        newConfig->setFullName(newConfigName + QLatin1Char('|') + platform->displayName());
                        m_newConfigurations.append(newConfig);
                        addConfiguration(newConfig);
                    }

                    addConfigurationToFiles(copyFrom, newConfigName + QLatin1Char('|') + platform->displayName());
                }
            }
        }
    }
}

void ConfigurationsBaseWidget::onRenameConfig(QString newConfigName, QString oldConfigNameWithPlatform)
{
    Platforms::Ptr platforms = m_vcProjDoc->platforms();

    if (!platforms || newConfigName.isEmpty() || oldConfigNameWithPlatform.isEmpty())
        return;

    QStringList splits = oldConfigNameWithPlatform.split(QLatin1Char('|'));

    if (splits.isEmpty())
        return;

    for (int i = 0; i < platforms->platformCount(); ++i) {
        IPlatform *platform = platforms->platform(i);
        QString targetConfigName = splits[0] + QLatin1Char('|') + platform->displayName();
        QString newName = newConfigName + QLatin1Char('|') + platform->displayName();
        IConfiguration *configInNew = configInNewConfigurations(targetConfigName);

        // if we are renaming newly added config
        if (configInNew) {
            configInNew->setFullName(newName);
        } else {
            // we are renaming a config that is already in the model
            bool targetAlreadyExists = false;
            QMapIterator<IConfiguration*, QString> it(m_renamedConfigurations);

            while (it.hasNext()) {
                it.next();

                if (it.value() == targetConfigName) {
                    IConfiguration* key = m_renamedConfigurations.key(targetConfigName);

                    if (key) {
                        m_renamedConfigurations.insert(key, newName);
                        targetAlreadyExists = true;
                        break;
                    }
                }
            }

            if (!targetAlreadyExists) {
                IConfiguration *config = m_configs->configurationContainer()->configuration(targetConfigName);
                if (config)
                    m_renamedConfigurations.insert(config, newName);
            }
        }

        m_configsWidget->renameConfiguration(newName, targetConfigName);
    }
}

void ConfigurationsBaseWidget::onRemoveConfig(QString configNameWithPlatform)
{
    Platforms::Ptr platforms = m_vcProjDoc->platforms();

    if (!platforms || configNameWithPlatform.isEmpty())
        return;

    QStringList splits = configNameWithPlatform.split(QLatin1Char('|'));

    if (splits.isEmpty())
        return;

    for (int i = 0; i < platforms->platformCount(); ++i) {
        IPlatform *platform = platforms->platform(i);
        QString targetConfigName = splits[0] + QLatin1Char('|') + platform->displayName();
        IConfiguration *config = m_configs->configurationContainer()->configuration(targetConfigName);

        // if config exists in the document model, add it to remove list
        if (config) {
            removeConfiguration(config);
            m_removedConfigurations.append(config->fullName());
        } else {
            // else remove it from the list of newly added configurations
            foreach (IConfiguration *configPtr, m_newConfigurations) {
                if (configPtr && configPtr->fullName() == targetConfigName) {
                    removeConfiguration(configPtr);
                    m_newConfigurations.removeAll(configPtr);
                    break;
                }
            }
        }
    }
}

void ConfigurationsBaseWidget::addConfiguration(IConfiguration *config)
{
    if (config)
        m_configsWidget->addConfiguration(config->fullName(), config->createSettingsWidget());
}

void ConfigurationsBaseWidget::removeConfiguration(IConfiguration *config)
{
    if (config)
        m_configsWidget->removeConfiguration(config->fullName());
}

IConfiguration *ConfigurationsBaseWidget::createConfiguration(const QString &configNameWithPlatform) const
{
    IConfiguration *config = new Configuration(QLatin1String("Configuration"));
    config->setFullName(configNameWithPlatform);

    ToolDescriptionDataManager *tDDM = ToolDescriptionDataManager::instance();

    if (tDDM) {
        for (int i = 0; i < tDDM->toolDescriptionCount(); ++i) {
            ToolDescription *toolDesc = tDDM->toolDescription(i);

            if (toolDesc) {
                IConfigurationBuildTool *configTool = toolDesc->createTool();

                if (configTool)
                    config->tools()->configurationBuildTools()->addTool(configTool);
            }
        }
    }

    return config;
}

IConfiguration *ConfigurationsBaseWidget::configInNewConfigurations(const QString &configNameWithPlatform) const
{
    foreach (IConfiguration *config, m_newConfigurations) {
        if (config && config->fullName() == configNameWithPlatform)
            return config;
    }

    return 0;
}

void ConfigurationsBaseWidget::addConfigurationToFiles(const QString &copyFromConfig, const QString &targetConfigName)
{
    Files::Ptr docFiles = m_vcProjDoc->files();
    if (docFiles) {

        for (int i = 0; i < docFiles->fileContainerCount(); ++i) {
            IFileContainer *fileContainer = docFiles->fileContainer(i);
            if (fileContainer)
                addConfigurationToFilesInFilter(fileContainer, copyFromConfig, targetConfigName);
        }

        for (int i = 0; i < docFiles->fileCount(); ++i) {
            IFile *file = docFiles->file(i);
            if (file)
                addConfigurationToFile(file, copyFromConfig, targetConfigName);
        }
    }
}

void ConfigurationsBaseWidget::addConfigurationToFilesInFilter(IFileContainer *filterPtr, const QString &copyFromConfig, const QString &targetConfigName)
{
    for (int i = 0; i < filterPtr->childCount(); ++i) {
        IFileContainer *fileContainer = filterPtr->fileContainer(i);
        if (fileContainer)
            addConfigurationToFilesInFilter(fileContainer, copyFromConfig, targetConfigName);
    }

    for (int i = 0; i < filterPtr->fileCount(); ++i) {
        IFile *file = filterPtr->file(i);
        if (file)
            addConfigurationToFile(file, copyFromConfig, targetConfigName);
    }
}

void ConfigurationsBaseWidget::addConfigurationToFile(IFile *filePtr, const QString &copyFromConfig, const QString &targetConfigName)
{
    IConfiguration *configPtr = filePtr->configurationContainer()->configuration(copyFromConfig);

    if (configPtr) {
        IConfiguration *newConfig = configPtr->clone();
        newConfig->setFullName(targetConfigName);
        m_newFilesConfigurations[filePtr].append(newConfig);
    }
}

Configurations2003Widget::Configurations2003Widget(Configurations *configs, VcProjectDocument *vcProjDoc)
    : ConfigurationsBaseWidget(configs, vcProjDoc)
{
}

Configurations2003Widget::~Configurations2003Widget()
{
}


Configurations2005Widget::Configurations2005Widget(Configurations *configs, VcProjectDocument *vcProjDoc)
    : ConfigurationsBaseWidget(configs, vcProjDoc)
{
}

Configurations2005Widget::~Configurations2005Widget()
{
}


Configurations2008Widget::Configurations2008Widget(Configurations *configs, VcProjectDocument *vcProjDoc)
    : ConfigurationsBaseWidget(configs, vcProjDoc)
{
}

Configurations2008Widget::~Configurations2008Widget()
{
}

} // namespace Internal
} // namespace VcProjectManager
