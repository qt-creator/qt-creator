/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "settingsmanager.h"
#include "designerconstants.h"

#include <QtCore/QDebug>

using namespace Designer::Internal;

void SettingsManager::beginGroup(const QString &prefix)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << addPrefix(prefix);
    m_settings.beginGroup(addPrefix(prefix));
}

void SettingsManager::endGroup()
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO;
    m_settings.endGroup();
}

bool SettingsManager::contains(const QString &key) const
{
    return m_settings.contains(addPrefix(key));
}

void SettingsManager::setValue(const QString &key, const QVariant &value)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO  << addPrefix(key) << ": " << value;
    m_settings.setValue(addPrefix(key), value);
}

QVariant SettingsManager::value(const QString &key, const QVariant &defaultValue) const
{
    QVariant result = m_settings.value(addPrefix(key), defaultValue);
    if (Designer::Constants::Internal::debug)
        qDebug() << Q_FUNC_INFO << addPrefix(key) << ": " << result;
    return result;
}

void SettingsManager::remove(const QString &key)
{
    m_settings.remove(addPrefix(key));
}

QString SettingsManager::addPrefix(const QString &name) const
{
    QString result = name;
    if (m_settings.group().isEmpty())
        result.insert(0, QLatin1String("Designer"));
    return result;
}
