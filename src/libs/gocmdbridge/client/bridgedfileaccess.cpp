// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bridgedfileaccess.h"

#include "cmdbridgeclient.h"
#include "cmdbridgetr.h"

#include <QElapsedTimer>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(faLog, "qtc.cmdbridge.fileaccess", QtWarningMsg);

using namespace Utils;

namespace CmdBridge {

FileAccess::~FileAccess() = default;

static ResultError exceptionError(
    const QString &context, const FilePath &filePath, const std::exception &e)
{
    QTC_CHECK(context.contains("%1"));
    QString message = context;
    if (!filePath.isEmpty())
        message = message.arg(filePath.toUserOutput()) + ": ";
    message += QString::fromLocal8Bit(e.what());
    qCWarning(faLog) << message;
    return ResultError(message);
}

Result<QString> run(const CommandLine &cmdLine, const QByteArray &inputData = {})
{
    Process p;
    p.setCommand(cmdLine);
    p.setProcessChannelMode(QProcess::MergedChannels);
    if (!inputData.isEmpty())
        p.setWriteData(inputData);
    p.runBlocking();
    if (p.exitCode() != 0) {
        return ResultError(Tr::tr("Command %1 failed with exit code %2: %3")
                                   .arg(cmdLine.toUserOutput())
                                   .arg(p.exitCode())
                                   .arg(p.readAllStandardOutput().left(500)));
    }
    return p.readAllStandardOutput().trimmed();
}

Result<> FileAccess::init(
    const FilePath &pathToBridge, const Environment &environment, bool deleteOnExit)
{
    m_environment = environment;
    m_client = std::make_unique<Client>(pathToBridge, environment);

    auto startResult = m_client->start(deleteOnExit);
    if (!startResult)
        return ResultError(QString("Could not start cmdbridge: %1").arg(startResult.error()));

    return ResultOk;
}

Result<> FileAccess::deployAndInit(
    const FilePath &libExecPath,
    const FilePath &remoteRootPath,
    const Environment &environment)
{
    if (remoteRootPath.isEmpty())
        return ResultError(Tr::tr("Remote root path is empty"));

    if (!remoteRootPath.isAbsolutePath())
        return ResultError(Tr::tr("Remote root path is not absolute"));

    const auto whichDD = run({remoteRootPath.withNewPath("which"), {"dd"}});

    if (!whichDD) // TODO: Support Windows?
        return ResultError(Tr::tr("Could not find dd on remote host: %1").arg(whichDD.error()));

    QElapsedTimer timer;
    timer.start();
    auto deco = [&] {
        return QString("%1  (%2 ms)").arg(remoteRootPath.host()).arg(timer.elapsed());
    };

    qCDebug(faLog) << deco() << "Found dd on remote host:" << *whichDD;

    const Result<QString> unameOs = run({remoteRootPath.withNewPath("uname"), {"-s"}});
    if (!unameOs) {
        return ResultError(
            QString("Could not determine OS on remote host: %1").arg(unameOs.error()));
    }

    const Result<OsType> osType = osTypeFromString(*unameOs);
    if (!osType)
        return ResultError(osType.error());

    qCDebug(faLog) << deco() << "Remote host OS:" << *unameOs;

    const Result<QString> unameArch = run({remoteRootPath.withNewPath("uname"), {"-m"}});
    if (!unameArch) {
        return ResultError(
            QString("Could not determine architecture on remote host: %1").arg(unameArch.error()));
    }

    const Result<OsArch> osArch = osArchFromString(*unameArch);
    if (!osArch)
        return ResultError(osArch.error());

    qCDebug(faLog) << deco() << "Remote host architecture:" << *unameArch;

    const Result<FilePath> cmdBridgePath = Client::getCmdBridgePath(*osType, *osArch, libExecPath);

    if (!cmdBridgePath) {
        return ResultError(
            QString("Could not determine compatible cmdbridge for remote host: %1")
                .arg(cmdBridgePath.error()));
    }

    qCDebug(faLog) << deco() << "Using cmdbridge at:" << *cmdBridgePath;

    if (remoteRootPath.isLocal())
        return init(*cmdBridgePath, environment, false);

    const auto cmdBridgeFileData = cmdBridgePath->fileContents();

    if (!cmdBridgeFileData) {
        return ResultError(
            QString("Could not read cmdbridge file: %1").arg(cmdBridgeFileData.error()));
    }

    const auto tmpFile = run(
        {remoteRootPath.withNewPath("mktemp"), {"-t", "cmdbridge.XXXXXXXXXX"}});

    if (!tmpFile) {
        return ResultError(
            QString("Could not create temporary file: %1").arg(tmpFile.error()));
    }

    qCDebug(faLog) << deco() << "Using temporary file:" << *tmpFile;
    const auto dd = run({remoteRootPath.withNewPath("dd"), {"of=" + *tmpFile}},
                        *cmdBridgeFileData);

    qCDebug(faLog) << deco() << "dd run";

    const auto makeExecutable = run({remoteRootPath.withNewPath("chmod"), {"+x", *tmpFile}});

    if (!makeExecutable) {
        return ResultError(
            QString("Could not make temporary file executable: %1").arg(makeExecutable.error()));
    }

    return init(remoteRootPath.withNewPath(*tmpFile), environment, true);
}

Result<bool> FileAccess::isExecutableFile(const FilePath &filePath) const
{
    try {
        auto f = m_client->is(filePath.nativePath(), CmdBridge::Client::Is::ExecutableFile);
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        return f->result();
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error checking executable file:" << e.what();
        return false;
    }
}

Result<bool> FileAccess::isReadableFile(const FilePath &filePath) const
{
    try {
        auto f = m_client->is(filePath.nativePath(), CmdBridge::Client::Is::ReadableFile);
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        return f->result();
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error checking readable file:" << e.what();
        return false;
    }
}

Result<bool> FileAccess::isWritableFile(const FilePath &filePath) const
{
    try {
        Result<QFuture<bool>> f = m_client->is(filePath.nativePath(),
                                                     CmdBridge::Client::Is::WritableFile);
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        return f->result();
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error checking writable file:" << e.what();
        return false;
    }
}

Result<bool> FileAccess::isReadableDirectory(const FilePath &filePath) const
{
    try {
        auto f = m_client->is(filePath.nativePath(), CmdBridge::Client::Is::ReadableDir);
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        return f->result();
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error checking readable directory:" << e.what();
        return false;
    }
}

Result<bool> FileAccess::isWritableDirectory(const FilePath &filePath) const
{
    try {
        auto f = m_client->is(filePath.nativePath(), CmdBridge::Client::Is::WritableDir);
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        return f->result();
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error checking writable directory:" << e.what();
        return false;
    }
}

Result<bool> FileAccess::isFile(const FilePath &filePath) const
{
    try {
        auto f = m_client->is(filePath.nativePath(), CmdBridge::Client::Is::File);
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        return f->result();
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error checking file:" << e.what();
        return false;
    }
}

Result<bool> FileAccess::isDirectory(const FilePath &filePath) const
{
    try {
        auto f = m_client->is(filePath.nativePath(), CmdBridge::Client::Is::Dir);
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        return f->result();
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error checking directory:" << e.what();
        return false;
    }
}

Result<bool> FileAccess::isSymLink(const FilePath &filePath) const
{
    try {
        auto f = m_client->is(filePath.nativePath(), CmdBridge::Client::Is::Symlink);
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        return f->result();
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error checking symlink:" << e.what();
        return false;
    }
}

Result<bool> FileAccess::exists(const FilePath &filePath) const
{
    try {
        auto f = m_client->is(filePath.nativePath(), CmdBridge::Client::Is::Exists);
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        return f->result();
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error checking existence:" << e.what();
        return false;
    }
}

Result<bool> FileAccess::hasHardLinks(const FilePath &filePath) const
{
    try {
        auto f = m_client->stat(filePath.nativePath());
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        return f->result().numHardLinks > 1;
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error checking hard links:" << e.what();
        return false;
    }
}

FilePathInfo::FileFlags fileInfoFlagsfromStatMode(uint mode)
{
    // Copied from stat.h
    enum st_mode {
        IFMT = 00170000,
        IFSOCK = 0140000,
        IFLNK = 0120000,
        IFREG = 0100000,
        IFBLK = 0060000,
        IFDIR = 0040000,
        IFCHR = 0020000,
        IFIFO = 0010000,
        ISUID = 0004000,
        ISGID = 0002000,
        ISVTX = 0001000,
        IRWXU = 00700,
        IRUSR = 00400,
        IWUSR = 00200,
        IXUSR = 00100,
        IRWXG = 00070,
        IRGRP = 00040,
        IWGRP = 00020,
        IXGRP = 00010,
        IRWXO = 00007,
        IROTH = 00004,
        IWOTH = 00002,
        IXOTH = 00001,
    };

    FilePathInfo::FileFlags result;

    if (mode & IRUSR)
        result |= FilePathInfo::ReadOwnerPerm;
    if (mode & IWUSR)
        result |= FilePathInfo::WriteOwnerPerm;
    if (mode & IXUSR)
        result |= FilePathInfo::ExeOwnerPerm;
    if (mode & IRGRP)
        result |= FilePathInfo::ReadGroupPerm;
    if (mode & IWGRP)
        result |= FilePathInfo::WriteGroupPerm;
    if (mode & IXGRP)
        result |= FilePathInfo::ExeGroupPerm;
    if (mode & IROTH)
        result |= FilePathInfo::ReadOtherPerm;
    if (mode & IWOTH)
        result |= FilePathInfo::WriteOtherPerm;
    if (mode & IXOTH)
        result |= FilePathInfo::ExeOtherPerm;

    // Copied from fs.go
    enum fsMode {
        Dir = 2147483648,
        Append = 1073741824,
        Exclusive = 536870912,
        Temporary = 268435456,
        Symlink = 134217728,
        Device = 67108864,
        NamedPipe = 33554432,
        Socket = 16777216,
        Setuid = 8388608,
        Setgid = 4194304,
        CharDevice = 2097152,
        Sticky = 1048576,
        Irregular = 524288,

        TypeMask = Dir | Symlink | NamedPipe | Socket | Device | CharDevice | Irregular
    };

    if ((mode & fsMode::TypeMask) == 0)
        result |= FilePathInfo::FileType;
    if (mode & fsMode::Symlink)
        result |= FilePathInfo::LinkType;
    if (mode & fsMode::Dir)
        result |= FilePathInfo::DirectoryType;
    if (mode & fsMode::Device)
        result |= FilePathInfo::LocalDiskFlag;

    if (result != 0) // There is no Exist flag, but if anything was set before, it must exist.
        result |= FilePathInfo::ExistsFlag;

    return result;
}

Result<qint64> FileAccess::bytesAvailable(const FilePath &filePath) const
{
    try {
        Result<QFuture<quint64>> f = m_client->freeSpace(filePath.nativePath());
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        return f->result();
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error getting free space:" << e.what();
        return -1;
    }
}

Result<QByteArray> FileAccess::fileId(const FilePath &filePath) const
{
    try {
        Result<QFuture<QString>> f = m_client->fileId(filePath.nativePath());
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        return f->result().toUtf8();
    } catch (const std::exception &e) {
        return exceptionError("Error getting file ID of %1", filePath, e);
    }
}

Result<FilePathInfo> FileAccess::filePathInfo(const FilePath &filePath) const
{
    if (filePath.path() == "/") { // TODO: Add FilePath::isRoot()
        const FilePathInfo
            r{4096,
              FilePathInfo::FileFlags(
                  FilePathInfo::ReadOwnerPerm | FilePathInfo::WriteOwnerPerm
                  | FilePathInfo::ExeOwnerPerm | FilePathInfo::ReadGroupPerm
                  | FilePathInfo::ExeGroupPerm | FilePathInfo::ReadOtherPerm
                  | FilePathInfo::ExeOtherPerm | FilePathInfo::DirectoryType
                  | FilePathInfo::LocalDiskFlag | FilePathInfo::ExistsFlag),
              QDateTime::currentDateTime()};

        return r;
    }

    try {
        Result<QFuture<Client::Stat>> f = m_client->stat(filePath.nativePath());
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        Client::Stat stat = f->result();
        return FilePathInfo{stat.size,
                fileInfoFlagsfromStatMode(stat.mode) | FilePathInfo::FileFlags(stat.usermode),
                stat.modTime};
    } catch (const std::exception &e) {
        QString message = "Error getting file path info of " + filePath.toUserOutput()
                          + QString::fromLocal8Bit(e.what());
        qDebug(faLog) << message;
        return ResultError(message);
    }
}

Result<FilePath> FileAccess::symLinkTarget(const FilePath &filePath) const
{
    try {
        Result<QFuture<QString>> f = m_client->readlink(filePath.nativePath());
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        return filePath.parentDir().resolvePath(filePath.withNewPath(f->result()).path());
    } catch (const std::system_error &e) {
        if (e.code().value() == ENOENT)
            return exceptionError("No such file: %1", filePath, e);
        if (e.code().value() == EINVAL)
            return exceptionError("Path %1 is not a symlink", filePath, e);
        return exceptionError("Error getting symlink target for %1", filePath, e);
    } catch (const std::exception &e) {
        return exceptionError("Error getting symlink target for %1", filePath, e);
    }
}

Result<QDateTime> FileAccess::lastModified(const FilePath &filePath) const
{
    const Result<FilePathInfo> res  = filePathInfo(filePath);
    if (!res)
        return ResultError(res.error());
    return res->lastModified;
}

Result<QFile::Permissions> FileAccess::permissions(const FilePath &filePath) const
{
    const Result<FilePathInfo> res  = filePathInfo(filePath);
    if (!res)
        return ResultError(res.error());
    return QFile::Permissions::fromInt(res->fileFlags & FilePathInfo::PermsMask);
}

Result<> FileAccess::setPermissions(const FilePath &filePath, QFile::Permissions perms) const
{
    try {
        Result<QFuture<void>> f = m_client->setPermissions(filePath.nativePath(), perms);
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        f->waitForFinished();
        return ResultOk;
    } catch (const std::exception &e) {
        return exceptionError("Error setting permissions for %1", filePath, e);
    }
}

Result<qint64> FileAccess::fileSize(const FilePath &filePath) const
{
    const Result<FilePathInfo> res  = filePathInfo(filePath);
    if (!res)
        return ResultError(res.error());
    return res->fileSize;
}

Result<QString> FileAccess::owner(const FilePath &filePath) const
{
    try {
        Result<QFuture<QString>> f = m_client->owner(filePath.nativePath());
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        return f->result();
    } catch (const std::exception &e) {
        return exceptionError("Error getting file owner of %1", filePath, e);
    }
}

Result<uint> FileAccess::ownerId(const FilePath &filePath) const
{
    try {
        Result<QFuture<uint>> f = m_client->ownerId(filePath.nativePath());
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        return f->result();
    } catch (const std::exception &e) {
        return exceptionError("Error getting file owner id of %1", filePath, e);
    }
}

Result<QString> FileAccess::group(const FilePath &filePath) const
{
    try {
        Result<QFuture<QString>> f = m_client->group(filePath.nativePath());
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        return f->result();
    } catch (const std::exception &e) {
        return exceptionError("Error getting file group of %1", filePath, e);
    }
}

Result<uint> FileAccess::groupId(const FilePath &filePath) const
{
    try {
        Result<QFuture<uint>> f = m_client->groupId(filePath.nativePath());
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        return f->result();
    } catch (const std::exception &e) {
        return exceptionError("Error getting file group id of %1", filePath, e);
    }
}

Result<QByteArray> FileAccess::fileContents(const FilePath &filePath,
                                            qint64 limit,
                                            qint64 offset) const
{
    try {
        Result<QFuture<QByteArray>> f = m_client->readFile(filePath.nativePath(),
                                                                 limit,
                                                                 offset);
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        f->waitForFinished();
        QByteArray data;
        for (const QByteArray &result : *f) {
            data.append(result);
        }
        return data;
    } catch (const std::exception &e) {
        return exceptionError("Error reading file %1", filePath, e);
    }
}

Result<qint64> FileAccess::writeFileContents(const FilePath &filePath,
                                                   const QByteArray &data) const
{
    try {
        Result<QFuture<qint64>> f = m_client->writeFile(filePath.nativePath(), data);
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        return f->result();
    } catch (const std::exception &e) {
        return exceptionError(Tr::tr("Error writing file: %1"), filePath, e);
    }
}

Result<> FileAccess::removeFile(const FilePath &filePath) const
{
    try {
        Result<QFuture<void>> f = m_client->removeFile(filePath.nativePath());
        if (!f)
            return ResultError(f.error());
        f->waitForFinished();
    } catch (const std::system_error &e) {
        if (e.code().value() == ENOENT)
            return ResultError(Tr::tr("File does not exist: %1").arg(filePath.toUserOutput()));
        return exceptionError(Tr::tr("Error removing file: %1"), filePath, e);
    } catch (const std::exception &e) {
        return exceptionError(Tr::tr("Error removing file: %1"), filePath, e);
    }

    return ResultOk;
}

Result<> FileAccess::removeRecursively(const FilePath &filePath) const
{
    try {
        auto f = m_client->removeRecursively(filePath.nativePath());
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        f->waitForFinished();
    } catch (const std::exception &e) {
        return exceptionError("Error removing %1 recursively", filePath, e);
    }
    return ResultOk;
}

Result<> FileAccess::ensureExistingFile(const FilePath &filePath) const
{
    try {
        auto f = m_client->ensureExistingFile(filePath.nativePath());
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        f->waitForFinished();
    } catch (const std::exception &e) {
        return exceptionError("Error ensure existing file %1", filePath, e);
    }
    return ResultOk;
}

Result<> FileAccess::createDirectory(const FilePath &filePath) const
{
    try {
        auto f = m_client->createDir(filePath.nativePath());
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        f->waitForFinished();
    } catch (const std::exception &e) {
        return exceptionError("Error creating directory %1", filePath, e);
    }
    return ResultOk;
}

Result<> FileAccess::copyFile(const FilePath &filePath, const FilePath &target) const
{
    try {
        auto f = m_client->copyFile(filePath.nativePath(), target.nativePath());
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        f->waitForFinished();
    } catch (const std::exception &e) {
        return exceptionError(QString("Error copying file from %2 to %1")
                                .arg(target.toUserOutput()), filePath, e);
    }
    return ResultOk;
}

Result<> FileAccess::createSymLink(const FilePath &filePath, const FilePath &symLink) const
{
    try {
        auto f = m_client->createSymLink(filePath.nativePath(), symLink.nativePath());
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        f->waitForFinished();
    } catch (const std::exception &e) {
        return exceptionError(QString("Error creating symlink from %2 to %1")
                                .arg(symLink.toUserOutput()), filePath, e);
    }
    return ResultOk;
}

Result<> FileAccess::renameFile(const FilePath &filePath, const FilePath &target) const
{
    try {
        Result<QFuture<void>> f
            = m_client->renameFile(filePath.nativePath(), target.nativePath());
        if (!f)
            return ResultError(f.error());
        f->waitForFinished();
        if (!f)
            return ResultError(f.error());
    } catch (const std::exception &e) {
        return exceptionError(QString("Error renaming file from %2 to %1")
                                .arg(target.toUserOutput()), filePath, e);
    }
    return ResultOk;
}

Result<std::unique_ptr<FilePathWatcher>> FileAccess::watch(const FilePath &filePath) const
{
    return m_client->watch(filePath.nativePath());
}

Result<> FileAccess::signalProcess(int pid, ControlSignal signal) const
{
    try {
        auto f = m_client->signalProcess(pid, signal);
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        f->waitForFinished();
    } catch (const std::exception &e) {
        return exceptionError(Tr::tr("Error killing process").arg(pid), {}, e);
    };

    return ResultOk;
}

Result<FilePath> FileAccess::createTemp(const FilePath &filePath, bool dir)
{
    try {
        QString path = filePath.nativePath();
        if (path.endsWith("XX")) {
            // Find first "X" from end of string "path"
            int i = path.size() - 1;
            for (; i > -1; i--)
                if (path[i] != 'X')
                    break;
            path = path.left(i + 1) + "*";
        } else {
            path += ".*";
        }

        Result<QFuture<FilePath>> f = dir ? m_client->createTempDir(path)
                                          : m_client->createTempFile(path);
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        f->waitForFinished();

        Result<FilePath> result = f->result();
        if (!result)
            return result;
        return filePath.withNewPath(result->path());
    } catch (const std::exception &e) {
        return exceptionError("Error creating temporary file for %1", filePath, e);
    }
}

Result<FilePath> FileAccess::createTempDir(const FilePath &filePath)
{
    return createTemp(filePath, true);
}

Result<FilePath> FileAccess::createTempFile(const FilePath &filePath)
{
    return createTemp(filePath, false);
}

Result<> FileAccess::iterateDirectory(
    const FilePath &filePath,
    const FilePath::IterateDirCallback &callback,
    const FileFilter &filter) const
{
    Result<QFuture<Client::FindData>> result = m_client->find(filePath.nativePath(), filter);
    QTC_ASSERT_RESULT(result, return ResultError(result.error()));

    int idx = 0;

    auto processResults = [&idx, &result, &callback, filePath]() {
        for (int i = idx; i < result->resultCount(); ++i, ++idx) {
            auto res = result->resultAt(i);
            if (!res.has_value()) {
                if (res.error().has_value())
                    qCWarning(faLog) << "Error iterating directory:" << *res.error();
                return;
            }

            const FilePath path = filePath.withNewPath(FilePath::fromUserInput(res->path).path());

            if (callback.index() == 0) {
                if (std::get<0>(callback)(path) != IterationPolicy::Continue) {
                    result->cancel();
                    return;
                }
            } else {
                FilePathInfo::FileFlags flags = fileInfoFlagsfromStatMode(res->mode);
                if (std::get<1>(callback)(path, FilePathInfo{res->size, flags, res->modTime})
                    != IterationPolicy::Continue) {
                    result->cancel();
                    return;
                }
            }
        }
    };

    QElapsedTimer t;
    t.start();

    while (!result->isFinished()) {
        if (result->isValid() && idx < result->resultCount()) {
            result->resultAt(idx);
            // Wait for the next result to become available
            processResults();
        }
    }
    processResults();

    qCDebug(faLog) << "Iterated directory" << filePath.toUserOutput() << "in" << t.elapsed() << "ms";
    return ResultOk;
}

Result<Environment> FileAccess::deviceEnvironment() const
{
    return m_environment;
}

} // namespace CmdBridge
