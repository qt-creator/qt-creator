// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fsenginehandler.h"

#include "diriterator.h"
#include "expected.h"
#include "fileiteratordevicesappender.h"
#include "filepathinfocache.h"
#include "fixedlistfsengine.h"
#include "fsengine.h"

#include "../algorithm.h"
#include "../filepath.h"
#include "../hostosinfo.h"
#include "../qtcassert.h"

#include <QtCore/private/qfsfileengine_p.h>
#include <QtCore/private/qabstractfileengine_p.h>

#include <QDateTime>
#include <QIODevice>
#include <QTemporaryFile>

namespace Utils::Internal {

class FSEngineImpl final : public QAbstractFileEngine
{
public:
    FSEngineImpl(FilePath filePath);
    ~FSEngineImpl() final;

public:
    bool open(QIODeviceBase::OpenMode openMode,
              std::optional<QFile::Permissions> permissions = std::nullopt) final;
    bool mkdir(const QString &dirName, bool createParentDirectories,
              std::optional<QFile::Permissions> permissions = std::nullopt) const final;
    bool close() final;
    bool flush() final;
    bool syncToDisk() final;
    qint64 size() const final;
    qint64 pos() const final;
    bool seek(qint64 pos) final;
    bool isSequential() const final;
    bool remove() final;
    bool copy(const QString &newName) final;
    bool rename(const QString &newName) final;
    bool renameOverwrite(const QString &newName) final;
    bool link(const QString &newName) final;
    bool rmdir(const QString &dirName, bool recurseParentDirectories) const final;
    bool setSize(qint64 size) final;
    bool caseSensitive() const final;
    bool isRelativePath() const final;
    QStringList entryList(QDir::Filters filters, const QStringList &filterNames) const final;
    FileFlags fileFlags(FileFlags type) const final;
    bool setPermissions(uint perms) final;
    QByteArray id() const final;
    QString fileName(FileName file) const final;
    uint ownerId(FileOwner) const final;
    QString owner(FileOwner) const final;

    // The FileTime change in QAbstractFileEngine, in qtbase, is in since Qt 6.7.1
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 1)
    using FileTime = QFile::FileTime;
#endif
    bool setFileTime(const QDateTime &newDate, FileTime time) final;
    QDateTime fileTime(FileTime time) const final;
    void setFileName(const QString &file) final;
    int handle() const final;

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    IteratorUniquePtr beginEntryList(
        const QString &path,
        QDirListing::IteratorFlags filters,
        const QStringList &filterNames) final;

    IteratorUniquePtr endEntryList() final { return {}; }
#else
    Iterator *beginEntryList(QDir::Filters filters, const QStringList &filterNames) final;
    Iterator *endEntryList() final { return nullptr; }
#endif

    qint64 read(char *data, qint64 maxlen) final;
    qint64 readLine(char *data, qint64 maxlen) final;
    qint64 write(const char *data, qint64 len) final;
    bool extension(Extension extension,
                   const ExtensionOption *option,
                   ExtensionReturn *output) final;
    bool supportsExtension(Extension extension) const final;

private:
    void ensureStorage();

    FilePath m_filePath;
    QTemporaryFile *m_tempStorage = nullptr;

    bool m_hasChangedContent{false};
};


FilePathInfoCache g_filePathInfoCache;

FilePathInfoCache::CachedData createCacheData(const FilePath &filePath) {
    FilePathInfoCache::CachedData data;
    data.filePathInfo = filePath.filePathInfo();
    data.timeout = QDateTime::currentDateTime().addSecs(60);
    return data;
};

FSEngineImpl::FSEngineImpl(FilePath filePath)
    : m_filePath(std::move(filePath))
{}

FSEngineImpl::~FSEngineImpl()
{
    delete m_tempStorage;
}

bool FSEngineImpl::open(QIODeviceBase::OpenMode openMode, std::optional<QFile::Permissions>)
{
    const FilePathInfoCache::CachedData data = g_filePathInfoCache.cached(m_filePath,
                                                                          createCacheData);
    bool exists = (data.filePathInfo.fileFlags & QAbstractFileEngine::ExistsFlag);

    if (data.filePathInfo.fileFlags & QAbstractFileEngine::DirectoryType)
        return false;

    g_filePathInfoCache.invalidate(m_filePath);

    ensureStorage();

    const bool isOpened = m_tempStorage->open();
    QTC_ASSERT(isOpened, return false);

    bool read = openMode & QIODevice::ReadOnly;
    bool write = openMode & QIODevice::WriteOnly;
    bool append = openMode & QIODevice::Append;

    if (!write && !exists)
        return false;

    if (openMode & QIODevice::NewOnly && exists)
        return false;

    if (read || append) {
        const Result<QByteArray> readResult = m_filePath.fileContents();
        QTC_ASSERT_RESULT(readResult, return false);

        const Result<qint64> writeResult = m_tempStorage->write(*readResult);
        QTC_ASSERT_RESULT(writeResult, return false);

        if (!append)
            m_tempStorage->seek(0);
    }

    if (write && !append)
        m_hasChangedContent = true;

    return true;
}

bool FSEngineImpl::close()
{
    const bool isFlushed = flush();
    QTC_ASSERT(isFlushed, return false);
    if (m_tempStorage)
        m_tempStorage->close();
    return true;
}

bool FSEngineImpl::flush()
{
    return syncToDisk();
}

bool FSEngineImpl::syncToDisk()
{
    if (m_hasChangedContent) {
        ensureStorage();
        const qint64 oldPos = m_tempStorage->pos();
        QTC_ASSERT(m_tempStorage->seek(0), return false);
        QTC_ASSERT(m_filePath.writeFileContents(m_tempStorage->readAll()), return false);
        m_tempStorage->seek(oldPos);
        m_hasChangedContent = false;
        return true;
    }

    return true;
}

qint64 FSEngineImpl::size() const
{
    return g_filePathInfoCache.cached(m_filePath, createCacheData).filePathInfo.fileSize;
}

qint64 FSEngineImpl::pos() const
{
    QTC_ASSERT(m_tempStorage, return 0);
    return m_tempStorage->pos();
}

bool FSEngineImpl::seek(qint64 pos)
{
    QTC_ASSERT(m_tempStorage, return false);
    return m_tempStorage->seek(pos);
}

bool FSEngineImpl::isSequential() const
{
    return false;
}

bool FSEngineImpl::remove()
{
    Result<> result = m_filePath.removeRecursively();
    if (!result)
        setError(QFile::RemoveError, result.error());
    return result.has_value();
}

bool FSEngineImpl::copy(const QString &newName)
{
    Result<> result = m_filePath.copyFile(FilePath::fromString(newName));
    if (!result)
        setError(QFile::CopyError, result.error());
    return bool(result);
}

bool FSEngineImpl::rename(const QString &newName)
{
    Result<> result = m_filePath.renameFile(FilePath::fromString(newName));
    if (!result)
        setError(QFile::RenameError, result.error());
    return bool(result);
}

bool FSEngineImpl::renameOverwrite(const QString &newName)
{
    Q_UNUSED(newName)
    return false;
}

bool FSEngineImpl::link(const QString &newName)
{
    Q_UNUSED(newName)
    return false;
}

bool FSEngineImpl::mkdir(const QString &dirName, bool createParentDirectories,
                         std::optional<QFile::Permissions>) const
{
    Q_UNUSED(createParentDirectories)
    return FilePath::fromString(dirName).createDir();
}

bool FSEngineImpl::rmdir(const QString &dirName, bool recurseParentDirectories) const
{
    if (recurseParentDirectories)
        return false;

    return m_filePath.pathAppended(dirName).removeRecursively().has_value();
}

bool FSEngineImpl::setSize(qint64 size)
{
    QTC_ASSERT(m_tempStorage, return false);
    return m_tempStorage->resize(size);
}

bool FSEngineImpl::caseSensitive() const
{
    // TODO?
    return true;
}

bool FSEngineImpl::isRelativePath() const
{
    return false;
}

QStringList FSEngineImpl::entryList(QDir::Filters filters, const QStringList &filterNames) const
{
    QStringList result;
    m_filePath.iterateDirectory(
        [&result](const FilePath &p, const FilePathInfo &fi) {
            result.append(p.toFSPathString());
            g_filePathInfoCache
                .cache(p,
                       new FilePathInfoCache::CachedData{fi,
                                                         QDateTime::currentDateTime().addSecs(60)});
            return IterationPolicy::Continue;
        },
        {filterNames, filters});
    return result;
}

QAbstractFileEngine::FileFlags FSEngineImpl::fileFlags(FileFlags type) const
{
    Q_UNUSED(type)
    return {g_filePathInfoCache.cached(m_filePath, createCacheData).filePathInfo.fileFlags.toInt()};
}

bool FSEngineImpl::setPermissions(uint /*perms*/)
{
    return false;
}

QByteArray FSEngineImpl::id() const
{
    return QAbstractFileEngine::id();
}

QString FSEngineImpl::fileName(FileName file) const
{
    switch (file) {
    case QAbstractFileEngine::AbsoluteName:
    case QAbstractFileEngine::DefaultName:
        return m_filePath.toFSPathString();
        break;
    case QAbstractFileEngine::BaseName:
        if (m_filePath.fileName().isEmpty())
            return m_filePath.host().toString();
        return m_filePath.fileName();
        break;
    case QAbstractFileEngine::PathName:
    case QAbstractFileEngine::AbsolutePathName:
        return m_filePath.parentDir().toFSPathString();
        break;
    case QAbstractFileEngine::CanonicalName:
        return m_filePath.canonicalPath().toFSPathString();
        break;
    case QAbstractFileEngine::CanonicalPathName:
        return m_filePath.canonicalPath().parentDir().toFSPathString();
        break;
    default:
    // case QAbstractFileEngine::LinkName:
    // case QAbstractFileEngine::BundleName:
    // case QAbstractFileEngine::JunctionName:
        return {};
        break;

    }

    return QAbstractFileEngine::fileName(file);
}

uint FSEngineImpl::ownerId(FileOwner) const
{
    return 1;
}

QString FSEngineImpl::owner(FileOwner) const
{
    return "<unknown>";
}

bool FSEngineImpl::setFileTime(const QDateTime &newDate, FileTime time)
{
    Q_UNUSED(newDate)
    Q_UNUSED(time)
    return false;
}

QDateTime FSEngineImpl::fileTime(FileTime time) const
{
    Q_UNUSED(time)
    return g_filePathInfoCache.cached(m_filePath, createCacheData).filePathInfo.lastModified;
}

void FSEngineImpl::setFileName(const QString &file)
{
    close();
    m_filePath = FilePath::fromString(file);
}

int FSEngineImpl::handle() const
{
    return 0;
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)

QAbstractFileEngine::IteratorUniquePtr FSEngineImpl::beginEntryList(
    const QString &path, QDirListing::IteratorFlags itFlags, const QStringList &filterNames)
{
    const auto [filters, iteratorFlags] = convertQDirListingIteratorFlags(itFlags);

    FilePaths paths{m_filePath.pathAppended(".")};
    m_filePath.iterateDirectory(
        [&paths](const FilePath &p, const FilePathInfo &fi) {
            paths.append(p);
            FilePathInfoCache::CachedData *data
                = new FilePathInfoCache::CachedData{fi, QDateTime::currentDateTime().addSecs(60)};
            g_filePathInfoCache.cache(p, data);
            return IterationPolicy::Continue;
        },
        {filterNames, filters, iteratorFlags});

    return std::make_unique<DirIterator>(std::move(paths), path, filters, filterNames);
}

#else
QAbstractFileEngine::Iterator *FSEngineImpl::beginEntryList(QDir::Filters filters,
                                                            const QStringList &filterNames)
{
    FilePaths paths{m_filePath.pathAppended(".")};
    m_filePath.iterateDirectory(
        [&paths](const FilePath &p, const FilePathInfo &fi) {
            paths.append(p);
            FilePathInfoCache::CachedData *data
                = new FilePathInfoCache::CachedData{fi,
                                                    QDateTime::currentDateTime().addSecs(60)};
            g_filePathInfoCache.cache(p, data);
            return IterationPolicy::Continue;
        },
        {filterNames, filters});

    return new DirIterator(std::move(paths));
}
#endif

qint64 FSEngineImpl::read(char *data, qint64 maxlen)
{
    return m_tempStorage->read(data, maxlen);
}

qint64 FSEngineImpl::readLine(char *data, qint64 maxlen)
{
    return m_tempStorage->readLine(data, maxlen);
}

qint64 FSEngineImpl::write(const char *data, qint64 len)
{
    qint64 bytesWritten = m_tempStorage->write(data, len);

    if (bytesWritten > 0)
        m_hasChangedContent = true;

    return bytesWritten;
}

bool FSEngineImpl::extension(Extension extension,
                             const ExtensionOption *option,
                             ExtensionReturn *output)
{
    Q_UNUSED(extension)
    Q_UNUSED(option)
    Q_UNUSED(output)
    return false;
}

bool FSEngineImpl::supportsExtension(Extension extension) const
{
    Q_UNUSED(extension)
    return false;
}

void FSEngineImpl::ensureStorage()
{
    if (!m_tempStorage)
        m_tempStorage = new QTemporaryFile;
}

class RootInjectFSEngine final : public QFSFileEngine
{
public:
    using QFSFileEngine::QFSFileEngine;

public:
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    IteratorUniquePtr beginEntryList(
        const QString &path,
        QDirListing::IteratorFlags filters,
        const QStringList &filterNames) override
    {
        return std::make_unique<FileIteratorWrapper>(
            QFSFileEngine::beginEntryList(path, filters, filterNames));
    }
#else
    Iterator *beginEntryList(QDir::Filters filters, const QStringList &filterNames) final
    {
        std::unique_ptr<QAbstractFileEngineIterator> baseIterator(
            QFSFileEngine::beginEntryList(filters, filterNames));
        return new FileIteratorWrapper(std::move(baseIterator));
    }
#endif
};

static FilePath removeDoubleSlash(const QString &fileName)
{
    // Reduce every two or more slashes to a single slash.
    QString result;
    const QChar slash = QChar('/');
    bool lastWasSlash = false;
    for (const QChar &ch : fileName) {
        if (ch == slash) {
            if (!lastWasSlash)
                result.append(ch);
            lastWasSlash = true;
        } else {
            result.append(ch);
            lastWasSlash = false;
        }
    }
    // We use fromString() here to not normalize / clean the path anymore.
    return FilePath::fromString(result);
}

static bool isRootPath(const QString &fileName)
{
    if (HostOsInfo::isWindowsHost()) {
        static const QChar lowerDriveLetter = HostOsInfo::root().path().front().toUpper();
        static const QChar upperDriveLetter = HostOsInfo::root().path().front().toLower();
        return fileName.size() == 3 && fileName[1] == ':' && fileName[2] == '/'
               && (fileName[0] == lowerDriveLetter || fileName[0] == upperDriveLetter);
     }

     return fileName.size() == 1 && fileName[0] == '/';
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
std::unique_ptr<QAbstractFileEngine>
#else
QAbstractFileEngine *
#endif
FSEngineHandler::create(const QString &fileName) const
{
    if (fileName.startsWith(':'))
        return {};

    static const QString rootPath = FilePath::specialRootPath();
    static const FilePath rootFilePath = FilePath::fromString(rootPath);

    const QString fixedFileName = QDir::cleanPath(fileName);

    if (fixedFileName == rootPath) {
        const FilePaths paths
            = Utils::transform(FSEngine::registeredDeviceSchemes(), [](const QString &scheme) {
                  return rootFilePath.pathAppended(scheme);
              });

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        return std::make_unique<FixedListFSEngine>(removeDoubleSlash(fileName), paths);
#else
        return new FixedListFSEngine(removeDoubleSlash(fileName), paths);
#endif
    }

    if (fixedFileName.startsWith(rootPath)) {
        const QStringList deviceSchemes = FSEngine::registeredDeviceSchemes();
        for (const QString &scheme : deviceSchemes) {
            if (fixedFileName == rootFilePath.pathAppended(scheme).toUrlishString()) {
                const FilePaths filteredRoots = Utils::filtered(FSEngine::registeredDeviceRoots(),
                                                                [scheme](const FilePath &root) {
                                                                    return root.scheme() == scheme;
                                                                });

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
                return std::make_unique<FixedListFSEngine>(removeDoubleSlash(fileName),
                                                           filteredRoots);
#else
                return new FixedListFSEngine(removeDoubleSlash(fileName), filteredRoots);
#endif
            }
        }

        FilePath fixedPath = FilePath::fromString(fixedFileName);

        if (!fixedPath.isLocal()) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
            return std::make_unique<FSEngineImpl>(removeDoubleSlash(fileName));
#else
            return new FSEngineImpl(removeDoubleSlash(fileName));
#endif
        }
    }

    if (isRootPath(fixedFileName)) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
        return std::make_unique<RootInjectFSEngine>(fileName);
#else
        return new RootInjectFSEngine(fileName);
#endif
    }

    return {};
}

} // Utils::Internal
