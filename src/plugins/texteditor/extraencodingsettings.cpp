// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extraencodingsettings.h"
#include "behaviorsettingswidget.h"

#include <utils/settingsutils.h>

#include <QLatin1String>
#include <QSettings>

// Keep this for compatibility reasons.
static const char kGroupPostfix[] = "EditorManager";
static const char kUtf8BomBehaviorKey[] = "Utf8BomBehavior";

using namespace TextEditor;

ExtraEncodingSettings::ExtraEncodingSettings() : m_utf8BomSetting(OnlyKeep)
{}

ExtraEncodingSettings::~ExtraEncodingSettings() = default;

void ExtraEncodingSettings::toSettings(const QString &category, QSettings *s) const
{
    Q_UNUSED(category)

    Utils::toSettings(QLatin1String(kGroupPostfix), QString(), s, this);
}

void ExtraEncodingSettings::fromSettings(const QString &category, QSettings *s)
{
    Q_UNUSED(category)

    *this = ExtraEncodingSettings();
    Utils::fromSettings(QLatin1String(kGroupPostfix), QString(), s, this);
}

QVariantMap ExtraEncodingSettings::toMap() const
{
    return {
        {kUtf8BomBehaviorKey, m_utf8BomSetting}
    };
}

void ExtraEncodingSettings::fromMap(const QVariantMap &map)
{
    m_utf8BomSetting = (Utf8BomSetting)map.value(kUtf8BomBehaviorKey, m_utf8BomSetting).toInt();
}

bool ExtraEncodingSettings::equals(const ExtraEncodingSettings &s) const
{
    return m_utf8BomSetting == s.m_utf8BomSetting;
}

QStringList ExtraEncodingSettings::lineTerminationModeNames()
{
    return {BehaviorSettingsWidget::tr("Unix (LF)"),
                BehaviorSettingsWidget::tr("Windows (CRLF)")};
}
