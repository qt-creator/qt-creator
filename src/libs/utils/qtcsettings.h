// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "store.h"

#include <QSettings>

namespace Utils {

class QTCREATOR_UTILS_EXPORT QtcSettings : private QSettings
{
public:
    using QSettings::QSettings;
    using QSettings::group;
    using QSettings::endGroup;
    using QSettings::allKeys;
    using QSettings::fileName;
    using QSettings::setParent;
    using QSettings::sync;
    using QSettings::beginReadArray;
    using QSettings::beginWriteArray;
    using QSettings::endArray;
    using QSettings::setArrayIndex;
    using QSettings::childGroups;
    using QSettings::status;
    using QSettings::clear;

    void beginGroup(const Key &prefix);

    QVariant value(const Key &key) const;
    QVariant value(const Key &key, const QVariant &def) const;
    void setValue(const Key &key, const QVariant &value);
    void remove(const Key &key);
    bool contains(const Key &key) const;

    KeyList childKeys() const;

    template<typename T>
    void setValueWithDefault(const Key &key, const T &val, const T &defaultValue)
    {
        if (val == defaultValue)
            remove(key);
        else
            setValue(key, val);
    }

    template<typename T>
    void setValueWithDefault(const Key &key, const T &val)
    {
        if (val == T())
            remove(key);
        else
            setValue(key, val);
    }
};

} // namespace Utils
