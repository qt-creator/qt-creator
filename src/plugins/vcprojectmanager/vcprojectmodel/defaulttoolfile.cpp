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
#include "defaulttoolfile.h"

namespace VcProjectManager {
namespace Internal {

DefaultToolFile::DefaultToolFile()
{
}

DefaultToolFile::DefaultToolFile(const DefaultToolFile &defToolFile)
{
    m_fileName = defToolFile.m_fileName;
}

DefaultToolFile &DefaultToolFile::operator =(const DefaultToolFile &defToolFile)
{
    if (this != &defToolFile)
        m_fileName = defToolFile.m_fileName;
    return *this;
}

DefaultToolFile::~DefaultToolFile()
{
}

void DefaultToolFile::processNode(const QDomNode &node)
{
    if (node.isNull())
        return;

    if (node.nodeType() == QDomNode::ElementNode)
        processNodeAttributes(node.toElement());
}

void DefaultToolFile::processNodeAttributes(const QDomElement &element)
{
    QDomNamedNodeMap namedNodeMap = element.attributes();

    if (namedNodeMap.size() == 1) {
        QDomNode domNode = namedNodeMap.item(0);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domAttr = domNode.toAttr();

            if (domAttr.name() == QLatin1String("FileName"))
                m_fileName = domAttr.value();
        }
    }
}

VcNodeWidget *DefaultToolFile::createSettingsWidget()
{
    return 0;
}

QDomNode DefaultToolFile::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement toolNode = domXMLDocument.createElement(QLatin1String("DefaultToolFile"));
    toolNode.setAttribute(QLatin1String("FileName"), m_fileName);
    return toolNode;
}

QString DefaultToolFile::fileName() const
{
    return m_fileName;
}

void DefaultToolFile::setFileName(const QString &fileName)
{
    m_fileName = fileName;
}

} // namespace Internal
} // namespace VcProjectManager
