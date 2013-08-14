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
#include "toolsection.h"
#include "toolattributes/itoolattribute.h"
#include "toolattributes/iattributedescriptiondataitem.h"
#include "../../widgets/toolwidgets/toolsectionsettingswidget.h"
#include "toolsectiondescription.h"

namespace VcProjectManager {
namespace Internal {

ToolSection::ToolSection(ToolSectionDescription *toolSectionDesc)
    : m_toolDesc(toolSectionDesc)
{
    for (int i = 0; i < toolSectionDesc->attributeDescriptionCount(); ++i) {
        if (toolSectionDesc->attributeDescription(i))
            m_toolAttributes.append(toolSectionDesc->attributeDescription(i)->createAttribute());
    }
}

ToolSection::ToolSection(const ToolSection &toolSec)
{
    qDeleteAll(m_toolAttributes);
    m_toolAttributes.clear();

    m_toolDesc = toolSec.m_toolDesc;

    foreach (const IToolAttribute *toolAttribute, toolSec.m_toolAttributes) {
        if (toolAttribute)
            m_toolAttributes.append(toolAttribute->clone());
    }
}

ToolSection::~ToolSection()
{
    qDeleteAll(m_toolAttributes);
}

IToolAttribute *ToolSection::toolAttribute(int index) const
{
    if (0 <= index && index < m_toolAttributes.size())
        return m_toolAttributes[index];
    return 0;
}

IToolAttribute *ToolSection::toolAttribute(const QString &attributeKey) const
{
    foreach (IToolAttribute *toolAttr, m_toolAttributes) {
        if (toolAttr->descriptionDataItem()->key() == attributeKey)
            return toolAttr;
    }

    return 0;
}

int ToolSection::toolAttributeCount() const
{
    return m_toolAttributes.size();
}

void ToolSection::addToolAttribute(IToolAttribute *toolAttribute)
{
    if (!toolAttribute || m_toolAttributes.contains(toolAttribute))
        return;

    foreach (IToolAttribute *toolAttr, m_toolAttributes) {
        if (toolAttr->descriptionDataItem()->key() == toolAttribute->descriptionDataItem()->key())
            return;
    }
    m_toolAttributes.append(toolAttribute);
}

void ToolSection::removeToolAttribute(IToolAttribute *toolAttribute)
{
    if (!toolAttribute)
        return;

    foreach (IToolAttribute *toolAttr, m_toolAttributes) {
        if (toolAttr->descriptionDataItem()->key() == toolAttribute->descriptionDataItem()->key()) {
            m_toolAttributes.removeOne(toolAttr);
            return;
        }
    }
}

ToolSectionDescription *ToolSection::sectionDescription() const
{
    return m_toolDesc;
}

ToolSectionSettingsWidget *ToolSection::createSettingsWidget()
{
    return new ToolSectionSettingsWidget(this);
}

} // namespace Internal
} // namespace VcProjectManager
