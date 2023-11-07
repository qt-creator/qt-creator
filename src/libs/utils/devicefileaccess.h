// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "hostosinfo.h"
#include "utils_global.h"

#include "fileutils.h"

class tst_unixdevicefileaccess; // For testing.

namespace Utils {

// Base class including dummy implementation usable as fallback.
class QTCREATOR_UTILS_EXPORT DeviceFileAccess
{
public:
    virtual ~DeviceFileAccess();

    virtual Environment deviceEnvironment() const;

protected:
    friend class FilePath;
    friend class ::tst_unixdevicefileaccess; // For testing.

    virtual QString mapToDevicePath(const QString &hostPath) const;

    virtual bool isExecutableFile(const FilePath &filePath) const;
    virtual bool isReadableFile(const FilePath &filePath) const;
    virtual bool isWritableFile(const FilePath &filePath) const;
    virtual bool isReadableDirectory(const FilePath &filePath) const;
    virtual bool isWritableDirectory(const FilePath &filePath) const;
    virtual bool isFile(const FilePath &filePath) const;
    virtual bool isDirectory(const FilePath &filePath) const;
    virtual bool isSymLink(const FilePath &filePath) const;
    virtual bool hasHardLinks(const FilePath &filePath) const;
    virtual expected_str<void> ensureWritableDirectory(const FilePath &filePath) const;
    virtual bool ensureExistingFile(const FilePath &filePath) const;
    virtual bool createDirectory(const FilePath &filePath) const;
    virtual bool exists(const FilePath &filePath) const;
    virtual bool removeFile(const FilePath &filePath) const;
    virtual bool removeRecursively(const FilePath &filePath, QString *error) const;
    virtual expected_str<void> copyFile(const FilePath &filePath, const FilePath &target) const;
    virtual expected_str<void> copyRecursively(const FilePath &filePath,
                                               const FilePath &target) const;
    virtual bool renameFile(const FilePath &filePath, const FilePath &target) const;

    virtual FilePath symLinkTarget(const FilePath &filePath) const;
    virtual FilePathInfo filePathInfo(const FilePath &filePath) const;
    virtual QDateTime lastModified(const FilePath &filePath) const;
    virtual QFile::Permissions permissions(const FilePath &filePath) const;
    virtual bool setPermissions(const FilePath &filePath, QFile::Permissions) const;
    virtual qint64 fileSize(const FilePath &filePath) const;
    virtual qint64 bytesAvailable(const FilePath &filePath) const;
    virtual QByteArray fileId(const FilePath &filePath) const;

    virtual std::optional<FilePath> refersToExecutableFile(
            const FilePath &filePath,
            FilePath::MatchScope matchScope) const;

    virtual void iterateDirectory(
            const FilePath &filePath,
            const FilePath::IterateDirCallback &callBack,
            const FileFilter &filter) const;

    virtual expected_str<QByteArray> fileContents(const FilePath &filePath,
                                                  qint64 limit,
                                                  qint64 offset) const;

    virtual expected_str<qint64> writeFileContents(const FilePath &filePath,
                                                   const QByteArray &data,
                                                   qint64 offset) const;

    virtual expected_str<FilePath> createTempFile(const FilePath &filePath);
};

class QTCREATOR_UTILS_EXPORT DesktopDeviceFileAccess : public DeviceFileAccess
{
public:
    ~DesktopDeviceFileAccess() override;

    static DesktopDeviceFileAccess *instance();

protected:
    bool isExecutableFile(const FilePath &filePath) const override;
    bool isReadableFile(const FilePath &filePath) const override;
    bool isWritableFile(const FilePath &filePath) const override;
    bool isReadableDirectory(const FilePath &filePath) const override;
    bool isWritableDirectory(const FilePath &filePath) const override;
    bool isFile(const FilePath &filePath) const override;
    bool isDirectory(const FilePath &filePath) const override;
    bool isSymLink(const FilePath &filePath) const override;
    bool hasHardLinks(const FilePath &filePath) const override;
    expected_str<void> ensureWritableDirectory(const FilePath &filePath) const override;
    bool ensureExistingFile(const FilePath &filePath) const override;
    bool createDirectory(const FilePath &filePath) const override;
    bool exists(const FilePath &filePath) const override;
    bool removeFile(const FilePath &filePath) const override;
    bool removeRecursively(const FilePath &filePath, QString *error) const override;
    expected_str<void> copyFile(const FilePath &filePath, const FilePath &target) const override;
    bool renameFile(const FilePath &filePath, const FilePath &target) const override;

    FilePath symLinkTarget(const FilePath &filePath) const override;
    FilePathInfo filePathInfo(const FilePath &filePath) const override;
    QDateTime lastModified(const FilePath &filePath) const override;
    QFile::Permissions permissions(const FilePath &filePath) const override;
    bool setPermissions(const FilePath &filePath, QFile::Permissions) const override;
    qint64 fileSize(const FilePath &filePath) const override;
    qint64 bytesAvailable(const FilePath &filePath) const override;
    QByteArray fileId(const FilePath &filePath) const override;

    std::optional<FilePath> refersToExecutableFile(
            const FilePath &filePath,
            FilePath::MatchScope matchScope) const override;

    void iterateDirectory(
            const FilePath &filePath,
            const FilePath::IterateDirCallback &callBack,
            const FileFilter &filter) const override;

    Environment deviceEnvironment() const override;

    expected_str<QByteArray> fileContents(const FilePath &filePath,
                                          qint64 limit,
                                          qint64 offset) const override;
    expected_str<qint64> writeFileContents(const FilePath &filePath,
                                           const QByteArray &data,
                                           qint64 offset) const override;

    expected_str<FilePath> createTempFile(const FilePath &filePath) override;

};

class QTCREATOR_UTILS_EXPORT UnixDeviceFileAccess : public DeviceFileAccess
{
public:
    ~UnixDeviceFileAccess() override;

protected:
    virtual RunResult runInShell(const CommandLine &cmdLine,
                                 const QByteArray &inputData = {}) const = 0;
    bool runInShellSuccess(const CommandLine &cmdLine, const QByteArray &stdInData = {}) const;

    bool isExecutableFile(const FilePath &filePath) const override;
    bool isReadableFile(const FilePath &filePath) const override;
    bool isWritableFile(const FilePath &filePath) const override;
    bool isReadableDirectory(const FilePath &filePath) const override;
    bool isWritableDirectory(const FilePath &filePath) const override;
    bool isFile(const FilePath &filePath) const override;
    bool isDirectory(const FilePath &filePath) const override;
    bool isSymLink(const FilePath &filePath) const override;
    bool hasHardLinks(const FilePath &filePath) const override;
    bool ensureExistingFile(const FilePath &filePath) const override;
    bool createDirectory(const FilePath &filePath) const override;
    bool exists(const FilePath &filePath) const override;
    bool removeFile(const FilePath &filePath) const override;
    bool removeRecursively(const FilePath &filePath, QString *error) const override;
    expected_str<void> copyFile(const FilePath &filePath, const FilePath &target) const override;
    bool renameFile(const FilePath &filePath, const FilePath &target) const override;

    FilePathInfo filePathInfo(const FilePath &filePath) const override;
    FilePath symLinkTarget(const FilePath &filePath) const override;
    QDateTime lastModified(const FilePath &filePath) const override;
    QFile::Permissions permissions(const FilePath &filePath) const override;
    bool setPermissions(const FilePath &filePath, QFile::Permissions) const override;
    qint64 fileSize(const FilePath &filePath) const override;
    qint64 bytesAvailable(const FilePath &filePath) const override;
    QByteArray fileId(const FilePath &filePath) const override;

    void iterateDirectory(
            const FilePath &filePath,
            const FilePath::IterateDirCallback &callBack,
            const FileFilter &filter) const override;

    Environment deviceEnvironment() const override;
    expected_str<QByteArray> fileContents(const FilePath &filePath,
                                          qint64 limit,
                                          qint64 offset) const override;
    expected_str<qint64> writeFileContents(const FilePath &filePath,
                                           const QByteArray &data,
                                           qint64 offset) const override;

    expected_str<FilePath> createTempFile(const FilePath &filePath) override;

    void findUsingLs(const QString &current,
                     const FileFilter &filter,
                     QStringList *found,
                     const QString &start) const;

private:
    bool iterateWithFind(const FilePath &filePath,
                         const FileFilter &filter,
                         const FilePath::IterateDirCallback &callBack) const;

    QStringList statArgs(const FilePath &filePath,
                         const QString &linuxFormat,
                         const QString &macFormat) const;

    mutable bool m_tryUseFind = true;
    mutable std::optional<bool> m_hasMkTemp;
};

} // Utils
