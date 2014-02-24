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
#include "publishingdata.h"
#include "generalattributecontainer.h"

#include <utils/qtcassert.h>

namespace VcProjectManager {
namespace Internal {

PublishingData::PublishingData()
{
    m_attributeContainer = new GeneralAttributeContainer;
}

PublishingData::PublishingData(const PublishingData &data)
{
    m_attributeContainer = new GeneralAttributeContainer;
    *m_attributeContainer = *data.m_attributeContainer;

    foreach (const IPublishingItem *item, data.m_publishingItems)
        m_publishingItems.append(item->clone());
}

PublishingData &PublishingData::operator =(const PublishingData &data)
{
    if (this != &data) {
        *m_attributeContainer = *data.m_attributeContainer;

        qDeleteAll(m_publishingItems);
        m_publishingItems.clear();
        foreach (const IPublishingItem *item, data.m_publishingItems)
            m_publishingItems.append(item->clone());
    }
    return *this;
}

PublishingData::~PublishingData()
{
    qDeleteAll(m_publishingItems);
    delete m_attributeContainer;
}

void PublishingData::processNode(const QDomNode &node)
{
    if (node.isNull())
        return;

    if (node.nodeType() == QDomNode::ElementNode)
        processNodeAttributes(node.toElement());

    if (node.hasChildNodes()) {
        QDomNode firstChild = node.firstChild();

        if (!firstChild.isNull())
            processPublishingItem(firstChild);
    }
}

VcNodeWidget *PublishingData::createSettingsWidget()
{
    return 0;
}

QDomNode PublishingData::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement publishingDataNode = domXMLDocument.createElement(QLatin1String("PublishingData"));
    m_attributeContainer->appendToXMLNode(publishingDataNode);

    foreach (const IPublishingItem *publish, m_publishingItems)
        publishingDataNode.appendChild(publish->toXMLDomNode(domXMLDocument));

    return publishingDataNode;
}

void PublishingData::addPublishingItem(IPublishingItem *item)
{
    if (!item || m_publishingItems.contains(item))
        return;

    m_publishingItems.append(item);
}

void PublishingData::removePublishingItem(IPublishingItem *item)
{
    if (!item || !m_publishingItems.contains(item))
        return;
    m_publishingItems.removeOne(item);
    delete item;
}

int PublishingData::publishingItemCount() const
{
    return m_publishingItems.size();
}

IPublishingItem *PublishingData::publishingItem(int index) const
{
    QTC_ASSERT(0 <= index && index < m_publishingItems.size(), return 0);
    return m_publishingItems[index];
}

void PublishingData::processPublishingItem(const QDomNode &publishingItemNode)
{
    PublishingItem *publishingItem(new PublishingItem);
    m_publishingItems.append(publishingItem);
    publishingItem->processNode(publishingItemNode);

    // process next sibling
    QDomNode nextSibling = publishingItemNode.nextSibling();
    if (!nextSibling.isNull())
        processPublishingItem(nextSibling);
}

IAttributeContainer *PublishingData::attributeContainer() const
{
    return m_attributeContainer;
}

void PublishingData::processNodeAttributes(const QDomElement &element)
{
    QDomNamedNodeMap namedNodeMap = element.attributes();

    for (int i = 0; i < namedNodeMap.size(); ++i) {
        QDomNode domNode = namedNodeMap.item(i);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domElement = domNode.toAttr();
            m_attributeContainer->setAttribute(domElement.name(), domElement.value());
        }
    }
}

} // namespace Internal
} // namespace VcProjectManager
