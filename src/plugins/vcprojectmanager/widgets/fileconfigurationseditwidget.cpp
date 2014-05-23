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
#include "fileconfigurationseditwidget.h"

#include <QVBoxLayout>
#include <utils/qtcassert.h>

#include "../interfaces/iattributecontainer.h"
#include "../interfaces/iattributedescriptiondataitem.h"
#include "../interfaces/iconfiguration.h"
#include "../interfaces/iconfigurationbuildtool.h"
#include "../interfaces/iconfigurationbuildtools.h"
#include "../interfaces/iconfigurations.h"
#include "../interfaces/ifile.h"
#include "../interfaces/ifiles.h"
#include "../interfaces/ifilecontainer.h"
#include "../interfaces/iplatform.h"
#include "../interfaces/iplatforms.h"
#include "../interfaces/itoolattribute.h"
#include "../interfaces/itoolattributecontainer.h"
#include "../interfaces/itooldescription.h"
#include "../interfaces/itools.h"
#include "../interfaces/itoolsection.h"
#include "../interfaces/itoolsectiondescription.h"
#include "../interfaces/isectioncontainer.h"
#include "../interfaces/ivisualstudioproject.h"

#include "../vcprojectmodel/configurationcontainer.h"
#include "../vcprojectmodel/tools/tool_constants.h"

#include "configurationswidget.h"
#include "configurationbasewidget.h"

namespace VcProjectManager {
namespace Internal {

FileConfigurationsEditWidget::FileConfigurationsEditWidget(IFile *file, IVisualStudioProject *project)
    : m_file(file),
      m_vsProject(project)
{
    QTC_ASSERT(m_vsProject, return);
    QTC_ASSERT(m_vsProject->configurations(), return);
    QTC_ASSERT(m_vsProject->configurations()->configurationContainer(), return);

    m_buildConfigurations = new ConfigurationContainer(*m_vsProject->configurations()->configurationContainer());
    m_configsWidget = new ConfigurationsWidget;
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    layout->addWidget(m_configsWidget);
    setLayout(layout);

    readFileBuildConfigurations();

    ConfigurationContainer *fileConfigContainer = m_fileConfigurations[m_file];

    for (int i = 0; i < fileConfigContainer->configurationCount(); ++i) {
        IConfiguration *config = fileConfigContainer->configuration(i);
        if (config)
            m_configsWidget->addConfiguration(config->fullName(), config->createSettingsWidget());
    }

    connect(m_configsWidget, SIGNAL(addNewConfigSignal(QString, QString)), this, SLOT(onAddNewConfig(QString, QString)));
    connect(m_configsWidget, SIGNAL(renameConfigSignal(QString,QString)), this, SLOT(onRenameConfig(QString, QString)));
    connect(m_configsWidget, SIGNAL(removeConfigSignal(QString)), this, SLOT(onRemoveConfig(QString)));
}

void FileConfigurationsEditWidget::saveData()
{
    ConfigurationContainer *newFileConfigContainer = m_fileConfigurations[m_file];

    QTC_ASSERT(newFileConfigContainer, return);

    for (int i = 0; i < newFileConfigContainer->configurationCount(); ++i) {
        VcNodeWidget *configWidget = m_configsWidget->configWidget(newFileConfigContainer->configuration(i)->fullName());
        if (configWidget)
            configWidget->saveData();
    }

    QMapIterator<IFile *, ConfigurationContainer *> it(m_fileConfigurations);

    while (it.hasNext()) {
        it.next();
        IFile *file = it.key();
        ConfigurationContainer *newConfigContainer = it.value();

        if (file && file->configurationContainer() && newConfigContainer)
            *file->configurationContainer() = *newConfigContainer;
    }

    QTC_ASSERT(m_vsProject, return);
    QTC_ASSERT(m_vsProject->configurations(), return);
    *m_vsProject->configurations()->configurationContainer() = *m_buildConfigurations;
}

void FileConfigurationsEditWidget::onAddNewConfig(QString newConfigName, QString copyFrom)
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

void FileConfigurationsEditWidget::onNewConfigAdded(IConfiguration *config)
{
    QTC_ASSERT(config, return);
    m_configsWidget->addConfiguration(config->fullName(), config->createSettingsWidget());
}

void FileConfigurationsEditWidget::onRenameConfig(QString newConfigName, QString oldConfigNameWithPlatform)
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

            QMapIterator<IFile *, ConfigurationContainer *> it(m_fileConfigurations);

            while (it.hasNext()) {
                it.next();
                config = it.value()->configuration(oldConfigName);
                if (config)
                    config->setFullName(newConfigNamePl);
            }
        }
    }
}

void FileConfigurationsEditWidget::onRemoveConfig(QString configNameWithPlatform)
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

void FileConfigurationsEditWidget::addConfigWidget(IConfiguration *config)
{
    QTC_ASSERT(config, return);
    m_configsWidget->addConfiguration(config->fullName(), config->createSettingsWidget());
}

void FileConfigurationsEditWidget::addConfigToProjectBuild(const QString &newConfigName, const QString &copyFrom)
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

void FileConfigurationsEditWidget::addConfigToFiles(const QString &newConfigName, const QString &copyFrom)
{
    QMapIterator<IFile *, ConfigurationContainer *> it(m_fileConfigurations);

    while (it.hasNext()) {
        it.next();
        ConfigurationContainer *container = it.value();

        if (container) {
            if (copyFrom.isEmpty()) {
                IConfiguration *config = m_vsProject->configurations()->configurationContainer()->configuration(newConfigName);
                leaveOnlyCppTool(config);
                container->addConfiguration(config);
            } else {
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
}

void FileConfigurationsEditWidget::readFileBuildConfigurations()
{
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

void FileConfigurationsEditWidget::readFileBuildConfigurations(IFileContainer *container)
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

void FileConfigurationsEditWidget::readFileBuildConfigurations(IFile *file)
{
    QTC_ASSERT(file, return);
    QTC_ASSERT(file->configurationContainer(), return);

    ConfigurationContainer *configCont = 0;

    if (file != m_file)
        configCont = new ConfigurationContainer(*(file->configurationContainer()));
    else {
        configCont = cloneFileConfigContainer(file);
        connect(configCont, SIGNAL(configurationAdded(IConfiguration*)), this, SLOT(addConfigWidget(IConfiguration*)));
    }

    m_fileConfigurations[file] = configCont;
}

ConfigurationContainer *FileConfigurationsEditWidget::cloneFileConfigContainer(IFile *file)
{
    QTC_ASSERT(file, return 0);

    ConfigurationContainer *configCont = new ConfigurationContainer(*(file->configurationContainer()));
    m_fileConfigurations[file] = configCont;

    // add any build configuration that does exist in project but not among file's build configurations
    for (int i = 0; i < m_buildConfigurations->configurationCount(); ++i) {
        IConfiguration *config = m_buildConfigurations->configuration(i);

        if (config) {
            IConfiguration *fileConfig = configCont->configuration(config->fullName());

            if (!fileConfig) {
                IConfiguration *newConfig = config->clone();
                leaveOnlyCppTool(newConfig);
                configCont->addConfiguration(newConfig);
            }
        }
    }

    return configCont;
}

void FileConfigurationsEditWidget::leaveOnlyCppTool(IConfiguration *config)
{
    QTC_ASSERT(config && config->tools() && config->tools()->configurationBuildTools(), return);

    int i = 0;
    while (config->tools()->configurationBuildTools()->toolCount() > 1)
    {
        IConfigurationBuildTool *tool = config->tools()->configurationBuildTools()->tool(i);

        if (tool->toolDescription()->toolKey() != QLatin1String(ToolConstants::strVCCLCompilerTool)) {
            config->tools()->configurationBuildTools()->removeTool(tool);
            delete tool;
        }

        else
            ++i;
    }
}

void FileConfigurationsEditWidget::cleanUpConfig(IConfiguration *config)
{
    QTC_ASSERT(config, return);

    VcNodeWidget *configWidget = m_configsWidget->configWidget(config->fullName());

    if (configWidget)
        configWidget->saveData();

    IConfiguration *projectConfig = m_vsProject->configurations()->configurationContainer()->configuration(config->fullName());

    if (projectConfig) {
        cleanUpConfigAttributes(config, projectConfig);
        cleanUpConfigTools(config, projectConfig);
    }
}

void FileConfigurationsEditWidget::cleanUpConfigAttributes(IConfiguration *config, IConfiguration *projectConfig)
{
    QTC_ASSERT(config && projectConfig, return);

    IAttributeContainer *configAttrCont = config->attributeContainer();
    IAttributeContainer *projConfigAttrCont = projectConfig->attributeContainer();

    if (configAttrCont) {
        for (int i = 0; i < configAttrCont->getAttributeCount();) {
            QString attrName = configAttrCont->getAttributeName(i);

            if (configAttrCont->attributeValue(attrName) == projConfigAttrCont->attributeValue(attrName))
                configAttrCont->removeAttribute(attrName);
            else
                ++i;
        }
    }

}

void FileConfigurationsEditWidget::cleanUpConfigTools(IConfiguration *config, IConfiguration *projectConfig)
{
    QTC_ASSERT(config && projectConfig, return);

    ITools *tools = config->tools();
    ITools *projectTools = projectConfig->tools();

    if (tools && projectTools && tools->configurationBuildTools() && projectTools->configurationBuildTools()) {
        IConfigurationBuildTool *tool = tools->configurationBuildTools()->tool(0);

        if (tool->toolDescription()) {
            IConfigurationBuildTool *projTool = projectTools->configurationBuildTools()->tool(tool->toolDescription()->toolKey());

            if (projTool)
                cleanUpConfigTool(tool, projTool);

            if (tool->allAttributesAreDefault())
                tools->configurationBuildTools()->removeTool(tool);
        }
    }
}

void FileConfigurationsEditWidget::cleanUpConfigTool(IConfigurationBuildTool *tool, IConfigurationBuildTool *projTool)
{
    QTC_ASSERT(tool && projTool, return);
    QTC_ASSERT(tool->sectionContainer() && projTool->sectionContainer(), return);

    ISectionContainer *secCont = tool->sectionContainer();
    ISectionContainer *projSecCont = projTool->sectionContainer();

    for (int i = 0; i < secCont->sectionCount(); ++i) {
        IToolSection *section = secCont->section(i);

        if (section && section->sectionDescription()) {
            IToolSection *projSection = projSecCont->section(section->sectionDescription()->displayName());

            if (projSection)
                cleanUpConfigToolSection(section, projSection);
        }
    }
}

void FileConfigurationsEditWidget::cleanUpConfigToolSection(IToolSection *toolSection, IToolSection *projToolSection)
{
    QTC_ASSERT(toolSection && projToolSection, return);
    IToolAttributeContainer *attrCont = toolSection->attributeContainer();
    IToolAttributeContainer *projAttrCont = projToolSection->attributeContainer();

    if (attrCont && projAttrCont) {
        for (int i = 0; i < attrCont->toolAttributeCount();) {
            IToolAttribute *attr = attrCont->toolAttribute(i);

            if (attr && attr->descriptionDataItem()) {
                IToolAttribute *projAttr = projAttrCont->toolAttribute(attr->descriptionDataItem()->key());

                if (projAttr && attr->value() == projAttr->value())
                    attrCont->removeToolAttribute(attr);
                else
                    ++i;
            }
            else
                ++i;
        }
    }
}

} // namespace Internal
} // namespace VcProjectManager
