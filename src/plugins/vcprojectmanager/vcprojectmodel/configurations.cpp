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
#include "../interfaces/iconfiguration.h"

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

    foreach (const IConfiguration *config, configs.m_configs)
        m_configs.append(config->clone());
}

Configurations &Configurations::operator =(const Configurations &configs)
{
    if (this != &configs) {
        m_vcProjDoc = configs.m_vcProjDoc;
        m_configs.clear();

        foreach (const IConfiguration *config, configs.m_configs)
            m_configs.append(config->clone());
    }

    return *this;
}

Configurations::~Configurations()
{
    qDeleteAll(m_configs);
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

    foreach (const IConfiguration *config, m_configs)
        configsNode.appendChild(config->toXMLDomNode(domXMLDocument));

    return configsNode;
}

void Configurations::addConfiguration(IConfiguration *config)
{
    if (m_configs.contains(config))
        return;

    // if there is already a configuration with the same name
    foreach (const IConfiguration *conf, m_configs) {
        if (config->fullName() == conf->fullName())
            return;
    }
    m_configs.append(config);
}

IConfiguration *Configurations::configuration(const QString &fullName) const
{
    foreach (IConfiguration *config, m_configs) {
        if (config->fullName() == fullName)
            return config;
    }
    return 0;
}

IConfiguration *Configurations::configuration(int index) const
{
    if (0 <= index && index < m_configs.size())
        return m_configs[index];
    return 0;
}

int Configurations::configurationCount() const
{
    return m_configs.size();
}

void Configurations::removeConfiguration(const QString &fullName)
{
    // if there is already a configuration with the same name
    foreach (IConfiguration *conf, m_configs) {
        if (conf->fullName() == fullName) {
            m_configs.removeOne(conf);
            delete conf;
            return;
        }
    }
}

bool Configurations::isEmpty() const
{
    return m_configs.isEmpty();
}

void Configurations::processConfiguration(const QDomNode &configurationNode)
{
    IConfiguration *config = 0;

    if (m_vcProjDoc->documentVersion() == VcDocConstants::DV_MSVC_2003)
        config = new Configuration2003(QLatin1String("Configuration"));
    else if (m_vcProjDoc->documentVersion() == VcDocConstants::DV_MSVC_2005)
        config = new Configuration2005(QLatin1String("Configuration"));
    else if (m_vcProjDoc->documentVersion() == VcDocConstants::DV_MSVC_2008)
        config = new Configuration2008(QLatin1String("Configuration"));

    if (config) {
        config->processNode(configurationNode);
        m_configs.append(config);
    }

    // process next sibling
    QDomNode nextSibling = configurationNode.nextSibling();
    if (!nextSibling.isNull())
        processConfiguration(nextSibling);
}

} // namespace Internal
} // namespace VcProjectManager
