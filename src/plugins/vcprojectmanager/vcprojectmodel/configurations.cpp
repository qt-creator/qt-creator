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
#include "configurations.h"

#include "configurationsfactory.h"
#include "vcprojectdocument.h"
#include "../widgets/configurationswidgets.h"

namespace VcProjectManager {
namespace Internal {

class ConfigurationsBaseWidget;

Configurations::Configurations(VcProjectDocument *vcProjDoc)
    : m_vcProjDoc(vcProjDoc)
{
}

Configurations::Configurations(const Configurations &configs)
{
    m_vcProjDoc = configs.m_vcProjDoc;

    foreach (const Configuration::Ptr &config, configs.m_configurations)
        m_configurations.append(config->clone());
}

Configurations &Configurations::operator =(const Configurations &configs)
{
    if (this != &configs) {
        m_vcProjDoc = configs.m_vcProjDoc;
        m_configurations.clear();

        foreach (const Configuration::Ptr &config, configs.m_configurations)
            m_configurations.append(config->clone());
    }

    return *this;
}

Configurations::~Configurations()
{
}

void Configurations::processNode(const QDomNode &node)
{
    if (node.isNull())
        return;

    if (node.hasChildNodes()) {
        QDomNode firstChild = node.firstChild();
        if (!firstChild.isNull())
            processConfiguration(firstChild);
    }
}

VcNodeWidget *Configurations::createSettingsWidget()
{
    ConfigurationsBaseWidget* widget = ConfigurationsFactory::createSettingsWidget(m_vcProjDoc, this);
    return widget;
}

QDomNode Configurations::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement configsNode = domXMLDocument.createElement(QLatin1String("Configurations"));

    foreach (const Configuration::Ptr &config, m_configurations)
        configsNode.appendChild(config->toXMLDomNode(domXMLDocument));

    return configsNode;
}

bool Configurations::isEmpty() const
{
    return m_configurations.isEmpty();
}

bool Configurations::appendConfiguration(Configuration::Ptr config)
{
    if (m_configurations.contains(config))
        return false;

    // if there is already a configuration with the same name
    foreach (const Configuration::Ptr &conf, m_configurations) {
        if (config->name() == conf->name())
            return false;
    }
    m_configurations.append(config);
    return true;
}

void Configurations::removeConfiguration(Configuration::Ptr config)
{
    m_configurations.removeAll(config);
}

Configuration::Ptr Configurations::configuration(const QString &configName)
{
    foreach (const Configuration::Ptr &config, m_configurations) {
        if (config->name() == configName)
            return config;
    }
    return Configuration::Ptr();
}

Configuration::Ptr Configurations::cloneConfiguration(const QString &newConfigName, const QString &configToClone)
{
    // don't add new configuration if some Configuration already has a name equal to newConfigName
    if (configuration(newConfigName))
        return Configuration::Ptr();

    foreach (const Configuration::Ptr &config, m_configurations) {
        if (config->name() == configToClone) {
            Configuration::Ptr clonedConfig = config->clone();
            clonedConfig->setName(newConfigName);
            m_configurations.append(clonedConfig);
            return clonedConfig;
        }
    }

    return Configuration::Ptr();
}

Configuration::Ptr Configurations::cloneConfiguration(const QString &newConfigName, Configuration::Ptr config)
{
    return cloneConfiguration(newConfigName, config->name());
}

QList<Configuration::Ptr> Configurations::configurations() const
{
    return m_configurations;
}

void Configurations::processConfiguration(const QDomNode &configurationNode)
{
    Configuration::Ptr configuration = ConfigurationsFactory::createConfiguration(m_vcProjDoc->documentVersion(), QLatin1String("Configuration"));
    configuration->processNode(configurationNode);
    m_configurations.append(configuration);

    // process next sibling
    QDomNode nextSibling = configurationNode.nextSibling();
    if (!nextSibling.isNull())
        processConfiguration(nextSibling);
}

} // namespace Internal
} // namespace VcProjectManager
