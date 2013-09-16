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
#include "global.h"

namespace VcProjectManager {
namespace Internal {

Global::Global()
{
}

Global::Global(const Global &global)
{
    m_name = global.m_name;
    m_value = global.m_value;
}

Global &Global::operator =(const Global &global)
{
    if (this != &global) {
        m_name = global.m_name;
        m_value = global.m_value;
    }

    return *this;
}

Global::~Global()
{
}

void Global::processNode(const QDomNode &node)
{
    if (node.isNull())
        return;

    if (node.nodeType() == QDomNode::ElementNode)
        processNodeAttributes(node.toElement());
}

VcNodeWidget *Global::createSettingsWidget()
{
    return 0;
}

QDomNode Global::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement globalNode = domXMLDocument.createElement(QLatin1String("Global"));

    globalNode.setAttribute(QLatin1String("Name"), m_name);
    globalNode.setAttribute(QLatin1String("Value"), m_value);

    return globalNode;
}

QString Global::name() const
{
    return m_name;
}

void Global::setName(const QString &name)
{
    m_name = name;
}

QString Global::value() const
{
    return m_value;
}

void Global::setValue(const QString &value)
{
    m_value = value;
}

void Global::processNodeAttributes(const QDomElement &element)
{
    QDomNamedNodeMap namedNodeMap = element.attributes();

    for (int i = 0; i < namedNodeMap.size(); ++i) {
        QDomNode domNode = namedNodeMap.item(i);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domElement = domNode.toAttr();

            if (domElement.name() == QLatin1String("Name"))
                m_name = domElement.value();

            else if (domElement.name() == QLatin1String("Value"))
                m_value = domElement.value();
        }
    }
}

} // namespace Internal
} // namespace VcProjectManager
