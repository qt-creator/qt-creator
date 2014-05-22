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
#include "booltoolattribute.h"
#include "integertoolattribute.h"
#include "stringlisttoolattribute.h"
#include "stringtoolattribute.h"
#include "toolattributeoption.h"

namespace VcProjectManager {
namespace Internal {

AttributeDescriptionDataItem::AttributeDescriptionDataItem(const QString &type,
                                                           const QString &key,
                                                           const QString &displayText,
                                                           const QString &descriptionText,
                                                           const QString &defaultVal)
    : m_key(key),
      m_displayText(displayText),
      m_descriptionText(descriptionText),
      m_defaultValue(defaultVal),
      m_type(type),
      m_firstOption(0)
{
}

AttributeDescriptionDataItem::~AttributeDescriptionDataItem()
{
    delete m_firstOption;
}

QString AttributeDescriptionDataItem::descriptionText() const
{
    return m_descriptionText;
}

QString AttributeDescriptionDataItem::displayText() const
{
    return m_displayText;
}

QString AttributeDescriptionDataItem::key() const
{
    return m_key;
}

ToolAttributeOption *AttributeDescriptionDataItem::firstOption() const
{
    return m_firstOption;
}

void AttributeDescriptionDataItem::setFirstOption(ToolAttributeOption *opt)
{
    m_firstOption = opt;
}

QString AttributeDescriptionDataItem::defaultValue() const
{
    return m_defaultValue;
}

IToolAttribute *AttributeDescriptionDataItem::createAttribute() const
{
    if (m_type == QLatin1String("Boolean"))
        return new BoolToolAttribute(this);
    else if (m_type == QLatin1String("Integer"))
        return new IntegerToolAttribute(this);
    else if (m_type == QLatin1String("String"))
        return new StringToolAttribute(this);
    else if (m_type == QLatin1String("StringList"))
        return new StringListToolAttribute(this);
    return 0;
}

QString AttributeDescriptionDataItem::optionalValue(const QString &key) const
{
    return m_optionalValues.value(key);
}

void AttributeDescriptionDataItem::setOptionalValue(const QString &key, const QString &value)
{
    m_optionalValues.insert(key, value);
}

void AttributeDescriptionDataItem::removeOptionalValue(const QString &key)
{
    m_optionalValues.remove(key);
}

QString AttributeDescriptionDataItem::type() const
{
    return m_type;
}

} // namespace Internal
} // namespace VcProjectManager
