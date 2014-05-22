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
#include "configuration.h"
#include "configurationcontainer.h"
#include "configurations.h"
#include "vcprojectdocument.h"
#include "vcprojectdocument_constants.h"
#include "../widgets/configurationseditwidget.h"

#include <QDomNode>

namespace VcProjectManager {
namespace Internal {

class ConfigurationsBaseWidget;

Configurations::Configurations(VcProjectDocument *vcProjDoc)
    : m_vcProjDoc(vcProjDoc)
{
    m_configurationContainer = new ConfigurationContainer;
}

Configurations::Configurations(const Configurations &configs)
{
    m_configurationContainer = new ConfigurationContainer;
    *m_configurationContainer = *(configs.m_configurationContainer);
    m_vcProjDoc = configs.m_vcProjDoc;
}

Configurations &Configurations::operator =(const Configurations &configs)
{
    if (this != &configs) {
        m_vcProjDoc = configs.m_vcProjDoc;
        *m_configurationContainer = *(configs.m_configurationContainer);
    }

    return *this;
}

Configurations::~Configurations()
{
    delete m_configurationContainer;
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
    return new ConfigurationsEditWidget(m_vcProjDoc, m_configurationContainer);
}

QDomNode Configurations::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement configsNode = domXMLDocument.createElement(QLatin1String("Configurations"));
    m_configurationContainer->appendToXMLNode(configsNode, domXMLDocument);
    return configsNode;
}

ConfigurationContainer *Configurations::configurationContainer() const
{
    return m_configurationContainer;
}

void Configurations::processConfiguration(const QDomNode &configurationNode)
{
    IConfiguration *config = new Configuration(QLatin1String("Configuration"));
    config->processNode(configurationNode);
    m_configurationContainer->addConfiguration(config);

    // process next sibling
    QDomNode nextSibling = configurationNode.nextSibling();
    if (!nextSibling.isNull())
        processConfiguration(nextSibling);
}

} // namespace Internal
} // namespace VcProjectManager
