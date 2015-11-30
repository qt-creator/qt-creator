/****************************************************************************
**
** Copyright (C) 2015 Thorben Kroeger <thorbenkroeger@gmail.com>.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "themecolors.h"
#include "colorvariable.h"
#include <utils/qtcassert.h>

namespace Core {
namespace Internal {
namespace ThemeEditor {

QSharedPointer<ColorVariable> ThemeColors::createVariable(const QColor &variableColor, const QString &variableName)
{
    ColorVariable::Ptr var(new ColorVariable(variableColor, variableName));
    insert(var);
    return var;
}

ColorRole::Ptr ThemeColors::createRole(const QString &roleName, QSharedPointer<ColorVariable> colorVariable)
{
    ColorRole::Ptr role(new ColorRole(roleName, colorVariable));
    insert(role);
    return role;
}

void ThemeColors::insert(ColorRole::Ptr color)
{
    m_colorRoles.append(color);
}

void ThemeColors::insert(ColorVariable::Ptr color)
{
    m_colorVariables.insert(color);
}

QSet<QSharedPointer<ColorVariable> > ThemeColors::colorVariables()
{
    return m_colorVariables;
}

void ThemeColors::removeVariable(QSharedPointer<ColorVariable> variable)
{
    QTC_ASSERT(m_colorVariables.contains(variable), return);
    m_colorVariables.remove(variable);
}

} // namespace ThemeEditor
} // namespace Internal
} // namespace Core
