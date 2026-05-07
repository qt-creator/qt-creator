// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bridgedfileaccess.h"

#include "cmdbridgeclient.h"
#include "cmdbridgetr.h"

#include <utils/async.h>
#include <utils/futuresynchronizer.h>

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFutureWatcher>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLoggingCategory>
#include <QTimer>

Q_LOGGING_CATEGORY(faLog, "qtc.cmdbridge.fileaccess", QtWarningMsg);

using namespace Utils;

namespace CmdBridge {

FileAccess::FileAccess(const std::function<void()> &errorExitHandler)
    : m_errorExitHandler(errorExitHandler)
{
}

FileAccess::~FileAccess()
{
    if (m_client) {
        // we do not care about issues while shutting down
        m_client->disconnect();
        // and we don't want to block the main thread while it does either.
        if (QThread::isMainThread()) {
            futureSynchronizer()->addFuture(
                asyncRun([client = std::move(m_client)]() mutable { client.reset(); }));
        }
    }
}

static QString str(const std::exception &e)
{
    return QString::fromLocal8Bit(e.what());
}

static QString str(const FilePath &f)
{
    return f.toUserOutput();
}

static ResultError logError(const QString &message)
{
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
        return ResultError(
            Tr::tr("Command \"%1\" failed with exit code %2: %3")
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
    if (m_errorExitHandler) {
        QObject::connect(m_client.get(), &Client::done, [this](const ProcessResultData &data) {
            if (data.m_exitCode != 0) {
                m_errorExitHandler();
            }
        });
    }

    auto startResult = m_client->start(deleteOnExit);
    if (!startResult)
        return logError(Tr::tr("Could not start cmdbridge: %1").arg(startResult.error()));

    return ResultOk;
}

FileAccess::DeployResult FileAccess::deployAndInit(
    const FilePath &libExecPath, const FilePath &remoteRootPath, const Environment &environment)
{
    const auto logError = [](const QString &msg, DeployError::Code code) {
        qCWarning(faLog) << msg;
        return make_unexpected(DeployError{msg, code});
    };
    const auto toDeployError = [](const Result<> &result) -> DeployResult {
        if (!result)
            return make_unexpected(DeployError{result.error(), DeployError::Other});
        return DeployResult();
    };

    if (remoteRootPath.isEmpty())
        return logError(Tr::tr("The remote root path is empty."), DeployError::Other);
    if (!remoteRootPath.isAbsolutePath())
        return logError(Tr::tr("The remote root path is not absolute."), DeployError::Other);

    const auto echo = run({remoteRootPath.withNewPath("echo"), {}});
    if (!echo)
        return logError(echo.error(), DeployError::EchoTestFailed);

    if (qtcEnvironmentVariableIsSet("QTC_DISABLE_CMDBRIDGE"))
        return logError("Disabled for testing!", DeployError::Disabled);

    const auto whichDD = run({remoteRootPath.withNewPath("which"), {"dd"}});

    if (!whichDD) { // TODO: Support Windows?
        return logError(
            Tr::tr("Could not find \"dd\" on the remote host: %1").arg(whichDD.error()),
            DeployError::Other);
    }

    QElapsedTimer timer;
    timer.start();
    auto deco = [&] {
        return QString("%1  (%2 ms)").arg(remoteRootPath.host()).arg(timer.elapsed());
    };

    qCDebug(faLog) << deco() << "Found dd on the remote host:" << *whichDD;

    const Result<QString> unameOs = run({remoteRootPath.withNewPath("uname"), {"-s"}});
    if (!unameOs) {
        return logError(
            Tr::tr("Could not determine the operating system on the remote host: %1")
                .arg(unameOs.error()),
            DeployError::Other);
    }

    const Result<OsType> osType = osTypeFromString(*unameOs);
    if (!osType)
        return logError(osType.error(), DeployError::Other);

    qCDebug(faLog) << deco() << "Remote host OS:" << *unameOs;

    const Result<QString> unameArch = run({remoteRootPath.withNewPath("uname"), {"-m"}});
    if (!unameArch) {
        return logError(
            Tr::tr("Could not determine the architecture of the remote host: %1")
                .arg(unameArch.error()),
            DeployError::Other);
    }

    const Result<OsArch> osArch = osArchFromString(*unameArch);
    if (!osArch)
        return logError(osArch.error(), DeployError::Other);

    qCDebug(faLog) << deco() << "Remote host architecture:" << *unameArch;

    const Result<FilePath> cmdBridgePath = Client::getCmdBridgePath(*osType, *osArch, libExecPath);

    if (!cmdBridgePath) {
        return logError(
            Tr::tr("Could not find a compatible cmdbridge for the remote host: %1")
                .arg(cmdBridgePath.error()),
            DeployError::Other);
    }

    qCDebug(faLog) << deco() << "Using cmdbridge at:" << *cmdBridgePath;

    if (remoteRootPath.isLocal())
        return toDeployError(init(*cmdBridgePath, environment, false));

    const auto cmdBridgeFileData = cmdBridgePath->fileContents();

    if (!cmdBridgeFileData) {
        return logError(
            Tr::tr("Could not read the cmdbridge file: %1").arg(cmdBridgeFileData.error()),
            DeployError::Other);
    }

    const QString xdgRuntimeDir = environment.value("XDG_RUNTIME_DIR");
    QStringList tmpFileArgs = {"-t", "cmdbridge.XXXXXXXXXX"};
    if (!xdgRuntimeDir.isEmpty())
        tmpFileArgs << QStringList{"-p", xdgRuntimeDir};

    const auto tmpFile = run({remoteRootPath.withNewPath("mktemp"), tmpFileArgs});

    if (!tmpFile) {
        return logError(
            Tr::tr("Could not create a temporary file: %1").arg(tmpFile.error()),
            DeployError::Other);
    }

    qCDebug(faLog) << deco() << "Using temporary file:" << *tmpFile;
    const auto dd = run({remoteRootPath.withNewPath("dd"), {"of=" + *tmpFile}},
                        *cmdBridgeFileData);

    qCDebug(faLog) << deco() << "dd run";

    const auto makeExecutable = run({remoteRootPath.withNewPath("chmod"), {"+x", *tmpFile}});

    if (!makeExecutable) {
        return logError(
            Tr::tr("Could not make the temporary file \"%1\" executable: %2")
                .arg(*tmpFile, makeExecutable.error()),
            DeployError::Other);
    }

    return toDeployError(init(remoteRootPath.withNewPath(*tmpFile), environment, true));
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
        return logError(
            Tr::tr("Could not get the file ID of \"%1\": %2").arg(str(filePath), str(e)));
    }
}

Result<bool> FileAccess::isSameFile(const FilePath &lhs, const FilePath &rhs) const
{
    try {
        Result<QFuture<bool>> f = m_client->isSameFile(lhs.nativePath(), rhs.nativePath());
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        return f->result();
    } catch (const std::exception &e) {
        return logError(
            Tr::tr("Could not check if \"%1\" and \"%2\" are the same file: %3")
                .arg(str(lhs), str(rhs), str(e)));
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
        return logError(
            Tr::tr("Could not get the file path info of \"%1\": %2").arg(str(filePath), str(e)));
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
            return ResultError(Tr::tr("The file \"%1\" does not exist.").arg(str(filePath)));
        if (e.code().value() == EINVAL) {
            return ResultError(
                Tr::tr("The path \"%1\" is not a symlink.").arg(str(filePath)));
        }
        return logError(
            Tr::tr("Could not get the symlink target for \"%1\": %2").arg(str(filePath), str(e)));
    } catch (const std::exception &e) {
        return logError(
            Tr::tr("Could not get the symlink target for \"%1\": %2").arg(str(filePath), str(e)));
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
        return logError(
            Tr::tr("Could not set permissions for \"%1\": %2").arg(str(filePath), str(e)));
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
        return logError(
            Tr::tr("Could not get the file owner of \"%1\": %2").arg(str(filePath), str(e)));
    }
}

Result<uint> FileAccess::ownerId(const FilePath &filePath) const
{
    try {
        Result<QFuture<uint>> f = m_client->ownerId(filePath.nativePath());
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        return f->result();
    } catch (const std::exception &e) {
        return logError(
            Tr::tr("Could not get the file owner ID of \"%1\": %2").arg(str(filePath), str(e)));
    }
}

Result<QString> FileAccess::group(const FilePath &filePath) const
{
    try {
        Result<QFuture<QString>> f = m_client->group(filePath.nativePath());
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        return f->result();
    } catch (const std::exception &e) {
        return logError(
            Tr::tr("Could not get the file group of \"%1\": %2").arg(str(filePath), str(e)));
    }
}

Result<uint> FileAccess::groupId(const FilePath &filePath) const
{
    try {
        Result<QFuture<uint>> f = m_client->groupId(filePath.nativePath());
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        return f->result();
    } catch (const std::exception &e) {
        return logError(
            Tr::tr("Could not get the file group ID of \"%1\": %2").arg(str(filePath), str(e)));
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
    } catch (const std::system_error &e) {
        if (e.code().value() == ENOENT)
            return logError(Tr::tr("The file \"%1\" does not exist.").arg(str(filePath)));
        return logError(Tr::tr("Could not read file \"%1\": %2").arg(str(filePath), str(e)));
    } catch (const std::exception &e) {
        return logError(Tr::tr("Could not read file \"%1\": %2").arg(str(filePath), str(e)));
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
        return logError(Tr::tr("Could not write file \"%1\": %2").arg(str(filePath), str(e)));
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
            return logError(Tr::tr("The file \"%1\" does not exist.").arg(str(filePath)));
        return logError(Tr::tr("Could not remove the file \"%1\": %2").arg(str(filePath), str(e)));
    } catch (const std::exception &e) {
        return logError(Tr::tr("Could not remove the file \"%1\": %2").arg(str(filePath), str(e)));
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
        return logError(
            Tr::tr("Could not remove \"%1\" recursively: %2").arg(str(filePath), str(e)));
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
        return logError(
            Tr::tr("Could not ensure that the file \"%1\" exists: %2").arg(str(filePath), str(e)));
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
        return logError(
            Tr::tr("Could not create the directory \"%1\": %2").arg(str(filePath), str(e)));
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
        return logError(
            Tr::tr("Could not copy the file \"%1\" to \"%2\": %3")
                .arg(str(filePath), str(target), str(e)));
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
        return logError(
            Tr::tr("Could not create a symlink from \"%1\" to \"%2\": %3")
                .arg(str(filePath), str(symLink), str(e)));
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
        return logError(
            Tr::tr("Could not rename the file \"%1\" to \"%2\": %3")
                .arg(str(filePath), str(target), str(e)));
    }
    return ResultOk;
}

std::vector<Result<std::unique_ptr<FilePathWatcher>>> FileAccess::watch(
    const FilePaths &filePaths) const
{
    // TODO: Watch whole sets of file paths for efficiency
    std::vector<Result<std::unique_ptr<FilePathWatcher>>> results;
    for (const FilePath &path : filePaths)
        results.push_back(m_client->watch(path.nativePath()));
    return results;
}

Result<> FileAccess::signalProcess(int pid, ControlSignal signal) const
{
    try {
        auto f = m_client->signalProcess(pid, signal);
        QTC_ASSERT_RESULT(f, return ResultError(f.error()));
        f->waitForFinished();
    } catch (const std::exception &e) {
        return logError(Tr::tr("Could not kill the process with PID %1: %2").arg(pid).arg(str(e)));
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
        return logError(
            Tr::tr("Could not create a temporary file for \"%1\": %2").arg(str(filePath), str(e)));
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

// ---------------------------------------------------------------------------
// LocalSocketForward implementation
// ---------------------------------------------------------------------------

// Private implementation class.  Q_OBJECT is used so that the instance can
// act as the context object in connect() calls, ensuring all lambdas are
// automatically disconnected when this object is destroyed.
//
// The forwarding direction is:
//   remote app  <->  Go socket server  <->  cmdbridge protocol
//       <->  (this) QLocalSocket  <->  local App socket server
//
// Multiple simultaneous remote clients are supported.  Each SocketServerConnect
// event carries a ConnId; a fresh QLocalSocket is created per ConnId so the
// local App sees an independent connection for every remote client.
class LocalSocketForwardImpl : public QObject
{
    Q_OBJECT

    struct ConnState
    {
        QLocalSocket *socket = nullptr;
        int retryMs = 25;       // current back-off interval
        int retryCount = 0;
    };

public:
    LocalSocketForwardImpl(
        Client *client,
        int socketId,
        const QString &localServerPath,
        const QString &remotePath,
        QFuture<Client::SocketServerEvent> eventFuture)
        : m_client(client)
        , m_socketId(socketId)
        , m_localServerPath(localServerPath)
        , m_remotePath(remotePath)
    {
        connect(&m_watcher,
                &QFutureWatcher<Client::SocketServerEvent>::resultReadyAt,
                this,
                [this](int idx) { handleEvent(m_watcher.resultAt(idx)); });
        connect(&m_watcher,
                &QFutureWatcher<Client::SocketServerEvent>::finished,
                this,
                [this] {
                    if (m_stopRequested) {
                        cleanupConnections();
                        deleteLater();
                    }
                });

        // createBridgeFileAccess() may run on a background thread that has no
        // Qt event loop.  QFutureWatcher::resultReadyAt is delivered via
        // QCoreApplication::postEvent(this), so both the watcher and this
        // object must live on a thread whose event loop is always running.
        // Move both before setFuture() so any events postEvent() enqueues
        // go directly to the main thread's queue.
        QThread *mainThread = QCoreApplication::instance()->thread();
        m_watcher.moveToThread(mainThread);
        QObject::moveToThread(mainThread);

        m_watcher.setFuture(eventFuture);
    }

    ~LocalSocketForwardImpl() override
    {
        // If requestStop() was already called the watcher and sockets are
        // being kept alive deliberately; just disconnect so that no slot fires
        // on a partially-destroyed object.
        m_watcher.disconnect();
        if (!m_stopRequested) {
            m_watcher.cancel();
            cleanupConnections();
            if (m_client)
                m_client->sendSocketStopForward(m_socketId);
        }
    }

    // Initiate graceful teardown: keep the watcher connected and the future
    // un-cancelled so that in-flight socketdata events keep flowing through
    // handleEvent until Go confirms (via "forwardserverstopped") that all
    // queued packets have been flushed.  The object then self-destructs.
    void requestStop()
    {
        QTC_ASSERT(!m_stopRequested, return);
        m_stopRequested = true;
        if (m_client)
            m_client->sendSocketStopForward(m_socketId);
    }

    QString remotePath() const { return m_remotePath; }

private:
    void handleEvent(const Client::SocketServerEvent &event)
    {
        if (const auto *conn = std::get_if<Client::SocketServerConnect>(&event)) {
            onRemoteConnect(conn->connId);
        } else if (const auto *data = std::get_if<Client::SocketServerData>(&event)) {
            auto it = m_connections.find(data->connId);
            if (it != m_connections.end() && it->socket)
                it->socket->write(data->bytes);
        } else if (const auto *close = std::get_if<Client::SocketServerClose>(&event)) {
            auto it = m_connections.find(close->connId);
            if (it != m_connections.end() && it->socket)
                it->socket->disconnectFromServer();
        }
    }

    void onRemoteConnect(int connId)
    {
        ConnState &cs = m_connections[connId];
        cs.retryMs = 25;
        cs.retryCount = 0;

        // Should not happen, but guard against a duplicate connect for the same connId.
        if (cs.socket) {
            if (m_client)
                m_client->sendSocketClose(m_socketId, connId);
            cs.socket->disconnect();
            cs.socket->deleteLater();
            cs.socket = nullptr;
        }

        qCDebug(faLog) << "LocalSocketForward: remote client" << connId
                       << "connected, forwarding to local server at" << m_localServerPath;

        QLocalSocket *socket = new QLocalSocket(this);
        cs.socket = socket;
        QPointer<QLocalSocket> weakSocket(socket);

        // The local App may not have called listen() yet when the remote client
        // connects.  Retry with exponential back-off instead of silently dropping.
        connect(
            socket,
            &QLocalSocket::errorOccurred,
            this,
            [this, connId, weakSocket](QLocalSocket::LocalSocketError error) {
                auto it = m_connections.find(connId);
                if (it == m_connections.end() || it->socket != weakSocket)
                    return;
                if (error == QLocalSocket::ServerNotFoundError
                    || error == QLocalSocket::ConnectionRefusedError) {
                    if (++it->retryCount > kMaxLocalConnectRetries) {
                        qCWarning(faLog) << "LocalSocketForward: gave up connecting to"
                                         << m_localServerPath << "after"
                                         << kMaxLocalConnectRetries << "retries";
                        if (m_client)
                            m_client->sendSocketClose(m_socketId, connId);
                        m_connections.erase(it);
                        if (weakSocket)
                            weakSocket->deleteLater();
                        return;
                    }
                    qCDebug(faLog) << "LocalSocketForward: connect to" << m_localServerPath
                                   << "failed (error" << error << "), retry"
                                   << it->retryCount << "of" << kMaxLocalConnectRetries;
                    const int delay = qMin(it->retryMs * 2, 1000);
                    it->retryMs = delay;
                    QTimer::singleShot(delay, this, [this, connId, weakSocket]() {
                        auto it2 = m_connections.find(connId);
                        if (it2 == m_connections.end() || it2->socket != weakSocket)
                            return;
                        if (weakSocket->state() == QLocalSocket::UnconnectedState)
                            weakSocket->connectToServer(m_localServerPath);
                    });
                } else {
                    qCWarning(faLog) << "LocalSocketForward: non-retryable error connecting to"
                                     << m_localServerPath << "- error:" << error
                                     << weakSocket->errorString();
                    if (m_client)
                        m_client->sendSocketClose(m_socketId, connId);
                    m_connections.erase(it);
                    if (weakSocket)
                        weakSocket->deleteLater();
                }
            });

        connect(socket, &QLocalSocket::readyRead, this, [this, connId, weakSocket]() {
            auto it = m_connections.find(connId);
            if (it != m_connections.end() && it->socket == weakSocket && m_client)
                m_client->sendSocketData(m_socketId, connId, weakSocket->readAll());
        });

        connect(socket, &QLocalSocket::disconnected, this, [this, connId, weakSocket]() {
            if (m_client)
                m_client->sendSocketClose(m_socketId, connId);
            auto it = m_connections.find(connId);
            if (it != m_connections.end() && it->socket == weakSocket)
                m_connections.erase(it);
            if (weakSocket)
                weakSocket->deleteLater();
        });

        socket->connectToServer(m_localServerPath);
    }

    void cleanupConnections()
    {
        for (const auto &cs : std::as_const(m_connections)) {
            if (cs.socket) {
                cs.socket->disconnect();
                cs.socket->deleteLater();
            }
        }
        m_connections.clear();
    }

    QPointer<Client> m_client;
    int m_socketId;
    QString m_localServerPath;
    QString m_remotePath;
    bool m_stopRequested = false;
    static constexpr int kMaxLocalConnectRetries = 30; // ~26 s at cap
    QMap<int, ConnState> m_connections;                // connId -> per-connection state
    QFutureWatcher<Client::SocketServerEvent> m_watcher;
};

class LocalSocketForwardHandle : public LocalSocketForward
{
public:
    explicit LocalSocketForwardHandle(std::unique_ptr<LocalSocketForwardImpl> impl)
        : m_impl(std::move(impl))
    {}

    ~LocalSocketForwardHandle() override
    {
        // Graceful teardown: keep LocalSocketForwardImpl alive until Go confirms
        // all in-flight packets have been flushed ("forwardserverstopped"), then
        // it self-destructs via deleteLater().
        m_impl.release()->requestStop();
    }

    FilePath path() const override
    {
        return FilePath::fromString(m_impl->remotePath());
    }

private:
    std::unique_ptr<LocalSocketForwardImpl> m_impl;
};

Result<std::unique_ptr<LocalSocketForward>> FileAccess::forwardLocalSocketServer(
    const QString &localSocketServerPath)
{
    if (!m_client)
        return ResultError(Tr::tr("The bridge is not initialized."));

    auto forwardResult = m_client->forwardSocketServer();
    if (!forwardResult)
        return ResultError(forwardResult.error());

    return std::make_unique<LocalSocketForwardHandle>(std::make_unique<LocalSocketForwardImpl>(
        m_client.get(),
        forwardResult->id,
        localSocketServerPath,
        forwardResult->remotePath,
        forwardResult->eventFuture));
}

} // namespace CmdBridge

#include "bridgedfileaccess.moc"
