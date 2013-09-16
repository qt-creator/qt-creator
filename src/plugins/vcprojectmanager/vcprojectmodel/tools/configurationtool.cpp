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
#include "configurationtool.h"
#include "toolattributes/itoolattribute.h"
#include "toolattributes/iattributedescriptiondataitem.h"
#include "toolattributes/tooldescriptiondatamanager.h"
#include "toolattributes/tooldescription.h"
#include "toolsectiondescription.h"
#include "toolsection.h"

namespace VcProjectManager {
namespace Internal {

ConfigurationTool::ConfigurationTool(ToolDescription *toolDesc)
    : m_toolDesc(toolDesc)
{
    for (int i = 0; i < toolDesc->sectionDescriptionCount(); ++i) {
        if (toolDesc->sectionDescription(i))
            m_toolSections.append(toolDesc->sectionDescription(i)->createToolSection());
    }
}

ConfigurationTool::ConfigurationTool(const ConfigurationTool &tool)
{
    qDeleteAll(m_toolSections);
    m_toolSections.clear();

    m_toolDesc = tool.m_toolDesc;
    foreach (ToolSection *toolSec, tool.m_toolSections)
        m_toolSections.append(new ToolSection(*toolSec));
}

ConfigurationTool::~ConfigurationTool()
{
    qDeleteAll(m_toolSections);
}

void ConfigurationTool::processNode(const QDomNode &node)
{
    if (node.isNull())
        return;

    if (node.nodeType() == QDomNode::ElementNode)
        processNodeAttributes(node.toElement());
}

QDomNode ConfigurationTool::toXMLDomNode(QDomDocument &domXMLDocument) const
{
    QDomElement toolNode = domXMLDocument.createElement(QLatin1String("Tool"));
    toolNode.setAttribute(QLatin1String("Name"), m_toolDesc->toolKey());

    foreach (ToolSection *toolSection, m_toolSections) {
        if (toolSection) {
            for (int i = 0; i < toolSection->toolAttributeCount(); ++i) {
                IToolAttribute *toolAttr = toolSection->toolAttribute(i);

                if (toolAttr && toolAttr->isUsed())
                    toolNode.setAttribute(toolAttr->descriptionDataItem()->key(), toolAttr->value());
            }
        }
    }

    return toolNode;
}

ToolDescription *ConfigurationTool::toolDescription() const
{
    return m_toolDesc;
}

ToolSection *ConfigurationTool::section(int index) const
{
    if (0 <= index && index < m_toolSections.size())
        return m_toolSections[index];
    return 0;
}

int ConfigurationTool::sectionCount() const
{
    return m_toolSections.size();
}

void ConfigurationTool::appendSection(ToolSection *section)
{
    if (!section)
        return;

    foreach (ToolSection *sec, m_toolSections) {
        if (sec && sec->sectionDescription()->name() == section->sectionDescription()->name())
            return;
    }
    m_toolSections.append(section);
}

void ConfigurationTool::removeSection(const QString &sectionName)
{
    QList<ToolSection *>::iterator it = m_toolSections.begin();

    while (it != m_toolSections.end()) {
        ToolSection *sec = *it;
        if (sec && sec->sectionDescription()->name() == sectionName) {
            m_toolSections.erase(it);
            delete sec;
            return;
        }
        ++it;
    }
}

VcNodeWidget *ConfigurationTool::createSettingsWidget()
{
    return new ToolSettingsWidget(this);
}

ConfigurationTool *ConfigurationTool::clone()
{
    return new ConfigurationTool(*this);
}

void ConfigurationTool::processNodeAttributes(const QDomElement &domElement)
{
    QDomNamedNodeMap namedNodeMap = domElement.attributes();

    for (int i = 0; i < namedNodeMap.size(); ++i) {
        QDomNode domNode = namedNodeMap.item(i);

        if (domNode.nodeType() == QDomNode::AttributeNode) {
            QDomAttr domElement = domNode.toAttr();

            if (domElement.name() != QLatin1String("Name")) {
                foreach (ToolSection *toolSection, m_toolSections) {
                    IToolAttribute *toolAttr = toolSection->toolAttribute(domElement.name());

                    if (toolAttr)
                        toolAttr->setValue(domElement.value());
                }
            }
        }
    }
}

} // namespace Internal
} // namespace VcProjectManager
