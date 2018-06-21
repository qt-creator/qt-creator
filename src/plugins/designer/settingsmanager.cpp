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

#include "settingsmanager.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

#include <QSettings>
#include <QDebug>

using namespace Designer::Internal;

void SettingsManager::beginGroup(const QString &prefix)
{
    Core::ICore::settings()->beginGroup(addPrefix(prefix));
}

void SettingsManager::endGroup()
{
    Core::ICore::settings()->endGroup();
}

bool SettingsManager::contains(const QString &key) const
{
    return Core::ICore::settings()->contains(addPrefix(key));
}

void SettingsManager::setValue(const QString &key, const QVariant &value)
{
    Core::ICore::settings()->setValue(addPrefix(key), value);
}

QVariant SettingsManager::value(const QString &key, const QVariant &defaultValue) const
{
    return Core::ICore::settings()->value(addPrefix(key), defaultValue);
}

void SettingsManager::remove(const QString &key)
{
    Core::ICore::settings()->remove(addPrefix(key));
}

QString SettingsManager::addPrefix(const QString &name) const
{
    QString result = name;
    if (Core::ICore::settings()->group().isEmpty())
        result.prepend("Designer");
    return result;
}
