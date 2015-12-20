/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "behaviorsettings.h"

#include <utils/settingsutils.h>

#include <QSettings>
#include <QString>

static const char mouseHidingKey[] = "MouseHiding";
static const char mouseNavigationKey[] = "MouseNavigation";
static const char scrollWheelZoomingKey[] = "ScrollWheelZooming";
static const char constrainTooltips[] = "ConstrainTooltips";
static const char camelCaseNavigationKey[] = "CamelCaseNavigation";
static const char keyboardTooltips[] = "KeyboardTooltips";
static const char groupPostfix[] = "BehaviorSettings";
static const char smartSelectionChanging[] = "SmartSelectionChanging";

namespace TextEditor {

BehaviorSettings::BehaviorSettings() :
    m_mouseHiding(true),
    m_mouseNavigation(true),
    m_scrollWheelZooming(true),
    m_constrainHoverTooltips(false),
    m_camelCaseNavigation(true),
    m_keyboardTooltips(false),
    m_smartSelectionChanging(true)
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
    map->insert(prefix + QLatin1String(mouseHidingKey), m_mouseHiding);
    map->insert(prefix + QLatin1String(mouseNavigationKey), m_mouseNavigation);
    map->insert(prefix + QLatin1String(scrollWheelZoomingKey), m_scrollWheelZooming);
    map->insert(prefix + QLatin1String(constrainTooltips), m_constrainHoverTooltips);
    map->insert(prefix + QLatin1String(camelCaseNavigationKey), m_camelCaseNavigation);
    map->insert(prefix + QLatin1String(keyboardTooltips), m_keyboardTooltips);
    map->insert(prefix + QLatin1String(smartSelectionChanging), m_smartSelectionChanging);
}

void BehaviorSettings::fromMap(const QString &prefix, const QVariantMap &map)
{
    m_mouseHiding =
            map.value(prefix + QLatin1String(mouseHidingKey), m_mouseHiding).toBool();
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
    m_smartSelectionChanging =
        map.value(prefix + QLatin1String(smartSelectionChanging), m_smartSelectionChanging)
           .toBool();
}

bool BehaviorSettings::equals(const BehaviorSettings &ds) const
{
    return m_mouseHiding == ds.m_mouseHiding
        && m_mouseNavigation == ds.m_mouseNavigation
        && m_scrollWheelZooming == ds.m_scrollWheelZooming
        && m_constrainHoverTooltips == ds.m_constrainHoverTooltips
        && m_camelCaseNavigation == ds.m_camelCaseNavigation
        && m_keyboardTooltips == ds.m_keyboardTooltips
        && m_smartSelectionChanging == ds.m_smartSelectionChanging
        ;
}

} // namespace TextEditor

