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

#include "extraencodingsettings.h"

#include <utils/settingsutils.h>

#include <QLatin1String>
#include <QSettings>

// Keep this for compatibility reasons.
static const char kGroupPostfix[] = "EditorManager";
static const char kUtf8BomBehaviorKey[] = "Utf8BomBehavior";

using namespace TextEditor;

ExtraEncodingSettings::ExtraEncodingSettings() : m_utf8BomSetting(OnlyKeep)
{}

ExtraEncodingSettings::~ExtraEncodingSettings()
{}

void ExtraEncodingSettings::toSettings(const QString &category, QSettings *s) const
{
    Q_UNUSED(category)

    Utils::toSettings(QLatin1String(kGroupPostfix), QString(), s, this);
}

void ExtraEncodingSettings::fromSettings(const QString &category, const QSettings *s)
{
    Q_UNUSED(category)

    *this = ExtraEncodingSettings();
    Utils::fromSettings(QLatin1String(kGroupPostfix), QString(), s, this);
}

void ExtraEncodingSettings::toMap(const QString &prefix, QVariantMap *map) const
{
    map->insert(prefix + QLatin1String(kUtf8BomBehaviorKey), m_utf8BomSetting);
}

void ExtraEncodingSettings::fromMap(const QString &prefix, const QVariantMap &map)
{
    m_utf8BomSetting = (Utf8BomSetting)
        map.value(prefix + QLatin1String(kUtf8BomBehaviorKey), m_utf8BomSetting).toInt();
}

bool ExtraEncodingSettings::equals(const ExtraEncodingSettings &s) const
{
    return m_utf8BomSetting == s.m_utf8BomSetting;
}
