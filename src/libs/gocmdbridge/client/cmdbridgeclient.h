// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cmdbridgeglobal.h"

#include <utils/filepath.h>
#include <utils/osspecificaspects.h>
#include <utils/processinterface.h>

#include <memory>
#include <variant>

#include <QByteArray>
#include <QFuture>
#include <QString>

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
    Utils::Result<QFuture<void>> createSymLink(const QString &source, const QString &symLink);
    Utils::Result<QFuture<void>> renameFile(const QString &source, const QString &target);

    Utils::Result<QFuture<Utils::FilePath>> createTempFile(const QString &path);
    Utils::Result<QFuture<Utils::FilePath>> createTempDir(const QString &path);

    Utils::Result<QFuture<void>> setPermissions(const QString &path, QFile::Permissions perms);

    Utils::Result<std::unique_ptr<Utils::FilePathWatcher>> watch(const Utils::FilePath &path);
    void stopWatch(int id);

    Utils::Result<QFuture<void>> signalProcess(int pid, Utils::ControlSignal signal);

    Utils::Result<QFuture<QString>> owner(const QString &path);
    Utils::Result<QFuture<uint>> ownerId(const QString &path);
    Utils::Result<QFuture<QString>> group(const QString &path);
    Utils::Result<QFuture<uint>> groupId(const QString &path);

    Utils::Result<QFuture<bool>> isSameFile(const QString &path1, const QString &path2);

    // Events streamed from the Go bridge for an active socket server forward.
    struct SocketServerConnect { int connId; };                     // A remote client has connected.
    struct SocketServerData { int connId; QByteArray bytes; };      // Bytes from the remote client.
    struct SocketServerClose { int connId; };                       // Remote client has disconnected.
    using SocketServerEvent
        = std::variant<SocketServerConnect, SocketServerData, SocketServerClose>;

    struct SocketServerForward
    {
        int id;
        QString remotePath;                        // Path of Go's socket server on the remote.
        QFuture<SocketServerEvent> eventFuture;    // Streams connect/data/close events.
    };

    // Tells Go to create a Unix socket server on the remote device.  Returns a
    // SocketServerForward that streams events; the caller responds to
    // SocketServerConnect by connecting to the local App's server and relays
    // SocketServerData / SocketServerClose in both directions.
    Utils::Result<SocketServerForward> forwardSocketServer();

    // Send raw bytes to the remote client identified by connId within forward id.
    void sendSocketData(int id, int connId, const QByteArray &data);

    // Notify Go that the local side has closed its end of a specific connection.
    void sendSocketClose(int id, int connId);

    // Tear down the Go socket server for this id entirely (closes listener + socket file).
    // Call this when the LocalSocketForward handle is destroyed.
    void sendSocketStopForward(int id);

protected:
    bool exit();

signals:
    void done(const Utils::ProcessResultData &resultData);

private:
    std::unique_ptr<Internal::ClientPrivate> d;
};

} // namespace CmdBridge
