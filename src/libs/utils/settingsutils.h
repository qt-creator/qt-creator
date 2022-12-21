// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QSettings>
#include <QString>
#include <QStringList>
#include <QVariant>

namespace Utils {

template <class SettingsClassT>
void fromSettings(const QString &postFix,
                  const QString &category,
                  QSettings *s,
                  SettingsClassT *obj)
{
    QVariantMap map;
    s->beginGroup(category + postFix);
    const QStringList keys = s->allKeys();
    for (const QString &key : keys)
        map.insert(key, s->value(key));
    s->endGroup();
    obj->fromMap(map);
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
    const QVariantMap map = obj->toMap();

    s->beginGroup(group);
    for (auto it = map.constBegin(), end = map.constEnd(); it != end; ++it)
        s->setValue(it.key(), it.value());
    s->endGroup();
}

} // Utils
