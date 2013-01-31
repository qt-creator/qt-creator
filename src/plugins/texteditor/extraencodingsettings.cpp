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
