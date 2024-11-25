// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "hostosinfo.h"
#include "namevalueitem.h"

namespace Utils {

class QTCREATOR_UTILS_EXPORT DictKey
{
public:
    DictKey(const QString &name, Qt::CaseSensitivity cs) : name(name), caseSensitivity(cs) {}

    friend bool operator==(const DictKey &k1, const DictKey &k2)
    {
        return k1.name.compare(k2.name, k1.caseSensitivity) == 0;
    }

    QString name;
    Qt::CaseSensitivity caseSensitivity;
};
inline bool operator<(const DictKey &k1, const DictKey &k2)
{
    return k1.name.compare(k2.name, k1.caseSensitivity) < 0;
}
inline bool operator>(const DictKey &k1, const DictKey &k2) { return k2 < k1; }

using NameValuePair = std::pair<QString, QString>;
using NameValuePairs = QList<NameValuePair>;
using NameValueMap = QMap<DictKey, QPair<QString, bool>>;

class QTCREATOR_UTILS_EXPORT NameValueDictionary
{
public:
    class const_iterator
    {
        NameValueMap::const_iterator it;

    public:
        const_iterator(NameValueMap::const_iterator it)
            : it(it)
        {}
        // clang-format off
        const_iterator &operator++() { ++it; return *this; }
        const_iterator &operator++(int) { it++; return *this; }
        const_iterator &operator--() { --it; return *this; }
        const_iterator &operator--(int) { it--; return *this; }
        // clang-format on

        bool operator==(const const_iterator &other) const { return it == other.it; }
        bool operator!=(const const_iterator &other) const { return it != other.it; }
        std::tuple<QString, QString, bool> operator*() const
        {
            return std::make_tuple(it.key().name, it.value().first, it.value().second);
        }

        QString key() const { return it.key().name; }
        QString value() const { return it.value().first; }
        bool enabled() const { return it.value().second; }

        using difference_type = NameValueMap::const_iterator::difference_type;
        using value_type = std::tuple<QString, QString, bool>;
        using pointer = const value_type *;
        using reference = const value_type &;
        using iterator_category = NameValueMap::const_iterator::iterator_category;
    };

    explicit NameValueDictionary(OsType osType = HostOsInfo::hostOs())
        : m_osType(osType)
    {}
    explicit NameValueDictionary(const QStringList &env, OsType osType = HostOsInfo::hostOs());
    explicit NameValueDictionary(const NameValuePairs &nameValues);

    QStringList toStringList() const;
    QString value(const QString &key) const;
    void set(const QString &key, const QString &value, bool enabled = true);
    void unset(const QString &key);
    void modify(const EnvironmentItems &items);
    /// Return the KeyValueDictionary changes necessary to modify this into the other environment.
    EnvironmentItems diff(const NameValueDictionary &other, bool checkAppendPrepend = false) const;
    bool hasKey(const QString &key) const;
    OsType osType() const;
    Qt::CaseSensitivity nameCaseSensitivity() const;

    QString userName() const;

    void clear();
    int size() const;

    const const_iterator begin() const { return const_iterator(m_values.begin()); }
    const const_iterator end() const { return const_iterator(m_values.end()); }
    const const_iterator find(const QString &key) const { return const_iterator(findKey(key)); }

    friend bool operator!=(const NameValueDictionary &first, const NameValueDictionary &second)
    {
        return !(first == second);
    }

    friend bool operator==(const NameValueDictionary &first, const NameValueDictionary &second)
    {
        return first.m_osType == second.m_osType && first.m_values == second.m_values;
    }

protected:
    friend class Environment;
    NameValueMap::iterator findKey(const QString &key);
    NameValueMap::const_iterator findKey(const QString &key) const;

    NameValueMap m_values;
    OsType m_osType;
};

} // namespace Utils
