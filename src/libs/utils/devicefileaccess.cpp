// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devicefileaccess.h"

#include "algorithm.h"
#include "commandline.h"
#include "environment.h"
#include "expected.h"
#include "fileutils.h"
#include "hostosinfo.h"
#include "osspecificaspects.h"
#include "qtcassert.h"
#include "textcodec.h"
#include "utilstr.h"

#ifndef UTILS_STATIC_LIBRARY
#include "qtcprocess.h"
#endif

#include <QFileSystemWatcher>
#include <QOperatingSystemVersion>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QTemporaryFile>
#include <QThread>

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
#include <cstdio>

namespace Utils {

// DeviceFileAccess

DeviceFileAccess::DeviceFileAccess() = default;

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

Result<> DeviceFileAccess::ensureWritableDirectory(const FilePath &filePath) const
{
    if (isWritableDirectory(filePath))
        return ResultOk;

    if (exists(filePath)) {
        return ResultError(Tr::tr("Path \"%1\" exists but is not a writable directory.")
                                   .arg(filePath.toUserOutput()));
    }

    const bool result = createDirectory(filePath);
    if (result)
        return ResultOk;

    return ResultError(Tr::tr("Failed to create directory \"%1\".").arg(filePath.toUserOutput()));
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

Result<> DeviceFileAccess::removeFile(const FilePath &filePath) const
{
    QTC_CHECK(false);
    return ResultError(
        Tr::tr("removeFile is not implemented for \"%1\".").arg(filePath.toUserOutput()));
}

Result<> DeviceFileAccess::removeRecursively(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return ResultError(ResultUnimplemented);
}

Result<> DeviceFileAccess::copyFile(const FilePath &filePath, const FilePath &target) const
{
    Q_UNUSED(target)
    QTC_CHECK(false);
    return ResultError(
        Tr::tr("copyFile is not implemented for \"%1\".").arg(filePath.toUserOutput()));
}

static Result<> copyRecursively_fallback(const FilePath &src, const FilePath &target)
{
    Result<> result = ResultOk;
    src.iterateDirectory(
        [&target, &src, &result](const FilePath &path) {
            const FilePath relative = path.relativePathFromDir(src);
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

Result<> DeviceFileAccess::copyRecursively(const FilePath &src, const FilePath &target) const
{
    if (!src.isDir()) {
        return ResultError(
            Tr::tr("Cannot copy from \"%1\", it is not a directory.").arg(src.toUserOutput()));
    }
    const Result<> result = target.ensureWritableDir();
    if (!result) {
        return ResultError(Tr::tr("Cannot copy \"%1\" to \"%2\": %3")
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
                     [&srcProcess, &targetProcess] {
                         targetProcess.writeRaw(srcProcess.readAllRawStandardOutput());
                     });

    srcProcess.setCommand({sourceTar, {"-cf", "-", "."}});
    srcProcess.setWorkingDirectory(src);
    targetProcess.setCommand({targetTar, {"xf", "-"}});
    targetProcess.setWorkingDirectory(target);

    targetProcess.start();
    targetProcess.waitForStarted();

    srcProcess.start();
    srcProcess.waitForFinished();

    targetProcess.closeWriteChannel();

    if (srcProcess.result() != ProcessResult::FinishedWithSuccess) {
        targetProcess.kill();
        return ResultError(
            Tr::tr("Failed to copy recursively from \"%1\" to \"%2\" while "
                   "trying to create tar archive from source: %3")
                .arg(src.toUserOutput(), target.toUserOutput(), srcProcess.readAllStandardError()));
    }

    targetProcess.waitForFinished();

    if (targetProcess.result() != ProcessResult::FinishedWithSuccess) {
        return ResultError(Tr::tr("Failed to copy recursively from \"%1\" to \"%2\" while "
                                      "trying to extract tar archive to target: %3")
                                   .arg(src.toUserOutput(),
                                        target.toUserOutput(),
                                        targetProcess.readAllStandardError()));
    }

    return ResultOk;
#endif
}

Result<> DeviceFileAccess::renameFile(const FilePath &filePath, const FilePath &target) const
{
    Q_UNUSED(target)
    QTC_CHECK(false);
    return ResultError(
        Tr::tr("renameFile is not implemented for \"%1\".").arg(filePath.toUserOutput()));
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

Result<QByteArray> DeviceFileAccess::fileContents(const FilePath &filePath,
                                                        qint64 limit,
                                                        qint64 offset) const
{
    Q_UNUSED(filePath)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    QTC_CHECK(false);
    return ResultError(
        Tr::tr("fileContents is not implemented for \"%1\".").arg(filePath.toUserOutput()));
}

Result<qint64> DeviceFileAccess::writeFileContents(const FilePath &filePath,
                                                         const QByteArray &data) const
{
    Q_UNUSED(filePath)
    Q_UNUSED(data)
    QTC_CHECK(false);
    return ResultError(
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
    return -1;
}

QString DeviceFileAccess::owner(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return {};
}

uint DeviceFileAccess::ownerId(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return -2;
}

QString DeviceFileAccess::group(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return {};
}

uint DeviceFileAccess::groupId(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return -2;
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

Result<FilePath> DeviceFileAccess::createTempFile(const FilePath &filePath)
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return ResultError(
        Tr::tr("createTempFile is not implemented for \"%1\".").arg(filePath.toUserOutput()));
}

Result<std::unique_ptr<FilePathWatcher>> DeviceFileAccess::watch(const FilePath &path) const
{
    Q_UNUSED(path)
    return ResultError(Tr::tr("watch is not implemented."));
}

TextCodec DeviceFileAccess::processStdOutCodec(const FilePath &executable) const
{
    Q_UNUSED(executable)
    return TextCodec::utf8(); // Good default nowadays.
}

TextCodec DeviceFileAccess::processStdErrCodec(const FilePath &executable) const
{
    Q_UNUSED(executable)
    return TextCodec::utf8(); // Good default nowadays.
}

// UnavailableDeviceFileAccess

UnavailableDeviceFileAccess::UnavailableDeviceFileAccess() = default;

UnavailableDeviceFileAccess::~UnavailableDeviceFileAccess() = default;

static QString unavailableMessage()
{
    return Tr::tr("Device is unavailable.");
}

QString UnavailableDeviceFileAccess::mapToDevicePath(const QString &hostPath) const
{
    return hostPath;
}

bool UnavailableDeviceFileAccess::isExecutableFile(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return false;
}

bool UnavailableDeviceFileAccess::isReadableFile(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return false;
}

bool UnavailableDeviceFileAccess::isWritableFile(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return false;
}

bool UnavailableDeviceFileAccess::isReadableDirectory(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return false;
}

bool UnavailableDeviceFileAccess::isWritableDirectory(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return false;
}

bool UnavailableDeviceFileAccess::isFile(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return false;
}

bool UnavailableDeviceFileAccess::isDirectory(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return false;
}

bool UnavailableDeviceFileAccess::isSymLink(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return false;
}

bool UnavailableDeviceFileAccess::hasHardLinks(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return false;
}

Result<> UnavailableDeviceFileAccess::ensureWritableDirectory(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return ResultError(unavailableMessage());
}

bool UnavailableDeviceFileAccess::ensureExistingFile(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return false;
}

bool UnavailableDeviceFileAccess::createDirectory(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return false;
}

bool UnavailableDeviceFileAccess::exists(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return false;
}

Result<> UnavailableDeviceFileAccess::removeFile(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return ResultError(unavailableMessage());
}

Result<> UnavailableDeviceFileAccess::removeRecursively(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return ResultError(ResultUnimplemented);
}

Result<> UnavailableDeviceFileAccess::copyFile(const FilePath &filePath, const FilePath &target) const
{
    Q_UNUSED(filePath)
    Q_UNUSED(target)
    return ResultError(unavailableMessage());
}

Result<> UnavailableDeviceFileAccess::copyRecursively(const FilePath &src, const FilePath &target) const
{
    Q_UNUSED(src)
    Q_UNUSED(target)
    return ResultError(unavailableMessage());
}

Result<> UnavailableDeviceFileAccess::renameFile(const FilePath &filePath, const FilePath &target) const
{
    Q_UNUSED(filePath)
    Q_UNUSED(target)
    return ResultError(unavailableMessage());
}

FilePath UnavailableDeviceFileAccess::symLinkTarget(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return {};
}

void UnavailableDeviceFileAccess::iterateDirectory(const FilePath &filePath,
                                                   const FilePath::IterateDirCallback &callBack,
                                                   const FileFilter &filter) const
{
    Q_UNUSED(filePath)
    Q_UNUSED(callBack)
    Q_UNUSED(filter)
}

Environment UnavailableDeviceFileAccess::deviceEnvironment() const
{
    return {};
}

Result<QByteArray> UnavailableDeviceFileAccess::fileContents(const FilePath &filePath,
                                                                   qint64 limit,
                                                                   qint64 offset) const
{
    Q_UNUSED(filePath)
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    return ResultError(unavailableMessage());
}

Result<qint64> UnavailableDeviceFileAccess::writeFileContents(const FilePath &filePath,
                                                                    const QByteArray &data) const
{
    Q_UNUSED(filePath)
    Q_UNUSED(data)
    return ResultError(unavailableMessage());
}

FilePathInfo UnavailableDeviceFileAccess::filePathInfo(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return {};
}

QDateTime UnavailableDeviceFileAccess::lastModified(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return {};
}

QFile::Permissions UnavailableDeviceFileAccess::permissions(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return {};
}

bool UnavailableDeviceFileAccess::setPermissions(const FilePath &filePath, QFile::Permissions) const
{
    Q_UNUSED(filePath)
    return false;
}

qint64 UnavailableDeviceFileAccess::fileSize(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return -1;
}

QString UnavailableDeviceFileAccess::owner(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return {};
}

uint UnavailableDeviceFileAccess::ownerId(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return -2;
}

QString UnavailableDeviceFileAccess::group(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return {};
}

uint UnavailableDeviceFileAccess::groupId(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return -2;
}

qint64 UnavailableDeviceFileAccess::bytesAvailable(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return -1;
}

QByteArray UnavailableDeviceFileAccess::fileId(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return {};
}

std::optional<FilePath> UnavailableDeviceFileAccess::refersToExecutableFile(
    const FilePath &filePath, FilePath::MatchScope matchScope) const
{
    Q_UNUSED(filePath)
    Q_UNUSED(matchScope)
    return {};
}

Result<FilePath> UnavailableDeviceFileAccess::createTempFile(const FilePath &filePath)
{
    Q_UNUSED(filePath)
    return ResultError(unavailableMessage());
}

Result<std::unique_ptr<FilePathWatcher>>
    UnavailableDeviceFileAccess::watch(const FilePath &path) const
{
    Q_UNUSED(path)
    return ResultError(unavailableMessage());
}

// DesktopDeviceFileAccess

class DesktopFilePathWatcher final : public FilePathWatcher
{
    Q_OBJECT

    class GlobalWatcher final
    {
    public:
        GlobalWatcher()
        {
            m_thread.setObjectName(QStringLiteral("DesktopFilePathWatcher"));
            m_thread.start();
            d.moveToThread(&m_thread);
            d.init();
        }
        ~GlobalWatcher()
        {
            m_thread.quit();
            m_thread.wait();
        }

        bool watch(DesktopFilePathWatcher *watcher) { return d.watch(watcher); }
        bool removeWatch(DesktopFilePathWatcher *watcher) { return d.removeWatch(watcher); }

        static GlobalWatcher &instance()
        {
            static GlobalWatcher theInstance;
            return theInstance;
        }

    private:
        class Private : public QObject
        {
        public:
            void init() { QMetaObject::invokeMethod(this, &Private::_init, Qt::QueuedConnection); }
            bool watch(DesktopFilePathWatcher *watcher)
            {
                bool result;
                QMetaObject::invokeMethod(
                    this,
                    [this, watcher] { return _watch(watcher); },
                    Qt::BlockingQueuedConnection,
                    &result);

                connect(
                    watcher,
                    &DesktopFilePathWatcher::continueWatch,
                    this,
                    [this](const FilePath &path) { m_watcher->addPath(path.path()); });

                return result;
            }
            bool removeWatch(DesktopFilePathWatcher *watcher)
            {
                bool result;
                QMetaObject::invokeMethod(
                    this,
                    [this, watcher] { return _removeWatch(watcher); },
                    Qt::BlockingQueuedConnection,
                    &result);
                return result;
            }

        protected:
            void _init()
            {
                m_watcher = new QFileSystemWatcher(this);

                connect(m_watcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &path) {
                    notify(path, true);
                });

                connect(
                    m_watcher,
                    &QFileSystemWatcher::directoryChanged,
                    this,
                    [this](const QString &path) { notify(path, false); });
            }
            void notify(const QString &path, bool isFile) const
            {
                const FilePath filePath = FilePath::fromString(path);
                auto it = m_watchClients.find(filePath);
                if (it == m_watchClients.end())
                    return;

                const bool continueWatch = isFile
                                         && !m_watcher->files()
                                                 .contains(
                                                     path);
                for (DesktopFilePathWatcher *watcher : it.value()) {
                    watcher->emitChanged();
                    if (continueWatch) {
                        QMetaObject::invokeMethod(
                            watcher, &DesktopFilePathWatcher::requestContinueWatch);
                    }
                }
            }
            bool _watch(DesktopFilePathWatcher *watcher)
            {
                const FilePath path = watcher->path();
                auto it = m_watchClients.find(path);

                if (it == m_watchClients.end()) {
                    if (!m_watcher->addPath(path.path()))
                        return false;
                    it = m_watchClients.emplace(path);
                }
                it->append(watcher);
                return true;
            }

            bool _removeWatch(DesktopFilePathWatcher *watcher)
            {
                const FilePath path = watcher->path();
                auto it = m_watchClients.find(path);
                QTC_ASSERT(it != m_watchClients.end(), return false);

                it->removeOne(watcher);
                if (it->size() == 0) {
                    m_watchClients.erase(it);
                    return m_watcher->removePath(path.path());
                }
                return true;
            }

        private:
            QFileSystemWatcher *m_watcher = nullptr;
            QHash<FilePath, QList<DesktopFilePathWatcher *>> m_watchClients;
        };
        Private d;
        QThread m_thread;
    };

public:
    DesktopFilePathWatcher(const FilePath &path)
        : m_path(path)
    {
        if (!GlobalWatcher::instance().watch(this)) {
            if (path.exists())
                m_error = Tr::tr("Failed to watch \"%1\".").arg(path.toUserOutput());
            else
                m_error
                    = Tr::tr("Failed to watch \"%1\", it does not exist.").arg(path.toUserOutput());
        }
    }

    ~DesktopFilePathWatcher()
    {
        if (m_error.isEmpty()) {
            QTC_CHECK(GlobalWatcher::instance().removeWatch(this));
        }
    }

    FilePath path() const { return m_path; }

    void emitChanged() { emit pathChanged(m_path); }

    QString error() const { return m_error; }

    void requestContinueWatch()
    {
        if (m_path.exists()) {
            emit continueWatch(m_path);
        }
    }

signals:
    void continueWatch(const FilePath& path);

private:
    const FilePath m_path;
    QString m_error;
};

DesktopDeviceFileAccess::DesktopDeviceFileAccess()
{
}

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
    const int r = stat(filePath.absoluteFilePath().path().toLocal8Bit().constData(), &s);
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

Result<> DesktopDeviceFileAccess::ensureWritableDirectory(const FilePath &filePath) const
{
    const QFileInfo fi(filePath.path());
    if (fi.isDir() && fi.isWritable())
        return ResultOk;

    if (fi.exists()) {
        return ResultError(Tr::tr("Path \"%1\" exists but is not a writable directory.")
                                   .arg(filePath.toUserOutput()));
    }

    const bool result = QDir().mkpath(filePath.path());
    if (result)
        return ResultOk;

    return ResultError(
        Tr::tr("Failed to create directory \"%1\".").arg(filePath.toUserOutput()));
}

bool DesktopDeviceFileAccess::ensureExistingFile(const FilePath &filePath) const
{
    QFile f(filePath.path());
    if (f.exists())
        return true;
    if (!f.open(QFile::WriteOnly))
        return false;
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

Result<> DesktopDeviceFileAccess::removeFile(const FilePath &filePath) const
{
    QFile f(filePath.path());
    if (!f.remove())
        return ResultError(f.errorString());
    return ResultOk;
}

static Result<> checkToRefuseRemoveStandardLocationDirectory(const QString &dirPath,
                                                           QStandardPaths::StandardLocation location)
{
    if (QStandardPaths::standardLocations(location).contains(dirPath))
        return ResultError(Tr::tr("Refusing to remove the standard directory \"%1\".")
                         .arg(QStandardPaths::displayName(location)));
    return ResultOk;
}

static Result<> checkToRefuseRemoveDirectory(const QDir &dir)
{
    if (dir.isRoot())
        return ResultError(Tr::tr("Refusing to remove root directory."));

    const QString dirPath = dir.path();
    if (dirPath == QDir::home().canonicalPath())
        return ResultError(Tr::tr("Refusing to remove your home directory."));

    if (Result<> res = checkToRefuseRemoveStandardLocationDirectory(dirPath, QStandardPaths::DocumentsLocation); !res)
        return res;
    if (Result<> res = checkToRefuseRemoveStandardLocationDirectory(dirPath, QStandardPaths::DownloadLocation); !res)
        return res;
    if (Result<> res = checkToRefuseRemoveStandardLocationDirectory(dirPath, QStandardPaths::AppDataLocation); !res)
        return res;
    if (Result<> res = checkToRefuseRemoveStandardLocationDirectory(dirPath, QStandardPaths::AppLocalDataLocation); !res)
        return res;
    return ResultOk;
}

Result<> DesktopDeviceFileAccess::removeRecursively(const FilePath &filePath) const
{
    QTC_ASSERT(filePath.isLocal(), return ResultError(ResultAssert));
    QFileInfo fileInfo = filePath.toFileInfo();
    if (!fileInfo.exists() && !fileInfo.isSymLink())
        return ResultOk;

    QFile::setPermissions(fileInfo.absoluteFilePath(), fileInfo.permissions() | QFile::WriteUser);

    if (fileInfo.isDir()) {
        QDir dir(fileInfo.absoluteFilePath());
        dir.setPath(dir.canonicalPath());
        if (Result<> res = checkToRefuseRemoveDirectory(dir); !res)
            return res;

        const QStringList fileNames = dir.entryList(QDir::Files | QDir::Hidden | QDir::System
                                                    | QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &fileName : fileNames) {
            if (Result<> res = removeRecursively(filePath / fileName); !res)
                return res;
        }
        if (!QDir::root().rmdir(dir.path()))
            return ResultError(Tr::tr("Failed to remove directory \"%1\".").arg(filePath.toUserOutput()));
    } else {
        if (!QFile::remove(filePath.path()))
            return ResultError(Tr::tr("Failed to remove file \"%1\".").arg(filePath.toUserOutput()));
    }
    return ResultOk;
}

Result<> DesktopDeviceFileAccess::copyFile(const FilePath &filePath, const FilePath &target) const
{
    QFile srcFile(filePath.path());

    if (srcFile.copy(target.path()))
        return ResultOk;
    return ResultError(
        Tr::tr("Failed to copy file \"%1\" to \"%2\": %3")
            .arg(filePath.toUserOutput(), target.toUserOutput(), srcFile.errorString()));
}

Result<> DesktopDeviceFileAccess::renameFile(
    const FilePath &filePath, const FilePath &target) const
{
    QFile f(filePath.path());

    if (f.rename(target.path()))
        return ResultOk;
    return ResultError(
        Tr::tr("Failed to rename file \"%1\" to \"%2\": %3")
            .arg(filePath.toUserOutput(), target.toUserOutput(), f.errorString()));
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

Result<QByteArray> DesktopDeviceFileAccess::fileContents(const FilePath &filePath,
                                                               qint64 limit,
                                                               qint64 offset) const
{
    const QString path = filePath.path();
    QFile f(path);
    if (!f.exists())
        return ResultError(Tr::tr("File \"%1\" does not exist.").arg(path));

    if (!f.open(QFile::ReadOnly))
        return ResultError(Tr::tr("Could not open File \"%1\".").arg(path));

    if (offset != 0)
        f.seek(offset);

    if (limit != -1)
        return f.read(limit);

    const QByteArray data = f.readAll();
    if (f.error() != QFile::NoError) {
        return ResultError(
            Tr::tr("Cannot read \"%1\": %2").arg(filePath.toUserOutput(), f.errorString()));
    }

    return data;
}

Result<qint64> DesktopDeviceFileAccess::writeFileContents(const FilePath &filePath,
                                                                const QByteArray &data) const
{
    QFile file(filePath.path());
    const bool isOpened = file.open(QFile::WriteOnly | QFile::Truncate);
    if (!isOpened)
        return ResultError(
            Tr::tr("Could not open file \"%1\" for writing.").arg(filePath.toUserOutput()));

    qint64 res = file.write(data);
    if (res != data.size())
        return ResultError(
            Tr::tr("Could not write to file \"%1\" (only %2 of %n byte(s) written).",
                   nullptr,
                   data.size())
                .arg(filePath.toUserOutput())
                .arg(res));
    return res;
}

Result<FilePath> DesktopDeviceFileAccess::createTempFile(const FilePath &filePath)
{
    QTemporaryFile file(filePath.path());
    file.setAutoRemove(false);
    if (!file.open()) {
        return ResultError(Tr::tr("Could not create temporary file in \"%1\" (%2).")
                                   .arg(filePath.toUserOutput())
                                   .arg(file.errorString()));
    }
    return filePath.withNewPath(file.fileName());
}

Result<std::unique_ptr<FilePathWatcher>> DesktopDeviceFileAccess::watch(const FilePath &path) const
{
    auto watcher = std::make_unique<DesktopFilePathWatcher>(path);
    if (watcher->error().isEmpty())
        return watcher;
    return ResultError(watcher->error());
}

TextCodec DesktopDeviceFileAccess::processStdOutCodec(const FilePath &executable) const
{
    Q_UNUSED(executable)
    return TextCodec::codecForLocale();
}

TextCodec DesktopDeviceFileAccess::processStdErrCodec(const FilePath &executable) const
{
    Q_UNUSED(executable)
    return TextCodec::codecForLocale();
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

QString DesktopDeviceFileAccess::owner(const FilePath &filePath) const
{
    return QFileInfo(filePath.path()).owner();
}

uint DesktopDeviceFileAccess::ownerId(const FilePath &filePath) const
{
    return QFileInfo(filePath.path()).ownerId();
}

QString DesktopDeviceFileAccess::group(const FilePath &filePath) const
{
    return QFileInfo(filePath.path()).group();
}

uint DesktopDeviceFileAccess::groupId(const FilePath &filePath) const
{
    return QFileInfo(filePath.path()).groupId();
}

// Copied from qfilesystemengine_win.cpp
#ifdef Q_OS_WIN

// File ID for Windows up to version 7.
static inline QByteArray fileIdWin7(HANDLE handle)
{
    BY_HANDLE_FILE_INFORMATION info;
    if (GetFileInformationByHandle(handle, &info)) {
        char buffer[sizeof "01234567:0123456701234567\0"];
        std::snprintf(buffer, sizeof(buffer),
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

Result<> UnixDeviceFileAccess::runInShellSuccess(const CommandLine &cmdLine,
                                               const QByteArray &stdInData) const
{
    const int retval = runInShell(cmdLine, stdInData).exitCode;
    if (retval != 0)
        return ResultError(QString("return value %1").arg(retval));
    return ResultOk;
}

bool UnixDeviceFileAccess::isExecutableFile(const FilePath &filePath) const
{
    const QString path = filePath.path();
    return runInShellSuccess({"test", {"-x", path}, OsType::OsTypeLinux}).has_value();
}

bool UnixDeviceFileAccess::isReadableFile(const FilePath &filePath) const
{
    const QString path = filePath.path();
    return runInShellSuccess({"test", {"-r", path, "-a", "-f", path}, OsType::OsTypeLinux}).has_value();
}

bool UnixDeviceFileAccess::isWritableFile(const FilePath &filePath) const
{
    const QString path = filePath.path();
    return runInShellSuccess({"test", {"-w", path, "-a", "-f", path}, OsType::OsTypeLinux}).has_value();
}

bool UnixDeviceFileAccess::isReadableDirectory(const FilePath &filePath) const
{
    const QString path = filePath.path();
    return runInShellSuccess({"test", {"-r", path, "-a", "-d", path}, OsType::OsTypeLinux}).has_value();
}

bool UnixDeviceFileAccess::isWritableDirectory(const FilePath &filePath) const
{
    const QString path = filePath.path();
    return runInShellSuccess({"test", {"-w", path, "-a", "-d", path}, OsType::OsTypeLinux}).has_value();
}

bool UnixDeviceFileAccess::isFile(const FilePath &filePath) const
{
    const QString path = filePath.path();
    return runInShellSuccess({"test", {"-f", path}, OsType::OsTypeLinux}).has_value();
}

bool UnixDeviceFileAccess::isDirectory(const FilePath &filePath) const
{
    if (filePath.isRootPath())
        return true;

    const QString path = filePath.path();
    return runInShellSuccess({"test", {"-d", path}, OsType::OsTypeLinux}).has_value();
}

bool UnixDeviceFileAccess::isSymLink(const FilePath &filePath) const
{
    const QString path = filePath.path();
    return runInShellSuccess({"test", {"-h", path}, OsType::OsTypeLinux}).has_value();
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
    return runInShellSuccess({"touch", {path}, OsType::OsTypeLinux}).has_value();
}

bool UnixDeviceFileAccess::createDirectory(const FilePath &filePath) const
{
    const QString path = filePath.path();
    return runInShellSuccess({"mkdir", {"-p", path}, OsType::OsTypeLinux}).has_value();
}

bool UnixDeviceFileAccess::exists(const FilePath &filePath) const
{
    const QString path = filePath.path();
    return runInShellSuccess({"test", {"-e", path}, OsType::OsTypeLinux}).has_value();
}

Result<> UnixDeviceFileAccess::removeFile(const FilePath &filePath) const
{
    RunResult result = runInShell({"rm", {filePath.path()}, OsType::OsTypeLinux});
    if (result.exitCode != 0)
        return ResultError(QString::fromUtf8(result.stdErr));
    return ResultOk;
}

Result<> UnixDeviceFileAccess::removeRecursively(const FilePath &filePath) const
{
    QTC_ASSERT(filePath.path().startsWith('/'), return ResultError(ResultAssert));

    const QString path = filePath.cleanPath().path();
    // We are expecting this only to be called in a context of build directories or similar.
    // Chicken out in some cases that _might_ be user code errors.
    QTC_ASSERT(path.startsWith('/'), return ResultError(ResultAssert));
    int levelsNeeded = path.startsWith("/home/") ? 4 : 3;
    if (path.startsWith("/tmp/"))
        levelsNeeded = 2;
    QTC_ASSERT(path.count('/') >= levelsNeeded, return ResultError(ResultAssert));

    RunResult result = runInShell({"rm", {"-rf", "--", path}, OsType::OsTypeLinux});

    if (result.exitCode != 0)
        return ResultError(QString::fromUtf8(result.stdErr));

    return ResultOk;
}

Result<> UnixDeviceFileAccess::copyFile(const FilePath &filePath, const FilePath &target) const
{
    const RunResult result = runInShell(
        {"cp", {filePath.path(), target.path()}, OsType::OsTypeLinux});

    if (result.exitCode != 0) {
        return ResultError(Tr::tr("Failed to copy file \"%1\" to \"%2\": %3")
                                   .arg(filePath.toUserOutput(),
                                        target.toUserOutput(),
                                        QString::fromUtf8(result.stdErr)));
    }
    return ResultOk;
}

Result<> UnixDeviceFileAccess::renameFile(const FilePath &filePath, const FilePath &target) const
{
    RunResult result = runInShell({"mv", {filePath.path(), target.path()}, OsType::OsTypeLinux});
    if (result.exitCode != 0) {
        return ResultError(Tr::tr("Failed to rename file \"%1\" to \"%2\": %3")
                                   .arg(filePath.toUserOutput(),
                                        target.toUserOutput(),
                                        QString::fromUtf8(result.stdErr)));
    }
    return ResultOk;
}

FilePath UnixDeviceFileAccess::symLinkTarget(const FilePath &filePath) const
{
    const RunResult result = runInShell(
        {"readlink", {"-n", "-e", filePath.path()}, OsType::OsTypeLinux});
    const QString out = QString::fromUtf8(result.stdOut);
    return out.isEmpty() ? FilePath() : filePath.withNewPath(out);
}

Result<QByteArray> UnixDeviceFileAccess::fileContents(const FilePath &filePath,
                                                            qint64 limit,
                                                            qint64 offset) const
{
    Result<FilePath> localSource = filePath.localSource();
    if (localSource && *localSource != filePath)
        return localSource->fileContents(limit, offset);

    QStringList args = {"if=" + filePath.path()};
    if (limit > 0 || offset > 0) {
        const qint64 gcd = std::gcd(limit, offset);
        args += QString("bs=%1").arg(gcd);
        args += QString("count=%1").arg(limit / gcd);
        args += QString("seek=%1").arg(offset / gcd);
    }
#ifndef UTILS_STATIC_LIBRARY
    const FilePath dd = filePath.withNewPath("dd");
    using namespace std::literals::chrono_literals;

    Process p;
    p.setCommand({dd, args, OsType::OsTypeLinux});
    p.runBlocking(0s); // Run forever
    if (p.exitCode() != 0) {
        return ResultError(Tr::tr("Failed reading file \"%1\": %2")
                                   .arg(filePath.toUserOutput(), p.readAllStandardError()));
    }
    return p.rawStdOut();
#else
    return ResultError(QString("Not implemented"));
#endif
}

Result<qint64> UnixDeviceFileAccess::writeFileContents(const FilePath &filePath,
                                                             const QByteArray &data) const
{
    Result<FilePath> localSource = filePath.localSource();
    if (localSource && *localSource != filePath)
        return localSource->writeFileContents(data);

    QStringList args = {"of=" + filePath.path()};
    RunResult result = runInShell({"dd", args, OsType::OsTypeLinux}, data);

    if (result.exitCode != 0) {
        return ResultError(Tr::tr("Failed writing file \"%1\": %2")
                                   .arg(filePath.toUserOutput(), QString::fromUtf8(result.stdErr)));
    }
    return data.size();
}

Result<FilePath> UnixDeviceFileAccess::createTempFile(const FilePath &filePath)
{
    if (!m_hasMkTemp.has_value())
        m_hasMkTemp = runInShellSuccess({"which", {"mktemp"}, OsType::OsTypeLinux}).has_value();

    QString tmplate = filePath.path();
    // Some mktemp implementations require a suffix of XXXXXX.
    // They will only accept the template if at least the last 6 characters are X.
    if (!tmplate.endsWith("XXXXXX"))
        tmplate += ".XXXXXX";

    if (m_hasMkTemp) {
        const RunResult result = runInShell({"mktemp", {tmplate}, OsType::OsTypeLinux});

        if (result.exitCode != 0) {
            return ResultError(
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
            return ResultError(Tr::tr("Failed creating temporary file \"%1\" (too many tries).")
                                       .arg(filePath.toUserOutput()));
        }
    } while (newPath.exists());

    const Result<qint64> createResult = newPath.writeFileContents({});

    if (!createResult)
        return ResultError(createResult.error());

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

/*
Convert QFileDevice::Permissions to Unix chmod flags.
The mode is copied from system libraries.
The logic is copied from qfiledevice_p.h "toMode_t" function.
*/
constexpr int toUnixChmod(QFileDevice::Permissions permissions)
{
    int mode = 0;
    if (permissions & (QFileDevice::ReadOwner | QFileDevice::ReadUser))
        mode |= 0000400; // S_IRUSR
    if (permissions & (QFileDevice::WriteOwner | QFileDevice::WriteUser))
        mode |= 0000200; // S_IWUSR
    if (permissions & (QFileDevice::ExeOwner | QFileDevice::ExeUser))
        mode |= 0000100; // S_IXUSR
    if (permissions & QFileDevice::ReadGroup)
        mode |= 0000040; // S_IRGRP
    if (permissions & QFileDevice::WriteGroup)
        mode |= 0000020; // S_IWGRP
    if (permissions & QFileDevice::ExeGroup)
        mode |= 0000010; // S_IXGRP
    if (permissions & QFileDevice::ReadOther)
        mode |= 0000004; // S_IROTH
    if (permissions & QFileDevice::WriteOther)
        mode |= 0000002; // S_IWOTH
    if (permissions & QFileDevice::ExeOther)
        mode |= 0000001; // S_IXOTH
    return mode;
}

bool UnixDeviceFileAccess::setPermissions(const FilePath &filePath, QFile::Permissions perms) const
{
    const int flags = toUnixChmod(perms);
    return runInShellSuccess(
        {"chmod", {"0" + QString::number(flags, 8), filePath.path()}, OsType::OsTypeLinux}).has_value();
}

qint64 UnixDeviceFileAccess::fileSize(const FilePath &filePath) const
{
    const QStringList args = statArgs(filePath, "%s", "%z");
    const RunResult result = runInShell({"stat", args, OsType::OsTypeLinux});
    return result.stdOut.toLongLong();
}

QString UnixDeviceFileAccess::owner(const FilePath &filePath) const
{
    const QStringList args = statArgs(filePath, "%U", "%U");
    const RunResult result = runInShell({"stat", args, OsType::OsTypeLinux});
    return QString::fromUtf8(result.stdOut);
}

uint UnixDeviceFileAccess::ownerId(const FilePath &filePath) const
{
    const QStringList args = statArgs(filePath, "%u", "%u");
    const RunResult result = runInShell({"stat", args, OsType::OsTypeLinux});
    return result.stdOut.toUInt();
}

QString UnixDeviceFileAccess::group(const FilePath &filePath) const
{
    const QStringList args = statArgs(filePath, "%G", "%G");
    const RunResult result = runInShell({"stat", args, OsType::OsTypeLinux});
    return QString::fromUtf8(result.stdOut);
}

uint UnixDeviceFileAccess::groupId(const FilePath &filePath) const
{
    const QStringList args = statArgs(filePath, "%g", "%g");
    const RunResult result = runInShell({"stat", args, OsType::OsTypeLinux});
    return result.stdOut.toUInt();
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
    if (filePath.path() == "/") { // TODO: Add FilePath::isRoot()
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

    for (const QString &entry : std::as_const(entries)) {
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

#include "devicefileaccess.moc"
