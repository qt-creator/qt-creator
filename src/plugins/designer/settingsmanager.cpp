/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#include "settingsmanager.h"
#include "designerconstants.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

#include <QSettings>
#include <QDebug>

using namespace Designer::Internal;

static inline QSettings *coreSettings()
{
    if (Core::ICore::instance())
        return Core::ICore::settings();
    return 0;
}

void SettingsManager::beginGroup(const QString &prefix)
{
    QSettings *settings = coreSettings();
    QTC_ASSERT(settings, return);
    settings->beginGroup(addPrefix(prefix));
}

void SettingsManager::endGroup()
{
    QSettings *settings = coreSettings();
    QTC_ASSERT(settings, return);
    settings->endGroup();
}

bool SettingsManager::contains(const QString &key) const
{
    const QSettings *settings = coreSettings();
    QTC_ASSERT(settings, return false);
    return settings->contains(addPrefix(key));
}

void SettingsManager::setValue(const QString &key, const QVariant &value)
{
    QSettings *settings = coreSettings();
    QTC_ASSERT(settings, return);
    settings->setValue(addPrefix(key), value);
}

QVariant SettingsManager::value(const QString &key, const QVariant &defaultValue) const
{
    const QSettings *settings = coreSettings();
    QTC_ASSERT(settings, return QVariant());
    return settings->value(addPrefix(key), defaultValue);
}

void SettingsManager::remove(const QString &key)
{
    QSettings *settings = coreSettings();
    QTC_ASSERT(settings, return);
    settings->remove(addPrefix(key));
}

QString SettingsManager::addPrefix(const QString &name) const
{
    const QSettings *settings = coreSettings();
    QTC_ASSERT(settings, return name);
    QString result = name;
    if (settings->group().isEmpty())
        result.prepend(QLatin1String("Designer"));
    return result;
}
