// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devicefileaccess.h"

#include "algorithm.h"
#include "commandline.h"
#include "environment.h"
#include "expected.h"
#include "hostosinfo.h"
#include "osspecificaspects.h"
#include "qtcassert.h"
#include "utilstr.h"

#ifndef UTILS_STATIC_LIBRARY
#include "process.h"
#endif

#include <QCoreApplication>
#include <QOperatingSystemVersion>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QStorageInfo>
#include <QTemporaryFile>

#ifdef Q_OS_WIN
#ifdef QTCREATOR_PCH_H
#define CALLBACK WINAPI
#endif
#include <shlobj.h>
#include <qt_windows.h>
#else
#include <qplatformdefs.h>
#endif

#include <algorithm>
#include <array>

namespace Utils {

// DeviceFileAccess

DeviceFileAccess::~DeviceFileAccess() = default;

// This takes a \a hostPath, typically the same as used with QFileInfo
// and returns the path component of a universal FilePath. Host and scheme
// of the latter are added by a higher level to form a FilePath.
QString DeviceFileAccess::mapToDevicePath(const QString &hostPath) const
{
    return hostPath;
}

bool DeviceFileAccess::isExecutableFile(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return false;
}

bool DeviceFileAccess::isReadableFile(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return false;
}

bool DeviceFileAccess::isWritableFile(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return false;
}

bool DeviceFileAccess::isReadableDirectory(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return false;
}

bool DeviceFileAccess::isWritableDirectory(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return false;
}

bool DeviceFileAccess::isFile(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return false;
}

bool DeviceFileAccess::isDirectory(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return false;
}

bool DeviceFileAccess::isSymLink(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return false;
}

bool DeviceFileAccess::hasHardLinks(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return false;
}

expected_str<void> DeviceFileAccess::ensureWritableDirectory(const FilePath &filePath) const
{
    if (isWritableDirectory(filePath))
        return {};

    if (exists(filePath)) {
        return make_unexpected(Tr::tr("Path \"%1\" exists but is not a writable directory.")
                                   .arg(filePath.toUserOutput()));
    }

    const bool result = createDirectory(filePath);
    if (result)
        return {};

    return make_unexpected(
        Tr::tr("Failed to create directory \"%1\".").arg(filePath.toUserOutput()));
}

bool DeviceFileAccess::ensureExistingFile(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return false;
}

bool DeviceFileAccess::createDirectory(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return false;
}

bool DeviceFileAccess::exists(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return false;
}

bool DeviceFileAccess::removeFile(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return false;
}

bool DeviceFileAccess::removeRecursively(const FilePath &filePath, QString *error) const
{
    Q_UNUSED(filePath)
    Q_UNUSED(error)
    QTC_CHECK(false);
    return false;
}

expected_str<void> DeviceFileAccess::copyFile(const FilePath &filePath, const FilePath &target) const
{
    Q_UNUSED(filePath)
    Q_UNUSED(target)
    QTC_CHECK(false);
    return make_unexpected(
        Tr::tr("copyFile is not implemented for \"%1\".").arg(filePath.toUserOutput()));
}

expected_str<void> copyRecursively_fallback(const FilePath &src, const FilePath &target)
{
    expected_str<void> result;
    src.iterateDirectory(
        [&target, &src, &result](const FilePath &path) {
            const FilePath relative = path.relativePathFrom(src);
            const FilePath targetPath = target.pathAppended(relative.path());
            result = targetPath.parentDir().ensureWritableDir();
            if (!result)
                return IterationPolicy::Stop;

            result = path.copyFile(targetPath);
            if (!result)
                return IterationPolicy::Stop;

            return IterationPolicy::Continue;
        },
        {{"*"}, QDir::NoDotAndDotDot | QDir::Files, QDirIterator::Subdirectories});

    return result;
}

expected_str<void> DeviceFileAccess::copyRecursively(const FilePath &src,
                                                     const FilePath &target) const
{
    if (!src.isDir()) {
        return make_unexpected(
            Tr::tr("Cannot copy from \"%1\", it is not a directory.").arg(src.toUserOutput()));
    }
    const expected_str<void> result = target.ensureWritableDir();
    if (!result) {
        return make_unexpected(Tr::tr("Cannot copy \"%1\" to \"%2\": %3")
                                   .arg(src.toUserOutput())
                                   .arg(target.toUserOutput())
                                   .arg(result.error()));
    }

#ifdef UTILS_STATIC_LIBRARY
    return copyRecursively_fallback(src, target);
#else
    const FilePath targetTar = target.withNewPath("tar").searchInPath();
    const FilePath sourceTar = src.withNewPath("tar").searchInPath();

    const bool isSrcOrTargetQrc = src.toFSPathString().startsWith(":/")
                                  || target.toFSPathString().startsWith(":/");

    if (isSrcOrTargetQrc || !targetTar.isExecutableFile() || !sourceTar.isExecutableFile())
        return copyRecursively_fallback(src, target);

    Process srcProcess;
    Process targetProcess;

    targetProcess.setProcessMode(ProcessMode::Writer);

    QObject::connect(&srcProcess,
                     &Process::readyReadStandardOutput,
                     &targetProcess,
                     [&srcProcess, &targetProcess]() {
                         targetProcess.writeRaw(srcProcess.readAllRawStandardOutput());
                     });

    srcProcess.setCommand({sourceTar, {"-C", src.path(), "-cf", "-", "."}});
    targetProcess.setCommand({targetTar, {"xf", "-", "-C", target.path()}});

    targetProcess.start();
    targetProcess.waitForStarted();

    srcProcess.start();
    srcProcess.waitForFinished();

    targetProcess.closeWriteChannel();

    if (srcProcess.result() != ProcessResult::FinishedWithSuccess) {
        targetProcess.kill();
        return make_unexpected(
            Tr::tr("Failed to copy recursively from \"%1\" to \"%2\" while "
                   "trying to create tar archive from source: %3")
                .arg(src.toUserOutput(), target.toUserOutput(), srcProcess.readAllStandardError()));
    }

    targetProcess.waitForFinished();

    if (targetProcess.result() != ProcessResult::FinishedWithSuccess) {
        return make_unexpected(Tr::tr("Failed to copy recursively from \"%1\" to \"%2\" while "
                                      "trying to extract tar archive to target: %3")
                                   .arg(src.toUserOutput(),
                                        target.toUserOutput(),
                                        targetProcess.readAllStandardError()));
    }

    return {};
#endif
}

bool DeviceFileAccess::renameFile(const FilePath &filePath, const FilePath &target) const
{
    Q_UNUSED(filePath)
    Q_UNUSED(target)
    QTC_CHECK(false);
    return false;
}

FilePath DeviceFileAccess::symLinkTarget(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return {};
}

void DeviceFileAccess::iterateDirectory(const FilePath &filePath,
                                        const FilePath::IterateDirCallback &callBack,
                                        const FileFilter &filter) const
{
    Q_UNUSED(filePath)
    Q_UNUSED(callBack)
    Q_UNUSED(filter)
    QTC_CHECK(false);
}

Environment DeviceFileAccess::deviceEnvironment() const
{
    QTC_CHECK(false);
    return {};
}

expected_str<QByteArray> DeviceFileAccess::fileContents(const FilePath &filePath,
                                                        qint64 limit,
                                                        qint64 offset) const
{
    Q_UNUSED(filePath)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    QTC_CHECK(false);
    return make_unexpected(
        Tr::tr("fileContents is not implemented for \"%1\".").arg(filePath.toUserOutput()));
}

expected_str<qint64> DeviceFileAccess::writeFileContents(const FilePath &filePath,
                                                         const QByteArray &data,
                                                         qint64 offset) const
{
    Q_UNUSED(filePath)
    Q_UNUSED(data)
    Q_UNUSED(offset)
    QTC_CHECK(false);
    return make_unexpected(
        Tr::tr("writeFileContents is not implemented for \"%1\".").arg(filePath.toUserOutput()));
}

FilePathInfo DeviceFileAccess::filePathInfo(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return {};
}

QDateTime DeviceFileAccess::lastModified(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return {};
}

QFile::Permissions DeviceFileAccess::permissions(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return {};
}

bool DeviceFileAccess::setPermissions(const FilePath &filePath, QFile::Permissions) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return false;
}

qint64 DeviceFileAccess::fileSize(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return false;
}

qint64 DeviceFileAccess::bytesAvailable(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return -1;
}

QByteArray DeviceFileAccess::fileId(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return {};
}

std::optional<FilePath> DeviceFileAccess::refersToExecutableFile(
    const FilePath &filePath, FilePath::MatchScope matchScope) const
{
    Q_UNUSED(matchScope)
    if (isExecutableFile(filePath))
        return filePath;
    return {};
}

expected_str<FilePath> DeviceFileAccess::createTempFile(const FilePath &filePath)
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return make_unexpected(
        Tr::tr("createTempFile is not implemented for \"%1\".").arg(filePath.toUserOutput()));
}

// DesktopDeviceFileAccess

DesktopDeviceFileAccess::~DesktopDeviceFileAccess() = default;

DesktopDeviceFileAccess *DesktopDeviceFileAccess::instance()
{
    static DesktopDeviceFileAccess theInstance;
    return &theInstance;
}

bool DesktopDeviceFileAccess::isExecutableFile(const FilePath &filePath) const
{
    const QFileInfo fi(filePath.path());
    return fi.isExecutable() && !fi.isDir();
}

static std::optional<FilePath> isWindowsExecutableHelper(const FilePath &filePath,
                                                         const QStringView suffix)
{
    const QFileInfo fi(filePath.path().append(suffix));
    if (!fi.isExecutable() || fi.isDir())
        return {};

    return filePath.withNewPath(FileUtils::normalizedPathName(fi.filePath()));
}

std::optional<FilePath> DesktopDeviceFileAccess::refersToExecutableFile(
    const FilePath &filePath, FilePath::MatchScope matchScope) const
{
    if (isExecutableFile(filePath))
        return filePath;

    if (HostOsInfo::isWindowsHost()) {
        if (matchScope == FilePath::WithExeSuffix || matchScope == FilePath::WithExeOrBatSuffix) {
            if (auto res = isWindowsExecutableHelper(filePath, u".exe"))
                return res;
        }
        if (matchScope == FilePath::WithBatSuffix || matchScope == FilePath::WithExeOrBatSuffix) {
            if (auto res = isWindowsExecutableHelper(filePath, u".bat"))
                return res;
        }
        if (matchScope == FilePath::WithAnySuffix) {
            // That's usually  .COM;.EXE;.BAT;.CMD;.VBS;.VBE;.JS;.JSE;.WSF;.WSH,
            static const QStringList exts = qtcEnvironmentVariable("PATHEXT").toLower().split(';');
            for (const QString &ext : exts) {
                if (auto res = isWindowsExecutableHelper(filePath, ext))
                    return res;
            }
        }
    }
    return {};
}

bool DesktopDeviceFileAccess::isReadableFile(const FilePath &filePath) const
{
    const QFileInfo fi(filePath.path());
    return fi.exists() && fi.isFile() && fi.isReadable();
}

bool DesktopDeviceFileAccess::isWritableFile(const FilePath &filePath) const
{
    const QFileInfo fi(filePath.path());
    return fi.exists() && fi.isFile() && fi.isWritable();
}

bool DesktopDeviceFileAccess::isReadableDirectory(const FilePath &filePath) const
{
    const QFileInfo fi(filePath.path());
    return fi.exists() && fi.isDir() && fi.isReadable();
}

bool DesktopDeviceFileAccess::isWritableDirectory(const FilePath &filePath) const
{
    const QFileInfo fi(filePath.path());
    return fi.exists() && fi.isDir() && fi.isWritable();
}

bool DesktopDeviceFileAccess::isFile(const FilePath &filePath) const
{
    const QFileInfo fi(filePath.path());
    return fi.isFile();
}

bool DesktopDeviceFileAccess::isDirectory(const FilePath &filePath) const
{
    const QFileInfo fi(filePath.path());
    return fi.isDir();
}

bool DesktopDeviceFileAccess::isSymLink(const FilePath &filePath) const
{
    const QFileInfo fi(filePath.path());
    return fi.isSymLink();
}

bool DesktopDeviceFileAccess::hasHardLinks(const FilePath &filePath) const
{
#ifdef Q_OS_UNIX
    struct stat s
    {};
    const int r = stat(filePath.absoluteFilePath().toString().toLocal8Bit().constData(), &s);
    if (r == 0) {
        // check for hardlinks because these would break without the atomic write implementation
        if (s.st_nlink > 1)
            return true;
    }
#elif defined(Q_OS_WIN)
    const HANDLE handle = CreateFile((wchar_t *) filePath.toUserOutput().utf16(),
                                     0,
                                     FILE_SHARE_READ,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_FLAG_BACKUP_SEMANTICS,
                                     NULL);
    if (handle == INVALID_HANDLE_VALUE)
        return false;

    FILE_STANDARD_INFO info;
    if (GetFileInformationByHandleEx(handle, FileStandardInfo, &info, sizeof(info)))
        return info.NumberOfLinks > 1;
#else
    Q_UNUSED(filePath)
#endif
    return false;
}

expected_str<void> DesktopDeviceFileAccess::ensureWritableDirectory(const FilePath &filePath) const
{
    const QFileInfo fi(filePath.path());
    if (fi.isDir() && fi.isWritable())
        return {};

    if (fi.exists()) {
        return make_unexpected(Tr::tr("Path \"%1\" exists but is not a writable directory.")
                                   .arg(filePath.toUserOutput()));
    }

    const bool result = QDir().mkpath(filePath.path());
    if (result)
        return {};

    return make_unexpected(
        Tr::tr("Failed to create directory \"%1\".").arg(filePath.toUserOutput()));
}

bool DesktopDeviceFileAccess::ensureExistingFile(const FilePath &filePath) const
{
    QFile f(filePath.path());
    if (f.exists())
        return true;
    f.open(QFile::WriteOnly);
    f.close();
    return f.exists();
}

bool DesktopDeviceFileAccess::createDirectory(const FilePath &filePath) const
{
    QDir dir(filePath.path());
    return dir.mkpath(dir.absolutePath());
}

bool DesktopDeviceFileAccess::exists(const FilePath &filePath) const
{
    return !filePath.isEmpty() && QFileInfo::exists(filePath.path());
}

bool DesktopDeviceFileAccess::removeFile(const FilePath &filePath) const
{
    return QFile::remove(filePath.path());
}

bool DesktopDeviceFileAccess::removeRecursively(const FilePath &filePath, QString *error) const
{
    QTC_ASSERT(!filePath.needsDevice(), return false);
    QFileInfo fileInfo = filePath.toFileInfo();
    if (!fileInfo.exists() && !fileInfo.isSymLink())
        return true;

    QFile::setPermissions(fileInfo.absoluteFilePath(), fileInfo.permissions() | QFile::WriteUser);

    if (fileInfo.isDir()) {
        QDir dir(fileInfo.absoluteFilePath());
        dir.setPath(dir.canonicalPath());
        if (dir.isRoot()) {
            if (error)
                *error = Tr::tr("Refusing to remove root directory.");
            return false;
        }
        if (dir.path() == QDir::home().canonicalPath()) {
            if (error)
                *error = Tr::tr("Refusing to remove your home directory.");
            return false;
        }

        const QStringList fileNames = dir.entryList(QDir::Files | QDir::Hidden | QDir::System
                                                    | QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &fileName : fileNames) {
            if (!removeRecursively(filePath / fileName, error))
                return false;
        }
        if (!QDir::root().rmdir(dir.path())) {
            if (error)
                *error = Tr::tr("Failed to remove directory \"%1\".").arg(filePath.toUserOutput());
            return false;
        }
    } else {
        if (!QFile::remove(filePath.path())) {
            if (error)
                *error = Tr::tr("Failed to remove file \"%1\".").arg(filePath.toUserOutput());
            return false;
        }
    }
    return true;
}

expected_str<void> DesktopDeviceFileAccess::copyFile(const FilePath &filePath,
                                                     const FilePath &target) const
{
    QFile srcFile(filePath.path());

    if (srcFile.copy(target.path()))
        return {};
    return make_unexpected(
        Tr::tr("Failed to copy file \"%1\" to \"%2\": %3")
            .arg(filePath.toUserOutput(), target.toUserOutput(), srcFile.errorString()));
}

bool DesktopDeviceFileAccess::renameFile(const FilePath &filePath, const FilePath &target) const
{
    return QFile::rename(filePath.path(), target.path());
}

FilePathInfo DesktopDeviceFileAccess::filePathInfo(const FilePath &filePath) const
{
    FilePathInfo result;

    QFileInfo fi(filePath.path());
    result.fileSize = fi.size();
    result.lastModified = fi.lastModified();
    result.fileFlags = (FilePathInfo::FileFlag) int(fi.permissions());

    if (fi.isDir())
        result.fileFlags |= FilePathInfo::DirectoryType;
    if (fi.isFile())
        result.fileFlags |= FilePathInfo::FileType;
    if (fi.exists())
        result.fileFlags |= FilePathInfo::ExistsFlag;
    if (fi.isSymbolicLink())
        result.fileFlags |= FilePathInfo::LinkType;
    if (fi.isBundle())
        result.fileFlags |= FilePathInfo::BundleType;
    if (fi.isHidden())
        result.fileFlags |= FilePathInfo::HiddenFlag;
    if (fi.isRoot())
        result.fileFlags |= FilePathInfo::RootFlag;

    return result;
}

FilePath DesktopDeviceFileAccess::symLinkTarget(const FilePath &filePath) const
{
    const QFileInfo info(filePath.path());
    if (!info.isSymLink())
        return {};
    return FilePath::fromString(info.symLinkTarget());
}

void DesktopDeviceFileAccess::iterateDirectory(const FilePath &filePath,
                                               const FilePath::IterateDirCallback &callBack,
                                               const FileFilter &filter) const
{
    QDirIterator it(filePath.path(), filter.nameFilters, filter.fileFilters, filter.iteratorFlags);
    while (it.hasNext()) {
        const FilePath path = FilePath::fromString(it.next());
        IterationPolicy res;
        if (callBack.index() == 0)
            res = std::get<0>(callBack)(path);
        else
            res = std::get<1>(callBack)(path, path.filePathInfo());
        if (res == IterationPolicy::Stop)
            return;
    }
}

Environment DesktopDeviceFileAccess::deviceEnvironment() const
{
    return Environment::systemEnvironment();
}

expected_str<QByteArray> DesktopDeviceFileAccess::fileContents(const FilePath &filePath,
                                                               qint64 limit,
                                                               qint64 offset) const
{
    const QString path = filePath.path();
    QFile f(path);
    if (!f.exists())
        return make_unexpected(Tr::tr("File \"%1\" does not exist.").arg(path));

    if (!f.open(QFile::ReadOnly))
        return make_unexpected(Tr::tr("Could not open File \"%1\".").arg(path));

    if (offset != 0)
        f.seek(offset);

    if (limit != -1)
        return f.read(limit);

    const QByteArray data = f.readAll();
    if (f.error() != QFile::NoError) {
        return make_unexpected(
            Tr::tr("Cannot read \"%1\": %2").arg(filePath.toUserOutput(), f.errorString()));
    }

    return data;
}

expected_str<qint64> DesktopDeviceFileAccess::writeFileContents(const FilePath &filePath,
                                                                const QByteArray &data,
                                                                qint64 offset) const
{
    QFile file(filePath.path());
    const bool isOpened = file.open(QFile::WriteOnly | QFile::Truncate);
    if (!isOpened)
        return make_unexpected(
            Tr::tr("Could not open file \"%1\" for writing.").arg(filePath.toUserOutput()));

    if (offset != 0)
        file.seek(offset);
    qint64 res = file.write(data);
    if (res != data.size())
        return make_unexpected(
            Tr::tr("Could not write to file \"%1\" (only %2 of %n byte(s) written).",
                   nullptr,
                   data.size())
                .arg(filePath.toUserOutput())
                .arg(res));
    return res;
}

expected_str<FilePath> DesktopDeviceFileAccess::createTempFile(const FilePath &filePath)
{
    QTemporaryFile file(filePath.path());
    file.setAutoRemove(false);
    if (!file.open()) {
        return make_unexpected(Tr::tr("Could not create temporary file in \"%1\" (%2).")
                                   .arg(filePath.toUserOutput())
                                   .arg(file.errorString()));
    }
    return filePath.withNewPath(file.fileName());
}

QDateTime DesktopDeviceFileAccess::lastModified(const FilePath &filePath) const
{
    return QFileInfo(filePath.path()).lastModified();
}

QFile::Permissions DesktopDeviceFileAccess::permissions(const FilePath &filePath) const
{
    return QFileInfo(filePath.path()).permissions();
}

bool DesktopDeviceFileAccess::setPermissions(const FilePath &filePath,
                                             QFile::Permissions permissions) const
{
    return QFile(filePath.path()).setPermissions(permissions);
}

qint64 DesktopDeviceFileAccess::fileSize(const FilePath &filePath) const
{
    return QFileInfo(filePath.path()).size();
}

qint64 DesktopDeviceFileAccess::bytesAvailable(const FilePath &filePath) const
{
    return QStorageInfo(filePath.path()).bytesAvailable();
}

// Copied from qfilesystemengine_win.cpp
#ifdef Q_OS_WIN

// File ID for Windows up to version 7.
static inline QByteArray fileIdWin7(HANDLE handle)
{
    BY_HANDLE_FILE_INFORMATION info;
    if (GetFileInformationByHandle(handle, &info)) {
        char buffer[sizeof "01234567:0123456701234567\0"];
        qsnprintf(buffer,
                  sizeof(buffer),
                  "%lx:%08lx%08lx",
                  info.dwVolumeSerialNumber,
                  info.nFileIndexHigh,
                  info.nFileIndexLow);
        return QByteArray(buffer);
    }
    return QByteArray();
}

// File ID for Windows starting from version 8.
static QByteArray fileIdWin8(HANDLE handle)
{
    QByteArray result;
    FILE_ID_INFO infoEx;
    if (GetFileInformationByHandleEx(handle,
                                     static_cast<FILE_INFO_BY_HANDLE_CLASS>(
                                         18), // FileIdInfo in Windows 8
                                     &infoEx,
                                     sizeof(FILE_ID_INFO))) {
        result = QByteArray::number(infoEx.VolumeSerialNumber, 16);
        result += ':';
        // Note: MinGW-64's definition of FILE_ID_128 differs from the MSVC one.
        result += QByteArray(reinterpret_cast<const char *>(&infoEx.FileId),
                             int(sizeof(infoEx.FileId)))
                      .toHex();
    }
    return result;
}

static QByteArray fileIdWin(HANDLE fHandle)
{
    return QOperatingSystemVersion::current() >= QOperatingSystemVersion::Windows8
               ? fileIdWin8(HANDLE(fHandle))
               : fileIdWin7(HANDLE(fHandle));
}
#endif

QByteArray DesktopDeviceFileAccess::fileId(const FilePath &filePath) const
{
    QByteArray result;

#ifdef Q_OS_WIN
    const HANDLE handle = CreateFile((wchar_t *) filePath.toUserOutput().utf16(),
                                     0,
                                     FILE_SHARE_READ,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_FLAG_BACKUP_SEMANTICS,
                                     NULL);
    if (handle != INVALID_HANDLE_VALUE) {
        result = fileIdWin(handle);
        CloseHandle(handle);
    }
#else // Copied from qfilesystemengine_unix.cpp
    if (Q_UNLIKELY(filePath.isEmpty()))
        return result;

    QT_STATBUF statResult;
    if (QT_STAT(filePath.path().toLocal8Bit().constData(), &statResult))
        return result;
    result = QByteArray::number(quint64(statResult.st_dev), 16);
    result += ':';
    result += QByteArray::number(quint64(statResult.st_ino));
#endif
    return result;
}

// UnixDeviceAccess

UnixDeviceFileAccess::~UnixDeviceFileAccess() = default;

bool UnixDeviceFileAccess::runInShellSuccess(const CommandLine &cmdLine,
                                             const QByteArray &stdInData) const
{
    return runInShell(cmdLine, stdInData).exitCode == 0;
}

bool UnixDeviceFileAccess::isExecutableFile(const FilePath &filePath) const
{
    const QString path = filePath.path();
    return runInShellSuccess({"test", {"-x", path}, OsType::OsTypeLinux});
}

bool UnixDeviceFileAccess::isReadableFile(const FilePath &filePath) const
{
    const QString path = filePath.path();
    return runInShellSuccess({"test", {"-r", path, "-a", "-f", path}, OsType::OsTypeLinux});
}

bool UnixDeviceFileAccess::isWritableFile(const FilePath &filePath) const
{
    const QString path = filePath.path();
    return runInShellSuccess({"test", {"-w", path, "-a", "-f", path}, OsType::OsTypeLinux});
}

bool UnixDeviceFileAccess::isReadableDirectory(const FilePath &filePath) const
{
    const QString path = filePath.path();
    return runInShellSuccess({"test", {"-r", path, "-a", "-d", path}, OsType::OsTypeLinux});
}

bool UnixDeviceFileAccess::isWritableDirectory(const FilePath &filePath) const
{
    const QString path = filePath.path();
    return runInShellSuccess({"test", {"-w", path, "-a", "-d", path}, OsType::OsTypeLinux});
}

bool UnixDeviceFileAccess::isFile(const FilePath &filePath) const
{
    const QString path = filePath.path();
    return runInShellSuccess({"test", {"-f", path}, OsType::OsTypeLinux});
}

bool UnixDeviceFileAccess::isDirectory(const FilePath &filePath) const
{
    if (filePath.isRootPath())
        return true;

    const QString path = filePath.path();
    return runInShellSuccess({"test", {"-d", path}, OsType::OsTypeLinux});
}

bool UnixDeviceFileAccess::isSymLink(const FilePath &filePath) const
{
    const QString path = filePath.path();
    return runInShellSuccess({"test", {"-h", path}, OsType::OsTypeLinux});
}

bool UnixDeviceFileAccess::hasHardLinks(const FilePath &filePath) const
{
    const QStringList args = statArgs(filePath, "%h", "%l");
    const RunResult result = runInShell({"stat", args, OsType::OsTypeLinux});
    return result.stdOut.toLongLong() > 1;
}

bool UnixDeviceFileAccess::ensureExistingFile(const FilePath &filePath) const
{
    const QString path = filePath.path();
    return runInShellSuccess({"touch", {path}, OsType::OsTypeLinux});
}

bool UnixDeviceFileAccess::createDirectory(const FilePath &filePath) const
{
    const QString path = filePath.path();
    return runInShellSuccess({"mkdir", {"-p", path}, OsType::OsTypeLinux});
}

bool UnixDeviceFileAccess::exists(const FilePath &filePath) const
{
    const QString path = filePath.path();
    return runInShellSuccess({"test", {"-e", path}, OsType::OsTypeLinux});
}

bool UnixDeviceFileAccess::removeFile(const FilePath &filePath) const
{
    return runInShellSuccess({"rm", {filePath.path()}, OsType::OsTypeLinux});
}

bool UnixDeviceFileAccess::removeRecursively(const FilePath &filePath, QString *error) const
{
    QTC_ASSERT(filePath.path().startsWith('/'), return false);

    const QString path = filePath.cleanPath().path();
    // We are expecting this only to be called in a context of build directories or similar.
    // Chicken out in some cases that _might_ be user code errors.
    QTC_ASSERT(path.startsWith('/'), return false);
    int levelsNeeded = path.startsWith("/home/") ? 4 : 3;
    if (path.startsWith("/tmp/"))
        levelsNeeded = 2;
    QTC_ASSERT(path.count('/') >= levelsNeeded, return false);

    RunResult result = runInShell({"rm", {"-rf", "--", path}, OsType::OsTypeLinux});
    if (error)
        *error = QString::fromUtf8(result.stdErr);
    return result.exitCode == 0;
}

expected_str<void> UnixDeviceFileAccess::copyFile(const FilePath &filePath,
                                                  const FilePath &target) const
{
    const RunResult result = runInShell(
        {"cp", {filePath.path(), target.path()}, OsType::OsTypeLinux});

    if (result.exitCode != 0) {
        return make_unexpected(Tr::tr("Failed to copy file \"%1\" to \"%2\": %3")
                                   .arg(filePath.toUserOutput(),
                                        target.toUserOutput(),
                                        QString::fromUtf8(result.stdErr)));
    }
    return {};
}

bool UnixDeviceFileAccess::renameFile(const FilePath &filePath, const FilePath &target) const
{
    return runInShellSuccess({"mv", {filePath.path(), target.path()}, OsType::OsTypeLinux});
}

FilePath UnixDeviceFileAccess::symLinkTarget(const FilePath &filePath) const
{
    const RunResult result = runInShell(
        {"readlink", {"-n", "-e", filePath.path()}, OsType::OsTypeLinux});
    const QString out = QString::fromUtf8(result.stdOut);
    return out.isEmpty() ? FilePath() : filePath.withNewPath(out);
}

expected_str<QByteArray> UnixDeviceFileAccess::fileContents(const FilePath &filePath,
                                                            qint64 limit,
                                                            qint64 offset) const
{
    QStringList args = {"if=" + filePath.path()};
    if (limit > 0 || offset > 0) {
        const qint64 gcd = std::gcd(limit, offset);
        args += QString("bs=%1").arg(gcd);
        args += QString("count=%1").arg(limit / gcd);
        args += QString("seek=%1").arg(offset / gcd);
    }
#ifndef UTILS_STATIC_LIBRARY
    const FilePath dd = filePath.withNewPath("dd");

    Process p;
    p.setCommand({dd, args, OsType::OsTypeLinux});
    p.runBlocking();
    if (p.exitCode() != 0) {
        return make_unexpected(Tr::tr("Failed reading file \"%1\": %2")
                                   .arg(filePath.toUserOutput(), p.readAllStandardError()));
    }
    return p.readAllRawStandardOutput();
#else
    return make_unexpected(QString("Not implemented"));
#endif
}

expected_str<qint64> UnixDeviceFileAccess::writeFileContents(const FilePath &filePath,
                                                             const QByteArray &data,
                                                             qint64 offset) const
{
    QStringList args = {"of=" + filePath.path()};
    if (offset != 0) {
        args.append("bs=1");
        args.append(QString("seek=%1").arg(offset));
    }
    RunResult result = runInShell({"dd", args, OsType::OsTypeLinux}, data);

    if (result.exitCode != 0) {
        return make_unexpected(Tr::tr("Failed writing file \"%1\": %2")
                                   .arg(filePath.toUserOutput(), QString::fromUtf8(result.stdErr)));
    }
    return data.size();
}

expected_str<FilePath> UnixDeviceFileAccess::createTempFile(const FilePath &filePath)
{
    if (!m_hasMkTemp.has_value())
        m_hasMkTemp = runInShellSuccess({"which", {"mktemp"}, OsType::OsTypeLinux});

    QString tmplate = filePath.path();
    // Some mktemp implementations require a suffix of XXXXXX.
    // They will only accept the template if at least the last 6 characters are X.
    if (!tmplate.endsWith("XXXXXX"))
        tmplate += ".XXXXXX";

    if (m_hasMkTemp) {
        const RunResult result = runInShell({"mktemp", {tmplate}, OsType::OsTypeLinux});

        if (result.exitCode != 0) {
            return make_unexpected(
                Tr::tr("Failed creating temporary file \"%1\": %2")
                    .arg(filePath.toUserOutput(), QString::fromUtf8(result.stdErr)));
        }

        return filePath.withNewPath(QString::fromUtf8(result.stdOut.trimmed()));
    }

    // Manually create a temporary/unique file.
    std::reverse_iterator<QChar *> firstX = std::find_if_not(std::rbegin(tmplate),
                                                             std::rend(tmplate),
                                                             [](QChar ch) { return ch == 'X'; });

    static constexpr std::array<QChar, 62> chars = {'0', '1', '2', '3', '4', '5', '6', '7', '8',
                                                    '9', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
                                                    'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q',
                                                    'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
                                                    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
                                                    'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
                                                    'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};

    std::uniform_int_distribution<> dist(0, int(chars.size() - 1));

    int maxTries = 10;
    FilePath newPath;
    do {
        for (QChar *it = firstX.base(); it != std::end(tmplate); ++it) {
            *it = chars[dist(*QRandomGenerator::global())];
        }
        newPath = filePath.withNewPath(tmplate);
        if (--maxTries == 0) {
            return make_unexpected(Tr::tr("Failed creating temporary file \"%1\" (too many tries).")
                                       .arg(filePath.toUserOutput()));
        }
    } while (newPath.exists());

    const expected_str<qint64> createResult = newPath.writeFileContents({});

    if (!createResult)
        return make_unexpected(createResult.error());

    return newPath;
}

QDateTime UnixDeviceFileAccess::lastModified(const FilePath &filePath) const
{
    const RunResult result = runInShell(
        {"stat", {"-L", "-c", "%Y", filePath.path()}, OsType::OsTypeLinux});
    qint64 secs = result.stdOut.toLongLong();
    const QDateTime dt = QDateTime::fromSecsSinceEpoch(secs, Qt::UTC);
    return dt;
}

QStringList UnixDeviceFileAccess::statArgs(const FilePath &filePath,
                                           const QString &linuxFormat,
                                           const QString &macFormat) const
{
    return (filePath.osType() == OsTypeMac ? QStringList{"-f", macFormat}
                                           : QStringList{"-c", linuxFormat})
           << "-L" << filePath.path();
}

QFile::Permissions UnixDeviceFileAccess::permissions(const FilePath &filePath) const
{
    QStringList args = statArgs(filePath, "%a", "%p");

    const RunResult result = runInShell({"stat", args, OsType::OsTypeLinux});
    const uint bits = result.stdOut.toUInt(nullptr, 8);
    QFileDevice::Permissions perm = {};
#define BIT(n, p) \
    if (bits & (1 << n)) \
    perm |= QFileDevice::p
    BIT(0, ExeOther);
    BIT(1, WriteOther);
    BIT(2, ReadOther);
    BIT(3, ExeGroup);
    BIT(4, WriteGroup);
    BIT(5, ReadGroup);
    BIT(6, ExeUser);
    BIT(7, WriteUser);
    BIT(8, ReadUser);
#undef BIT
    return perm;
}

bool UnixDeviceFileAccess::setPermissions(const FilePath &filePath, QFile::Permissions perms) const
{
    const int flags = int(perms);
    return runInShellSuccess(
        {"chmod", {QString::number(flags, 16), filePath.path()}, OsType::OsTypeLinux});
}

qint64 UnixDeviceFileAccess::fileSize(const FilePath &filePath) const
{
    const QStringList args = statArgs(filePath, "%s", "%z");
    const RunResult result = runInShell({"stat", args, OsType::OsTypeLinux});
    return result.stdOut.toLongLong();
}

qint64 UnixDeviceFileAccess::bytesAvailable(const FilePath &filePath) const
{
    const RunResult result = runInShell({"df", {"-k", filePath.path()}, OsType::OsTypeLinux});
    return FileUtils::bytesAvailableFromDFOutput(result.stdOut);
}

QByteArray UnixDeviceFileAccess::fileId(const FilePath &filePath) const
{
    const QStringList args = statArgs(filePath, "%D:%i", "%d:%i");

    const RunResult result = runInShell({"stat", args, OsType::OsTypeLinux});
    if (result.exitCode != 0)
        return {};

    return result.stdOut;
}

FilePathInfo UnixDeviceFileAccess::filePathInfo(const FilePath &filePath) const
{
    if (filePath.path() == "/") // TODO: Add FilePath::isRoot()
    {
        const FilePathInfo r{4096,
                             FilePathInfo::FileFlags(
                                 FilePathInfo::ReadOwnerPerm | FilePathInfo::WriteOwnerPerm
                                 | FilePathInfo::ExeOwnerPerm | FilePathInfo::ReadGroupPerm
                                 | FilePathInfo::ExeGroupPerm | FilePathInfo::ReadOtherPerm
                                 | FilePathInfo::ExeOtherPerm | FilePathInfo::DirectoryType
                                 | FilePathInfo::LocalDiskFlag | FilePathInfo::ExistsFlag),
                             QDateTime::currentDateTime()};

        return r;
    }

    const QStringList args = statArgs(filePath, "%f %Y %s", "%p %m %z");

    const RunResult stat = runInShell({"stat", args, OsType::OsTypeLinux});
    return FileUtils::filePathInfoFromTriple(QString::fromLatin1(stat.stdOut),
                                             filePath.osType() == OsTypeMac ? 8 : 16);
}

// returns whether 'find' could be used.
bool UnixDeviceFileAccess::iterateWithFind(const FilePath &filePath,
                                           const FileFilter &filter,
                                           const FilePath::IterateDirCallback &callBack) const
{
    QTC_CHECK(filePath.isAbsolutePath());

    CommandLine cmdLine{"find", filter.asFindArguments(filePath.path()), OsType::OsTypeLinux};

    // TODO: Using stat -L will always return the link target, not the link itself.
    // We may wan't to add the information that it is a link at some point.

    const QString statFormat = filePath.osType() == OsTypeMac ? QLatin1String("-f \"%p %m %z\"")
                                                              : QLatin1String("-c \"%f %Y %s\"");

    if (callBack.index() == 1)
        cmdLine.addArgs(QString(R"(-exec echo -n \"{}\"" " \; -exec stat -L %1 "{}" \;)")
                            .arg(statFormat),
                        CommandLine::Raw);

    const RunResult result = runInShell(cmdLine);
    const QString out = QString::fromUtf8(result.stdOut);
    if (result.exitCode != 0) {
        // Find returns non-zero exit code for any error it encounters, even if it finds some files.

        if (!out.startsWith('"' + filePath.path())) {
            if (!filePath.exists()) // File does not exist, so no files to find.
                return true;

            // If the output does not start with the path we are searching in, find has failed.
            // Possibly due to unknown options.
            return false;
        }
    }

    QStringList entries = out.split("\n", Qt::SkipEmptyParts);
    if (entries.isEmpty())
        return true;

    const int modeBase = filePath.osType() == OsTypeMac ? 8 : 16;

    const auto toFilePath = [&filePath, &callBack, modeBase](const QString &entry) {
        if (callBack.index() == 0)
            return std::get<0>(callBack)(filePath.withNewPath(entry));

        const QString fileName = entry.mid(1, entry.lastIndexOf('\"') - 1);
        const QString infos = entry.mid(fileName.length() + 3);

        const FilePathInfo fi = FileUtils::filePathInfoFromTriple(infos, modeBase);
        if (!fi.fileFlags)
            return IterationPolicy::Continue;

        const FilePath fp = filePath.withNewPath(fileName);
        // Do not return the entry for the directory we are searching in.
        if (fp.path() == filePath.path())
            return IterationPolicy::Continue;
        return std::get<1>(callBack)(fp, fi);
    };

    // Remove the first line, this can be the directory we are searching in.
    // as long as we do not specify "mindepth > 0"
    if (entries.front() == filePath.path())
        entries.pop_front();

    for (const QString &entry : entries) {
        if (toFilePath(entry) == IterationPolicy::Stop)
            break;
    }

    return true;
}

void UnixDeviceFileAccess::findUsingLs(const QString &current,
                                       const FileFilter &filter,
                                       QStringList *found,
                                       const QString &start) const
{
    const RunResult result = runInShell(
        {"ls", {"-1", "-a", "-p", "--", current}, OsType::OsTypeLinux});
    const QStringList entries = QString::fromUtf8(result.stdOut).split('\n', Qt::SkipEmptyParts);
    for (QString entry : entries) {
        const QChar last = entry.back();
        if (last == '/') {
            entry.chop(1);
            if (filter.iteratorFlags.testFlag(QDirIterator::Subdirectories) && entry != "."
                && entry != "..")
                findUsingLs(current + '/' + entry, filter, found, start + entry + "/");
        }
        found->append(start + entry);
    }
}

// Used on 'ls' output on unix-like systems.
static void iterateLsOutput(const FilePath &base,
                            const QStringList &entries,
                            const FileFilter &filter,
                            const FilePath::IterateDirCallback &callBack)
{
    const QList<QRegularExpression> nameRegexps
        = transform(filter.nameFilters, [](const QString &filter) {
              QRegularExpression re;
              re.setPattern(QRegularExpression::wildcardToRegularExpression(filter));
              QTC_CHECK(re.isValid());
              return re;
          });

    const auto nameMatches = [&nameRegexps](const QString &fileName) {
        for (const QRegularExpression &re : nameRegexps) {
            const QRegularExpressionMatch match = re.match(fileName);
            if (match.hasMatch())
                return true;
        }
        return nameRegexps.isEmpty();
    };

    // FIXME: Handle filters. For now bark on unsupported options.
    QTC_CHECK(filter.fileFilters == QDir::NoFilter);

    for (const QString &entry : entries) {
        if (!nameMatches(entry))
            continue;
        const FilePath current = base.pathAppended(entry);
        IterationPolicy res;
        if (callBack.index() == 0)
            res = std::get<0>(callBack)(current);
        else
            res = std::get<1>(callBack)(current, current.filePathInfo());
        if (res == IterationPolicy::Stop)
            break;
    }
}

void UnixDeviceFileAccess::iterateDirectory(const FilePath &filePath,
                                            const FilePath::IterateDirCallback &callBack,
                                            const FileFilter &filter) const
{
    // We try to use 'find' first, because that can filter better directly.
    // Unfortunately, it's not installed on all devices by default.
    if (m_tryUseFind) {
        if (iterateWithFind(filePath, filter, callBack))
            return;
        // Remember the failure for the next time and use the 'ls' fallback below.
        m_tryUseFind = false;
    }

    // if we do not have find - use ls as fallback
    QStringList entries;
    findUsingLs(filePath.path(), filter, &entries, {});
    iterateLsOutput(filePath, entries, filter, callBack);
}

Environment UnixDeviceFileAccess::deviceEnvironment() const
{
    const RunResult result = runInShell({"env", {}, OsType::OsTypeLinux});
    const QString out = QString::fromUtf8(result.stdOut);
    return Environment(out.split('\n', Qt::SkipEmptyParts), OsTypeLinux);
}

} // namespace Utils
