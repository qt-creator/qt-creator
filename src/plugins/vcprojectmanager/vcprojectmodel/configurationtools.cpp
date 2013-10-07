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
#include "configurationtools.h"
#include "../interfaces/iconfigurationtool.h"
#include "../interfaces/itooldescription.h"

namespace VcProjectManager {
namespace Internal {

ConfigurationTools::ConfigurationTools()
{
}

ITools &ConfigurationTools::operator =(const ITools &tools)
{
    if (this != &tools) {
        qDeleteAll(m_tools);
        m_tools.clear();

        for (int i = 0; i < tools.toolCount(); ++i) {
            IConfigurationTool *tool = tools.tool(i);

            if (tool)
                m_tools.append(tool->clone());
        }
    }

    return *this;
}

void ConfigurationTools::addTool(IConfigurationTool *tool)
{
    if (!tool || m_tools.contains(tool))
        return;

    foreach (IConfigurationTool *toolPtr, m_tools) {
        if (toolPtr->toolDescription()->toolKey() == tool->toolDescription()->toolKey())
            return;
    }

    m_tools.append(tool);
}

void ConfigurationTools::removeTool(IConfigurationTool *tool)
{
    foreach (IConfigurationTool *toolPtr, m_tools) {
        if (toolPtr->toolDescription()->toolKey() == tool->toolDescription()->toolKey()) {
            m_tools.removeOne(toolPtr);
            delete toolPtr;
            return;
        }
    }
}

IConfigurationTool *ConfigurationTools::tool(const QString &toolKey) const
{
    foreach (IConfigurationTool *toolPtr, m_tools) {
        if (toolPtr->toolDescription()->toolKey() == toolKey) {
            return toolPtr;
        }
    }

    return 0;
}

IConfigurationTool *ConfigurationTools::tool(int index) const
{
    if (0 <= index && index < m_tools.size())
        return m_tools[index];

    return 0;
}

int ConfigurationTools::toolCount() const
{
    return m_tools.size();
}

void ConfigurationTools::appendToXMLNode(QDomElement &domElement, QDomDocument &domDocument) const
{
    foreach (const IConfigurationTool *confTool, m_tools)
        domElement.appendChild(confTool->toXMLDomNode(domDocument));
}

} // namespace Internal
} // namespace VcProjectManager
