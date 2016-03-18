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

#pragma once

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
