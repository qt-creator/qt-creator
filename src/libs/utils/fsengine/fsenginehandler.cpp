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
    FileFlags fileFlags(FileFlags type) const final;
    bool setPermissions(uint perms) final;
    QByteArray id() const final;
    QString fileName(FileName file) const final;
    uint ownerId(FileOwner) const final;
    QString owner(FileOwner) const final;

    // The FileTime change in QAbstractFileEngine, in qtbase, is in since Qt 6.7.1
    using FileTime = QFile::FileTime;
    bool setFileTime(const QDateTime &newDate, FileTime time) final;
    QDateTime fileTime(FileTime time) const final;
    void setFileName(const QString &file) final;
    int handle() const final;

    IteratorUniquePtr beginEntryList(
        const QString &path,
        QDirListing::IteratorFlags filters,
        const QStringList &filterNames) final;
    IteratorUniquePtr endEntryList() final { return {}; }

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

void invalidateFileInfoCache(const Utils::FilePath &path)
{
    g_filePathInfoCache.invalidate(path);
}

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
        // Reading can legitimately fail at runtime (e.g. a dangling symlink that the
        // cached info still reports as existing, or insufficient permissions). That is
        // not a programming error, so fail the open gracefully instead of asserting.
        if (!readResult)
            return false;

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
    case QAbstractFileEngine::BaseName:
        if (m_filePath.fileName().isEmpty())
            return m_filePath.host().toString();
        return m_filePath.fileName();
    case QAbstractFileEngine::PathName:
    case QAbstractFileEngine::AbsolutePathName:
        return m_filePath.parentDir().toFSPathString();
    case QAbstractFileEngine::CanonicalName:
        return m_filePath.canonicalPath().toFSPathString();
    case QAbstractFileEngine::CanonicalPathName:
        return m_filePath.canonicalPath().parentDir().toFSPathString();
    default:
    // case QAbstractFileEngine::LinkName:
    // case QAbstractFileEngine::BundleName:
    // case QAbstractFileEngine::JunctionName:
        return {};
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
    IteratorUniquePtr beginEntryList(
        const QString &path,
        QDirListing::IteratorFlags filters,
        const QStringList &filterNames) override
    {
        return std::make_unique<FileIteratorWrapper>(
            QFSFileEngine::beginEntryList(path, filters, filterNames));
    }
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

std::unique_ptr<QAbstractFileEngine>
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

        return std::make_unique<FixedListFSEngine>(removeDoubleSlash(fileName), paths);
    }

    if (fixedFileName.startsWith(rootPath)) {
        const QStringList deviceSchemes = FSEngine::registeredDeviceSchemes();
        for (const QString &scheme : deviceSchemes) {
            if (fixedFileName == rootFilePath.pathAppended(scheme).toUrlishString()) {
                const FilePaths filteredRoots = Utils::filtered(FSEngine::registeredDeviceRoots(),
                                                                [scheme](const FilePath &root) {
                                                                    return root.scheme() == scheme;
                                                                });

                return std::make_unique<FixedListFSEngine>(removeDoubleSlash(fileName),
                                                           filteredRoots);
            }
        }

        FilePath fixedPath = FilePath::fromString(fixedFileName);

        if (!fixedPath.isLocal())
            return std::make_unique<FSEngineImpl>(removeDoubleSlash(fileName));
    }

    if (isRootPath(fixedFileName))
        return std::make_unique<RootInjectFSEngine>(fileName);

    return {};
}

} // Utils::Internal
