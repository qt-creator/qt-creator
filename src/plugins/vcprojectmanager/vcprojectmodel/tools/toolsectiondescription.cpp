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
#include "toolsectiondescription.h"
#include "../../interfaces/iattributedescriptiondataitem.h"
#include "toolsection.h"

namespace VcProjectManager {
namespace Internal {

ToolSectionDescription::ToolSectionDescription()
{
}

IToolSection *ToolSectionDescription::createToolSection() const
{
    return new ToolSection(this);
}

QString ToolSectionDescription::name() const
{
    return m_name;
}

void ToolSectionDescription::setName(const QString &name)
{
    m_name = name;
}

IAttributeDescriptionDataItem *ToolSectionDescription::attributeDescription(const QString &attributeKey) const
{
    foreach (IAttributeDescriptionDataItem *descDataItem, m_toolAttributeDescs) {
        if (descDataItem->key() == attributeKey)
            return descDataItem;
    }
    return 0;
}

IAttributeDescriptionDataItem *ToolSectionDescription::attributeDescription(int index) const
{
    if (0 <= index && index <= m_toolAttributeDescs.size())
        return m_toolAttributeDescs[index];
    return 0;
}

int ToolSectionDescription::attributeDescriptionCount() const
{
    return m_toolAttributeDescs.size();
}

void ToolSectionDescription::addAttributeDescription(IAttributeDescriptionDataItem *attributeDesc)
{
    if (!attributeDesc || m_toolAttributeDescs.contains(attributeDesc))
        return;

    foreach (IAttributeDescriptionDataItem *descDataItem, m_toolAttributeDescs) {
        if (descDataItem->key() == attributeDesc->key())
            return;
    }
    m_toolAttributeDescs.append(attributeDesc);
}

void ToolSectionDescription::removeAttributeDescription(IAttributeDescriptionDataItem *attributeDesc)
{
    if (!attributeDesc)
        return;

    foreach (IAttributeDescriptionDataItem *descDataItem, m_toolAttributeDescs) {
        if (descDataItem->key() == attributeDesc->key()) {
            m_toolAttributeDescs.removeOne(descDataItem);
            return;
        }
    }
}

void ToolSectionDescription::removeAttributeDescription(const QString &attributeKey)
{
    foreach (IAttributeDescriptionDataItem *descDataItem, m_toolAttributeDescs) {
        if (descDataItem->key() == attributeKey) {
            m_toolAttributeDescs.removeOne(descDataItem);
            return;
        }
    }
}

} // namespace Internal
} // namespace VcProjectManager
