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
#include "toolattributeoption.h"

namespace VcProjectManager {
namespace Internal {

ToolAttributeOption::ToolAttributeOption()
    : m_nextOption(0),
      m_isNull(true)
{
}

ToolAttributeOption::ToolAttributeOption(const QString &description, const QString &value)
    : m_description(description),
      m_value(value),
      m_nextOption(0),
      m_isNull(false)
{
}

ToolAttributeOption::~ToolAttributeOption()
{
    delete m_nextOption;
}

QString ToolAttributeOption::description() const
{
    return m_description;
}

void ToolAttributeOption::setDescription(const QString &description)
{
    m_description = description;
}

QString ToolAttributeOption::value() const
{
    return m_value;
}

void ToolAttributeOption::setValue(const QString &value)
{
    m_value = value;
}

bool ToolAttributeOption::isNull() const
{
    return m_isNull;
}

ToolAttributeOption *ToolAttributeOption::nextOption() const
{
    return m_nextOption;
}

void ToolAttributeOption::appendOption(ToolAttributeOption *option)
{
    if (m_nextOption)
        option->m_nextOption = m_nextOption;

    m_nextOption = option;
}



} // namespace Internal
} // namespace VcProjectManager
