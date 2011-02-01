/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "extraencodingsettings.h"

#include <utils/settingsutils.h>

#include <QtCore/QLatin1String>
#include <QtCore/QSettings>

// Keep this for compatibility reasons.
static const char * const kGroupPostfix = "EditorManager";
static const char * const kUtf8BomBehaviorKey = "Utf8BomBehavior";

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
