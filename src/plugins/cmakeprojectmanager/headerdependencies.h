// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>
#include <utils/result.h>

#include <QHash>
#include <QList>

#include <functional>

namespace CMakeProjectManager::Internal {

class SourceScan
{
public:
    Utils::FilePath source;
    quint64 flagsHash = 0;

    friend bool operator==(const SourceScan &, const SourceScan &) = default;
};

using SourceScans = QList<SourceScan>;

class HeaderDependencyStore
{
public:
    using StatFunction = std::function<qint64(const Utils::FilePath &path)>;

    static constexpr qint64 MissingFile = -1;

    HeaderDependencyStore();
    explicit HeaderDependencyStore(const StatFunction &stat);

    bool insert(const SourceScan &scan, const Utils::FilePaths &dependencies, qint64 scanTime);
    bool retainSources(const Utils::FilePaths &sources);
    void clear();

    bool isEmpty() const;
    Utils::FilePaths sources() const;
    Utils::FilePaths dependencies(const Utils::FilePath &source) const;

    SourceScans staleScans(const SourceScans &scans);

    void forgetTimes();
    int statCount() const;

    Utils::Result<> save(const Utils::FilePath &storeFile) const;
    Utils::Result<> load(const Utils::FilePath &storeFile);

private:
    class Entry
    {
    public:
        QList<int> dependencyIds;
        quint64 flagsHash = 0;
        qint64 scanTime = 0;
    };

    int idForPath(const Utils::FilePath &path);
    int findPath(const Utils::FilePath &path) const;
    qint64 lastModified(int id);
    bool isStale(const SourceScan &scan);

    StatFunction m_stat;
    Utils::FilePaths m_paths;
    QHash<Utils::FilePath, int> m_pathIds;
    QList<qint64> m_times;
    QHash<int, Entry> m_entries;
    int m_statCount = 0;
};

Utils::FilePaths projectHeaders(const Utils::FilePaths &dependencies,
                                const Utils::FilePath &sourceDirectory,
                                const Utils::FilePath &buildDirectory);

} // namespace CMakeProjectManager::Internal
