/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "behaviorsettings.h"

#include <QtCore/QSettings>
#include <QtCore/QString>

static const char * const mouseNavigationKey = "MouseNavigation";
static const char * const groupPostfix = "BehaviorSettings";

namespace TextEditor {

BehaviorSettings::BehaviorSettings() :
    m_mouseNavigation(true)
{
}

void BehaviorSettings::toSettings(const QString &category, QSettings *s) const
{
    QString group = QLatin1String(groupPostfix);
    if (!category.isEmpty())
        group.insert(0, category);
    s->beginGroup(group);
    s->setValue(QLatin1String(mouseNavigationKey), m_mouseNavigation);
    s->endGroup();
}

void BehaviorSettings::fromSettings(const QString &category, const QSettings *s)
{
    QString group = QLatin1String(groupPostfix);
    if (!category.isEmpty())
        group.insert(0, category);
    group += QLatin1Char('/');

    *this = BehaviorSettings(); // Assign defaults

    m_mouseNavigation = s->value(group + QLatin1String(mouseNavigationKey), m_mouseNavigation).toBool();
}

bool BehaviorSettings::equals(const BehaviorSettings &ds) const
{
    return m_mouseNavigation == ds.m_mouseNavigation
        ;
}

} // namespace TextEditor

