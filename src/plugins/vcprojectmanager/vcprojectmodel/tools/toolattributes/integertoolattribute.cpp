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
#include "attributedescriptiondataitem.h"
#include "integertoolattribute.h"
#include "../../../widgets/toolwidgets/integertoolattributesettingsitem.h"

namespace VcProjectManager {
namespace Internal {

IntegerToolAttribute::IntegerToolAttribute(const AttributeDescriptionDataItem *descDataItem)
    : m_descDataItem(descDataItem),
      m_isUsed(false)
{
}

IntegerToolAttribute::IntegerToolAttribute(const IntegerToolAttribute &attr)
{
    m_isUsed = attr.m_isUsed;
    m_descDataItem = attr.m_descDataItem;
    m_attributeValue = attr.m_attributeValue;
}

const IAttributeDescriptionDataItem *IntegerToolAttribute::descriptionDataItem() const
{
    return m_descDataItem;
}

IToolAttributeSettingsWidget *IntegerToolAttribute::createSettingsItem()
{
    return new IntegerToolAttributeSettingsItem(this);
}

QString IntegerToolAttribute::value() const
{
    if (m_isUsed)
        return m_attributeValue;
    return m_descDataItem->defaultValue();
}

void IntegerToolAttribute::setValue(const QString &value)
{
    if (!m_isUsed && value == m_descDataItem->defaultValue())
        return;
    m_attributeValue = value;
    m_isUsed = true;
}

bool IntegerToolAttribute::isUsed() const
{
    return m_isUsed;
}

IToolAttribute *IntegerToolAttribute::clone() const
{
    return new IntegerToolAttribute(*this);
}

} // namespace Internal
} // namespace VcProjectManager
