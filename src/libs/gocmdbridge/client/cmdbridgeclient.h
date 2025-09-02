// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmdbridgeglobal.h"

#include <utils/filepath.h>
#include <utils/osspecificaspects.h>
#include <utils/processinterface.h>

#include <memory>

#include <QFuture>

namespace CmdBridge {

namespace Internal {
struct ClientPrivate;
}
class QTCREATOR_CMDBRIDGE_EXPORT Client : public QObject
{
    Q_OBJECT
public:
    Client(const Utils::FilePath &remoteCmdBridgePath, const Utils::Environment &env);
    ~Client();

    Utils::Result<> start(bool deleteOnExit = false);

    static Utils::Result<Utils::FilePath> getCmdBridgePath(Utils::OsType osType,
                                                                 Utils::OsArch osArch,
                                                                 const Utils::FilePath &libExecPath);

    using ExecResult = std::variant<std::pair<QByteArray, QByteArray>, int>;

    Utils::Result<QFuture<ExecResult>> execute(const Utils::CommandLine &cmdLine,
                                                     const Utils::Environment &env = {},
                                                     const QByteArray &stdIn = {});

    struct FindEntry
    {
        QString type;
        int id;
        QString path;
        qint64 size;
        int mode;
        bool isDir;
        QDateTime modTime;
    };

    using FindData = Utils::expected<FindEntry, std::optional<QString>>;

    // find() will at least return one result.
    // It will either be a "FindEntry", or an error message, or a null optional in case no entries
    // were found.
    // That way it is safe to wait for results using QFuture::resultAt().
    Utils::Result<QFuture<FindData>> find(const QString &directory,
                                                const Utils::FileFilter &filter);

    enum class Is {
        ReadableFile = 0,
        WritableFile = 1,
        ExecutableFile = 2,
        ReadableDir = 3,
        WritableDir = 4,
        File = 5,
        Dir = 6,
        Exists = 7,
        Symlink = 8,
    };

    Utils::Result<QFuture<bool>> is(const QString &path, Is is);

    struct Stat
    {
        qint64 size;
        int mode;
        quint32 usermode;
        QDateTime modTime;
        int numHardLinks;
        bool isDir;
    };

    Utils::Result<QFuture<Stat>> stat(const QString &path);

    Utils::Result<QFuture<QString>> readlink(const QString &path);
    Utils::Result<QFuture<QString>> fileId(const QString &path);
    Utils::Result<QFuture<quint64>> freeSpace(const QString &path);

    Utils::Result<QFuture<QByteArray>> readFile(const QString &path,
                                                      qint64 limit,
                                                      qint64 offset);

    Utils::Result<QFuture<qint64>> writeFile(const QString &path, const QByteArray &data);

    Utils::Result<QFuture<void>> removeFile(const QString &path);
    Utils::Result<QFuture<void>> removeRecursively(const QString &path);

    Utils::Result<QFuture<void>> ensureExistingFile(const QString &path);
    Utils::Result<QFuture<void>> createDir(const QString &path);

    Utils::Result<QFuture<void>> copyFile(const QString &source, const QString &target);
    Utils::Result<QFuture<void>> renameFile(const QString &source, const QString &target);

    Utils::Result<QFuture<Utils::FilePath>> createTempFile(const QString &path);

    Utils::Result<QFuture<void>> setPermissions(const QString &path, QFile::Permissions perms);

    Utils::Result<std::unique_ptr<Utils::FilePathWatcher>> watch(const QString &path);
    void stopWatch(int id);

    Utils::Result<QFuture<void>> signalProcess(int pid, Utils::ControlSignal signal);

    Utils::Result<QFuture<QString>> owner(const QString &path);
    Utils::Result<QFuture<uint>> ownerId(const QString &path);
    Utils::Result<QFuture<QString>> group(const QString &path);
    Utils::Result<QFuture<uint>> groupId(const QString &path);

protected:
    bool exit();

signals:
    void done(const Utils::ProcessResultData &resultData);

private:
    std::unique_ptr<Internal::ClientPrivate> d;
};

} // namespace CmdBridge
