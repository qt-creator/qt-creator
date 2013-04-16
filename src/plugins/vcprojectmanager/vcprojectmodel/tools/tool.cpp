/**************************************************************************
**
** Copyright (c) 2013 Bojan Petrovic
** Copyright (c) 2013 Radovan Zivkvoic
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
#include "tool.h"
#include <QStringList>

namespace VcProjectManager {
namespace Internal {

Tool::Tool()
{
}

Tool::Tool(const Tool &tool)
{
    m_anyAttribute = tool.m_anyAttribute;
    m_name = tool.name();
}

Tool &Tool::operator=(const Tool &tool)
{
    if (this != &tool) {
        m_anyAttribute = tool.m_anyAttribute;
        m_name = tool.name();
    }
    return *this;
}

Tool::~Tool()
{
}

void Tool::processNode(const QDomNode &node)
{
    if (node.isNull())
        return;

    if (node.nodeType() == QDomNode::ElementNode)
        processNodeAttributes(node.toElement());
}

void Tool::processNodeAttributes(const QDomElement &element)
{
    QDomNamedNodeMap namedNodeMap = element.attributes();

    for (int i = 0; i < namedNodeMap.size(); ++i) {
        QDomNode domNode = namedNodeMap.item(i);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domElement = domNode.toAttr();

            if (domElement.name() == QLatin1String("Name"))
                m_name = domElement.value();

            else
                m_anyAttribute.insert(domElement.name(), domElement.value());
        }
    }
}

QDomNode Tool::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement toolNode = domXMLDocument.createElement(QLatin1String("Tool"));
    toolNode.setAttribute(QLatin1String("Name"), m_name);

    QHashIterator<QString, QString> it(m_anyAttribute);

    while (it.hasNext()) {
        it.next();
        toolNode.setAttribute(it.key(), it.value());
    }

    return toolNode;
}

QString Tool::name() const
{
    return m_name;
}

void Tool::setName(const QString &name)
{
    m_name = name;
}

QString Tool::attributeValue(const QString &attributeName) const
{
    return m_anyAttribute.value(attributeName);
}

void Tool::setAttribute(const QString &attributeName, const QString &attributeValue)
{
    m_anyAttribute.insert(attributeName, attributeValue);
}

void Tool::clearAttribute(const QString &attributeName)
{
    if (m_anyAttribute.contains(attributeName))
        m_anyAttribute.insert(attributeName, QString());
}

void Tool::removeAttribute(const QString &attributeName)
{
    m_anyAttribute.remove(attributeName);
}

bool Tool::containsAttribute(const QString &attributeName) const
{
    return m_anyAttribute.contains(attributeName);
}

bool Tool::readBooleanAttribute(const QString &attributeName, bool defaultBoolValue) const
{
    QString val = attributeValue(QString(attributeName));
    if (val.toLower() == QVariant(!defaultBoolValue).toString())
        return !defaultBoolValue;

    return defaultBoolValue; // default
}

void Tool::setBooleanAttribute(const QString &attributeName, bool value, bool defaultValue)
{
    if (!containsAttribute(attributeName) && value == defaultValue)
        return;

    setAttribute(attributeName, QVariant(value).toString());
}

QStringList Tool::readStringListAttribute(const QString &attributeName, const QString &splitter) const
{
    if (containsAttribute(attributeName)) {
        QString val = attributeValue(attributeName);
        return val.split(splitter);
    }

    return QStringList();
}

void Tool::setStringListAttribute(const QString &attributeName, const QStringList &values, const QString &joinCharacters)
{
    if (values.isEmpty() && !containsAttribute(attributeName))
        return;

    QString incDirs;
    if (values.size()) {
        incDirs.append(values.first());

        for (int i = 1; i < values.size(); ++i) {
            incDirs.append(joinCharacters);
            incDirs.append(values.at(i));
        }
    }
    setAttribute(attributeName, incDirs);
}

QString Tool::readStringAttribute(const QString &attributeName, const QString &defaultValue) const
{
    if (containsAttribute(attributeName))
        return attributeValue(attributeName);

    return defaultValue; // default
}

void Tool::setStringAttribute(const QString &attributeName, const QString &attributeValue, const QString &defaultValue)
{
    if (!containsAttribute(attributeName) && attributeValue == defaultValue)
        return;
    setAttribute(attributeName, attributeValue);
}

void Tool::setIntegerEnumAttribute(const QString &attributeName, int value, int defaultValue)
{
    if (!containsAttribute(attributeName) && value == defaultValue)
        return;
    setAttribute(attributeName, QVariant(value).toString());
}

} // namespace Internal
} // namespace VcProjectManager
