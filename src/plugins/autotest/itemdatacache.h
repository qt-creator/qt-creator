// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "itestframework.h"
#include "testtreeitem.h"

#include <QRegularExpression>
#include <QVariantHash>

#include <optional>

namespace Autotest {
namespace Internal {

template<class T>
class ItemDataCache
{
public:
    void insert(ITestTreeItem *item, const T &value)
    {
        m_cache[item->cacheName()] = {0, value, item->testBase()->type()};
    }

    /* \a type represents an OR'ed value of ITestBase::TestBaseType */
    void evolve(int type)
    {
        auto it = m_cache.begin(), end = m_cache.end();
        while (it != end) {
            if ((it->type & type) && it->generation++ >= maxGen)
                it = m_cache.erase(it);
            else
                ++it;
        }
    }

    std::optional<T> get(ITestTreeItem *item)
    {
        auto entry = m_cache.find(item->cacheName());
        if (entry == m_cache.end())
            return std::nullopt;
        entry->generation = 0;
        return std::make_optional(entry->value);
    };

    void clear() { m_cache.clear(); }
    bool isEmpty() const { return m_cache.isEmpty(); }

    QVariantMap toSettings(const T &valueToIgnore) const
    {
        QVariantMap result;
        for (auto it = m_cache.cbegin(), end = m_cache.cend(); it != end; ++it) {
            if (it.value().value == valueToIgnore)
                continue;
            result.insert(QString::number(it.value().type) + '@'
                          + it.key(), QVariant::fromValue(it.value().value));
        }
        return result;
    }

    void fromSettings(const QVariantMap &stored)
    {
        static const QRegularExpression regex("^((\\d+)@)?(.*)$");
        m_cache.clear();
        for (auto it = stored.cbegin(), end = stored.cend(); it != end; ++it) {
            const QRegularExpressionMatch match = regex.match(it.key());
            ITestBase::TestBaseType type = match.hasMatch()
                    ? static_cast<ITestBase::TestBaseType>(match.captured(2).toInt())
                    : ITestBase::Framework;
            m_cache[match.captured(3)] = {0, qvariant_cast<T>(it.value()), type};
        }
    }

private:
    static constexpr int maxGen = 10;
    struct Entry
    {
        int generation = 0;
        T value;
        ITestBase::TestBaseType type;
    };
    QHash<QString, Entry> m_cache;
};

} // namespace Internal
} // namespace Autotest
