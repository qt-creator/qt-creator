// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <QPair>
#include <QWaitCondition>

#include <functional>
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
        QMutexLocker locker(&m_mutex);
        insertImpl(key, values);
    }

    // The cached values of the key, or those that compute() returns. Threads that ask for a key
    // that another thread is computing wait for its result instead of computing it again.
    // What was being computed when the cache was invalidated is neither waited for nor kept.
    T value(const K &key, const std::function<T()> &compute)
    {
        int generation;
        {
            QMutexLocker locker(&m_mutex);
            while (true) {
                if (std::optional<T> cached = checkImpl(key))
                    return *cached;
                if (!m_computing.contains(key))
                    break;
                m_computed.wait(&m_mutex);
            }
            m_computing.append(key);
            generation = m_generation;
        }
        T values = compute();
        QMutexLocker locker(&m_mutex);
        if (generation == m_generation) {
            m_computing.removeOne(key);
            insertImpl(key, values);
        }
        m_computed.wakeAll();
        return values;
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
        m_computing.clear();
        ++m_generation;
        m_computed.wakeAll(); // The waiters compute for themselves.
    }

private:
    void insertImpl(const K &key, const T &values)
    {
        if (!checkImpl(key)) {
            if (m_cache.size() < Size) {
                m_cache.push_back({key, values});
            } else {
                std::rotate(m_cache.begin(), std::next(m_cache.begin()), m_cache.end());
                m_cache.back() = {key, values};
            }
        }
    }

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
    QList<CacheItem> m_cache;
    QList<K> m_computing;
    int m_generation = 0;
    QWaitCondition m_computed;
};

} // namespace ProjectExplorer
