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
#include "projectreference.h"
#include "configurationcontainer.h"
#include "generalattributecontainer.h"
#include "vcprojectdocument_constants.h"
#include "configuration.h"

namespace VcProjectManager {
namespace Internal {

ProjectReference::ProjectReference()
{
    m_configurations = new ConfigurationContainer;
    m_attributeContainer = new GeneralAttributeContainer;
}

ProjectReference::ProjectReference(const ProjectReference &projRef)
{
    m_configurations = new ConfigurationContainer;
    m_attributeContainer = new GeneralAttributeContainer;

    *m_attributeContainer = *projRef.m_attributeContainer;
    *m_configurations = *projRef.m_configurations;
}

ProjectReference &ProjectReference::operator =(const ProjectReference &projRef)
{
    if (this != &projRef) {
        *m_attributeContainer = *projRef.m_attributeContainer;
        *m_configurations = *projRef.m_configurations;
    }
    return *this;
}

ProjectReference::~ProjectReference()
{
    delete m_attributeContainer;
    delete m_configurations;
}

void ProjectReference::processNode(const QDomNode &node)
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

VcNodeWidget *ProjectReference::createSettingsWidget()
{
    return 0;
}

QDomNode ProjectReference::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement projRefNode = domXMLDocument.createElement(QLatin1String("ProjectReference"));

    m_attributeContainer->appendToXMLNode(projRefNode);
    m_configurations->appendToXMLNode(projRefNode, domXMLDocument);

    return projRefNode;
}

IAttributeContainer *ProjectReference::attributeContainer() const
{
    return m_attributeContainer;
}

ConfigurationContainer *ProjectReference::configurationContainer() const
{
    return m_configurations;
}

QString ProjectReference::type() const
{
    return QLatin1String(VcDocConstants::PROJECT_REFERENCE);
}

IReference *ProjectReference::clone() const
{
    return new ProjectReference(*this);
}

void ProjectReference::processReferenceConfig(const QDomNode &referenceConfig)
{
    IConfiguration *referenceConfiguration = new Configuration(QLatin1String("ReferenceConfiguration"));
    referenceConfiguration->processNode(referenceConfig);
    m_configurations->addConfiguration(referenceConfiguration);

    // process next sibling
    QDomNode nextSibling = referenceConfig.nextSibling();
    if (!nextSibling.isNull())
        processReferenceConfig(nextSibling);
}

void ProjectReference::processNodeAttributes(const QDomElement &element)
{
    QDomNamedNodeMap namedNodeMap = element.attributes();

    for (int i = 0; i < namedNodeMap.size(); ++i) {
        QDomNode domNode = namedNodeMap.item(i);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domElement = domNode.toAttr();

            if (domElement.name() == QLatin1String(VcDocConstants::PROJECT_REFERENCE_COPY_LOCAL) ||
                    domElement.name() == QLatin1String(VcDocConstants::PROJECT_REFERENCE_COPY_LOCAL_DEPENDENCIES) ||
                    domElement.name() == QLatin1String(VcDocConstants::PROJECT_REFERENCE_COPY_LOCAL_SATELITE_ASSEMBLIES) ||
                    domElement.name() == QLatin1String(VcDocConstants::PROJECT_REFERENCE_NAME) ||
                    domElement.name() == QLatin1String(VcDocConstants::PROJECT_REFERENCE_REFERENCED_PROJECT_IDENTIFIER) ||
                    domElement.name() == QLatin1String(VcDocConstants::PROJECT_REFERENCE_RELATIVE_PATH_FROM_SOLUTION) ||
                    domElement.name() == QLatin1String(VcDocConstants::PROJECT_REFERENCE_RELATIVE_PATH_TO_PROJECT) ||
                    domElement.name() == QLatin1String(VcDocConstants::PROJECT_REFERENCE_USE_DEPENDENCIES_IN_BUILD) ||
                    domElement.name() == QLatin1String(VcDocConstants::PROJECT_REFERENCE_USE_IN_BUILD))
                m_attributeContainer->setAttribute(domElement.name(), domElement.value());
        }
    }
}

} // namespace Internal
} // namespace VcProjectManager
