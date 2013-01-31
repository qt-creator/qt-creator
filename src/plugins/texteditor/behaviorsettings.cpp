/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "behaviorsettings.h"

#include <utils/settingsutils.h>

#include <QSettings>
#include <QString>

static const char mouseNavigationKey[] = "MouseNavigation";
static const char scrollWheelZoomingKey[] = "ScrollWheelZooming";
static const char constrainTooltips[] = "ConstrainTooltips";
static const char camelCaseNavigationKey[] = "CamelCaseNavigation";
static const char keyboardTooltips[] = "KeyboardTooltips";
static const char groupPostfix[] = "BehaviorSettings";

namespace TextEditor {

BehaviorSettings::BehaviorSettings() :
    m_mouseNavigation(true),
    m_scrollWheelZooming(true),
    m_constrainHoverTooltips(false),
    m_camelCaseNavigation(true),
    m_keyboardTooltips(false)
{
}

void BehaviorSettings::toSettings(const QString &category, QSettings *s) const
{
    Utils::toSettings(QLatin1String(groupPostfix), category, s, this);
}

void BehaviorSettings::fromSettings(const QString &category, const QSettings *s)
{
    *this = BehaviorSettings();
    Utils::fromSettings(QLatin1String(groupPostfix), category, s, this);
}

void BehaviorSettings::toMap(const QString &prefix, QVariantMap *map) const
{
    map->insert(prefix + QLatin1String(mouseNavigationKey), m_mouseNavigation);
    map->insert(prefix + QLatin1String(scrollWheelZoomingKey), m_scrollWheelZooming);
    map->insert(prefix + QLatin1String(constrainTooltips), m_constrainHoverTooltips);
    map->insert(prefix + QLatin1String(camelCaseNavigationKey), m_camelCaseNavigation);
    map->insert(prefix + QLatin1String(keyboardTooltips), m_keyboardTooltips);
}

void BehaviorSettings::fromMap(const QString &prefix, const QVariantMap &map)
{
    m_mouseNavigation =
        map.value(prefix + QLatin1String(mouseNavigationKey), m_mouseNavigation).toBool();
    m_scrollWheelZooming =
        map.value(prefix + QLatin1String(scrollWheelZoomingKey), m_scrollWheelZooming).toBool();
    m_constrainHoverTooltips =
        map.value(prefix + QLatin1String(constrainTooltips), m_constrainHoverTooltips).toBool();
    m_camelCaseNavigation =
        map.value(prefix + QLatin1String(camelCaseNavigationKey), m_camelCaseNavigation).toBool();
    m_keyboardTooltips =
        map.value(prefix + QLatin1String(keyboardTooltips), m_keyboardTooltips).toBool();
}

bool BehaviorSettings::equals(const BehaviorSettings &ds) const
{
    return m_mouseNavigation == ds.m_mouseNavigation
        && m_scrollWheelZooming == ds.m_scrollWheelZooming
        && m_constrainHoverTooltips == ds.m_constrainHoverTooltips
        && m_camelCaseNavigation == ds.m_camelCaseNavigation
        && m_keyboardTooltips == ds.m_keyboardTooltips
        ;
}

} // namespace TextEditor

