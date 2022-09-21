// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "fsengine_impl.h"

#include "diriterator.h"

#include "../filepath.h"
#include "../qtcassert.h"

#include <QIODevice>
#include <QDateTime>

namespace Utils {

namespace Internal {

FSEngineImpl::FSEngineImpl(FilePath filePath)
    : m_filePath(std::move(filePath))
{}

FSEngineImpl::~FSEngineImpl()
{
    delete m_tempStorage;
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
bool FSEngineImpl::open(QIODeviceBase::OpenMode openMode, std::optional<QFile::Permissions>)
#else
bool FSEngineImpl::open(QIODevice::OpenMode openMode)
#endif
{
    ensureStorage();

    QTC_ASSERT(m_tempStorage->open(), return false);

    bool read = openMode & QIODevice::ReadOnly;
    bool write = openMode & QIODevice::WriteOnly;
    bool append = openMode & QIODevice::Append;

    if (!write && !m_filePath.exists())
        return false;

    if (openMode & QIODevice::NewOnly && m_filePath.exists())
        return false;

    if (read || append) {
        const std::optional<QByteArray> contents = m_filePath.fileContents();
        QTC_ASSERT(contents && m_tempStorage->write(*contents) >= 0, return false);

        if (!append)
            m_tempStorage->seek(0);
    }

    if (write && !append)
        m_hasChangedContent = true;

    return true;
}

bool FSEngineImpl::close()
{
    QTC_ASSERT(flush(), return false);
    QTC_ASSERT(m_tempStorage, ensureStorage());
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
    return m_filePath.fileSize();
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
    return m_filePath.removeRecursively();
}

bool FSEngineImpl::copy(const QString &newName)
{
    return m_filePath.copyFile(FilePath::fromString(newName));
}

bool FSEngineImpl::rename(const QString &newName)
{
    return m_filePath.renameFile(FilePath::fromString(newName));
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

#if QT_VERSION >= QT_VERSION_CHECK(6, 3, 0)
bool FSEngineImpl::mkdir(const QString &dirName, bool createParentDirectories,
                         std::optional<QFile::Permissions>) const
#else
bool FSEngineImpl::mkdir(const QString &dirName, bool createParentDirectories) const
#endif
{
    Q_UNUSED(createParentDirectories)
    return FilePath::fromString(dirName).createDir();
}

bool FSEngineImpl::rmdir(const QString &dirName, bool recurseParentDirectories) const
{
    if (recurseParentDirectories)
        return false;

    return m_filePath.pathAppended(dirName).removeRecursively();
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
        [&result](const FilePath &p) {
            result.append(p.toFSPathString());
            return true;
        },
        {filterNames, filters});
    return result;
}

QAbstractFileEngine::FileFlags FSEngineImpl::fileFlags(FileFlags type) const
{
    FileFlags result{0};

    if (type & FileInfoAll && m_filePath.exists()) {
        result |= QAbstractFileEngine::ExistsFlag;

        if (type & DirectoryType && m_filePath.isDir())
            result |= QAbstractFileEngine::DirectoryType;
        if (type & FileType && m_filePath.isFile())
            result |= QAbstractFileEngine::FileType;

        if (type & PermsMask) {
            result |= FileFlags::fromInt(m_filePath.permissions().toInt());
        }
    }

    return result;
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
        return m_filePath.baseName();
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
    return m_filePath.lastModified();
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

bool FSEngineImpl::cloneTo(QAbstractFileEngine *target)
{
    return QAbstractFileEngine::cloneTo(target);
}

QAbstractFileEngine::Iterator *FSEngineImpl::beginEntryList(QDir::Filters filters,
                                                            const QStringList &filterNames)
{
    FilePaths paths{m_filePath.pathAppended(".")};
    m_filePath.iterateDirectory(
        [&paths](const FilePath &p) {
            paths.append(p);
            return true;
        },
        {filterNames, filters});

    return new DirIterator(std::move(paths));
}

QAbstractFileEngine::Iterator *FSEngineImpl::endEntryList()
{
    return nullptr;
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

} // namespace Internal
} // namespace Utils
