/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "testtreeitem.h"

#include <utils/optional.h>

#include <QVariantHash>

namespace Autotest {
namespace Internal {

template<class T>
class ItemDataCache
{
public:
    void insert(TestTreeItem *item, const T &value) { m_cache[item->cacheName()] = {0, value}; }
    void evolve()
    {
        auto it = m_cache.begin(), end = m_cache.end();
        while (it != end)
            it = it->generation++ >= maxGen ? m_cache.erase(it) : ++it;
    }

    Utils::optional<T> get(TestTreeItem *item)
    {
        auto entry = m_cache.find(item->cacheName());
        if (entry == m_cache.end())
            return Utils::nullopt;
        entry->generation = 0;
        return Utils::make_optional(entry->value);
    };

    void clear() { m_cache.clear(); }
    bool isEmpty() const { return m_cache.isEmpty(); }

    QVariantMap toSettings() const
    {
        QVariantMap result;
        for (auto it = m_cache.cbegin(), end = m_cache.cend(); it != end; ++it)
            result.insert(it.key(), QVariant::fromValue(it.value().value));
        return result;
    }

    void fromSettings(const QVariantMap &stored)
    {
        m_cache.clear();
        for (auto it = stored.cbegin(), end = stored.cend(); it != end; ++it)
            m_cache[it.key()] = {0, qvariant_cast<T>(it.value())};
    }

private:
    static constexpr int maxGen = 10;
    struct Entry
    {
        int generation = 0;
        T value;
    };
    QHash<QString, Entry> m_cache;
};

} // namespace Internal
} // namespace Autotest
