// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmdbridgeclient.h"

#include "cmdbridgetr.h"

#include <utils/qtcprocess.h>
#include <utils/synchronizedvalue.h>

#include <QCborArray>
#include <QCborMap>
#include <QCborStreamReader>
#include <QFuture>
#include <QFutureWatcher>
#include <QJsonDocument>
#include <QLoggingCategory>
#include <QPromise>
#include <QThread>

Q_LOGGING_CATEGORY(clientLog, "qtc.cmdbridge.client", QtWarningMsg)

#define ASSERT_TYPE(expectedtype) \
    QTC_ASSERT(map.value("Type").toString() == expectedtype, promise.finish(); \
               return JobResult::Done)

using namespace Utils;

namespace CmdBridge {

enum class JobResult {
    Continue,
    Done,
};

namespace Internal {

struct ClientPrivate
{
    FilePath remoteCmdBridgePath;

    // Only access from the thread
    Process *process = nullptr;
    QThread *thread = nullptr;

    struct Jobs
    {
        int nextId = 0;
        QMap<int, std::function<JobResult(QVariantMap)>> map;
    };

    Utils::SynchronizedValue<Jobs> jobs;

    QMap<int, std::shared_ptr<QPromise<FilePath>>> watchers;

    expected_str<void> readPacket(QCborStreamReader &reader);
    std::optional<expected_str<void>> handleWatchResults(const QVariantMap &map);
};

QString decodeString(QCborStreamReader &reader)
{
    QString result;
    auto r = reader.readString();
    while (r.status == QCborStreamReader::Ok) {
        result += r.data;
        r = reader.readString();
    }

    if (r.status == QCborStreamReader::Error) {
        // handle error condition
        result.clear();
    }
    return result;
}

QByteArray decodeByteArray(QCborStreamReader &reader)
{
    QByteArray result;
    auto r = reader.readByteArray();
    while (r.status == QCborStreamReader::Ok) {
        result += r.data;
        r = reader.readByteArray();
    }

    if (r.status == QCborStreamReader::Error) {
        // handle error condition
        result.clear();
    }
    return result;
}

QVariant simpleToVariant(QCborSimpleType s)
{
    switch (s) {
    case QCborSimpleType::False:
        return false;
    case QCborSimpleType::True:
        return true;
    case QCborSimpleType::Null:
        return {};
    case QCborSimpleType::Undefined:
        return {};
    }
    return {};
}

static QVariant readVariant(QCborStreamReader &reader);

QVariant decodeArray(QCborStreamReader &reader)
{
    QVariantList result;
    reader.enterContainer();
    while (reader.lastError() == QCborError::NoError && reader.hasNext()) {
        result.append(readVariant(reader));
    }
    reader.leaveContainer();
    return result;
}

static QVariant readVariant(QCborStreamReader &reader)
{
    QVariant result;

    switch (reader.type()) {
    case QCborStreamReader::UnsignedInteger:
        result = reader.toUnsignedInteger();
        break;
    case QCborStreamReader::NegativeInteger:
        result = reader.toInteger();
        break;
    case QCborStreamReader::ByteString:
        return decodeByteArray(reader);
    case QCborStreamReader::TextString:
        return decodeString(reader);
    case QCborStreamReader::Array:
        return decodeArray(reader);
    case QCborStreamReader::Map:
        QTC_ASSERT(!"Not implemented", return {});
    case QCborStreamReader::Tag:
        // QCborTag tag = reader.toTag();
        QTC_ASSERT(!"Not implemented", return {});
    case QCborStreamReader::SimpleType:
        result = simpleToVariant(reader.toSimpleType());
        break;
    case QCborStreamReader::HalfFloat: {
        float f;
        qfloat16 qf = reader.toFloat16();
        qFloatFromFloat16(&f, &qf, 1);
        result = f;
        break;
    }
    case QCborStreamReader::Float:
        result = reader.toFloat();
        break;
    case QCborStreamReader::Double:
        result = reader.toDouble();
        break;
    case QCborStreamReader::Invalid:
        QTC_ASSERT(!"Invalid type", return {});
    }
    reader.next();
    return result;
}

std::optional<expected_str<void>> ClientPrivate::handleWatchResults(const QVariantMap &map)
{
    const QString type = map.value("Type").toString();
    if (type == "watchEvent") {
        auto id = map.value("Id").toInt();
        auto it = watchers.find(id);

        if (it == watchers.end())
            return make_unexpected(QString("No watcher found for id %1").arg(id));

        auto promise = it.value();
        if (!promise->isCanceled())
            promise->addResult(FilePath::fromUserInput(map.value("Path").toString()));

        return expected_str<void>{};
    } else if (type == "removewatchresult") {
        auto id = map.value("Id").toInt();
        watchers.remove(id);
        return expected_str<void>{};
    }

    return std::nullopt;
}

expected_str<void> ClientPrivate::readPacket(QCborStreamReader &reader)
{
    if (!reader.enterContainer())
        return make_unexpected(QString("The packet did not contain a container"));

    Q_ASSERT(QThread::currentThread() == thread);

    QVariantMap map;

    while (reader.lastError() == QCborError::NoError && reader.hasNext()) {
        auto key = reader.type() == QCborStreamReader::String ? decodeString(reader) : QString();
        map.insert(key, readVariant(reader));
    }

    if (!reader.leaveContainer())
        return make_unexpected(QString("The packet did not contain a finalized map"));

    if (!map.contains("Id")) {
        return make_unexpected(QString("The packet did not contain an Id"));
    }

    auto watchHandled = handleWatchResults(map);
    if (watchHandled)
        return *watchHandled;

    auto id = map.value("Id").toInt();
    auto j = jobs.readLocked();
    auto it = j->map.find(id);
    if (it == j->map.end())
        return make_unexpected(
            QString("No job found for packet with id %1: %2")
                .arg(id)
                .arg(QString::fromUtf8(QJsonDocument::fromVariant(map).toJson())));

    if (it.value()(map) == JobResult::Done) {
        j.unlock();
        auto jw = jobs.writeLocked();
        jw->map.remove(id);
    }

    return {};
}

} // namespace Internal

Client::Client(const Utils::FilePath &remoteCmdBridgePath)
    : d(new Internal::ClientPrivate())
{
    d->remoteCmdBridgePath = remoteCmdBridgePath;
}

Client::~Client()
{
    d->thread->quit();
    d->thread->wait();
}

expected_str<QFuture<Environment>> Client::start()
{
    d->thread = new QThread(this);
    d->thread->setObjectName("CmdBridgeClientThread");
    d->thread->start();

    d->process = new Process();
    d->process->moveToThread(d->thread);

    std::shared_ptr<QPromise<Environment>> envPromise = std::make_shared<QPromise<Environment>>();

    expected_str<void> result;

    d->jobs.writeLocked()->map.insert(-1, [envPromise](QVariantMap map) {
        envPromise->start();
        QString type = map.value("Type").toString();
        QTC_CHECK(type == "environment");
        if (type == "environment") {
            OsType osType = osTypeFromString(map.value("OsType").toString());
            Environment env(map.value("Env").toStringList(), osType);
            envPromise->addResult(env);
        } else if (type == "error") {
            QString err = map.value("Error", QString{}).toString();
            qCWarning(clientLog) << "Error: " << err;
            envPromise->setException(std::make_exception_ptr(std::runtime_error(err.toStdString())));
        }

        envPromise->finish();

        return JobResult::Done;
    });

    QMetaObject::invokeMethod(
        d->process,
        [this]() -> expected_str<void> {
            d->process->setCommand({d->remoteCmdBridgePath, {}});
            d->process->setProcessMode(ProcessMode::Writer);
            d->process->setProcessChannelMode(QProcess::ProcessChannelMode::SeparateChannels);

            connect(d->process, &Process::done, d->process, [this] {
                if (d->process->resultData().m_exitCode != 0) {
                    qCWarning(clientLog).noquote() << d->process->resultData().m_errorString;
                    qCWarning(clientLog).noquote() << d->process->readAllStandardError();
                    qCWarning(clientLog).noquote() << d->process->readAllStandardOutput();
                }

                auto j = d->jobs.writeLocked();
                for (auto it = j->map.begin(); it != j->map.end();) {
                    auto func = it.value();
                    auto id = it.key();
                    it = j->map.erase(it);
                    func(QVariantMap{{"Type", "error"}, {"Id", id}, {"Error", "Process exited"}});
                }

                emit done(d->process->resultData());
            });

            auto stateMachine = [state = int(0), packetSize(0), packetData = QByteArray(), this](
                                    QByteArray &buffer) mutable {
                static const QByteArray MagicCode{GOBRIDGE_MAGIC_PACKET_MARKER};

                if (state == 0) {
                    int start = buffer.indexOf(MagicCode);
                    if (start == -1) {
                        // Broken package, search for next magic marker
                        qCWarning(clientLog) << "Magic marker was not found";
                        // If we don't find a magic marker, the rest of the buffer is trash.
                        buffer.clear();
                    } else {
                        buffer.remove(0, start + MagicCode.size());
                        state = 1;
                    }
                }

                if (state == 1) {
                    QDataStream ds(buffer);
                    ds >> packetSize;
                    // TODO: Enforce max size in bridge.
                    if (packetSize > 0 && packetSize < 16384) {
                        state = 2;
                        buffer.remove(0, sizeof(packetSize));
                    } else {
                        // Broken package, search for next magic marker
                        qCWarning(clientLog) << "Invalid packet size" << packetSize;
                        state = 0;
                    }
                }

                if (state == 2) {
                    auto packetDataRemaining = packetSize - packetData.size();
                    auto dataAvailable = buffer.size();
                    auto availablePacketData = qMin(packetDataRemaining, dataAvailable);
                    packetData.append(buffer.left(availablePacketData));
                    buffer.remove(0, availablePacketData);

                    if (packetData.size() == packetSize) {
                        QCborStreamReader reader;
                        reader.addData(packetData);
                        packetData.clear();
                        state = 0;
                        auto result = d->readPacket(reader);
                        QTC_CHECK_EXPECTED(result);
                    }
                }
            };

            connect(
                d->process,
                &Process::readyReadStandardError,
                d->process,
                [this, buffer = QByteArray(), stateMachine]() mutable {
                    buffer.append(d->process->readAllRawStandardError());
                    while (!buffer.isEmpty())
                        stateMachine(buffer);
                });

            connect(d->process, &Process::readyReadStandardOutput, d->process, [this] {
                qCWarning(clientLog).noquote() << d->process->readAllStandardOutput();
            });

            d->process->start();

            if (!d->process->waitForStarted())
                return make_unexpected(
                    Tr::tr("Failed starting bridge process: %1").arg(d->process->errorString()));
            return {};
        },
        Qt::BlockingQueuedConnection,
        &result);

    if (!result)
        return make_unexpected(result.error());

    return envPromise->future();
}

enum class Errors {
    Handle,
    DontHandle,
};

template<class R>
static Utils::expected_str<QFuture<R>> createJob(
    Internal::ClientPrivate *d,
    QCborMap args,
    const std::function<JobResult(QVariantMap map, QPromise<R> &promise)> &resultFunc,
    Errors handleErrors = Errors::Handle)
{
    if (!d->process || !d->process->isRunning())
        return make_unexpected(Tr::tr("Bridge process not running"));

    std::shared_ptr<QPromise<R>> promise = std::make_shared<QPromise<R>>();
    QFuture<R> future = promise->future();

    promise->start();

    auto j = d->jobs.writeLocked();
    int id = j->nextId++;
    j->map.insert(id, [handleErrors, promise, resultFunc](QVariantMap map) {
        QString type = map.value("Type").toString();
        if (handleErrors == Errors::Handle && type == "error") {
            const QString err = map.value("Error", QString{}).toString();
            const QString errType = map.value("ErrorType", QString{}).toString();
            if (errType == "ENOENT") {
                promise->setException(
                    std::make_exception_ptr(std::system_error(ENOENT, std::generic_category())));
                promise->finish();
            } else {
                qCWarning(clientLog) << "Error (" << errType << "):" << err;
                promise->setException(
                    std::make_exception_ptr(std::runtime_error(err.toStdString())));
                promise->finish();
            }
            return JobResult::Done;
        }

        JobResult result = resultFunc(map, *promise);
        if (result == JobResult::Done)
            promise->finish();

        return result;
    });

    args.insert(QString("Id"), id);

    QMetaObject::invokeMethod(
        d->process,
        [d, args]() { d->process->writeRaw(args.toCborValue().toCbor()); },
        Qt::QueuedConnection);

    return future;
}

static Utils::expected_str<QFuture<void>> createVoidJob(
    Internal::ClientPrivate *d, const QCborMap &args, const QString &resulttype)
{
    return createJob<void>(d, args, [resulttype](QVariantMap map, QPromise<void> &promise) {
        ASSERT_TYPE(resulttype);
        promise.finish();
        return JobResult::Done;
    });
}

expected_str<QFuture<Client::ExecResult>> Client::execute(
    const Utils::CommandLine &cmdLine, const Utils::Environment &env, const QByteArray &stdIn)
{
    QCborMap execArgs = QCborMap{
        {"Args",
         QCborArray::fromStringList(
             QStringList() << cmdLine.executable().nativePath() << cmdLine.splitArguments())}};
    if (env.hasChanges())
        execArgs.insert(QCborValue("Env"), QCborArray::fromStringList(env.toStringList()));

    if (!stdIn.isEmpty())
        execArgs.insert(QCborValue("Stdin"), QCborValue(stdIn));

    QCborMap exec{{"Type", "exec"}, {"Exec", execArgs}};

    return createJob<ExecResult>(d.get(), exec, [](QVariantMap map, QPromise<ExecResult> &promise) {
        QString type = map.value("Type").toString();
        if (type == "execdata") {
            QByteArray stdOut = map.value("Stdout").toByteArray();
            QByteArray stdErr = map.value("Stderr").toByteArray();
            promise.addResult(std::make_pair(stdOut, stdErr));
            return JobResult::Continue;
        }

        promise.addResult(map.value("Code").toInt());
        return JobResult::Done;
    });
}

expected_str<QFuture<Client::FindData>> Client::find(
    const QString &directory, const Utils::FileFilter &filter)
{
    // TODO: golang's walkDir does not support automatically following symlinks.
    if (filter.iteratorFlags.testFlag(QDirIterator::FollowSymlinks))
        return make_unexpected(Tr::tr("FollowSymlinks is not supported"));

    QCborMap findArgs{
        {"Type", "find"},
        {"Find",
         QCborMap{
             {"Directory", directory},
             {"FileFilters", filter.fileFilters.toInt()},
             {"NameFilters", QCborArray::fromStringList(filter.nameFilters)},
             {"IteratorFlags", filter.iteratorFlags.toInt()}}}};

    return createJob<FindData>(
        d.get(),
        findArgs,
        [hasEntries = false,
         cache = QList<FindData>()](QVariantMap map, QPromise<FindData> &promise) mutable {
            if (promise.isCanceled())
                return JobResult::Done;

            QString type = map.value("Type").toString();
            if (type == "finddata") {
                hasEntries = true;
                FindEntry data;
                data.type = map.value("Type").toString();
                data.id = map.value("Id").toInt();
                data.path = map.value("Path").toString();
                data.size = map.value("Size").toLongLong();
                data.mode = map.value("Mode").toInt();
                data.isDir = map.value("IsDir").toBool();
                data.modTime = QDateTime::fromSecsSinceEpoch(map.value("ModTime").toULongLong());

                cache.append(data);
                if (cache.size() > 1000) {
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
                    for (const auto &entry : cache)
                        promise.addResult(entry);
#else
                    promise.addResults(cache);
#endif
                    cache.clear();
                }
                return JobResult::Continue;
            } else if (type == "error") {
                hasEntries = true;
                promise.addResult(make_unexpected(map.value("Error", QString{}).toString()));
                return JobResult::Done;
            }

            if (cache.size() > 0) {
#if QT_VERSION < QT_VERSION_CHECK(6, 6, 0)
                for (const auto &entry : cache)
                    promise.addResult(entry);
#else
                promise.addResults(cache);
#endif
            } else if (!hasEntries) {
                promise.addResult(make_unexpected(std::nullopt));
            }

            return JobResult::Done;
        },
        Errors::DontHandle);
}

Utils::expected_str<QFuture<QString>> Client::readlink(const QString &path)
{
    return createJob<QString>(
        d.get(),
        QCborMap{{"Type", "readlink"}, {"Path", path}},
        [](const QVariantMap &map, QPromise<QString> &promise) {
            ASSERT_TYPE("readlinkresult");

            promise.addResult(map.value("Target").toString());
            return JobResult::Done;
        });
}

Utils::expected_str<QFuture<QString>> Client::fileId(const QString &path)
{
    return createJob<QString>(
        d.get(),
        QCborMap{{"Type", "fileid"}, {"Path", path}},
        [](QVariantMap map, QPromise<QString> &promise) {
            ASSERT_TYPE("fileidresult");

            promise.addResult(map.value("FileId").toString());
            return JobResult::Done;
        });
}

Utils::expected_str<QFuture<quint64>> Client::freeSpace(const QString &path)
{
    return createJob<quint64>(
        d.get(),
        QCborMap{{"Type", "freespace"}, {"Path", path}},
        [](QVariantMap map, QPromise<quint64> &promise) {
            ASSERT_TYPE("freespaceresult");
            promise.addResult(map.value("FreeSpace").toULongLong());
            return JobResult::Done;
        });
}

Utils::expected_str<QFuture<QByteArray>> Client::readFile(
    const QString &path, qint64 limit, qint64 offset)
{
    return createJob<QByteArray>(
        d.get(),
        QCborMap{
            {"Type", "readfile"},
            {"ReadFile", QCborMap{{"Path", path}, {"Limit", limit}, {"Offset", offset}}}},
        [](QVariantMap map, QPromise<QByteArray> &promise) mutable {
            QString type = map.value("Type").toString();

            if (type == "readfiledata") {
                promise.addResult(map.value("Contents").toByteArray());
                return JobResult::Continue;
            }

            ASSERT_TYPE("readfiledone");
            return JobResult::Done;
        });
}

Utils::expected_str<QFuture<qint64>> Client::writeFile(
    const QString &path, const QByteArray &contents)
{
    return createJob<qint64>(
        d.get(),
        QCborMap{
            {"Type", "writefile"},
            {"WriteFile",
             QCborMap{
                 {"Path", path},
                 {"Contents", contents},
             }}},
        [](QVariantMap map, QPromise<qint64> &promise) {
            ASSERT_TYPE("writefileresult");
            promise.addResult(map.value("WrittenBytes").toLongLong());
            return JobResult::Done;
        });
}

Utils::expected_str<QFuture<void>> Client::removeFile(const QString &path)
{
    return createVoidJob(d.get(), QCborMap{{"Type", "remove"}, {"Path", path}}, "removeresult");
}

Utils::expected_str<QFuture<void>> Client::removeRecursively(const QString &path)
{
    return createVoidJob(d.get(), QCborMap{{"Type", "removeall"}, {"Path", path}}, "removeallresult");
}

Utils::expected_str<QFuture<void>> Client::ensureExistingFile(const QString &path)
{
    return createVoidJob(
        d.get(),
        QCborMap{{"Type", "ensureexistingfile"}, {"Path", path}},
        "ensureexistingfileresult");
}

Utils::expected_str<QFuture<void>> Client::createDir(const QString &path)
{
    return createVoidJob(d.get(), QCborMap{{"Type", "createdir"}, {"Path", path}}, "createdirresult");
}

Utils::expected_str<QFuture<void>> Client::copyFile(const QString &source, const QString &target)
{
    return createVoidJob(
        d.get(),
        QCborMap{
            {"Type", "copyfile"},
            {"CopyFile", QCborMap{{"Source", source}, {"Target", target}}},
        },
        "copyfileresult");
}

Utils::expected_str<QFuture<void>> Client::renameFile(const QString &source, const QString &target)
{
    return createVoidJob(
        d.get(),
        QCborMap{
            {"Type", "renamefile"},
            {"RenameFile", QCborMap{{"Source", source}, {"Target", target}}},
        },
        "renamefileresult");
}

Utils::expected_str<QFuture<FilePath>> Client::createTempFile(const QString &path)
{
    return createJob<FilePath>(
        d.get(),
        QCborMap{{"Type", "createtempfile"}, {"Path", path}},
        [](QVariantMap map, QPromise<FilePath> &promise) {
            ASSERT_TYPE("createtempfileresult");

            promise.addResult(FilePath::fromUserInput(map.value("Path").toString()));

            return JobResult::Done;
        });
}

Utils::expected_str<QFuture<void>> Client::setPermissions(
    const QString &path, QFile::Permissions perms)
{
    // Convert the QFileDevice::Permissions to unix style permissions
    uint p = perms.toInt() & 0xF0FF;
    p = ((p & 0xF000) >> 4 | (p & 0xFF));
    p = ((p & 0xF00) >> 2) | ((p & 0xF0) >> 1) | (p & 0xF);

    return createVoidJob(
        d.get(),
        QCborMap{
            {"Type", "setpermissions"}, {"SetPermissions", QCborMap{{"Path", path}, {"Mode", p}}}},
        "setpermissionsresult");
}

class GoFilePathWatcher : public FilePathWatcher
{
    QFutureWatcher<FilePath> m_futureWatcher;

public:
    using Watch = QFuture<FilePath>;

public:
    GoFilePathWatcher(Watch watch)
    {
        connect(&m_futureWatcher, &QFutureWatcher<FilePath>::resultReadyAt, this, [this](int idx) {
            emit pathChanged(m_futureWatcher.resultAt(idx));
        });

        m_futureWatcher.setFuture(watch);
    }

    ~GoFilePathWatcher() override
    {
        m_futureWatcher.disconnect();
        m_futureWatcher.cancel();
    }
};

void Client::stopWatch(int id)
{
    QMetaObject::invokeMethod(d->process, [this, id]() mutable {
        QCborMap stopWatch{{"Type", "stopwatch"}, {"Id", id}};
        d->watchers.remove(id);
        d->process->writeRaw(stopWatch.toCborValue().toCbor());
    });
}

Utils::expected_str<std::unique_ptr<FilePathWatcher>> Client::watch(const QString &path)
{
    auto jobResult = createJob<GoFilePathWatcher::Watch>(
        d.get(),
        QCborMap{{"Type", "watch"}, {"Path", path}},
        [this](QVariantMap map, QPromise<GoFilePathWatcher::Watch> &promise) {
            ASSERT_TYPE("addwatchresult");

            auto watchPromise = std::make_shared<QPromise<FilePath>>();
            QFuture<FilePath> watchFuture = watchPromise->future();
            watchPromise->start();
            auto watcherId = map.value("Id").toInt();
            d->watchers.insert(watcherId, std::move(watchPromise));

            promise.addResult(watchFuture);

            QFutureWatcher<FilePath> *watcher = new QFutureWatcher<FilePath>();
            connect(watcher, &QFutureWatcher<FilePath>::canceled, this, [this, watcherId, watcher] {
                stopWatch(watcherId);
                watcher->deleteLater();
            });
            connect(this, &QObject::destroyed, watcher, [watcher] { watcher->deleteLater(); });
            watcher->setFuture(watchFuture);
            return JobResult::Done;
        });

    if (!jobResult)
        return make_unexpected(jobResult.error());

    try {
        return std::make_unique<GoFilePathWatcher>(jobResult->result());
    } catch (const std::exception &e) {
        return make_unexpected(QString::fromUtf8(e.what()));
    }
}

Utils::expected_str<QFuture<void>> Client::signalProcess(int pid, Utils::ControlSignal signal)
{
    QString signalString;
    switch (signal) {
    case Utils::ControlSignal::Interrupt:
        signalString = "interrupt";
        break;
    case ControlSignal::Terminate:
        signalString = "terminate";
        break;
    case ControlSignal::Kill:
        signalString = "kill";
        break;
    case ControlSignal::KickOff:
        return make_unexpected(Tr::tr("Kickoff signal is not supported"));
    case ControlSignal::CloseWriteChannel:
        return make_unexpected(Tr::tr("CloseWriteChannel signal is not supported"));
    }

    return createVoidJob(
        d.get(),
        QCborMap{{"Type", "signal"}, {"signal", QCborMap{{"Pid", pid}, {"Signal", signalString}}}},
        "signalsuccess");
}

Utils::expected_str<QFuture<Client::Stat>> Client::stat(const QString &path)
{
    return createJob<Stat>(
        d.get(),
        QCborMap{{"Type", "stat"}, {"Stat", QCborMap{{"Path", path}}}},
        [](QVariantMap map, QPromise<Stat> &promise) {
            ASSERT_TYPE("statresult");

            Stat stat;
            stat.size = map.value("Size").toLongLong();
            stat.mode = map.value("Mode").toInt();
            stat.usermode = map.value("UserMode").toUInt();
            stat.modTime = QDateTime::fromSecsSinceEpoch(map.value("ModTime").toULongLong());
            stat.numHardLinks = map.value("NumHardLinks").toInt();
            stat.isDir = map.value("IsDir").toBool();

            promise.addResult(stat);

            return JobResult::Done;
        });
}

expected_str<QFuture<bool>> Client::is(const QString &path, Is is)
{
    return createJob<bool>(
        d.get(),
        QCborMap{{"Type", "is"}, {"Is", QCborMap{{"Path", path}, {"Check", static_cast<int>(is)}}}},
        [](QVariantMap map, QPromise<bool> &promise) {
            ASSERT_TYPE("isresult");

            promise.addResult(map.value("Result").toBool());

            return JobResult::Done;
        });
}

expected_str<FilePath> Client::getCmdBridgePath(
    OsType osType, OsArch osArch, const FilePath &libExecPath)
{
    static const QMap<OsType, QString> typeToString = {
        {OsType::OsTypeWindows, QStringLiteral("windows")},
        {OsType::OsTypeLinux, QStringLiteral("linux")},
        {OsType::OsTypeMac, QStringLiteral("darwin")},
        {OsType::OsTypeOtherUnix, QStringLiteral("linux")},
        {OsType::OsTypeOther, QStringLiteral("other")},
    };

    static const QMap<OsArch, QString> archToString = {
        {OsArch::OsArchX86, QStringLiteral("386")},
        {OsArch::OsArchAMD64, QStringLiteral("amd64")},
        {OsArch::OsArchArm, QStringLiteral("arm")},
        {OsArch::OsArchArm64, QStringLiteral("arm64")},
        {OsArch::OsArchUnknown, QStringLiteral("unknown")},
    };

    const QString type = typeToString.value(osType);
    const QString arch = archToString.value(osArch);

    const QString cmdBridgeName = QStringLiteral("cmdbridge-%1-%2").arg(type, arch);

    const FilePath result = libExecPath.resolvePath(cmdBridgeName);
    if (result.exists())
        return result;

    return make_unexpected(
        QString(Tr::tr("No command bridge found for architecture %1-%2")).arg(type, arch));
}

} // namespace CmdBridge
