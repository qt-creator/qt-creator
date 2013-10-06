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
#include "publishingdata.h"
#include "generalattributecontainer.h"

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

    foreach (const PublishingItem::Ptr &item, data.m_publishingItems)
        m_publishingItems.append(PublishingItem::Ptr(new PublishingItem(*item)));
}

PublishingData &PublishingData::operator =(const PublishingData &data)
{
    if (this != &data) {
        *m_attributeContainer = *data.m_attributeContainer;

        m_publishingItems.clear();
        foreach (const PublishingItem::Ptr &item, data.m_publishingItems)
            m_publishingItems.append(PublishingItem::Ptr(new PublishingItem(*item)));
    }
    return *this;
}

PublishingData::~PublishingData()
{
    m_publishingItems.clear();
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

    foreach (const PublishingItem::Ptr &publish, m_publishingItems)
        publishingDataNode.appendChild(publish->toXMLDomNode(domXMLDocument));

    return publishingDataNode;
}

bool PublishingData::isEmpty() const
{
    return m_publishingItems.isEmpty() && !m_attributeContainer->getAttributeCount();
}

void PublishingData::processPublishingItem(const QDomNode &publishingItemNode)
{
    PublishingItem::Ptr publishingItem(new PublishingItem);
    m_publishingItems.append(publishingItem);
    publishingItem->processNode(publishingItemNode);

    // process next sibling
    QDomNode nextSibling = publishingItemNode.nextSibling();
    if (!nextSibling.isNull())
        processPublishingItem(nextSibling);
}

void PublishingData::addPublishingItem(PublishingItem::Ptr item)
{
    if (m_publishingItems.contains(item))
        return;
    m_publishingItems.append(item);
}

void PublishingData::removePublishingItem(PublishingItem::Ptr item)
{
    m_publishingItems.removeAll(item);
}

QList<PublishingItem::Ptr> PublishingData::publishingItems() const
{
    return m_publishingItems;
}

QList<PublishingItem::Ptr> PublishingData::publishingItems(const QString &attributeName, const QString &attributeValue) const
{
    QList<PublishingItem::Ptr> items;

    foreach (const PublishingItem::Ptr &item, m_publishingItems) {
        if (item->attributeContainer()->attributeValue(attributeName) == attributeValue)
            items.append(item);
    }

    return items;
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
