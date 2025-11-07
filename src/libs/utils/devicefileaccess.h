// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "hostosinfo.h"
#include "utils_global.h"

#include "filepath.h"

class tst_unixdevicefileaccess; // For testing.

namespace Utils {

class CommandLine;
class RunResult;
class TextEncoding;

// Base class including dummy implementation usable as fallback.
class QTCREATOR_UTILS_EXPORT DeviceFileAccess
    : public std::enable_shared_from_this<DeviceFileAccess>
{
public:
    DeviceFileAccess();
    virtual ~DeviceFileAccess();

    virtual Result<Environment> deviceEnvironment() const;

protected:
    friend class FilePath;
    friend class ::tst_unixdevicefileaccess; // For testing.

    virtual QString mapToDevicePath(const QString &hostPath) const;

    virtual Result<bool> isExecutableFile(const FilePath &filePath) const;
    virtual Result<bool> isReadableFile(const FilePath &filePath) const;
    virtual Result<bool> isWritableFile(const FilePath &filePath) const;
    virtual Result<bool> isReadableDirectory(const FilePath &filePath) const;
    virtual Result<bool> isWritableDirectory(const FilePath &filePath) const;
    virtual Result<bool> isFile(const FilePath &filePath) const;
    virtual Result<bool> isDirectory(const FilePath &filePath) const;
    virtual Result<bool> isSymLink(const FilePath &filePath) const;
    virtual Result<bool> hasHardLinks(const FilePath &filePath) const;
    virtual Result<> ensureWritableDirectory(const FilePath &filePath) const;
    virtual Result<> ensureExistingFile(const FilePath &filePath) const;
    virtual Result<> createDirectory(const FilePath &filePath) const;
    virtual Result<bool> exists(const FilePath &filePath) const;
    virtual Result<> removeFile(const FilePath &filePath) const;
    virtual Result<> removeRecursively(const FilePath &filePath) const;
    virtual Result<> copyFile(const FilePath &filePath, const FilePath &target) const;
    virtual Result<> createSymLink(const FilePath &filePath, const FilePath &symLink) const;
    virtual Result<> copyRecursively(const FilePath &filePath, const FilePath &target) const;
    virtual Result<> renameFile(const FilePath &filePath, const FilePath &target) const;

    virtual Result<FilePath> symLinkTarget(const FilePath &filePath) const;
    virtual Result<FilePathInfo> filePathInfo(const FilePath &filePath) const;
    virtual Result<QDateTime> lastModified(const FilePath &filePath) const;
    virtual Result<QFile::Permissions> permissions(const FilePath &filePath) const;
    virtual Result<> setPermissions(const FilePath &filePath, QFile::Permissions) const;
    virtual Result<qint64> fileSize(const FilePath &filePath) const;
    virtual Result<QString> owner(const FilePath &filePath) const;
    virtual Result<uint> ownerId(const FilePath &filePath) const;
    virtual Result<QString> group(const FilePath &filePath) const;
    virtual Result<uint> groupId(const FilePath &filePath) const;
    virtual Result<qint64> bytesAvailable(const FilePath &filePath) const;
    virtual Result<QByteArray> fileId(const FilePath &filePath) const;

    virtual Result<std::optional<FilePath>> refersToExecutableFile(
            const FilePath &filePath,
            FilePath::MatchScope matchScope) const;

    virtual Result<> iterateDirectory(
            const FilePath &filePath,
            const FilePath::IterateDirCallback &callBack,
            const FileFilter &filter) const;

    virtual Result<QByteArray> fileContents(const FilePath &filePath,
                                            qint64 limit,
                                            qint64 offset) const;

    virtual Result<qint64> writeFileContents(const FilePath &filePath,
                                             const QByteArray &data) const;

    virtual Result<FilePath> createTempFile(const FilePath &filePath);
    virtual Result<FilePath> createTempDir(const FilePath &filePath);

    virtual std::vector<Result<std::unique_ptr<FilePathWatcher>>> watch(const FilePaths &paths) const;

    virtual TextEncoding processStdOutEncoding(const FilePath &executable) const;
    virtual TextEncoding processStdErrEncoding(const FilePath &executable) const;
};

class QTCREATOR_UTILS_EXPORT DesktopDeviceFileAccess : public DeviceFileAccess
{
public:
    DesktopDeviceFileAccess();
    ~DesktopDeviceFileAccess() override;

    static DeviceFileAccessPtr instance();

protected:
    Result<bool> isExecutableFile(const FilePath &filePath) const override;
    Result<bool> isReadableFile(const FilePath &filePath) const override;
    Result<bool> isWritableFile(const FilePath &filePath) const override;
    Result<bool> isReadableDirectory(const FilePath &filePath) const override;
    Result<bool> isWritableDirectory(const FilePath &filePath) const override;
    Result<bool> isFile(const FilePath &filePath) const override;
    Result<bool> isDirectory(const FilePath &filePath) const override;
    Result<bool> isSymLink(const FilePath &filePath) const override;
    Result<bool> hasHardLinks(const FilePath &filePath) const override;
    Result<> ensureExistingFile(const FilePath &filePath) const override;
    Result<> createDirectory(const FilePath &filePath) const override;
    Result<bool> exists(const FilePath &filePath) const override;
    Result<> removeFile(const FilePath &filePath) const override;
    Result<> removeRecursively(const FilePath &filePath) const override;
    Result<> copyFile(const FilePath &filePath, const FilePath &target) const override;
    Result<> createSymLink(const FilePath &filePath, const FilePath &symLink) const override;
    Result<> renameFile(const FilePath &filePath, const FilePath &target) const override;

    Result<FilePath> symLinkTarget(const FilePath &filePath) const override;
    Result<FilePathInfo> filePathInfo(const FilePath &filePath) const override;
    Result<QDateTime> lastModified(const FilePath &filePath) const override;
    Result<QFile::Permissions> permissions(const FilePath &filePath) const override;
    Result<> setPermissions(const FilePath &filePath, QFile::Permissions) const override;
    Result<qint64> fileSize(const FilePath &filePath) const override;
    Result<QString> owner(const FilePath &filePath) const override;
    Result<uint> ownerId(const FilePath &filePath) const override;
    Result<QString> group(const FilePath &filePath) const override;
    Result<uint> groupId(const FilePath &filePath) const override;
    Result<qint64> bytesAvailable(const FilePath &filePath) const override;
    Result<QByteArray> fileId(const FilePath &filePath) const override;

    Result<std::optional<FilePath>> refersToExecutableFile(
            const FilePath &filePath,
            FilePath::MatchScope matchScope) const override;

    Result<> iterateDirectory(
            const FilePath &filePath,
            const FilePath::IterateDirCallback &callBack,
            const FileFilter &filter) const override;

    Result<Environment> deviceEnvironment() const override;

    Result<QByteArray> fileContents(const FilePath &filePath,
                                          qint64 limit,
                                          qint64 offset) const override;
    Result<qint64> writeFileContents(const FilePath &filePath,
                                           const QByteArray &data) const override;

    Result<FilePath> createTempFile(const FilePath &filePath) override;
    Result<FilePath> createTempDir(const FilePath &filePath) override;

    std::vector<Result<std::unique_ptr<FilePathWatcher>>> watch(
        const FilePaths &paths) const override;

    TextEncoding processStdOutEncoding(const FilePath &executable) const override;
    TextEncoding processStdErrEncoding(const FilePath &executable) const override;
};

class QTCREATOR_UTILS_EXPORT UnixDeviceFileAccess : public DeviceFileAccess
{
public:
    ~UnixDeviceFileAccess() override;

protected:
    virtual Result<RunResult> runInShellImpl(const CommandLine &cmdLine,
                                             const QByteArray &inputData = {}) const = 0;
    Result<QByteArray> runInShell(const CommandLine &cmdLine, const QByteArray &stdInData = {}) const;
    Result<bool> runInShellSuccess(const CommandLine &cmdLine, const QByteArray &stdInData = {}) const;

    Result<bool> isExecutableFile(const FilePath &filePath) const override;
    Result<bool> isReadableFile(const FilePath &filePath) const override;
    Result<bool> isWritableFile(const FilePath &filePath) const override;
    Result<bool> isReadableDirectory(const FilePath &filePath) const override;
    Result<bool> isWritableDirectory(const FilePath &filePath) const override;
    Result<bool> isFile(const FilePath &filePath) const override;
    Result<bool> isDirectory(const FilePath &filePath) const override;
    Result<bool> isSymLink(const FilePath &filePath) const override;
    Result<bool> hasHardLinks(const FilePath &filePath) const override;
    Result<> ensureExistingFile(const FilePath &filePath) const override;
    Result<> createDirectory(const FilePath &filePath) const override;
    Result<bool> exists(const FilePath &filePath) const override;
    Result<> removeFile(const FilePath &filePath) const override;
    Result<> removeRecursively(const FilePath &filePath) const override;
    Result<> copyFile(const FilePath &filePath, const FilePath &target) const override;
    Result<> createSymLink(const FilePath &filePath, const FilePath &symLink) const override;
    Result<> renameFile(const FilePath &filePath, const FilePath &target) const override;

    Result<FilePathInfo> filePathInfo(const FilePath &filePath) const override;
    Result<FilePath> symLinkTarget(const FilePath &filePath) const override;
    Result<QDateTime> lastModified(const FilePath &filePath) const override;
    Result<QFile::Permissions> permissions(const FilePath &filePath) const override;
    Result<> setPermissions(const FilePath &filePath, QFile::Permissions) const override;
    Result<qint64> fileSize(const FilePath &filePath) const override;
    Result<QString> owner(const FilePath &filePath) const override;
    Result<uint> ownerId(const FilePath &filePath) const override;
    Result<QString> group(const FilePath &filePath) const override;
    Result<uint> groupId(const FilePath &filePath) const override;
    Result<qint64> bytesAvailable(const FilePath &filePath) const override;
    Result<QByteArray> fileId(const FilePath &filePath) const override;

    Result<> iterateDirectory(
            const FilePath &filePath,
            const FilePath::IterateDirCallback &callBack,
            const FileFilter &filter) const override;

    Result<Environment> deviceEnvironment() const override;
    Result<QByteArray> fileContents(const FilePath &filePath,
                                          qint64 limit,
                                          qint64 offset) const override;
    Result<qint64> writeFileContents(const FilePath &filePath,
                                           const QByteArray &data) const override;

    Result<FilePath> createTempFile(const FilePath &filePath) override;
    Result<FilePath> createTempDir(const FilePath &filePath) override;

    Result<> findUsingLs(const QString &current,
                         const FileFilter &filter,
                         QStringList *found,
                         const QString &start) const;

private:
    Result<FilePath> createTempPath(const FilePath &filePath, bool createDir);
    Result<> iterateWithFind(const FilePath &filePath,
                             const FileFilter &filter,
                             const FilePath::IterateDirCallback &callBack) const;

    QStringList statArgs(const FilePath &filePath,
                         const QString &linuxFormat,
                         const QString &macFormat) const;

    mutable bool m_tryUseFind = true;
    mutable std::optional<bool> m_hasMkTemp;
};

} // Utils
