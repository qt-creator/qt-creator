/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <QMutex>
#include <QMutexLocker>
#include <QStringList>
#include <QVector>

#include <utils/optional.h>

namespace ProjectExplorer {

template<class T, int Size = 16>
class Cache
{
public:
    Cache() { m_cache.reserve(Size); }
    Cache(const Cache &other) = delete;
    Cache &operator =(const Cache &other) = delete;

    Cache(Cache &&other)
    {
        using std::swap;

        QMutexLocker otherLocker(&other.m_mutex);
        swap(m_cache, other.m_cache);
    }

    Cache &operator =(Cache &&other)
    {
        using std::swap;

        QMutexLocker locker(&m_mutex);
        QMutexLocker otherLocker(&other.m_mutex);
        auto temporay(std::move(other.m_cache)); // Make sure other.m_cache is empty!
        swap(m_cache, temporay);
        return *this;
    }

    void insert(const QStringList &compilerArguments, const T &values)
    {
        CacheItem runResults;
        runResults.first = compilerArguments;
        runResults.second = values;

        QMutexLocker locker(&m_mutex);
        if (!checkImpl(compilerArguments)) {
            if (m_cache.size() < Size) {
                m_cache.push_back(runResults);
            } else {
                std::rotate(m_cache.begin(), std::next(m_cache.begin()), m_cache.end());
                m_cache.back() = runResults;
            }
        }
    }

    Utils::optional<T> check(const QStringList &compilerArguments)
    {
        QMutexLocker locker(&m_mutex);
        return checkImpl(compilerArguments);
    }

    void invalidate()
    {
        QMutexLocker locker(&m_mutex);
        m_cache.clear();
    }

private:
    Utils::optional<T> checkImpl(const QStringList &compilerArguments)
    {
        auto it = std::stable_partition(m_cache.begin(), m_cache.end(), [&](const CacheItem &ci) {
            return ci.first != compilerArguments;
        });
        if (it != m_cache.end())
            return m_cache.back().second;
        return {};
    }

    using CacheItem = QPair<QStringList, T>;

    QMutex m_mutex;
    QVector<CacheItem> m_cache;
};

} // namespace ProjectExplorer
