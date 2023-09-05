// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "persistentcachestore.h"

#include "filepath.h"
#include "fileutils.h"

#include <QMap>
#include <QMutex>
#include <QStandardPaths>

namespace Utils {

class PrivateGlobal
{
public:
    QMutex mutex;
    QMap<Key, Store> caches;
};

static expected_str<FilePath> cacheFolder()
{
    static const FilePath folder = FilePath::fromUserInput(QStandardPaths::writableLocation(
                                       QStandardPaths::CacheLocation))
                                   / "CachedStores";
    static expected_str<void> created = folder.ensureWritableDir();
    static expected_str<FilePath> result = created ? folder
                                                   : expected_str<FilePath>(
                                                       make_unexpected(created.error()));

    QTC_ASSERT_EXPECTED(result, return result);
    return result;
}

static PrivateGlobal &globals()
{
    static PrivateGlobal global;
    return global;
}

static expected_str<FilePath> filePathFromKey(const Key &cacheKey)
{
    static const expected_str<FilePath> folder = cacheFolder();
    if (!folder)
        return folder;

    return (*folder / FileUtils::fileSystemFriendlyName(stringFromKey(cacheKey))).withSuffix(".json");
}

expected_str<Store> PersistentCacheStore::byKey(const Key &cacheKey)
{
    const expected_str<FilePath> path = filePathFromKey(cacheKey);
    if (!path)
        return make_unexpected(path.error());

    QMutexLocker locker(&globals().mutex);

    auto it = globals().caches.find(cacheKey);
    if (it != globals().caches.end())
        return it.value();

    const expected_str<QByteArray> contents = path->fileContents();
    if (!contents)
        return make_unexpected(contents.error());

    auto result = storeFromJson(*contents);
    if (!result)
        return result;

    if (result->value("__cache_key__").toString() != stringFromKey(cacheKey)) {
        return make_unexpected(QString("Cache key mismatch: \"%1\" to \"%2\" in \"%3\".")
                                   .arg(stringFromKey(cacheKey))
                                   .arg(result->value("__cache_key__").toString())
                                   .arg(path->toUserOutput()));
    }

    return result;
}

expected_str<void> PersistentCacheStore::write(const Key &cacheKey, const Store &store)
{
    const expected_str<FilePath> path = filePathFromKey(cacheKey);
    if (!path)
        return make_unexpected(path.error());

    QMutexLocker locker(&globals().mutex);
    globals().caches.insert(cacheKey, store);

    // TODO: The writing of the store data could be done in a separate thread in the future.
    Store storeCopy = store;
    storeCopy.insert("__cache_key__", stringFromKey(cacheKey));
    storeCopy.insert("__last_modified__", QDateTime::currentDateTime().toString(Qt::ISODate));
    QByteArray json = jsonFromStore(storeCopy);
    const expected_str<qint64> result = path->writeFileContents(json);
    if (!result)
        return make_unexpected(result.error());
    return {};
}

expected_str<void> PersistentCacheStore::clear(const Key &cacheKey)
{
    const expected_str<FilePath> path = filePathFromKey(cacheKey);
    if (!path)
        return make_unexpected(path.error());

    QMutexLocker locker(&globals().mutex);
    globals().caches.remove(cacheKey);

    if (!path->removeFile())
        return make_unexpected(QString("Failed to remove cache file."));
    return {};
}

} // namespace Utils
