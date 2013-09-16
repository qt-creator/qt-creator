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
#include "publishingitem.h"

namespace VcProjectManager {
namespace Internal {

PublishingItem::PublishingItem()
{
}

PublishingItem::PublishingItem(const PublishingItem &item)
{
    m_anyAttribute = item.m_anyAttribute;
}

PublishingItem &PublishingItem::operator =(const PublishingItem &item)
{
    if (this != &item)
        m_anyAttribute = item.m_anyAttribute;
    return *this;
}

PublishingItem::~PublishingItem()
{
}

void PublishingItem::processNode(const QDomNode &node)
{
    if (node.isNull())
        return;

    if (node.nodeType() == QDomNode::ElementNode)
        processNodeAttributes(node.toElement());
}

VcNodeWidget *PublishingItem::createSettingsWidget()
{
    return 0;
}

QDomNode PublishingItem::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement publishingItemNode = domXMLDocument.createElement(QLatin1String("PublishingItem"));

    QHashIterator<QString, QString> it(m_anyAttribute);

    while (it.hasNext()) {
        it.next();
        publishingItemNode.setAttribute(it.key(), it.value());
    }

    return publishingItemNode;
}

QString PublishingItem::attributeValue(const QString &attributeName) const
{
    return m_anyAttribute.value(attributeName);
}

void PublishingItem::setAttribute(const QString &attributeName, const QString &attributeValue)
{
    m_anyAttribute.insert(attributeName, attributeValue);
}

void PublishingItem::clearAttribute(const QString &attributeName)
{
    if (m_anyAttribute.contains(attributeName))
        m_anyAttribute.insert(attributeName, QString());
}

void PublishingItem::removeAttribute(const QString &attributeName)
{
    m_anyAttribute.remove(attributeName);
}

void PublishingItem::processNodeAttributes(const QDomElement &element)
{
    QDomNamedNodeMap namedNodeMap = element.attributes();

    for (int i = 0; i < namedNodeMap.size(); ++i) {
        QDomNode domNode = namedNodeMap.item(i);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domElement = domNode.toAttr();
            m_anyAttribute.insert(domElement.name(), domElement.value());
        }
    }
}

} // namespace Internal
} // namespace VcProjectManager
