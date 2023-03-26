// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMutex>
#include <QMutexLocker>
#include <QPair>
#include <QVector>

#include <optional>

namespace ProjectExplorer {

template<class K, class T, int Size = 16>
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

    void insert(const K &key, const T &values)
    {
        CacheItem runResults;
        runResults.first = key;
        runResults.second = values;

        QMutexLocker locker(&m_mutex);
        if (!checkImpl(key)) {
            if (m_cache.size() < Size) {
                m_cache.push_back(runResults);
            } else {
                std::rotate(m_cache.begin(), std::next(m_cache.begin()), m_cache.end());
                m_cache.back() = runResults;
            }
        }
    }

    std::optional<T> check(const K &key)
    {
        QMutexLocker locker(&m_mutex);
        return checkImpl(key);
    }

    void invalidate()
    {
        QMutexLocker locker(&m_mutex);
        m_cache.clear();
    }

private:
    std::optional<T> checkImpl(const K &key)
    {
        auto it = std::stable_partition(m_cache.begin(), m_cache.end(), [&](const CacheItem &ci) {
            return ci.first != key;
        });
        if (it != m_cache.end())
            return m_cache.back().second;
        return {};
    }

    using CacheItem = QPair<K, T>;

    QMutex m_mutex;
    QVector<CacheItem> m_cache;
};

} // namespace ProjectExplorer
