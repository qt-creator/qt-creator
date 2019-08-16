/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "fileutils.h"
#include "hostosinfo.h"
#include "namevalueitem.h"

namespace Utils {

class QTCREATOR_UTILS_EXPORT DictKey
{
public:
    DictKey(const QString &name, Qt::CaseSensitivity cs) : name(name), caseSensitivity(cs) {}

    QString name;
    Qt::CaseSensitivity caseSensitivity;
};
inline bool operator<(const DictKey &k1, const DictKey &k2)
{
    return k1.name.compare(k2.name, k1.caseSensitivity) < 0;
}
inline bool operator>(const DictKey &k1, const DictKey &k2) { return k2 < k1; }

using NameValuePair = std::pair<QString, QString>;
using NameValuePairs = QVector<NameValuePair>;
using NameValueMap = QMap<DictKey, QPair<QString, bool>>;

class QTCREATOR_UTILS_EXPORT NameValueDictionary
{
public:
    using const_iterator = NameValueMap::const_iterator;

    explicit NameValueDictionary(OsType osType = HostOsInfo::hostOs())
        : m_osType(osType)
    {}
    explicit NameValueDictionary(const QStringList &env, OsType osType = HostOsInfo::hostOs());
    explicit NameValueDictionary(const NameValuePairs &nameValues);

    QStringList toStringList() const;
    QString value(const QString &key) const;
    void set(const QString &key, const QString &value, bool enabled = true);
    void unset(const QString &key);
    void modify(const NameValueItems &items);
    /// Return the KeyValueDictionary changes necessary to modify this into the other environment.
    NameValueItems diff(const NameValueDictionary &other, bool checkAppendPrepend = false) const;
    bool hasKey(const QString &key) const;
    OsType osType() const;
    Qt::CaseSensitivity nameCaseSensitivity() const;

    QString userName() const;

    void clear();
    int size() const;

    QString key(const_iterator it) const { return it.key().name; }
    QString value(const_iterator it) const { return it.value().first; }
    bool isEnabled(const_iterator it) const { return it.value().second; }

    const_iterator constBegin() const { return m_values.constBegin(); }
    const_iterator constEnd() const { return m_values.constEnd(); }
    const_iterator constFind(const QString &name) const;

    friend bool operator!=(const NameValueDictionary &first, const NameValueDictionary &second)
    {
        return !(first == second);
    }

    friend bool operator==(const NameValueDictionary &first, const NameValueDictionary &second)
    {
        return first.m_osType == second.m_osType && first.m_values == second.m_values;
    }

protected:
    NameValueMap::iterator findKey(const QString &key);
    const_iterator findKey(const QString &key) const;

    NameValueMap m_values;
    OsType m_osType;
};

} // namespace Utils
