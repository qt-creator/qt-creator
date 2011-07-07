/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "tabpreferences.h"
#include "tabsettings.h"
#include "texteditorconstants.h"

using namespace TextEditor;

static const char *settingsSuffixKey = "TabPreferences";

static const char *currentFallbackKey = "CurrentFallback";

static QList<IFallbackPreferences *> toFallbackList(
    const QList<TabPreferences *> &fallbacks)
{
    QList<IFallbackPreferences *> fallbackList;
    for (int i = 0; i < fallbacks.count(); i++)
        fallbackList << fallbacks.at(i);
    return fallbackList;
}

TabPreferences::TabPreferences(
    const QList<IFallbackPreferences *> &fallbacks, QObject *parent)
    : IFallbackPreferences(fallbacks, parent)
{
    connect(this, SIGNAL(currentValueChanged(QVariant)),
            this, SLOT(slotCurrentValueChanged(QVariant)));
}

TabPreferences::TabPreferences(
    const QList<TabPreferences *> &fallbacks, QObject *parent)
    : IFallbackPreferences(toFallbackList(fallbacks), parent)
{
    connect(this, SIGNAL(currentValueChanged(QVariant)),
            this, SLOT(slotCurrentValueChanged(QVariant)));
}

QVariant TabPreferences::value() const
{
    QVariant v;
    v.setValue(settings());
    return v;
}

void TabPreferences::setValue(const QVariant &value)
{
    if (!value.canConvert<TabSettings>())
        return;

    setSettings(value.value<TabSettings>());
}

TabSettings TabPreferences::settings() const
{
    return m_data;
}

void TabPreferences::setSettings(const TextEditor::TabSettings &data)
{
    if (m_data == data)
        return;

    m_data = data;

    QVariant v;
    v.setValue(data);
    emit valueChanged(v);
    emit settingsChanged(m_data);
    if (!currentFallback()) {
        emit currentValueChanged(v);
    }
}

TabSettings TabPreferences::currentSettings() const
{
    QVariant v = currentValue();
    if (!v.canConvert<TabSettings>()) {
        // warning
        return TabSettings();
    }
    return v.value<TabSettings>();
}

void TabPreferences::slotCurrentValueChanged(const QVariant &value)
{
    if (!value.canConvert<TabSettings>())
        return;

    emit currentSettingsChanged(value.value<TabSettings>());
}

QString TabPreferences::settingsSuffix() const
{
    return settingsSuffixKey;
}

void TabPreferences::toMap(const QString &prefix, QVariantMap *map) const
{
    m_data.toMap(prefix, map);
    map->insert(prefix + QLatin1String(currentFallbackKey), currentFallbackId());
}

void TabPreferences::fromMap(const QString &prefix, const QVariantMap &map)
{
    m_data.fromMap(prefix, map);
    setCurrentFallback(map.value(prefix + QLatin1String(currentFallbackKey), Constants::GLOBAL_SETTINGS_ID).toString());
}

