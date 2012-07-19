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

#ifndef SETTINGSUTILS_H
#define SETTINGSUTILS_H

#include <QSettings>
#include <QString>
#include <QStringList>
#include <QVariant>

namespace Utils {

template <class SettingsClassT>
void fromSettings(const QString &postFix,
                  const QString &category,
                  const QSettings *s,
                  SettingsClassT *obj)
{
    QVariantMap map;
    const QStringList &keys = s->allKeys();
    foreach (const QString &key, keys)
        map.insert(key, s->value(key));

    QString group = postFix;
    if (!category.isEmpty())
        group.insert(0, category);
    group += QLatin1Char('/');
    obj->fromMap(group, map);
}

template <class SettingsClassT>
void toSettings(const QString &postFix,
                const QString &category,
                QSettings *s,
                const SettingsClassT *obj)
{
    QString group = postFix;
    if (!category.isEmpty())
        group.insert(0, category);
    group += QLatin1Char('/');

    QVariantMap map;
    obj->toMap(group, &map);
    QVariantMap::const_iterator it = map.constBegin();
    for (; it != map.constEnd(); ++it)
        s->setValue(it.key(), it.value());
}

} // Utils

#endif // SETTINGSUTILS_H
