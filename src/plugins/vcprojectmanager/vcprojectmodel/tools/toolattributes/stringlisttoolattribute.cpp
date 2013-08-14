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
#include "stringlisttoolattribute.h"
#include "attributedescriptiondataitem.h"
#include "../../../widgets/toolwidgets/stringlisttoolattributesettingsitem.h"

namespace VcProjectManager {
namespace Internal {

StringListToolAttribute::StringListToolAttribute(const AttributeDescriptionDataItem *descDataItem)
    : m_isUsed(false),
      m_descDataItem(descDataItem)
{
}

StringListToolAttribute::StringListToolAttribute(const StringListToolAttribute &attr)
{
    m_isUsed = attr.m_isUsed;
    m_descDataItem = attr.m_descDataItem;
    m_attributeValue = attr.m_attributeValue;
}

const IAttributeDescriptionDataItem *StringListToolAttribute::descriptionDataItem() const
{
    return m_descDataItem;
}

IToolAttributeSettingsWidget *StringListToolAttribute::createSettingsItem()
{
    return new StringListToolAttributeSettingsItem(this);
}

QString StringListToolAttribute::value() const
{
    if (m_isUsed)
        return m_attributeValue.join(m_descDataItem->optionalValue(QLatin1String("separator")));
    return m_descDataItem->defaultValue();
}

void StringListToolAttribute::setValue(const QString &value)
{
    if (!m_isUsed && value == m_descDataItem->defaultValue())
        return;

    m_attributeValue = value.split(m_descDataItem->optionalValue(QLatin1String("separator")));
    m_isUsed = true;
}

bool StringListToolAttribute::isUsed() const
{
    return m_isUsed;
}

IToolAttribute *StringListToolAttribute::clone() const
{
    return new StringListToolAttribute(*this);
}

} // namespace Internal
} // namespace VcProjectManager
