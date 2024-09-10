// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bridgedfileaccess.h"

#include "cmdbridgeclient.h"
#include "cmdbridgetr.h"

#include <QFutureWatcher>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(faLog, "qtc.cmdbridge.fileaccess", QtWarningMsg);

using namespace Utils;

namespace CmdBridge {

FileAccess::~FileAccess() = default;

expected_str<QString> run(const CommandLine &cmdLine, const QByteArray &inputData = {})
{
    Process p;
    p.setCommand(cmdLine);
    p.setProcessChannelMode(QProcess::MergedChannels);
    if (!inputData.isEmpty())
        p.setWriteData(inputData);
    p.runBlocking();
    if (p.exitCode() != 0) {
        return make_unexpected(Tr::tr("Command failed with exit code %1: %2")
                                   .arg(p.exitCode())
                                   .arg(p.readAllStandardOutput()));
    }
    return p.readAllStandardOutput().trimmed();
}

Utils::expected_str<void> FileAccess::init(const Utils::FilePath &pathToBridge)
{
    m_client = std::make_unique<Client>(pathToBridge);

    auto startResult = m_client->start();
    if (!startResult)
        return make_unexpected(QString("Could not start cmdbridge: %1").arg(startResult.error()));

    try {
        if (!startResult->isValid())
            startResult->waitForFinished();
        m_environment = startResult->takeResult();
    } catch (const std::exception &e) {
        return make_unexpected(
            Tr::tr("Error starting cmdbridge: %1").arg(QString::fromLocal8Bit(e.what())));
    }
    return {};
}

expected_str<void> FileAccess::deployAndInit(
    const FilePath &libExecPath, const FilePath &remoteRootPath)
{
    if (remoteRootPath.isEmpty())
        return make_unexpected(Tr::tr("Remote root path is empty"));

    if (!remoteRootPath.isAbsolutePath())
        return make_unexpected(Tr::tr("Remote root path is not absolute"));

    const auto whichDD = run({remoteRootPath.withNewPath("which"), {"dd"}});

    if (!whichDD) // TODO: Support Windows?
        return make_unexpected(Tr::tr("Could not find dd on remote host: %1").arg(whichDD.error()));

    qCDebug(faLog) << "Found dd on remote host:" << *whichDD;

    const expected_str<QString> unameOs = run({remoteRootPath.withNewPath("uname"), {"-s"}});
    if (!unameOs) {
        return make_unexpected(
            QString("Could not determine OS on remote host: %1").arg(unameOs.error()));
    }
    Utils::expected_str<OsType> osType = osTypeFromString(*unameOs);
    if (!osType)
        return make_unexpected(osType.error());

    qCDebug(faLog) << "Remote host OS:" << *unameOs;

    const expected_str<QString> unameArch = run({remoteRootPath.withNewPath("uname"), {"-m"}});
    if (!unameArch) {
        return make_unexpected(
            QString("Could not determine architecture on remote host: %1").arg(unameArch.error()));
    }

    const Utils::expected_str<OsArch> osArch = osArchFromString(*unameArch);
    if (!osArch)
        return make_unexpected(osArch.error());

    qCDebug(faLog) << "Remote host architecture:" << *unameArch;

    const Utils::expected_str<Utils::FilePath> cmdBridgePath
        = Client::getCmdBridgePath(*osType, *osArch, libExecPath);

    if (!cmdBridgePath) {
        return make_unexpected(
            QString("Could not determine compatible cmdbridge for remote host: %1")
                .arg(cmdBridgePath.error()));
    }

    qCDebug(faLog) << "Using cmdbridge at:" << *cmdBridgePath;

    if (remoteRootPath.needsDevice()) {
        const auto cmdBridgeFileData = cmdBridgePath->fileContents();

        if (!cmdBridgeFileData) {
            return make_unexpected(
                QString("Could not read cmdbridge file: %1").arg(cmdBridgeFileData.error()));
        }

        const auto tmpFile = run(
            {remoteRootPath.withNewPath("mktemp"), {"-t", "cmdbridge.XXXXXXXXXX"}});

        if (!tmpFile) {
            return make_unexpected(
                QString("Could not create temporary file: %1").arg(tmpFile.error()));
        }

        qCDebug(faLog) << "Using temporary file:" << *tmpFile;
        const auto dd = run({remoteRootPath.withNewPath("dd"), {"of=" + *tmpFile, "bs=1"}},
                            *cmdBridgeFileData);

        const auto makeExecutable = run({remoteRootPath.withNewPath("chmod"), {"+x", *tmpFile}});

        if (!makeExecutable) {
            return make_unexpected(
                QString("Could not make temporary file executable: %1").arg(makeExecutable.error()));
        }

        return init(remoteRootPath.withNewPath(*tmpFile));
    }

    return init(*cmdBridgePath);
}

bool FileAccess::isExecutableFile(const FilePath &filePath) const
{
    try {
        auto f = m_client->is(filePath.nativePath(), CmdBridge::Client::Is::ExecutableFile);
        QTC_ASSERT_EXPECTED(f, return false);
        return f->result();
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error checking executable file:" << e.what();
        return false;
    }
}

bool FileAccess::isReadableFile(const FilePath &filePath) const
{
    try {
        auto f = m_client->is(filePath.nativePath(), CmdBridge::Client::Is::ReadableFile);
        QTC_ASSERT_EXPECTED(f, return false);
        return f->result();
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error checking readable file:" << e.what();
        return false;
    }
}

bool FileAccess::isWritableFile(const FilePath &filePath) const
{
    try {
        expected_str<QFuture<bool>> f = m_client->is(filePath.nativePath(),
                                                     CmdBridge::Client::Is::WritableFile);
        QTC_ASSERT_EXPECTED(f, return false);
        return f->result();
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error checking writable file:" << e.what();
        return false;
    }
}

bool FileAccess::isReadableDirectory(const FilePath &filePath) const
{
    try {
        auto f = m_client->is(filePath.nativePath(), CmdBridge::Client::Is::ReadableDir);
        QTC_ASSERT_EXPECTED(f, return false);
        return f->result();
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error checking readable directory:" << e.what();
        return false;
    }
}

bool FileAccess::isWritableDirectory(const FilePath &filePath) const
{
    try {
        auto f = m_client->is(filePath.nativePath(), CmdBridge::Client::Is::WritableDir);
        QTC_ASSERT_EXPECTED(f, return false);
        return f->result();
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error checking writable directory:" << e.what();
        return false;
    }
}

bool FileAccess::isFile(const FilePath &filePath) const
{
    try {
        auto f = m_client->is(filePath.nativePath(), CmdBridge::Client::Is::File);
        QTC_ASSERT_EXPECTED(f, return false);
        return f->result();
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error checking file:" << e.what();
        return false;
    }
}

bool FileAccess::isDirectory(const FilePath &filePath) const
{
    try {
        auto f = m_client->is(filePath.nativePath(), CmdBridge::Client::Is::Dir);
        QTC_ASSERT_EXPECTED(f, return false);
        return f->result();
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error checking directory:" << e.what();
        return false;
    }
}

bool FileAccess::isSymLink(const FilePath &filePath) const
{
    try {
        auto f = m_client->is(filePath.nativePath(), CmdBridge::Client::Is::Symlink);
        QTC_ASSERT_EXPECTED(f, return false);
        return f->result();
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error checking symlink:" << e.what();
        return false;
    }
}

bool FileAccess::exists(const FilePath &filePath) const
{
    try {
        auto f = m_client->is(filePath.nativePath(), CmdBridge::Client::Is::Exists);
        QTC_ASSERT_EXPECTED(f, return false);
        return f->result();
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error checking existence:" << e.what();
        return false;
    }
}

bool FileAccess::hasHardLinks(const FilePath &filePath) const
{
    try {
        auto f = m_client->stat(filePath.nativePath());
        QTC_ASSERT_EXPECTED(f, return false);
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

qint64 FileAccess::bytesAvailable(const FilePath &filePath) const
{
    try {
        expected_str<QFuture<quint64>> f = m_client->freeSpace(filePath.nativePath());
        QTC_ASSERT_EXPECTED(f, return -1);
        return f->result();
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error getting free space:" << e.what();
        return -1;
    }
}

QByteArray FileAccess::fileId(const FilePath &filePath) const
{
    try {
        expected_str<QFuture<QString>> f = m_client->fileId(filePath.nativePath());
        QTC_ASSERT_EXPECTED(f, return {});
        return f->result().toUtf8();
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error getting file ID:" << e.what();
        return {};
    }
}

FilePathInfo FileAccess::filePathInfo(const FilePath &filePath) const
{
    if (filePath.path() == "/") // TODO: Add FilePath::isRoot()
    {
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
        expected_str<QFuture<Client::Stat>> f = m_client->stat(filePath.nativePath());
        QTC_ASSERT_EXPECTED(f, return {});
        Client::Stat stat = f->result();
        return {stat.size,
                fileInfoFlagsfromStatMode(stat.mode) | FilePathInfo::FileFlags(stat.usermode),
                stat.modTime};
    } catch (const std::exception &e) {
        qCDebug(faLog) << "Error getting file path info:" << e.what();
        return {};
    }
}

FilePath FileAccess::symLinkTarget(const FilePath &filePath) const
{
    try {
        expected_str<QFuture<QString>> f = m_client->readlink(filePath.nativePath());
        QTC_ASSERT_EXPECTED(f, return {});
        return filePath.parentDir().resolvePath(filePath.withNewPath(f->result()).path());
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error getting symlink target:" << e.what();
        return {};
    }
}

QDateTime FileAccess::lastModified(const FilePath &filePath) const
{
    return filePathInfo(filePath).lastModified;
}

QFile::Permissions FileAccess::permissions(const FilePath &filePath) const
{
    return QFile::Permissions::fromInt(filePathInfo(filePath).fileFlags & FilePathInfo::PermsMask);
}

bool FileAccess::setPermissions(const FilePath &filePath, QFile::Permissions perms) const
{
    try {
        expected_str<QFuture<void>> f = m_client->setPermissions(filePath.nativePath(), perms);
        QTC_ASSERT_EXPECTED(f, return false);
        f->waitForFinished();
        return true;
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error setting permissions:" << e.what();
        return false;
    }
}

qint64 FileAccess::fileSize(const FilePath &filePath) const
{
    return filePathInfo(filePath).fileSize;
}

expected_str<QByteArray> FileAccess::fileContents(const FilePath &filePath,
                                                  qint64 limit,
                                                  qint64 offset) const
{
    try {
        expected_str<QFuture<QByteArray>> f = m_client->readFile(filePath.nativePath(),
                                                                 limit,
                                                                 offset);
        QTC_ASSERT_EXPECTED(f, return {});
        f->waitForFinished();
        QByteArray data;
        for (const QByteArray &result : *f) {
            data.append(result);
        }
        return data;
    } catch (const std::exception &e) {
        return make_unexpected(
            Tr::tr("Error reading file: %1").arg(QString::fromLocal8Bit(e.what())));
    }
}

expected_str<qint64> FileAccess::writeFileContents(const FilePath &filePath,
                                                   const QByteArray &data) const
{
    try {
        expected_str<QFuture<qint64>> f = m_client->writeFile(filePath.nativePath(), data);
        QTC_ASSERT_EXPECTED(f, return {});
        return f->result();
    } catch (const std::exception &e) {
        return make_unexpected(
            Tr::tr("Error writing file: %1").arg(QString::fromLocal8Bit(e.what())));
    }
}

expected_str<void> FileAccess::removeFile(const Utils::FilePath &filePath) const
{
    try {
        Utils::expected_str<QFuture<void>> f = m_client->removeFile(filePath.nativePath());
        if (!f)
            return make_unexpected(f.error());
        f->waitForFinished();
    } catch (const std::system_error &e) {
        if (e.code().value() == ENOENT)
            return make_unexpected(Tr::tr("File does not exist"));
        qCWarning(faLog) << "Error removing file:" << e.what();
        return make_unexpected(
            Tr::tr("Error removing file: %1").arg(QString::fromLocal8Bit(e.what())));
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error removing file:" << e.what();
        return make_unexpected(
            Tr::tr("Error removing file: %1").arg(QString::fromLocal8Bit(e.what())));
    }

    return {};
}

bool FileAccess::removeRecursively(const Utils::FilePath &filePath, QString *error) const
{
    try {
        auto f = m_client->removeRecursively(filePath.nativePath());
        QTC_ASSERT_EXPECTED(f, return false);
        f->waitForFinished();
        return true;
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error removing directory:" << e.what();
        if (error)
            *error = QString::fromLocal8Bit(e.what());
        return false;
    }
}

bool FileAccess::ensureExistingFile(const Utils::FilePath &filePath) const
{
    try {
        auto f = m_client->ensureExistingFile(filePath.nativePath());
        QTC_ASSERT_EXPECTED(f, return false);
        f->waitForFinished();
        return true;
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error ensuring existing file:" << e.what();
        return false;
    }
}
bool FileAccess::createDirectory(const Utils::FilePath &filePath) const
{
    try {
        auto f = m_client->createDir(filePath.nativePath());
        QTC_ASSERT_EXPECTED(f, return false);
        f->waitForFinished();
        return true;
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error creating directory:" << e.what();
        return false;
    }
}

expected_str<void> FileAccess::copyFile(const Utils::FilePath &filePath,
                                        const Utils::FilePath &target) const
{
    try {
        auto f = m_client->copyFile(filePath.nativePath(), target.nativePath());
        QTC_ASSERT_EXPECTED(f, return {});
        f->waitForFinished();
        return {};
    } catch (const std::exception &e) {
        return make_unexpected(
            Tr::tr("Error copying file: %1").arg(QString::fromLocal8Bit(e.what())));
    }
}

bool FileAccess::renameFile(const Utils::FilePath &filePath, const Utils::FilePath &target) const
{
    try {
        auto f = m_client->renameFile(filePath.nativePath(), target.nativePath());
        QTC_ASSERT_EXPECTED(f, return false);
        f->waitForFinished();
        return true;
    } catch (const std::exception &e) {
        qCWarning(faLog) << "Error renaming file:" << e.what();
        return false;
    }
}

Environment FileAccess::deviceEnvironment() const
{
    return m_environment;
}


expected_str<std::unique_ptr<FilePathWatcher>> FileAccess::watch(const FilePath &filePath) const
{
    return m_client->watch(filePath.nativePath());
}

Utils::expected_str<void> FileAccess::signalProcess(int pid, ControlSignal signal) const
{
    try {
        auto f = m_client->signalProcess(pid, signal);
        QTC_ASSERT_EXPECTED(f, return {});
        f->waitForFinished();
        return {};
    } catch (const std::exception &e) {
        return make_unexpected(
            Tr::tr("Error killing process: %1").arg(QString::fromLocal8Bit(e.what())));
    };
}

expected_str<FilePath> FileAccess::createTempFile(const FilePath &filePath)
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

        Utils::expected_str<QFuture<Utils::FilePath>> f = m_client->createTempFile(path);
        QTC_ASSERT_EXPECTED(f, return {});
        f->waitForFinished();

        expected_str<FilePath> result = f->result();
        if (!result)
            return result;
        return filePath.withNewPath(result->path());
    } catch (const std::exception &e) {
        return make_unexpected(
            Tr::tr("Error creating temporary file: %1").arg(QString::fromLocal8Bit(e.what())));
    }
}

void FileAccess::iterateDirectory(const FilePath &filePath,
                                  const FilePath::IterateDirCallback &callback,
                                  const FileFilter &filter) const
{
    auto result = m_client->find(filePath.nativePath(), filter);
    QTC_ASSERT_EXPECTED(result, return);

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
        if (result->isValid()) {
            result->resultAt(idx);
            // Wait for the next result to become available
            processResults();
        }
    }
    processResults();

    qCDebug(faLog) << "Iterated directory" << filePath.toString() << "in" << t.elapsed() << "ms";
}

} // namespace CmdBridge
