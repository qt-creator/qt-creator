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

#include "itestframework.h"
#include "testtreeitem.h"

#include <utils/optional.h>

#include <QRegularExpression>
#include <QVariantHash>

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

    Utils::optional<T> get(ITestTreeItem *item)
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
            result.insert(QString::number(it.value().type) + '@'
                          + it.key(), QVariant::fromValue(it.value().value));
        return result;
    }

    void fromSettings(const QVariantMap &stored)
    {
        const QRegularExpression regex("^((\\d+)@)?(.*)$");
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
