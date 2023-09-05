// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "store.h"
#include "qtcsettings.h"

#include <QSettings>
#include <QStringList>
#include <QVariant>

namespace Utils {

template <class SettingsClassT>
void fromSettings(const Key &postFix,
                  const Key &category,
                  QtcSettings *s,
                  SettingsClassT *obj)
{
    Store map;
    s->beginGroup(category + postFix);
    const KeyList keys = keysFromStrings(s->allKeys());
    for (const Key &key : keys)
        map.insert(key, s->value(key));
    s->endGroup();
    obj->fromMap(map);
}

template <class SettingsClassT>
void toSettings(const Key &postFix,
                const Key &category,
                QtcSettings *s,
                const SettingsClassT *obj)
{
    Key group = postFix;
    if (!category.isEmpty())
        group.insert(0, category);
    const Store map = obj->toMap();

    s->beginGroup(group);
    for (auto it = map.constBegin(), end = map.constEnd(); it != end; ++it)
        s->setValue(it.key(), it.value());
    s->endGroup();
}

} // Utils
