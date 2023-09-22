// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "store.h"

#include <QSettings>

namespace Utils {

// FIXME: In theory, this could be private or aggregated.
class QTCREATOR_UTILS_EXPORT QtcSettings : public QSettings
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

    void beginGroup(const Key &prefix) { QSettings::beginGroup(stringFromKey(prefix)); }

    QVariant value(const Key &key) const { return QSettings::value(stringFromKey(key)); }
    QVariant value(const Key &key, const QVariant &def) const;
    void setValue(const Key &key, const QVariant &value);
    void remove(const Key &key) { QSettings::remove(stringFromKey(key)); }
    bool contains(const Key &key) const { return QSettings::contains(stringFromKey(key)); }

    KeyList childKeys() const;

    template<typename T>
    void setValueWithDefault(const Key &key, const T &val, const T &defaultValue)
    {
        setValueWithDefault(this, key, val, defaultValue);
    }

    template<typename T>
    static void setValueWithDefault(QtcSettings *settings,
                                    const Key &key,
                                    const T &val,
                                    const T &defaultValue)
    {
        if (val == defaultValue)
            settings->QSettings::remove(stringFromKey(key));
        else
            settings->QSettings::setValue(stringFromKey(key), QVariant::fromValue(val));
    }


    template<typename T>
    void setValueWithDefault(const Key &key, const T &val)
    {
        setValueWithDefault(this, key, val);
    }

    template<typename T>
    static void setValueWithDefault(QtcSettings *settings, const Key &key, const T &val)
    {
        if (val == T())
            settings->QSettings::remove(stringFromKey(key));
        else
            settings->QSettings::setValue(stringFromKey(key), QVariant::fromValue(val));
    }
};

} // namespace Utils
