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
#include "toolsectioncontainer.h"
#include "../../../interfaces/itoolsection.h"
#include "../../../interfaces/itoolsectiondescription.h"
#include "../../../interfaces/itoolattribute.h"
#include "../../../interfaces/iattributedescriptiondataitem.h"
#include "../../../interfaces/itoolattributecontainer.h"

namespace VcProjectManager {
namespace Internal {

ToolSectionContainer::ToolSectionContainer()
{
}

ToolSectionContainer::ToolSectionContainer(const ToolSectionContainer &toolSec)
{
    qDeleteAll(m_toolSections);
    m_toolSections.clear();

    foreach (IToolSection *sec, toolSec.m_toolSections) {
        if (sec)
            m_toolSections.append(sec->clone());
    }
}

ISectionContainer &ToolSectionContainer::operator =(ISectionContainer &toolSec)
{
    if (this != &toolSec) {
        qDeleteAll(m_toolSections);
        m_toolSections.clear();

        for (int i = 0; i < toolSec.sectionCount(); ++i) {
            IToolSection *sec = toolSec.section(i);
            if (sec)
                m_toolSections.append(sec->clone());
        }
    }

    return *this;
}

ToolSectionContainer::~ToolSectionContainer()
{
    qDeleteAll(m_toolSections);
}

IToolSection *ToolSectionContainer::section(int index) const
{
    if (0 <= index && index < m_toolSections.size())
        return m_toolSections[index];
    return 0;
}

IToolSection *ToolSectionContainer::section(const QString &sectionName) const
{
    foreach (IToolSection *sec, m_toolSections) {
        if (sec && sec->sectionDescription()->displayName() == sectionName) {
            return sec;
        }
    }
    return 0;
}

int ToolSectionContainer::sectionCount() const
{
    return m_toolSections.size();
}

void ToolSectionContainer::appendSection(IToolSection *section)
{
    if (!section)
        return;

    foreach (IToolSection *sec, m_toolSections) {
        if (sec && sec->sectionDescription()->displayName() == section->sectionDescription()->displayName())
            return;
    }
    m_toolSections.append(section);
}

void ToolSectionContainer::removeSection(const QString &sectionName)
{
    foreach (IToolSection *sec, m_toolSections) {
        if (sec && sec->sectionDescription()->displayName() == sectionName) {
            m_toolSections.removeOne(sec);
            delete sec;
            return;
        }
    }
}

void ToolSectionContainer::appendToXMLNode(QDomElement &elementNode)
{
    foreach (IToolSection *toolSection, m_toolSections) {
        if (toolSection) {
            for (int i = 0; i < toolSection->attributeContainer()->toolAttributeCount(); ++i) {
                IToolAttribute *toolAttr = toolSection->attributeContainer()->toolAttribute(i);

                if (toolAttr && toolAttr->isUsed())
                    elementNode.setAttribute(toolAttr->descriptionDataItem()->key(), toolAttr->value());
            }
        }
    }
}

} // namespace Internal
} // namespace VcProjectManager
