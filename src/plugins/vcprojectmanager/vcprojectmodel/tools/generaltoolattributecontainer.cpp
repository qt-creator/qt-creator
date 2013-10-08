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
#include "generaltoolattributecontainer.h"
#include "../../interfaces/iattributedescriptiondataitem.h"
#include "../../interfaces/itoolattribute.h"

namespace VcProjectManager {
namespace Internal {

GeneralToolAttributeContainer::GeneralToolAttributeContainer()
{
}

GeneralToolAttributeContainer::GeneralToolAttributeContainer(const GeneralToolAttributeContainer &container)
{
    foreach (IToolAttribute *toolAttr, container.m_toolAttributes)
        m_toolAttributes.append(toolAttr->clone());
}

GeneralToolAttributeContainer &GeneralToolAttributeContainer::operator=(const GeneralToolAttributeContainer &container)
{
    if (this != &container) {
        qDeleteAll(m_toolAttributes);
        foreach (IToolAttribute *toolAttr, container.m_toolAttributes)
            m_toolAttributes.append(toolAttr->clone());
    }
    return *this;
}

GeneralToolAttributeContainer::~GeneralToolAttributeContainer()
{
    qDeleteAll(m_toolAttributes);
}

IToolAttribute *GeneralToolAttributeContainer::toolAttribute(int index) const
{
    if (0 <= index && index < m_toolAttributes.size())
        return m_toolAttributes[index];
    return 0;
}

IToolAttribute *GeneralToolAttributeContainer::toolAttribute(const QString &attributeKey) const
{
    foreach (IToolAttribute *toolAttr, m_toolAttributes) {
        if (toolAttr->descriptionDataItem()->key() == attributeKey)
            return toolAttr;
    }

    return 0;
}

int GeneralToolAttributeContainer::toolAttributeCount() const
{
    return m_toolAttributes.size();
}

void GeneralToolAttributeContainer::addToolAttribute(IToolAttribute *toolAttribute)
{
    if (!toolAttribute || m_toolAttributes.contains(toolAttribute))
        return;

    foreach (IToolAttribute *toolAttr, m_toolAttributes) {
        if (toolAttr->descriptionDataItem()->key() == toolAttribute->descriptionDataItem()->key())
            return;
    }
    m_toolAttributes.append(toolAttribute);
}

void GeneralToolAttributeContainer::removeToolAttribute(IToolAttribute *toolAttribute)
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

} // Internal
} // VcProjectManager
