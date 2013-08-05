/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkvoic
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
#include "../vcprojectmodel/configuration.h"
#include "../vcprojectmodel/tools/toolfactory.h"
#include "configurationwidgets.h"

namespace VcProjectManager {
namespace Internal {

ConfigurationsBaseWidget::ConfigurationsBaseWidget(Configurations *configs, VcProjectDocument *vcProjDoc)
    : m_configs(configs),
      m_vcProjDoc(vcProjDoc)
{
    m_configsWidget = new ConfigurationsWidget;

    if (m_configs) {
        QList<Configuration::Ptr> configs = m_configs->configurations();

        foreach (const Configuration::Ptr &config, configs)
            addConfiguration(config.data());
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
        Configuration::Ptr foundConfig = m_configs->configuration(removeConfigName);
        if (foundConfig)
            m_configs->removeConfiguration(foundConfig);
    }

    // rename configurations that were renamed
    QMapIterator<Configuration::Ptr, QString> it(m_renamedConfigurations);

    while (it.hasNext()) {
        it.next();
        Configuration::Ptr config = it.key();
        config->setName(it.value());
    }

    // add new configurations
    foreach (const Configuration::Ptr &newConfig, m_newConfigurations)
        m_configs->appendConfiguration(newConfig);

    // save data for every configuration
    QList<ConfigurationBaseWidget *> configWidgets = m_configsWidget->configWidgets();
    foreach (ConfigurationBaseWidget *configWidget, configWidgets) {
        if (configWidget)
            configWidget->saveData();
    }
}

QList<Configuration::Ptr> ConfigurationsBaseWidget::newConfigurations() const
{
    return m_newConfigurations;
}

QList<QString> ConfigurationsBaseWidget::removedConfigurations() const
{
    return m_removedConfigurations;
}

QMap<Configuration::Ptr, QString> ConfigurationsBaseWidget::renamedConfigurations() const
{
    return m_renamedConfigurations;
}

void ConfigurationsBaseWidget::onAddNewConfig(QString newConfigName, QString copyFrom)
{
    Platforms::Ptr platforms = m_vcProjDoc->platforms();

    if (platforms && !newConfigName.isEmpty()) {
        if (copyFrom.isEmpty()) {
            QList<Platform::Ptr> platformList = platforms->platforms();
            foreach (const Platform::Ptr &platform, platformList) {
                Configuration::Ptr newConfig = createConfiguration(newConfigName + QLatin1Char('|') + platform->name());

                if (newConfig) {
                    newConfig->setAttribute(QLatin1String("OutputDirectory"), QLatin1String("$(SolutionDir)$(ConfigurationName)"));
                    newConfig->setAttribute(QLatin1String("IntermediateDirectory"), QLatin1String("$(ConfigurationName)"));
                    newConfig->setAttribute(QLatin1String("ConfigurationType"), QLatin1String("1"));
                    m_newConfigurations.append(newConfig);
                    addConfiguration(newConfig.data());
                }
            }
        } else {
            Configuration::Ptr config = m_configs->configuration(copyFrom);

            if (config) {
                QList<Platform::Ptr> platformList = platforms->platforms();

                foreach (const Platform::Ptr &platform, platformList) {
                    Configuration::Ptr newConfig = config->clone();

                    if (newConfig) {
                        newConfig->setName(newConfigName + QLatin1Char('|') + platform->name());
                        m_newConfigurations.append(newConfig);
                        addConfiguration(newConfig.data());
                    }
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

    QList<Platform::Ptr> platformList = platforms->platforms();
    foreach (const Platform::Ptr &platform, platformList) {
        QString targetConfigName = splits[0] + QLatin1Char('|') + platform->name();
        QString newName = newConfigName + QLatin1Char('|') + platform->name();
        Configuration::Ptr configInNew = configInNewConfigurations(targetConfigName);

        // if we are renaming newly added config
        if (configInNew) {
            configInNew->setName(newName);
        } else {
            // we are renaming a config that is already in the model
            bool targetAlreadyExists = false;
            QMapIterator<Configuration::Ptr, QString> it(m_renamedConfigurations);

            while (it.hasNext()) {
                it.next();

                if (it.value() == targetConfigName) {
                    Configuration::Ptr key = m_renamedConfigurations.key(targetConfigName);

                    if (key) {
                        m_renamedConfigurations.insert(key, newName);
                        targetAlreadyExists = true;
                        break;
                    }
                }
            }

            if (!targetAlreadyExists) {
                Configuration::Ptr config = m_configs->configuration(targetConfigName);
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

    QList<Platform::Ptr> platformList = platforms->platforms();
    foreach (const Platform::Ptr &platform, platformList) {
        QString targetConfigName = splits[0] + QLatin1Char('|') + platform->name();
        Configuration::Ptr config = m_configs->configuration(targetConfigName);

        // if config exists in the document model, add it to remove list
        if (config) {
            removeConfiguration(config.data());
            m_removedConfigurations.append(config->name());
        } else {
            // else remove it from the list of newly added configurations
            foreach (const Configuration::Ptr &configPtr, m_newConfigurations) {
                if (configPtr && configPtr->name() == targetConfigName) {
                    removeConfiguration(configPtr.data());
                    m_newConfigurations.removeAll(configPtr);
                    break;
                }
            }
        }
    }
}

void ConfigurationsBaseWidget::addConfiguration(Configuration *config)
{
    if (config)
        m_configsWidget->addConfiguration(config->nodeWidgetName(), config->createSettingsWidget());
}

void ConfigurationsBaseWidget::removeConfiguration(Configuration *config)
{
    if (config)
        m_configsWidget->removeConfiguration(config->name());
}

Configuration::Ptr ConfigurationsBaseWidget::createConfiguration(const QString &configNameWithPlatform) const
{
    Configuration::Ptr config = ConfigurationsFactory::createConfiguration(m_vcProjDoc->documentVersion(), QLatin1String("Configuration"));
    config->setName(configNameWithPlatform);

    Tool::Ptr tool = ToolFactory::createTool(QLatin1String("VCPreBuildEventTool"));
    config->addTool(tool);

    tool = ToolFactory::createTool(QLatin1String("VCCustomBuildTool"));
    config->addTool(tool);

    tool = ToolFactory::createTool(QLatin1String("VCXMLDataGeneratorTool"));
    config->addTool(tool);

    tool = ToolFactory::createTool(QLatin1String("VCWebServiceProxyGeneratorTool"));
    config->addTool(tool);

    tool = ToolFactory::createTool(QLatin1String("VCMIDLTool"));
    config->addTool(tool);

    tool = ToolFactory::createTool(QLatin1String("VCCLCompilerTool"));
    config->addTool(tool);

    tool = ToolFactory::createTool(QLatin1String("VCManagedResourceCompilerTool"));
    config->addTool(tool);

    tool = ToolFactory::createTool(QLatin1String("VCResourceCompilerTool"));
    config->addTool(tool);

    tool = ToolFactory::createTool(QLatin1String("VCPreLinkEventTool"));
    config->addTool(tool);

    tool = ToolFactory::createTool(QLatin1String("VCLinkerTool"));
    config->addTool(tool);

    tool = ToolFactory::createTool(QLatin1String("VCALinkTool"));
    config->addTool(tool);

    tool = ToolFactory::createTool(QLatin1String("VCManifestTool"));
    config->addTool(tool);

    tool = ToolFactory::createTool(QLatin1String("VCXDCMakeTool"));
    config->addTool(tool);

    tool = ToolFactory::createTool(QLatin1String("VCBscMakeTool"));
    config->addTool(tool);

    tool = ToolFactory::createTool(QLatin1String("VCFxCopTool"));
    config->addTool(tool);

    tool = ToolFactory::createTool(QLatin1String("VCAppVerifierTool"));
    config->addTool(tool);

    tool = ToolFactory::createTool(QLatin1String("VCPostBuildEventTool"));
    config->addTool(tool);

    return config;
}

Configuration::Ptr ConfigurationsBaseWidget::configInNewConfigurations(const QString &configNameWithPlatform) const
{
    foreach (const Configuration::Ptr &config, m_newConfigurations) {
        if (config && config->name() == configNameWithPlatform)
            return config;
    }

    return Configuration::Ptr();
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
