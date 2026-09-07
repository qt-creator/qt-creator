// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "headerdependencies.h"

#include "cmakeprojectmanagertr.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QDataStream>

#include <limits>

using namespace Utils;

namespace CMakeProjectManager::Internal {

/*!
    \class CMakeProjectManager::Internal::HeaderDependencyStore

    Maps every scanned translation unit to the headers it includes, and answers
    which of those translation units have to be scanned again.

    Paths are interned: a header included by a thousand translation units is
    held once and referred to by an integer everywhere else. Timestamps are
    kept in an array parallel to the interned paths, so that header is also
    only ever queried from the file system once per refresh, no matter how many
    translation units name it.

    A translation unit needs scanning again when it is unknown, when the
    command line it was scanned with changed, when it or one of its headers has
    disappeared, or when one of them was modified after the scan ran. A header
    that gains a new \c #include is covered by the last case; a header that
    starts shadowing another one on the include path is not, and is expected to
    arrive as a command line change instead.
*/

static constexpr qint64 unknownTime = std::numeric_limits<qint64>::min();
static constexpr quint32 storeMagic = 0x716863db;
static constexpr quint32 storeVersion = 1;

static qint64 statFile(const FilePath &path)
{
    const QDateTime modified = path.lastModified();
    return modified.isValid() ? modified.toMSecsSinceEpoch() : HeaderDependencyStore::MissingFile;
}

HeaderDependencyStore::HeaderDependencyStore()
    : m_stat(statFile)
{}

HeaderDependencyStore::HeaderDependencyStore(const StatFunction &stat)
    : m_stat(stat)
{
    QTC_CHECK(m_stat);
}

int HeaderDependencyStore::idForPath(const FilePath &path)
{
    const auto it = m_pathIds.constFind(path);
    if (it != m_pathIds.constEnd())
        return *it;

    const int id = m_paths.size();
    m_paths.append(path);
    m_times.append(unknownTime);
    m_pathIds.insert(path, id);
    return id;
}

int HeaderDependencyStore::findPath(const FilePath &path) const
{
    return m_pathIds.value(path, -1);
}

/*!
    Records that \a scan found \a dependencies at \a scanTime, and returns
    whether that changed what the store holds for its source.

    A scan that finds what is already stored moves the scan time and nothing
    else. Reporting that as a new result would ask the callers to act on it,
    and a translation unit that stays stale after a scan - one that includes a
    header which is not generated yet - would ask them again on every pass.
*/
bool HeaderDependencyStore::insert(const SourceScan &scan,
                                   const FilePaths &dependencies,
                                   qint64 scanTime)
{
    Entry entry;
    entry.flagsHash = scan.flagsHash;
    entry.scanTime = scanTime;
    entry.dependencyIds.reserve(dependencies.size());
    for (const FilePath &dependency : dependencies)
        entry.dependencyIds.append(idForPath(dependency));
    Utils::sort(entry.dependencyIds);
    entry.dependencyIds.erase(std::unique(entry.dependencyIds.begin(), entry.dependencyIds.end()),
                              entry.dependencyIds.end());

    const int sourceId = idForPath(scan.source);
    const auto it = m_entries.constFind(sourceId);
    const bool changed = it == m_entries.constEnd() || it->flagsHash != entry.flagsHash
                         || it->dependencyIds != entry.dependencyIds;

    m_entries.insert(sourceId, entry);
    return changed;
}

/*!
    Drops what is stored for every source outside \a sources, and returns
    whether anything was dropped.

    A source that leaves the project would otherwise keep its result for as
    long as the build directory lives.
*/
bool HeaderDependencyStore::retainSources(const FilePaths &sources)
{
    QSet<int> keep;
    keep.reserve(sources.size());
    for (const FilePath &source : sources) {
        const int id = findPath(source);
        if (id != -1)
            keep.insert(id);
    }

    const int before = m_entries.size();
    for (auto it = m_entries.begin(); it != m_entries.end();) {
        if (keep.contains(it.key()))
            ++it;
        else
            it = m_entries.erase(it);
    }

    return m_entries.size() != before;
}

void HeaderDependencyStore::clear()
{
    m_paths.clear();
    m_pathIds.clear();
    m_times.clear();
    m_entries.clear();
    m_statCount = 0;
}

bool HeaderDependencyStore::isEmpty() const
{
    return m_entries.isEmpty();
}

FilePaths HeaderDependencyStore::sources() const
{
    FilePaths result;
    result.reserve(m_entries.size());
    for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it)
        result.append(m_paths.at(it.key()));
    return result;
}

FilePaths HeaderDependencyStore::dependencies(const FilePath &source) const
{
    const int id = findPath(source);
    if (id == -1)
        return {};

    const auto it = m_entries.constFind(id);
    if (it == m_entries.constEnd())
        return {};

    return Utils::transform(it->dependencyIds, [this](int id) { return m_paths.at(id); });
}

qint64 HeaderDependencyStore::lastModified(int id)
{
    QTC_ASSERT(id >= 0 && id < m_times.size(), return MissingFile);

    if (m_times.at(id) == unknownTime) {
        m_times[id] = m_stat(m_paths.at(id));
        ++m_statCount;
    }
    return m_times.at(id);
}

bool HeaderDependencyStore::isStale(const SourceScan &scan)
{
    const int sourceId = findPath(scan.source);
    if (sourceId == -1)
        return true;

    const auto it = m_entries.constFind(sourceId);
    if (it == m_entries.constEnd())
        return true;

    if (it->flagsHash != scan.flagsHash)
        return true;

    const qint64 sourceTime = lastModified(sourceId);
    if (sourceTime == MissingFile || sourceTime > it->scanTime)
        return true;

    for (int id : it->dependencyIds) {
        const qint64 time = lastModified(id);
        if (time == MissingFile || time > it->scanTime)
            return true;
    }

    return false;
}

SourceScans HeaderDependencyStore::staleScans(const SourceScans &scans)
{
    return Utils::filtered(scans, [this](const SourceScan &scan) { return isStale(scan); });
}

void HeaderDependencyStore::forgetTimes()
{
    m_times.fill(unknownTime);
    m_statCount = 0;
}

int HeaderDependencyStore::statCount() const
{
    return m_statCount;
}

/*!
    Writes the store to \a storeFile.

    Only the paths that the entries still name are written, and they are
    interned again as they are: a header that no source includes any more, or
    one that retainSources() dropped along with its source, leaves the store
    when it is written rather than staying in it for the life of the build
    directory.
*/
Result<> HeaderDependencyStore::save(const FilePath &storeFile) const
{
    QList<int> writtenPaths;
    QHash<int, int> writtenIds;
    const auto intern = [&writtenPaths, &writtenIds](int id) {
        const auto it = writtenIds.constFind(id);
        if (it != writtenIds.constEnd())
            return *it;

        const int writtenId = writtenPaths.size();
        writtenIds.insert(id, writtenId);
        writtenPaths.append(id);
        return writtenId;
    };

    for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
        intern(it.key());
        for (int id : it->dependencyIds)
            intern(id);
    }

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_0);

    stream << storeMagic << storeVersion;

    stream << quint32(writtenPaths.size());
    for (int id : writtenPaths)
        stream << m_paths.at(id).toUrlishString();

    stream << quint32(m_entries.size());
    for (auto it = m_entries.constBegin(); it != m_entries.constEnd(); ++it) {
        stream << quint32(writtenIds.value(it.key())) << it->flagsHash << it->scanTime
               << quint32(it->dependencyIds.size());
        for (int id : it->dependencyIds)
            stream << quint32(writtenIds.value(id));
    }

    if (const Result<> created = storeFile.parentDir().ensureWritableDir(); !created)
        return created;

    const Result<qint64> written = storeFile.writeFileContents(data);
    if (!written)
        return ResultError(written.error());

    return ResultOk;
}

Result<> HeaderDependencyStore::load(const FilePath &storeFile)
{
    clear();

    const Result<QByteArray> data = storeFile.fileContents();
    if (!data)
        return ResultError(data.error());

    QDataStream stream(*data);
    stream.setVersion(QDataStream::Qt_6_0);

    quint32 magic = 0;
    quint32 version = 0;
    stream >> magic >> version;
    if (magic != storeMagic) {
        return ResultError(Tr::tr("\"%1\" is not a header dependency store.")
                               .arg(storeFile.toUserOutput()));
    }
    if (version != storeVersion) {
        return ResultError(Tr::tr("The header dependency store \"%1\" has version %2, "
                                  "but version %3 was expected.")
                               .arg(storeFile.toUserOutput())
                               .arg(version)
                               .arg(storeVersion));
    }

    quint32 pathCount = 0;
    stream >> pathCount;
    m_paths.reserve(pathCount);
    m_times.reserve(pathCount);
    for (quint32 i = 0; i < pathCount; ++i) {
        QString path;
        stream >> path;
        if (stream.status() != QDataStream::Ok)
            break;
        m_paths.append(FilePath::fromString(path));
        m_times.append(unknownTime);
        m_pathIds.insert(m_paths.last(), int(i));
    }

    quint32 entryCount = 0;
    stream >> entryCount;
    m_entries.reserve(entryCount);
    for (quint32 i = 0; i < entryCount; ++i) {
        quint32 sourceId = 0;
        quint32 dependencyCount = 0;
        Entry entry;
        stream >> sourceId >> entry.flagsHash >> entry.scanTime >> dependencyCount;
        if (stream.status() != QDataStream::Ok)
            break;

        entry.dependencyIds.reserve(dependencyCount);
        for (quint32 j = 0; j < dependencyCount; ++j) {
            quint32 id = 0;
            stream >> id;
            if (int(id) < m_paths.size())
                entry.dependencyIds.append(int(id));
        }

        if (int(sourceId) < m_paths.size())
            m_entries.insert(int(sourceId), entry);
    }

    if (stream.status() != QDataStream::Ok) {
        clear();
        return ResultError(Tr::tr("The header dependency store \"%1\" is truncated or corrupt.")
                               .arg(storeFile.toUserOutput()));
    }

    return ResultOk;
}

/*!
    Returns the headers among \a dependencies that belong to the project, that
    is, the ones below \a sourceDirectory or \a buildDirectory.

    The remaining headers come from the toolchain, the platform SDK or an
    installed package. They outnumber the project's own headers by orders of
    magnitude and are already covered by the include paths of the project part,
    so handing them to the code model as project files would only cost memory
    and indexing time.
*/
FilePaths projectHeaders(const FilePaths &dependencies,
                         const FilePath &sourceDirectory,
                         const FilePath &buildDirectory)
{
    return Utils::filtered(dependencies, [&](const FilePath &dependency) {
        return dependency.isChildOf(sourceDirectory) || dependency.isChildOf(buildDirectory);
    });
}

} // namespace CMakeProjectManager::Internal
