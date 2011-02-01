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

#ifndef SETTINGSUTILS_H
#define SETTINGSUTILS_H

#include <QtCore/QString>
#include <QtCore/QLatin1String>
#include <QtCore/QSettings>
#include <QtCore/QVariant>

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
