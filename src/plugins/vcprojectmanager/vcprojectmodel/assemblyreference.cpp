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
#include "assemblyreference.h"
#include "configuration.h"
#include "configurationcontainer.h"
#include "generalattributecontainer.h"
#include "vcprojectdocument_constants.h"

#include <QDomNode>
#include <QVariant>

namespace VcProjectManager {
namespace Internal {

AssemblyReference::AssemblyReference()
{
    m_attributeContainer = new GeneralAttributeContainer;
    m_configurations = new ConfigurationContainer;
}

AssemblyReference::AssemblyReference(const AssemblyReference &asmRef)
{
    m_attributeContainer = new GeneralAttributeContainer;
    m_configurations = new ConfigurationContainer;

    *m_attributeContainer = *asmRef.m_attributeContainer;
    *m_configurations = *asmRef.m_configurations;

    for (int i = 0; i < asmRef.m_configurations->configurationCount(); ++i) {
        IConfiguration *config = asmRef.m_configurations->configuration(i);
        if (config)
            m_configurations->addConfiguration(config->clone());
    }
}

AssemblyReference &AssemblyReference::operator =(const AssemblyReference &asmRef)
{
    if (this != &asmRef) {
        *m_attributeContainer = *asmRef.m_attributeContainer;
        *m_configurations = *asmRef.m_configurations;

        for (int i = 0; i < asmRef.m_configurations->configurationCount(); ++i) {
            IConfiguration *config = asmRef.m_configurations->configuration(i);
            if (config)
                m_configurations->addConfiguration(config->clone());
        }
    }

    return *this;
}

AssemblyReference::~AssemblyReference()
{
    delete m_attributeContainer;
    delete m_configurations;
}

void AssemblyReference::processNode(const QDomNode &node)
{
    if (node.isNull())
        return;

    if (node.nodeType() == QDomNode::ElementNode)
        processNodeAttributes(node.toElement());

    if (node.hasChildNodes()) {
        QDomNode firstChild = node.firstChild();
        if (!firstChild.isNull())
            processReferenceConfig(firstChild);
    }
}

VcNodeWidget *AssemblyReference::createSettingsWidget()
{
    return 0;
}

QDomNode AssemblyReference::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement assemblyRefNode = domXMLDocument.createElement(QLatin1String("AssemblyReference"));
    m_configurations->appendToXMLNode(assemblyRefNode, domXMLDocument);
    m_attributeContainer->appendToXMLNode(assemblyRefNode);
    return assemblyRefNode;
}

IAttributeContainer *AssemblyReference::attributeContainer() const
{
    return m_attributeContainer;
}

ConfigurationContainer *AssemblyReference::configurationContainer() const
{
    return m_configurations;
}

QString AssemblyReference::type() const
{
    return QLatin1String(VcDocConstants::ASSEMBLY_REFERENCE);
}

IReference *AssemblyReference::clone() const
{
    return new AssemblyReference(*this);
}

void AssemblyReference::processNodeAttributes(const QDomElement &element)
{
    QDomNamedNodeMap namedNodeMap = element.attributes();

    if (namedNodeMap.size() == 1) {
        QDomNode domNode = namedNodeMap.item(0);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domElement = domNode.toAttr();

            if (domElement.name() == QLatin1String(VcDocConstants::ASSEMBLY_REFERENCE_ASSEMBLY_NAME) ||
                    domElement.name() == QLatin1String(VcDocConstants::ASSEMBLY_REFERENCE_COPY_LOCAL) ||
                    domElement.name() == QLatin1String(VcDocConstants::ASSEMBLY_REFERENCE_COPY_LOCAL_DEPENDENCIES) ||
                    domElement.name() == QLatin1String(VcDocConstants::ASSEMBLY_REFERENCE_COPY_LOCAL_SATELITE_ASSEMBLIES) ||
                    domElement.name() == QLatin1String(VcDocConstants::ASSEMBLY_REFERENCE_MIN_FRAMEWORK_VERSION) ||
                    domElement.name() == QLatin1String(VcDocConstants::ASSEMBLY_REFERENCE_RELATIVE_PATH) ||
                    domElement.name() == QLatin1String(VcDocConstants::ASSEMBLY_REFERENCE_SUB_TYPE) ||
                    domElement.name() == QLatin1String(VcDocConstants::ASSEMBLY_REFERENCE_USE_DEPENDENCIES_IN_BUILD) ||
                    domElement.name() == QLatin1String(VcDocConstants::ASSEMBLY_REFERENCE_USE_IN_BUILD))
                m_attributeContainer->setAttribute(domElement.name(), domElement.value());
        }
    }
}

void AssemblyReference::processReferenceConfig(const QDomNode &referenceConfig)
{
    IConfiguration *referenceConfiguration = new Configuration(QLatin1String("ReferenceConfiguration"));
    referenceConfiguration->processNode(referenceConfig);
    m_configurations->addConfiguration(referenceConfiguration);

    // process next sibling
    QDomNode nextSibling = referenceConfig.nextSibling();
    if (!nextSibling.isNull())
        processReferenceConfig(nextSibling);
}

} // namespace Internal
} // namespace VcProjectManager
