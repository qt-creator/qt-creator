/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/
#include <settingsmanager.h>
#include <QtCore/QDebug>

using namespace Designer::Internal;

namespace {
    bool debug = false;
}

void SettingsManager::beginGroup(const QString &prefix)
{
    if (debug)
        qDebug() << "Designer - beginning group " << addPrefix(prefix);
    m_settings.beginGroup(addPrefix(prefix));
}

void SettingsManager::endGroup()
{
    if (debug)
        qDebug() << "Designer - end group";
    m_settings.endGroup();
}

bool SettingsManager::contains(const QString &key) const
{
    return m_settings.contains(addPrefix(key));
}

void SettingsManager::setValue(const QString &key, const QVariant &value)
{
    if (debug)
        qDebug() << "Designer - storing " << addPrefix(key) << ": " << value;
    m_settings.setValue(addPrefix(key), value);
}

QVariant SettingsManager::value(const QString &key, const QVariant &defaultValue) const
{
    QVariant result = m_settings.value(addPrefix(key), defaultValue);
    if (debug)
        qDebug() << "Designer - retrieving " << addPrefix(key) << ": " << result;
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
