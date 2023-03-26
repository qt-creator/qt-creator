// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../filepath.h"

#include <QCache>
#include <QMutex>
#include <QMutexLocker>

namespace Utils::Internal {

class FilePathInfoCache
{
public:
    struct CachedData
    {
        FilePathInfo filePathInfo;
        QDateTime timeout;
    };

    using RetrievalFunction = CachedData (*)(const FilePath &);

public:
    FilePathInfoCache()
        : m_cache(50000)
    {}

    CachedData cached(const FilePath &filePath, const RetrievalFunction &retrievalFunction)
    {
        QMutexLocker lk(&m_mutex);
        CachedData *data = m_cache.object(filePath);

        // If the cache entry is too old, don't use it ...
        if (data && data->timeout < QDateTime::currentDateTime())
            data = nullptr;

        // If no data was found, retrieve it and store it in the cache ...
        if (!data) {
            data = new CachedData;
            *data = retrievalFunction(filePath);
            if (Q_UNLIKELY(!m_cache.insert(filePath, data))) {
                // This path will never happen, but to silence coverity we
                // have to check it since insert in theory could delete
                // the object if a cost bigger than the cache size is
                // specified.
                return {};
            }
        }

        // Return a copy of the data, so it cannot be deleted by the cache
        return *data;
    }

    void cache(const FilePath &path, CachedData *data)
    {
        QMutexLocker lk(&m_mutex);
        m_cache.insert(path, data);
    }

    void cache(const QList<QPair<FilePath, CachedData>> &fileDataList)
    {
        QMutexLocker lk(&m_mutex);
        for (const auto &[path, data] : fileDataList)
            m_cache.insert(path, new CachedData(data));
    }

    void invalidate(const FilePath &path)
    {
        QMutexLocker lk(&m_mutex);
        m_cache.remove(path);
    }

private:
    QMutex m_mutex;
    QCache<FilePath, CachedData> m_cache;
};

} // namespace Utils::Internal
