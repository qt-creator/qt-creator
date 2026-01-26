// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "devicefileaccess.h"

#include "algorithm.h"
#include "commandline.h"
#include "environment.h"
#include "fileutils.h"
#include "hostosinfo.h"
#include "osspecificaspects.h"
#include "qtcassert.h"
#include "textcodec.h"
#include "utilstr.h"

#ifndef UTILS_STATIC_LIBRARY
#include "qtcprocess.h"
#endif

#include <QElapsedTimer>
#include <QFileSystemWatcher>
#include <QOperatingSystemVersion>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QThread>
#include <QTimer>

#ifdef Q_OS_WIN
#ifdef QTCREATOR_PCH_H
#define CALLBACK WINAPI
#endif
#include <shlobj.h>
#include <winioctl.h>
#include <qt_windows.h>
#else
#include <qplatformdefs.h>
#endif

#include <algorithm>
#include <array>
#include <cstdio>

namespace Utils {

static ResultError notImplementedError(const QString &function, const FilePath &filePath)
{
    return ResultError(ResultUnimplemented,
        Tr::tr("Function \"%1\" is not implemented for \"%2\".").arg(function, filePath.toUserOutput()));
}

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

Result<bool> DeviceFileAccess::isExecutableFile(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return ResultError(ResultAssert);
}

Result<bool> DeviceFileAccess::isReadableFile(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return ResultError(ResultAssert);
}

Result<bool> DeviceFileAccess::isWritableFile(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return ResultError(ResultAssert);
}

Result<bool> DeviceFileAccess::isReadableDirectory(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return ResultError(ResultAssert);
}

Result<bool> DeviceFileAccess::isWritableDirectory(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return ResultError(ResultAssert);
}

Result<bool> DeviceFileAccess::isFile(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return ResultError(ResultAssert);
}

Result<bool> DeviceFileAccess::isDirectory(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return ResultError(ResultAssert);
}

Result<bool> DeviceFileAccess::isSymLink(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return ResultError(ResultAssert);
}

Result<bool> DeviceFileAccess::hasHardLinks(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return ResultError(ResultAssert);
}

Result<> DeviceFileAccess::ensureWritableDirectory(const FilePath &filePath) const
{
    return isWritableDirectory(filePath).and_then([&](bool isWritableDir) {
        if (isWritableDir)
            return ResultOk;

        return exists(filePath).and_then([&](bool exists) -> Result<> {
            if (!exists)
                return createDirectory(filePath);

            return ResultError(
                Tr::tr("Path \"%1\" exists but is not a writable directory.")
                    .arg(filePath.toUserOutput()));
        });
    });
}

Result<> DeviceFileAccess::ensureExistingFile(const FilePath &filePath) const
{
    QTC_CHECK(false);
    return notImplementedError("ensureExistingFile()", filePath);
}

Result<> DeviceFileAccess::createDirectory(const FilePath &filePath) const
{
    QTC_CHECK(false);
    return notImplementedError("createDirectory()", filePath);
}

Result<bool> DeviceFileAccess::exists(const FilePath &filePath) const
{
    QTC_CHECK(false);
    return notImplementedError("exists()", filePath);
}

Result<> DeviceFileAccess::removeFile(const FilePath &filePath) const
{
    QTC_CHECK(false);
    return notImplementedError("removeFile", filePath);
}

Result<> DeviceFileAccess::removeRecursively(const FilePath &filePath) const
{
    QTC_CHECK(false);
    return notImplementedError("removeRecursively", filePath);
}

Result<> DeviceFileAccess::copyFile(const FilePath &filePath, const FilePath &target) const
{
    Q_UNUSED(target)
    QTC_CHECK(false);
    return ResultError(
        Tr::tr("copyFile is not implemented for \"%1\".").arg(filePath.toUserOutput()));
}

Result<> DeviceFileAccess::createSymLink(const FilePath &filePath, const FilePath &symLink) const
{
    Q_UNUSED(symLink)
    QTC_CHECK(false);
    return ResultError(
        Tr::tr("createSymLink is not implemented for \"%1\".").arg(filePath.toUserOutput()));
}

static Result<> copyRecursively_fallback(const FilePath &src, const FilePath &target)
{
    Result<> result = ResultOk;
    src.iterateDirectory(
        [&target, &src, &result](const FilePath &path) {
            const QString relative = path.relativePathFromDir(src);
            const FilePath targetPath = target.pathAppended(relative);
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
    return ResultError(ResultAssert,
        Tr::tr("renameFile is not implemented for \"%1\".").arg(filePath.toUserOutput()));
}

Result<FilePath> DeviceFileAccess::symLinkTarget(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return {};
}

Result<> DeviceFileAccess::iterateDirectory(
    const FilePath &filePath,
    const FilePath::IterateDirCallback &callBack,
    const FileFilter &filter) const
{
    Q_UNUSED(callBack)
    Q_UNUSED(filter)
    QTC_CHECK(false);
    return notImplementedError("iterateDirectory()", filePath);
}

Result<Environment> DeviceFileAccess::deviceEnvironment() const
{
    QTC_CHECK(false);
    return notImplementedError("deviceEnvironment()", "");
}

Result<QByteArray> DeviceFileAccess::fileContents(const FilePath &filePath,
                                                  qint64 limit,
                                                  qint64 offset) const
{
    Q_UNUSED(limit)
    Q_UNUSED(offset)
    QTC_CHECK(false);
    return notImplementedError("fileContents()", filePath);
}

Result<qint64> DeviceFileAccess::writeFileContents(const FilePath &filePath,
                                                   const QByteArray &data) const
{
    Q_UNUSED(data)
    QTC_CHECK(false);
    return notImplementedError("writeFileContents()", filePath);
}

Result<FilePathInfo> DeviceFileAccess::filePathInfo(const FilePath &filePath) const
{
    QTC_CHECK(false);
    return notImplementedError("filePathInfo()", filePath);
}

Result<QDateTime> DeviceFileAccess::lastModified(const FilePath &filePath) const
{
    QTC_CHECK(false);
    return notImplementedError("lastModified()", filePath);
}

Result<QFile::Permissions> DeviceFileAccess::permissions(const FilePath &filePath) const
{
    QTC_CHECK(false);
    return notImplementedError("permissions()", filePath);
}

Result<> DeviceFileAccess::setPermissions(const FilePath &filePath, QFile::Permissions) const
{
    QTC_CHECK(false);
    return notImplementedError("setPermissions()", filePath);
}

Result<qint64> DeviceFileAccess::fileSize(const FilePath &filePath) const
{
    QTC_CHECK(false);
    return notImplementedError("fileSize()", filePath);
}

Result<QString> DeviceFileAccess::owner(const FilePath &filePath) const
{
    QTC_CHECK(false);
    return notImplementedError("owner()", filePath);
}

Result<uint> DeviceFileAccess::ownerId(const FilePath &filePath) const
{
    QTC_CHECK(false);
    return notImplementedError("ownerId()", filePath);
}

Result<QString> DeviceFileAccess::group(const FilePath &filePath) const
{
    QTC_CHECK(false);
    return notImplementedError("group()", filePath);
}

Result<uint> DeviceFileAccess::groupId(const FilePath &filePath) const
{
    QTC_CHECK(false);
    return notImplementedError("groupId()", filePath);
}

Result<qint64> DeviceFileAccess::bytesAvailable(const FilePath &filePath) const
{
    QTC_CHECK(false);
    return notImplementedError("bytesAvailable()", filePath);
}

Result<QByteArray> DeviceFileAccess::fileId(const FilePath &filePath) const
{
    QTC_CHECK(false);
    return notImplementedError("fileId()", filePath);
}

Result<std::optional<FilePath>> DeviceFileAccess::refersToExecutableFile(
    const FilePath &filePath, FilePath::MatchScope matchScope) const
{
    Q_UNUSED(matchScope)
    const Result<bool> res = isExecutableFile(filePath);
    if (!res)
        return ResultError(res.error());
    if (*res)
        return filePath;
    return {};
}

Result<FilePath> DeviceFileAccess::createTempFile(const FilePath &filePath)
{
    QTC_CHECK(false);
    return notImplementedError("createTempFile()", filePath);
}

Result<FilePath> DeviceFileAccess::createTempDir(const FilePath &filePath)
{
    Q_UNUSED(filePath)
    QTC_CHECK(false);
    return notImplementedError("createTempDir()", filePath);
}

std::vector<Result<std::unique_ptr<FilePathWatcher>>> DeviceFileAccess::watch(
    const FilePaths &filePaths) const
{
    return Utils::transform<std::vector>(filePaths, [](const FilePath &path) {
        return Result<std::unique_ptr<FilePathWatcher>>(notImplementedError("watch()", path));
    });
}

TextEncoding DeviceFileAccess::processStdOutEncoding(const FilePath &executable) const
{
    Q_UNUSED(executable)
    return TextEncoding::Utf8; // Good default nowadays.
}

TextEncoding DeviceFileAccess::processStdErrEncoding(const FilePath &executable) const
{
    Q_UNUSED(executable)
    return TextEncoding::Utf8; // Good default nowadays.
}

bool DeviceFileAccess::supportsAtomicSaveFile(const FilePath &filePath) const
{
    if (hasHardLinks(filePath))
        return false;
    return true;
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

        QList<Result<>> watch(const QList<DesktopFilePathWatcher *> &watchers)
        {
            return d.watch(watchers);
        }

        Result<> removeWatch(DesktopFilePathWatcher *watcher) { return d.removeWatch(watcher); }

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
            QList<Result<>> watch(const QList<DesktopFilePathWatcher *> &watchers)
            {
                QList<Result<>> results;
                QMetaObject::invokeMethod(
                    this,
                    [this, watchers] { return _watch(watchers); },
                    Qt::BlockingQueuedConnection,
                    &results);
                return results;
            }
            Result<> removeWatch(DesktopFilePathWatcher *watcher)
            {
                Result<> result;
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
                // magic numbers
                static constexpr int compressInterval = 50;
                static constexpr qint64 maxReschedulingInterval = 300;
                m_notifyTimer = new QTimer(this);
                m_notifyTimer->setInterval(compressInterval);
                m_notifyTimer->setSingleShot(true);
                connect(m_notifyTimer, &QTimer::timeout, this, &Private::notify);

                m_watcher = new QFileSystemWatcher(this);
                const auto addPath = [this](const QString &path) {
                    // Restart the timer if it wasn't already running,
                    // or we already saw an event for that file - so we only send a single
                    // signal for multiple consecutive events for the same file.
                    // The QFileSystemWatcher can send multiple ones while the file is modified,
                    // and if the file is removed and readded (like with "atomic" writes or version
                    // control operations) we also only want a single event.
                    // But guard that rescheduling by a max delay.
                    const bool isNew = Utils::insert(m_scheduledPaths, FilePath::fromString(path));
                    if (!m_notifyTimer->isActive()) {
                        m_schedulingStarted.start();
                        m_notifyTimer->start();
                    } else if (!isNew && m_schedulingStarted.elapsed() < maxReschedulingInterval) {
                        m_notifyTimer->start();
                    }
                };
                connect(m_watcher, &QFileSystemWatcher::fileChanged, this, addPath);
                connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, addPath);
            }

            void notify()
            {
                const QSet<FilePath> paths = m_scheduledPaths;
                m_scheduledPaths.clear();
                FilePaths toReAdd;
                QList<DesktopFilePathWatcher *> toNotify;
                const QStringList watchedFiles = m_watcher->files();
                const QStringList watchedDirectories = m_watcher->directories();
                for (const FilePath &filePath : paths) {
                    auto it = m_watchClients.find(filePath);
                    if (it == m_watchClients.end())
                        continue;

                    // If the file was removed and readded (like with "atomic" writes), we
                    // might loose the watch, so possibly readd
                    const bool reinitWatch = !watchedFiles.contains(filePath.path())
                                             && !watchedDirectories.contains(filePath.path())
                                             && filePath.exists();
                    if (reinitWatch)
                        toReAdd << filePath;
                    toNotify += it.value();
                }
                if (!toReAdd.isEmpty()) {
                    const QStringList failedPaths = m_watcher->addPaths(
                        Utils::transform(toReAdd, &FilePath::path));
                    if (!failedPaths.isEmpty())
                        qWarning()
                            << "Failed to re-add watches for following files:" << failedPaths;
                }
                for (DesktopFilePathWatcher *watcher : std::as_const(toNotify))
                    watcher->emitChanged();
            }
            QList<Result<>> _watch(const QList<DesktopFilePathWatcher *> &watchers)
            {
                QList<Result<>> results(watchers.size()); // initialize everything with "Ok"
                QList<std::pair<int, DesktopFilePathWatcher *>> newToWatch; // (index, watcher)
                // Check if we need to add the file watch, otherwise just keep note
                // of the new client for the already watched file.
                for (int i = 0; i < watchers.size(); ++i) {
                    DesktopFilePathWatcher *watcher = watchers.at(i);
                    const FilePath path = watcher->path();
                    auto it = m_watchClients.find(path);
                    if (it == m_watchClients.end())
                        newToWatch += std::pair<int, DesktopFilePathWatcher *>(i, watcher);
                    else
                        it->append(watcher);
                }
                if (newToWatch.isEmpty())
                    return results;
                // add the required new watches
                const QStringList failedPaths = m_watcher->addPaths(
                    Utils::transform(
                        newToWatch, [](const std::pair<int, DesktopFilePathWatcher *> &item) {
                            return item.second->path().path();
                        }));
                // Keep note of the clients for successful ones, add error for failed ones
                for (const std::pair<int, DesktopFilePathWatcher *> &item :
                     std::as_const(newToWatch)) {
                    const FilePath path = item.second->path();
                    if (failedPaths.contains(path.path())) {
                        if (!path.exists()) {
                            results[item.first] = ResultError(
                                Tr::tr("Failed to watch \"%1\", it does not exist.")
                                    .arg(path.toUserOutput()));
                        } else {
                            results[item.first] = ResultError(
                                Tr::tr("Failed to watch \"%1\".").arg(path.toUserOutput()));
                        }
                    } else {
                        m_watchClients[path].append(item.second);
                    }
                }
                return results;
            }

            Result<> _removeWatch(DesktopFilePathWatcher *watcher)
            {
                const FilePath path = watcher->path();
                auto it = m_watchClients.find(path);
                QTC_ASSERT(
                    it != m_watchClients.end(),
                    return ResultError(
                        Tr::tr("Failed to remove watcher for \"%1\", it was not found.")
                            .arg(path.toUserOutput())));

                it->removeOne(watcher);
                if (it->size() == 0) {
                    m_watchClients.erase(it);
                    if (!m_watcher->removePath(path.path())) {
                        if (!path.exists())
                            return ResultOk;
                        return ResultError(
                            Tr::tr("Failed to remove watcher for \"%1\".").arg(path.toUserOutput()));
                    }
                }
                return ResultOk;
            }

        private:
            QFileSystemWatcher *m_watcher = nullptr;
            QHash<FilePath, QList<DesktopFilePathWatcher *>> m_watchClients;
            QTimer *m_notifyTimer = nullptr;
            QElapsedTimer m_schedulingStarted;
            QSet<FilePath> m_scheduledPaths;
        };
        Private d;
        QThread m_thread;
    };

public:
    DesktopFilePathWatcher(const FilePath &path)
        : m_path(path)
    {
    }

    ~DesktopFilePathWatcher()
    {
        if (m_isWatched) {
            QTC_CHECK_RESULT(GlobalWatcher::instance().removeWatch(this));
        }
    }

    FilePath path() const { return m_path; }

    void emitChanged() { emit pathChanged(m_path); }

    static std::vector<Result<std::unique_ptr<FilePathWatcher>>> watch(const FilePaths &paths)
    {
        std::vector<std::unique_ptr<DesktopFilePathWatcher>> watchers
            = Utils::transform<std::vector>(paths, [](const FilePath &path) {
                  return std::make_unique<DesktopFilePathWatcher>(path);
              });
        const QList<Result<>> watchResults = GlobalWatcher::instance().watch(
            Utils::transform<QList>(watchers, &std::unique_ptr<DesktopFilePathWatcher>::get));
        std::vector<Result<std::unique_ptr<FilePathWatcher>>> results;
        for (int i = 0; i < watchResults.size(); ++i) {
            if (watchResults.at(i)) {
                watchers.at(i)->m_isWatched = true;
                results.push_back(std::move(watchers.at(i)));
            } else {
                results.push_back(ResultError(watchResults.at(i).error()));
            }
        }
        return results;
    }

private:
    const FilePath m_path;
    bool m_isWatched = false;
};

DesktopDeviceFileAccess::DesktopDeviceFileAccess()
{
}

DesktopDeviceFileAccess::~DesktopDeviceFileAccess() = default;

DeviceFileAccessPtr DesktopDeviceFileAccess::instance()
{
    static auto theInstance = std::make_shared<DesktopDeviceFileAccess>();
    return theInstance;
}

Result<bool> DesktopDeviceFileAccess::isExecutableFile(const FilePath &filePath) const
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

Result<std::optional<FilePath>> DesktopDeviceFileAccess::refersToExecutableFile(
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

Result<bool> DesktopDeviceFileAccess::isReadableFile(const FilePath &filePath) const
{
    const QFileInfo fi(filePath.path());
    return fi.exists() && fi.isFile() && fi.isReadable();
}

Result<bool> DesktopDeviceFileAccess::isWritableFile(const FilePath &filePath) const
{
    const QFileInfo fi(filePath.path());
    return fi.exists() && fi.isFile() && fi.isWritable();
}

Result<bool> DesktopDeviceFileAccess::isReadableDirectory(const FilePath &filePath) const
{
    const QFileInfo fi(filePath.path());
    return fi.exists() && fi.isDir() && fi.isReadable();
}

Result<bool> DesktopDeviceFileAccess::isWritableDirectory(const FilePath &filePath) const
{
    const QFileInfo fi(filePath.path());
    return fi.exists() && fi.isDir() && fi.isWritable();
}

Result<bool> DesktopDeviceFileAccess::isFile(const FilePath &filePath) const
{
    const QFileInfo fi(filePath.path());
    return fi.isFile();
}

Result<bool> DesktopDeviceFileAccess::isDirectory(const FilePath &filePath) const
{
    const QFileInfo fi(filePath.path());
    return fi.isDir();
}

Result<bool> DesktopDeviceFileAccess::isSymLink(const FilePath &filePath) const
{
    const QFileInfo fi(filePath.path());
    return fi.isSymLink();
}

Result<bool> DesktopDeviceFileAccess::hasHardLinks(const FilePath &filePath) const
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


Result<> DesktopDeviceFileAccess::ensureExistingFile(const FilePath &filePath) const
{
    QFile f(filePath.path());
    if (f.exists())
        return ResultOk;
    if (!f.open(QFile::WriteOnly))
        return ResultError(f.errorString());
    f.close();
    return f.exists() ? ResultOk : ResultError(
        Tr::tr("Could not create file \"%1\".").arg(filePath.toUserOutput()));
}

Result<> DesktopDeviceFileAccess::createDirectory(const FilePath &filePath) const
{
    QDir dir(filePath.path());
    return dir.mkpath(dir.absolutePath()) ? ResultOk : ResultError(
        Tr::tr("Could not create directory \"%1\".").arg(filePath.toUserOutput()));
}

Result<bool> DesktopDeviceFileAccess::exists(const FilePath &filePath) const
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

static Result<> createSymLinkGeneric(const FilePath &filePath, const FilePath &symLink)
{
    QFile srcFile(filePath.path());
    if (srcFile.link(symLink.path()))
        return ResultOk;
    return ResultError(srcFile.errorString());
}

static Result<> createSymLinkWindows(const FilePath &filePath, const FilePath &symLink)
{
#ifdef Q_OS_WIN
    const QString nativeFilePath = filePath.nativePath();
    const QString nativeSymLink = symLink.nativePath();
    DWORD flags = SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE;
    if (filePath.isDir())
        flags |= SYMBOLIC_LINK_FLAG_DIRECTORY;
    if (!CreateSymbolicLink(reinterpret_cast<const wchar_t*>(nativeSymLink.utf16()),
                            reinterpret_cast<const wchar_t*>(nativeFilePath.utf16()), flags)) {
        return ResultError(qt_error_string(GetLastError()));
    }
    return ResultOk;
#else
    Q_UNUSED(filePath)
    Q_UNUSED(symLink)
    QTC_CHECK(false);
    return ResultError(
        Tr::tr("createSymlinkWindows() called unexpectedly on a non-Windows platform."));
#endif
}

Result<> DesktopDeviceFileAccess::createSymLink(
    const FilePath &filePath, const FilePath &symLink) const
{
    const Result<> res = HostOsInfo::isWindowsHost()
                             ? createSymLinkWindows(filePath, symLink)
                             : createSymLinkGeneric(filePath, symLink);
    if (res)
        return res;
    return ResultError(
        Tr::tr("Failed to create symbolic link to \"%1\" at \"%2\": %3")
            .arg(filePath.toUserOutput(), symLink.toUserOutput(), res.error()));
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

Result<FilePathInfo> DesktopDeviceFileAccess::filePathInfo(const FilePath &filePath) const
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

Result<FilePath> DesktopDeviceFileAccess::symLinkTarget(const FilePath &filePath) const
{
    const QFileInfo info(filePath.path());
    if (!info.isSymLink())
        return {};
    return FilePath::fromString(info.symLinkTarget());
}

Result<> DesktopDeviceFileAccess::iterateDirectory(
    const FilePath &filePath,
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
            return ResultOk;
    }
    return ResultOk;
}

Result<Environment> DesktopDeviceFileAccess::deviceEnvironment() const
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

Result<FilePath> DesktopDeviceFileAccess::createTempDir(const FilePath &filePath)
{
    QTemporaryDir dir(filePath.path());
    dir.setAutoRemove(false);
    if (!dir.isValid()) {
        return ResultError(
            Tr::tr("Could not create temporary directory in \"%1\" (%2).")
                .arg(filePath.toUserOutput())
                .arg(dir.errorString()));
    }
    return filePath.withNewPath(dir.path());
}

std::vector<Result<std::unique_ptr<FilePathWatcher>>> DesktopDeviceFileAccess::watch(
    const FilePaths &paths) const
{
    return DesktopFilePathWatcher::watch(paths);
}

TextEncoding DesktopDeviceFileAccess::processStdOutEncoding(const FilePath &executable) const
{
    Q_UNUSED(executable)
    return TextEncoding::encodingForLocale();
}

TextEncoding DesktopDeviceFileAccess::processStdErrEncoding(const FilePath &executable) const
{
    Q_UNUSED(executable)
    return TextEncoding::encodingForLocale();
}

static bool localFileSystemSupportsAtomicSaveFile(const FilePath &path)
{
    QTC_ASSERT(path.isLocal(), return true);
#ifdef Q_OS_WIN
    const HANDLE handle = CreateFile(
        (wchar_t *) path.nativePath().utf16(),
        0,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        NULL);
    if (handle != INVALID_HANDLE_VALUE) {
        FILESYSTEM_STATISTICS stats;
        DWORD bytesReturned;
        bool success = DeviceIoControl(
            handle,
            FSCTL_FILESYSTEM_GET_STATISTICS,
            NULL,
            0,
            &stats,
            sizeof(stats),
            &bytesReturned,
            NULL);
        CloseHandle(handle);
        if (success || GetLastError() == ERROR_MORE_DATA) {
            return stats.FileSystemType != FILESYSTEM_STATISTICS_TYPE_FAT
                   && stats.FileSystemType != FILESYSTEM_STATISTICS_TYPE_EXFAT;
        }
    }
#else
    Q_UNUSED(path)
#endif
    return true;
}

bool DesktopDeviceFileAccess::supportsAtomicSaveFile(const FilePath &filePath) const
{
    if (!localFileSystemSupportsAtomicSaveFile(filePath))
        return false;
    return DeviceFileAccess::supportsAtomicSaveFile(filePath);
}

Result<QDateTime> DesktopDeviceFileAccess::lastModified(const FilePath &filePath) const
{
    return QFileInfo(filePath.path()).lastModified();
}

Result<QFile::Permissions> DesktopDeviceFileAccess::permissions(const FilePath &filePath) const
{
    return QFileInfo(filePath.path()).permissions();
}

Result<> DesktopDeviceFileAccess::setPermissions(
    const FilePath &filePath, QFile::Permissions permissions) const
{
    if (QFile(filePath.path()).setPermissions(permissions))
        return ResultOk;
    return ResultError(
        Tr::tr("Could not change permissions for \"%1\".").arg(filePath.toUserOutput()));
}

Result<qint64> DesktopDeviceFileAccess::fileSize(const FilePath &filePath) const
{
    return QFileInfo(filePath.path()).size();
}

Result<qint64> DesktopDeviceFileAccess::bytesAvailable(const FilePath &filePath) const
{
    return QStorageInfo(filePath.path()).bytesAvailable();
}

Result<QString> DesktopDeviceFileAccess::owner(const FilePath &filePath) const
{
    return QFileInfo(filePath.path()).owner();
}

Result<uint> DesktopDeviceFileAccess::ownerId(const FilePath &filePath) const
{
    return QFileInfo(filePath.path()).ownerId();
}

Result<QString> DesktopDeviceFileAccess::group(const FilePath &filePath) const
{
    return QFileInfo(filePath.path()).group();
}

Result<uint> DesktopDeviceFileAccess::groupId(const FilePath &filePath) const
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

Result<QByteArray> DesktopDeviceFileAccess::fileId(const FilePath &filePath) const
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

Result<bool> UnixDeviceFileAccess::runInShellSuccess(const CommandLine &cmdLine,
                                                     const QByteArray &stdInData) const
{
    const Result<RunResult> res = runInShellImpl(cmdLine, stdInData);
    if (!res)
        return ResultError(res.error());
    return res->exitCode == 0;
}

Result<QByteArray> UnixDeviceFileAccess::runInShell(
    const CommandLine &cmdLine, const QByteArray &stdInData) const
{
    const Result<RunResult> res = runInShellImpl(cmdLine, stdInData);
    if (!res)
        return ResultError(res.error());
    if (res->exitCode != 0) {
        return ResultError(Tr::tr("Command \"%1\" failed: %2")
            .arg(cmdLine.toUserOutput(), QString::fromUtf8(res->stdErr)));
    }
    return res->stdOut;
}

Result<bool> UnixDeviceFileAccess::isExecutableFile(const FilePath &filePath) const
{
    const QString path = filePath.path();
    return runInShellSuccess({"test", {"-x", path}, OsType::OsTypeLinux});
}

Result<bool> UnixDeviceFileAccess::isReadableFile(const FilePath &filePath) const
{
    const QString path = filePath.path();
    return runInShellSuccess({"test", {"-r", path, "-a", "-f", path}, OsType::OsTypeLinux});
}

Result<bool> UnixDeviceFileAccess::isWritableFile(const FilePath &filePath) const
{
    const QString path = filePath.path();
    return runInShellSuccess({"test", {"-w", path, "-a", "-f", path}, OsType::OsTypeLinux});
}

Result<bool> UnixDeviceFileAccess::isReadableDirectory(const FilePath &filePath) const
{
    const QString path = filePath.path();
    return runInShellSuccess({"test", {"-r", path, "-a", "-d", path}, OsType::OsTypeLinux});
}

Result<bool> UnixDeviceFileAccess::isWritableDirectory(const FilePath &filePath) const
{
    const QString path = filePath.path();
    return runInShellSuccess({"test", {"-w", path, "-a", "-d", path}, OsType::OsTypeLinux});
}

Result<bool> UnixDeviceFileAccess::isFile(const FilePath &filePath) const
{
    const QString path = filePath.path();
    return runInShellSuccess({"test", {"-f", path}, OsType::OsTypeLinux});
}

Result<bool> UnixDeviceFileAccess::isDirectory(const FilePath &filePath) const
{
    if (filePath.isRootPath())
        return true;

    const QString path = filePath.path();
    return runInShellSuccess({"test", {"-d", path}, OsType::OsTypeLinux});
}

Result<bool> UnixDeviceFileAccess::isSymLink(const FilePath &filePath) const
{
    const QString path = filePath.path();
    return runInShellSuccess({"test", {"-h", path}, OsType::OsTypeLinux});
}

Result<bool> UnixDeviceFileAccess::hasHardLinks(const FilePath &filePath) const
{
    const QStringList args = statArgs(filePath, "%h", "%l");
    const Result<QByteArray> res = runInShell({"stat", args, OsType::OsTypeLinux});
    if (!res)
        return ResultError(res.error());
    return res->toLongLong() > 1;
}

Result<> UnixDeviceFileAccess::ensureExistingFile(const FilePath &filePath) const
{
    const QString path = filePath.path();
    const Result<bool> res = runInShellSuccess({"touch", {path}, OsType::OsTypeLinux});
    if (!res)
        return ResultError(res.error());
    if (!*res)
        return ResultError(Tr::tr("Could not create file \"%1\".").arg(filePath.toUserOutput()));
    return ResultOk;
}

Result<> UnixDeviceFileAccess::createDirectory(const FilePath &filePath) const
{
    const QString path = filePath.path();
    const Result<bool> res = runInShellSuccess({"mkdir", {"-p", path}, OsType::OsTypeLinux});
    if (!res)
        return ResultError(res.error());
    if (!*res)
        return ResultError(
            Tr::tr("Could not create directory \"%1\".").arg(filePath.toUserOutput()));
    return ResultOk;
}

Result<bool> UnixDeviceFileAccess::exists(const FilePath &filePath) const
{
    const QString path = filePath.path();
    return runInShellSuccess({"test", {"-e", path}, OsType::OsTypeLinux});
}

Result<> UnixDeviceFileAccess::removeFile(const FilePath &filePath) const
{
    const Result<QByteArray> res = runInShell({"rm", {filePath.path()}, OsType::OsTypeLinux});
    if (!res)
        return ResultError(res.error());
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

    const Result<RunResult> res = runInShellImpl({"rm", {"-rf", "--", path}, OsType::OsTypeLinux});
    if (!res)
        return ResultError(res.error());

    if (res->exitCode != 0)
        return ResultError(QString::fromUtf8(res->stdErr));

    return ResultOk;
}

Result<> UnixDeviceFileAccess::copyFile(const FilePath &filePath, const FilePath &target) const
{
    const Result<RunResult> res = runInShellImpl(
        {"cp", {filePath.path(), target.path()}, OsType::OsTypeLinux});
    if (!res)
        return ResultError(res.error());

    if (res->exitCode != 0) {
        return ResultError(Tr::tr("Failed to copy file \"%1\" to \"%2\": %3")
                                   .arg(filePath.toUserOutput(),
                                        target.toUserOutput(),
                                        QString::fromUtf8(res->stdErr)));
    }
    return ResultOk;
}

Result<> UnixDeviceFileAccess::createSymLink(const FilePath &filePath, const FilePath &symLink) const
{
    const Result<RunResult> res = runInShellImpl(
        {"ln", {"-s", filePath.path(), symLink.path()}, OsType::OsTypeLinux});
    if (!res)
        return ResultError(res.error());

    if (res->exitCode != 0) {
        return ResultError(Tr::tr("Failed to create symbolic link for file \"%1\" at \"%2\": %3")
                               .arg(filePath.toUserOutput(),
                                    symLink.toUserOutput(),
                                    QString::fromUtf8(res->stdErr)));
    }
    return ResultOk;
}

Result<> UnixDeviceFileAccess::renameFile(const FilePath &filePath, const FilePath &target) const
{
    const Result<RunResult> res = runInShellImpl({"mv", {filePath.path(), target.path()}, OsType::OsTypeLinux});
    if (!res)
        return ResultError(res.error());
    if (res->exitCode != 0) {
        return ResultError(Tr::tr("Failed to rename file \"%1\" to \"%2\": %3")
                                   .arg(filePath.toUserOutput(),
                                        target.toUserOutput(),
                                        QString::fromUtf8(res->stdErr)));
    }
    return ResultOk;
}

Result<FilePath> UnixDeviceFileAccess::symLinkTarget(const FilePath &filePath) const
{
    const Result<RunResult> res = runInShellImpl(
        {"readlink", {"-n", "-e", filePath.path()}, OsType::OsTypeLinux});
    if (!res)
        return ResultError(res.error());
    const QString out = QString::fromUtf8(res->stdOut);
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
    p.runBlocking(60min); // Run "forever"
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
    Result<QByteArray> res = runInShell({"dd", args, OsType::OsTypeLinux}, data);

    if (!res)
        return ResultError(res.error());
    return data.size();
}

Result<FilePath> UnixDeviceFileAccess::createTempPath(const FilePath &filePath, bool createDir)
{
    if (!m_hasMkTemp.has_value())
        m_hasMkTemp = runInShellSuccess({"which", {"mktemp"}, OsType::OsTypeLinux}).has_value();

    QString tmplate = filePath.path();
    // Some mktemp implementations require a suffix of XXXXXX.
    // They will only accept the template if at least the last 6 characters are X.
    if (!tmplate.endsWith("XXXXXX"))
        tmplate += ".XXXXXX";

    if (m_hasMkTemp) {
        QStringList args;
        if (createDir)
            args << "-d";
        args << tmplate;
        const Result<QByteArray> res = runInShell({"mktemp", args, OsType::OsTypeLinux});
        if (!res)
            return ResultError(res.error());

        return filePath.withNewPath(QString::fromUtf8(res->trimmed()));
    }

    // Manually create a temporary/unique file or directory.
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
            QString msg = createDir
                              ? Tr::tr("Failed creating temporary directory \"%1\" (too many tries).")
                              : Tr::tr("Failed creating temporary files \"%1\" (too many tries).");

            return ResultError(msg.arg(filePath.toUserOutput()));
        }
    } while (newPath.exists());

    Result<qint64> createResult;
    if (createDir)
        createResult = newPath.createDir();
    else
        createResult = newPath.writeFileContents({});

    if (!createResult)
        return ResultError(createResult.error());

    return newPath;
}

Result<FilePath> UnixDeviceFileAccess::createTempDir(const FilePath &filePath)
{
    return createTempPath(filePath, true);
}

Result<FilePath> UnixDeviceFileAccess::createTempFile(const FilePath &filePath)
{
    return createTempPath(filePath, false);
}

Result<QDateTime> UnixDeviceFileAccess::lastModified(const FilePath &filePath) const
{
    const Result<RunResult> res = runInShellImpl(
        {"stat", {"-L", "-c", "%Y", filePath.path()}, OsType::OsTypeLinux});
    if (!res)
        return ResultError(res.error());
    qint64 secs = res->stdOut.toLongLong();
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

Result<QFile::Permissions> UnixDeviceFileAccess::permissions(const FilePath &filePath) const
{
    QStringList args = statArgs(filePath, "%a", "%p");

    const Result<RunResult> res = runInShellImpl({"stat", args, OsType::OsTypeLinux});
    if (!res)
        return ResultError(res.error());
    const uint bits = res->stdOut.toUInt(nullptr, 8);
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

Result<> UnixDeviceFileAccess::setPermissions(const FilePath &filePath, QFile::Permissions perms) const
{
    const int flags = toUnixChmod(perms);
    const Result<bool> res = runInShellSuccess(
        {"chmod", {"0" + QString::number(flags, 8), filePath.path()}, OsType::OsTypeLinux});
    if (!res)
        return ResultError(res.error());
    return ResultOk;
}

Result<qint64> UnixDeviceFileAccess::fileSize(const FilePath &filePath) const
{
    const QStringList args = statArgs(filePath, "%s", "%z");
    const Result<QByteArray> res = runInShell({"stat", args, OsType::OsTypeLinux});
    if (!res)
        return ResultError(res.error());
    return res->toLongLong();
}

Result<QString> UnixDeviceFileAccess::owner(const FilePath &filePath) const
{
    const QStringList args = statArgs(filePath, "%U", "%U");
    const Result<QByteArray> res = runInShell({"stat", args, OsType::OsTypeLinux});
    if (!res)
        return ResultError(res.error());
    return QString::fromUtf8(*res);
}

Result<uint> UnixDeviceFileAccess::ownerId(const FilePath &filePath) const
{
    const QStringList args = statArgs(filePath, "%u", "%u");
    const Result<QByteArray> res = runInShell({"stat", args, OsType::OsTypeLinux});
    if (!res)
        return ResultError(res.error());
    return res->toUInt();
}

Result<QString> UnixDeviceFileAccess::group(const FilePath &filePath) const
{
    const QStringList args = statArgs(filePath, "%G", "%G");
    const Result<QByteArray> res = runInShell({"stat", args, OsType::OsTypeLinux});
    if (!res)
        return ResultError(res.error());
    return QString::fromUtf8(*res);
}

Result<uint> UnixDeviceFileAccess::groupId(const FilePath &filePath) const
{
    const QStringList args = statArgs(filePath, "%g", "%g");
    const Result<QByteArray> res = runInShell({"stat", args, OsType::OsTypeLinux});
    if (!res)
        return ResultError(res.error());
    return res->toUInt();
}

Result<qint64> UnixDeviceFileAccess::bytesAvailable(const FilePath &filePath) const
{
    const Result<QByteArray> res = runInShell({"df", {"-k", filePath.path()}, OsType::OsTypeLinux});
    if (!res)
        return ResultError(res.error());
    return FileUtils::bytesAvailableFromDFOutput(*res);
}

Result<QByteArray> UnixDeviceFileAccess::fileId(const FilePath &filePath) const
{
    const QStringList args = statArgs(filePath, "%D:%i", "%d:%i");
    const Result<QByteArray> res = runInShell({"stat", args, OsType::OsTypeLinux});
    if (!res)
        return ResultError(res.error());
    return *res;
}

Result<FilePathInfo> UnixDeviceFileAccess::filePathInfo(const FilePath &filePath) const
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

    const Result<QByteArray> res = runInShell({"stat", args, OsType::OsTypeLinux});
    if (!res)
        return ResultError(res.error());

    return FileUtils::filePathInfoFromTriple(QString::fromLatin1(*res),
                                             filePath.osType() == OsTypeMac ? 8 : 16);
}

// returns whether 'find' could be used.
Result<> UnixDeviceFileAccess::iterateWithFind(
    const FilePath &filePath,
    const FileFilter &filter,
    const FilePath::IterateDirCallback &callBack) const
{
    QTC_CHECK(filePath.isAbsolutePath());

    CommandLine cmdLine{"find", filter.asFindArguments(filePath.path()), OsType::OsTypeLinux};

    // TODO: Using stat -L will always return the link target, not the link itself.
    // We may wan't to add the information that it is a link at some point.

    const QString statFormat = filePath.osType() == OsTypeMac ? QLatin1String("-f \"%p %m %z\"")
                                                              : QLatin1String("-c \"%f %Y %s\"");
    // Some versions of 'find' fail to substitute {} if it doesn't stand for itself,
    // so we keep some spacing around it.
    static const QString startQuoteIn = "\\\" ";
    static const QString endQuoteIn = " \\\"";
    static const QString startQuoteOut = "\" ";
    static const QString endQuoteOut = " \"";
    static const int quoteOutSize = 2;
    const bool includeStats = callBack.index() == 1;
    if (includeStats)
        cmdLine.addArgs(
            QString(R"(-exec echo -n %1{}%2" " \; -exec stat -L %3 "{}" \;)")
                .arg(startQuoteIn, endQuoteIn, statFormat),
            CommandLine::Raw);

    const Result<RunResult> res = runInShellImpl(cmdLine);
    if (!res)
        return ResultError(res.error());
    const QString out = QString::fromUtf8(res->stdOut);
    if (res->exitCode != 0) {
        // Find returns non-zero exit code for any error it encounters, even if it finds some files.
        const QString expectedPrefix = (includeStats ? startQuoteOut : QString()) + filePath.path();
        if (!out.startsWith(expectedPrefix)) {
            if (!filePath.exists()) // File does not exist, so no files to find.
                return ResultOk;

            // If the output does not start with the path we are searching in, find has failed.
            // Possibly due to unknown options.
            return ResultError("Find failed");
        }
    }

    QStringList entries = out.split("\n", Qt::SkipEmptyParts);
    if (entries.isEmpty())
        return ResultOk;

    const int modeBase = filePath.osType() == OsTypeMac ? 8 : 16;

    const auto toFilePath = [&filePath, &callBack, modeBase](const QString &entry) {
        if (callBack.index() == 0)
            return std::get<0>(callBack)(filePath.withNewPath(entry));

        const QString fileName
            = entry.mid(quoteOutSize, entry.lastIndexOf(endQuoteOut) - quoteOutSize);
        const QString infos = entry.mid(fileName.size() + quoteOutSize * 2 + 1);

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

    return ResultOk;
}

Result<> UnixDeviceFileAccess::findUsingLs(
    const QString &current, const FileFilter &filter, QStringList *found, const QString &start) const
{
    const Result<RunResult> res = runInShellImpl(
        {"ls", {"-1", "-a", "-p", "--", current}, OsType::OsTypeLinux});
    if (!res)
        return ResultError(res.error());
    // 'ls' returns non-zero exit code for any error it encounters, even if it finds some files,
    // so ignore the exit code
    const QStringList entries = QString::fromUtf8(res->stdOut).split('\n', Qt::SkipEmptyParts);
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
    return ResultOk;
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

Result<> UnixDeviceFileAccess::iterateDirectory(
    const FilePath &filePath,
    const FilePath::IterateDirCallback &callBack,
    const FileFilter &filter) const
{
    // We try to use 'find' first, because that can filter better directly.
    // Unfortunately, it's not installed on all devices by default.
    if (m_tryUseFind) {
        const Result<> res = iterateWithFind(filePath, filter, callBack);
        if (res)
            return ResultOk;
        // Remember the failure for the next time and use the 'ls' fallback below.
        m_tryUseFind = false;
    }

    // if we do not have find - use ls as fallback
    QStringList entries;
    const Result<> res = findUsingLs(filePath.path(), filter, &entries, {});
    if (!res)
        return ResultError(res.error());

    iterateLsOutput(filePath, entries, filter, callBack);
    return ResultOk;
}

Result<Environment> UnixDeviceFileAccess::deviceEnvironment() const
{
    const Result<QByteArray> res = runInShell({"env", {}, OsType::OsTypeLinux});
    if (!res)
        return ResultError(res.error());
    const QString out = QString::fromUtf8(*res);
    return Environment(out.split('\n', Qt::SkipEmptyParts), OsTypeLinux);
}

} // namespace Utils

#include "devicefileaccess.moc"
