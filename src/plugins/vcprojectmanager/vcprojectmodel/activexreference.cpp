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
#include "activexreference.h"

#include <QVariant>
#include "configurationsfactory.h"
#include "generalattributecontainer.h"
#include "configurationcontainer.h"

namespace VcProjectManager {
namespace Internal {

ActiveXReference::ActiveXReference()
{
}

ActiveXReference::ActiveXReference(const ActiveXReference &ref)
{
    m_attributeContainer = new GeneralAttributeContainer;
    m_configurations = new ConfigurationContainer;
    *m_attributeContainer = *ref.m_attributeContainer;
    *m_configurations = *ref.m_configurations;
}

IReference &ActiveXReference::operator =(const IReference &ref)
{
    if (this != &ref) {
        *m_attributeContainer = *ref.attributeContainer();
        *m_configurations = *ref.configurations();
    }

    return *this;
}

ActiveXReference::~ActiveXReference()
{
    delete m_attributeContainer;
    delete m_configurations;
}

void ActiveXReference::processNode(const QDomNode &node)
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

void ActiveXReference::processNodeAttributes(const QDomElement &element)
{
    QDomNamedNodeMap namedNodeMap = element.attributes();

    for (int i = 0; i < namedNodeMap.size(); ++i) {
        QDomNode domNode = namedNodeMap.item(i);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domElement = domNode.toAttr();

            if (domElement.name() == QLatin1String(VcDocConstants::ACTIVEX_REFERENCE_CONTROL_GUID) ||
                    domElement.name() == QLatin1String(VcDocConstants::ACTIVEX_REFERENCE_CONTROL_VERSION) ||
                    domElement.name() == QLatin1String(VcDocConstants::ACTIVEX_REFERENCE_WRAPPER_TOOL) ||
                    domElement.name() == QLatin1String(VcDocConstants::ACTIVEX_REFERENCE_LOCAL_ID) ||
                    domElement.name() == QLatin1String(VcDocConstants::ACTIVEX_REFERENCE_USE_IN_BUILD) ||
                    domElement.name() == QLatin1String(VcDocConstants::ACTIVEX_REFERENCE_COPY_LOCAL) ||
                    domElement.name() == QLatin1String(VcDocConstants::ACTIVEX_REFERENCE_COPY_LOCAL_DEPENDENCIES) ||
                    domElement.name() == QLatin1String(VcDocConstants::ACTIVEX_REFERENCE_COPY_LOCAL_SATELITE_ASSEMBLIES) ||
                    domElement.name() == QLatin1String(VcDocConstants::ACTIVEX_REFERENCE_USE_DEPENDENCIES_IN_BUILD))
                m_attributeContainer->setAttribute(domElement.name(), domElement.value());
        }
    }
}

VcNodeWidget *ActiveXReference::createSettingsWidget()
{
    // TODO(Radovan): Finish implementation
    return 0;
}

QDomNode ActiveXReference::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement activeXNode = domXMLDocument.createElement(QLatin1String("ActiveXReference"));
    m_attributeContainer->appendToXMLNode(activeXNode);
    m_configurations->appendToXMLNode(activeXNode, domXMLDocument);

    return activeXNode;
}

IAttributeContainer *ActiveXReference::attributeContainer() const
{
    return m_attributeContainer;
}

ConfigurationContainer *ActiveXReference::configurations() const
{
    return m_configurations;
}

QString ActiveXReference::type() const
{
    return QLatin1String(VcDocConstants::ACTIVEX_REFERENCE);
}

void ActiveXReference::processReferenceConfig(const QDomNode &referenceConfig)
{
    IConfiguration *referenceConfiguration = createReferenceConfiguration();
    referenceConfiguration->processNode(referenceConfig);
    m_configurations->addConfiguration(referenceConfiguration);

    // process next sibling
    QDomNode nextSibling = referenceConfig.nextSibling();
    if (!nextSibling.isNull())
        processReferenceConfig(nextSibling);
}

ActiveXReference2003::ActiveXReference2003(const ActiveXReference2003 &ref)
    : ActiveXReference(ref)
{
}

ActiveXReference2003::~ActiveXReference2003()
{
}

ActiveXReference::Ptr ActiveXReference2003::clone() const
{
    return ActiveXReference::Ptr(new ActiveXReference2003(*this));
}

ActiveXReference2003::ActiveXReference2003()
{
}

IConfiguration* ActiveXReference2003::createReferenceConfiguration() const
{
    return new Configuration2003(QLatin1String("ReferenceConfiguration"));
}


ActiveXReference2005::ActiveXReference2005(const ActiveXReference2005 &ref)
    : ActiveXReference2003(ref)
{
}

ActiveXReference2005::~ActiveXReference2005()
{
}

ActiveXReference::Ptr ActiveXReference2005::clone() const
{
    return ActiveXReference::Ptr(new ActiveXReference2005(*this));
}

ActiveXReference2005::ActiveXReference2005()
{
}

IConfiguration* ActiveXReference2005::createReferenceConfiguration() const
{
    return new Configuration2005(QLatin1String("ReferenceConfiguration"));
}


ActiveXReference2008::ActiveXReference2008(const ActiveXReference2008 &ref)
    : ActiveXReference2005(ref)
{
}

ActiveXReference2008::~ActiveXReference2008()
{
}

ActiveXReference::Ptr ActiveXReference2008::clone() const
{
    return ActiveXReference::Ptr(new ActiveXReference2005(*this));
}

ActiveXReference2008::ActiveXReference2008()
{
}

IConfiguration* ActiveXReference2008::createReferenceConfiguration() const
{
    return new Configuration2008(QLatin1String("ReferenceConfiguration"));
}


ActiveXReferenceFactory &ActiveXReferenceFactory::instance()
{
    static ActiveXReferenceFactory am;
    return am;
}

ActiveXReference::Ptr ActiveXReferenceFactory::create(VcDocConstants::DocumentVersion version)
{
    ActiveXReference::Ptr ref;

    switch (version) {
    case VcDocConstants::DV_MSVC_2003:
        ref = ActiveXReference::Ptr(new ActiveXReference2003);
        break;
    case VcDocConstants::DV_MSVC_2005:
        ref = ActiveXReference::Ptr(new ActiveXReference2005);
        break;
    case VcDocConstants::DV_MSVC_2008:
        ref = ActiveXReference::Ptr(new ActiveXReference2008);
        break;
    }

    return ref;
}

} // namespace Internal
} // namespace VcProjectManager
