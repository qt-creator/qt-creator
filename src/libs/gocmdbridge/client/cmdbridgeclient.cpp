// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmdbridgeclient.h"

#include "cmdbridgetr.h"

#include <utils/environment.h>
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
#include <QTimer>

#include <atomic>

static Q_LOGGING_CATEGORY(clientLog, "qtc.cmdbridge.client", QtWarningMsg)

#define ASSERT_TYPE(expectedtype) \
    if (map.value("Type").toString() != expectedtype) { \
        const QString err = QString("Unexpected result type: %1, expected: %2") \
                                .arg(map.value("Type").toString(), expectedtype); \
        promise.setException(std::make_exception_ptr(std::runtime_error(err.toStdString()))); \
        promise.finish(); \
        return JobResult::Done; \
    }

using namespace Utils;

namespace CmdBridge {

enum class JobResult {
    Continue,
    Done,
};

// A bridge that is still there exits within milliseconds. One that is not (because the
// device is gone or the connection died) must not stall the teardown, so give up early.
static constexpr std::chrono::seconds shutdownTimeout{2};
// Long enough for two of those, and for a thread that is merely slow to get there.
static constexpr std::chrono::seconds threadStopTimeout{10};

namespace Internal {

struct ClientPrivate
{
    FilePath remoteCmdBridgePath;
    QByteArray startMarker;
    Environment environment;

    // Only access from the thread
    Process *process = nullptr;
    QThread *thread = nullptr;
    QTimer *watchDogTimer = nullptr;
    bool shuttingDown = false;
    // Set by ~Client when it gives up on the thread and leaves this behind. Read on
    // that thread, written on the one destroying the client.
    std::atomic<bool> clientGone = false;

    // The handlers are shared, not copied, when taken out of the map: some of them
    // keep state across the packets of one job (see Client::find).
    using JobHandler = std::shared_ptr<std::function<JobResult(QVariantMap)>>;

    struct Jobs
    {
        int nextId = 0;
        QMap<int, JobHandler> map;
    };

    Utils::SynchronizedValue<Jobs> jobs;

    struct Watcher
    {
        std::shared_ptr<QPromise<FilePath>> promise;
        // The device-qualified path passed to watch(). Reported back on every
        // change so, like the local watcher, a directory watch always emits the
        // watched directory (not the individual child that changed).
        FilePath watchedPath;
    };
    QMap<int, Watcher> watchers;
    QMap<int, std::shared_ptr<QPromise<Client::SocketServerEvent>>> socketServerForwards;

    Result<> readPacket(QCborStreamReader &reader);
    std::optional<Result<>> handleWatchResults(const QVariantMap &map);
    std::optional<Result<>> handleSocketResults(const QVariantMap &map);

    // The following three run in the thread that owns the process.
    void concludePendingJobs(const QString &error, bool normalExit);
    void handleProcessDone();
    void shutdown();
};

static QString decodeString(QCborStreamReader &reader)
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

static QByteArray decodeByteArray(QCborStreamReader &reader)
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

static QVariant simpleToVariant(QCborSimpleType s)
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

static QVariant decodeArray(QCborStreamReader &reader)
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

std::optional<Result<>> ClientPrivate::handleWatchResults(const QVariantMap &map)
{
    const QString type = map.value("Type").toString();
    if (type == "watchEvent") {
        auto id = map.value("Id").toInt();
        auto it = watchers.find(id);

        if (it == watchers.end())
            return ResultError(QString("No watcher found for id %1").arg(id));

        const Watcher &watcher = it.value();
        // Ignore the changed path the bridge reports (the individual child for a
        // directory watch) and emit the watched path, matching the local
        // watcher's contract. The stored path is already device-qualified.
        if (!watcher.promise->isCanceled())
            watcher.promise->addResult(watcher.watchedPath);

        return Result<>{};
    } else if (type == "removewatchresult") {
        auto id = map.value("Id").toInt();
        watchers.remove(id);
        return Result<>{};
    }

    return std::nullopt;
}

std::optional<Result<>> ClientPrivate::handleSocketResults(const QVariantMap &map)
{
    const QString type = map.value("Type").toString();

    auto addEvent = [&](int id, Client::SocketServerEvent event) -> std::optional<Result<>> {
        const auto it = socketServerForwards.find(id);
        if (it == socketServerForwards.end())
            return Result<>{}; // Already torn down - ignore late events from Go.
        const auto promise = it.value();
        if (!promise->isCanceled())
            promise->addResult(std::move(event));
        return Result<>{};
    };

    if (type == "socketconnect") {
        const int connId = map.value("ConnId").toInt();
        return addEvent(map.value("Id").toInt(), Client::SocketServerConnect{connId});
    }

    if (type == "socketdata") {
        const int connId = map.value("ConnId").toInt();
        const QByteArray bytes = map.value("Data").toByteArray();
        return addEvent(map.value("Id").toInt(), Client::SocketServerData{connId, bytes});
    }

    if (type == "socketclose") {
        const int connId = map.value("ConnId").toInt();
        const auto id = map.value("Id").toInt();
        auto result = addEvent(id, Client::SocketServerClose{connId});
        // The server keeps running (more clients may connect later), so we do
        // NOT finish the promise or erase it from the map here.
        return result;
    }

    if (type == "forwardserverstopped") {
        const auto id = map.value("Id").toInt();
        const auto it = socketServerForwards.find(id);
        if (it != socketServerForwards.end()) {
            it.value()->finish();
            socketServerForwards.erase(it);
        }
        return Result<>{};
    }

    return std::nullopt;
}

Result<> ClientPrivate::readPacket(QCborStreamReader &reader)
{
    if (!reader.enterContainer())
        return ResultError(QString("The packet did not contain a container"));

    Q_ASSERT(QThread::currentThread() == thread);

    QVariantMap map;

    while (reader.lastError() == QCborError::NoError && reader.hasNext()) {
        auto key = reader.type() == QCborStreamReader::String ? decodeString(reader) : QString();
        map.insert(key, readVariant(reader));
    }

    if (!reader.leaveContainer())
        return ResultError(QString("The packet did not contain a finalized map"));

    if (!map.contains("Id")) {
        return ResultError(QString("The packet did not contain an Id"));
    }

    auto watchHandled = handleWatchResults(map);
    if (watchHandled)
        return *watchHandled;

    auto socketHandled = handleSocketResults(map);
    if (socketHandled)
        return *socketHandled;

    auto id = map.value("Id").toInt();

    // Drop the lock before running the handler. The handler completes a promise,
    // which can end up back here on this thread, and taking the write lock then
    // would deadlock against our own read lock. The handler is kept alive by the
    // shared pointer, so removing it from the map meanwhile is fine.
    JobHandler job;
    {
        auto j = jobs.readLocked();
        auto it = j->map.find(id);
        if (it == j->map.end())
            return ResultError(
                QString("No job found for packet with id %1: %2")
                    .arg(id)
                    .arg(QString::fromUtf8(QJsonDocument::fromVariant(map).toJson())));
        job = it.value();
    }

    if ((*job)(map) == JobResult::Done)
        jobs.writeLocked()->map.remove(id);

    return {};
}

void ClientPrivate::concludePendingJobs(const QString &error, bool normalExit)
{
    // Take the handlers out first and run them without the lock, as they
    // complete promises, which can end up in readPacket() on this thread.
    QList<std::pair<int, JobHandler>> pending;
    {
        auto j = jobs.writeLocked();
        for (auto it = j->map.cbegin(); it != j->map.cend(); ++it)
            pending.append({it.key(), it.value()});
        j->map.clear();
    }

    for (const auto &[id, func] : pending) {
        (*func)(QVariantMap{
            {"Type", "error"},
            {"Id", id},
            {"Error", error},
            {"ErrorType", (normalExit ? "NormalExit" : "ErrorExit")}});
    }

    // Finish any outstanding socket forward promises so that
    // QFutureWatcher/QFuture waiters do not block forever.
    for (auto &promise : socketServerForwards)
        promise->finish();
    socketServerForwards.clear();
}

void ClientPrivate::handleProcessDone()
{
    if (!shuttingDown && process->resultData().m_exitCode != 0) {
        qCWarning(clientLog) << "Process exited with error code:"
                             << process->resultData().m_exitCode
                             << "Error:" << process->errorString()
                             << "StandardError:" << process->readAllStandardError()
                             << "StandardOutput:" << process->readAllStandardOutput();
    }

    // Nothing is going to be sent or answered anymore. The timer is already gone if the
    // process finished during the teardown.
    if (watchDogTimer)
        watchDogTimer->stop();
    concludePendingJobs(
        Tr::tr("Process exited: %1").arg(process->errorString()), process->exitCode() == 0);
}

void ClientPrivate::shutdown()
{
    QTC_ASSERT(QThread::currentThread() == thread, return);

    // Whatever happens to the process from here on, we asked for it.
    shuttingDown = true;
    delete watchDogTimer;
    watchDogTimer = nullptr;

    if (process->isRunning()) {
        // The server answers "exit" by exiting, so the process going away is the only
        // reply there is - and waiting for it is also what gets the request written.
        process->writeRaw(QCborMap{{"Type", "exit"}, {"Id", -1}}.toCborValue().toCbor());
        if (!process->waitForFinished(shutdownTimeout)) {
            process->stop();
            process->waitForFinished(shutdownTimeout);
        }
    }

    // Whether the server heard us or not, from here on nothing can conclude the jobs.
    concludePendingJobs(Tr::tr("The bridge was shut down."), true);

    delete process;
    process = nullptr;
}

} // namespace Internal

Client::Client(const Utils::FilePath &remoteCmdBridgePath, const Utils::Environment &env)
    : d(new Internal::ClientPrivate())
{
    d->remoteCmdBridgePath = remoteCmdBridgePath;
    d->environment = env;
}

Client::~Client()
{
    if (!d->thread)
        return; // Never started.

    // The bridge is torn down from QThread::finished, so we cannot be that thread.
    QTC_ASSERT(QThread::currentThread() != d->thread, return);

    // Stopping the event loop makes the thread tear the bridge down, see start(). A
    // queued call would do that too, but only while there is somebody delivering it,
    // and by the time the last device goes away there may not be.
    d->thread->quit();

    if (d->thread->wait(threadStopTimeout)) {
        delete d->thread;
        d->thread = nullptr;
        return;
    }

    // Whatever is stuck in there must not be destroyed while it runs, and neither must
    // the state it is still working on. Leave both to the exiting process; the bridge
    // has a watchdog and exits by itself. The handlers work on the released data, but
    // this object is gone, so tell them to stop reporting to it.
    qCWarning(clientLog) << "Bridge thread did not stop, leaving it behind.";
    d->clientGone = true;
    (void) d.release();
}

void Client::setStartMarker(const QByteArray &marker)
{
    d->startMarker = marker;
}

Result<> Client::start(bool deleteOnExit)
{
    d->thread = new QThread;
    d->thread->setObjectName("CmdBridgeClientThread");
    d->thread->start();

    d->process = new Process();
    d->process->moveToThread(d->thread);

    d->watchDogTimer = new QTimer();
    d->watchDogTimer->setInterval(1000);
    d->watchDogTimer->moveToThread(d->thread);

    // Tear the bridge down in the thread that owns the process, once its event loop has
    // been stopped by ~Client. Being called from the signal, and not through an event,
    // is what makes the teardown work even when nobody delivers those anymore.
    // Neither of these may go through the client: it can be destroyed while the thread
    // is still running, in which case the data below outlives it, see ~Client.
    Internal::ClientPrivate *p = d.get();
    connect(
        d->thread, &QThread::finished, d->process, [p] { p->shutdown(); }, Qt::DirectConnection);

    connect(d->watchDogTimer, &QTimer::timeout, d->process, [p] {
        QTC_ASSERT(p->process, return);
        QCborMap args;
        args.insert(QString("Id"), -1);
        args.insert(QString("Type"), QString("ping"));
        p->process->writeRaw(args.toCborValue().toCbor());
    });
    connect(d->process, &Process::started, d->watchDogTimer, qOverload<>(&QTimer::start));

    Result<> result = ResultOk;

    QMetaObject::invokeMethod(
        d->process,
        [this, p, deleteOnExit]() -> Result<> {
            QStringList args;
            if (deleteOnExit)
                args << "-deleteOnStart";
            // Allow overriding the server-side watchdog timeout for debugging.
            // A value of 0 disables the watchdog entirely.
            const QString watchdogTimeout
                = Utils::qtcEnvironmentVariable("QTC_CMDBRIDGE_WATCHDOG_TIMEOUT");
            if (!watchdogTimeout.isEmpty())
                args << "-watchdogTimeout" << watchdogTimeout;
            if (!d->startMarker.isEmpty()) {
                args << "-pidMarker" << QString::fromUtf8(d->startMarker);
                d->process->setExtraData("Ssh.TargetReportsPid", true);
            }
            d->process->setCommand({d->remoteCmdBridgePath, args});
            d->process->setEnvironment(d->environment);
            d->process->setProcessMode(ProcessMode::Writer);
            d->process->setProcessChannelMode(ProcessChannelMode::SeparateChannels);
            // Make sure the process has a codec, otherwise it will try to ask us recursively
            // and dead lock.
            d->process->setUtf8Codec();

            // The process is kept alive and the thread keeps running until the client is
            // destroyed, so that the teardown always finds both of them where it left them.
            connect(d->process, &Process::done, d->process, [this, p] {
                p->handleProcessDone();
                // Only the signal needs the client, and it may be gone by now.
                if (!p->clientGone)
                    emit done(p->process->resultData());
            });

            auto stateMachine =
                [markerOffset = 0, state = int(0), packetSize = qint32(0), packetData = QByteArray(), p](
                    QByteArray &buffer) mutable -> bool {
                    static const QByteArray MagicCode{GOBRIDGE_MAGIC_PACKET_MARKER};

                    if (state == 0) {
                        const auto offsetMagicCode = MagicCode.mid(markerOffset);
                        int start = buffer.indexOf(offsetMagicCode);
                        if (start == -1) {
                            if (buffer.size() < offsetMagicCode.size()
                                && offsetMagicCode.startsWith(buffer)) {
                                // Partial magic marker?
                                markerOffset += buffer.size();
                                buffer.clear();
                                return false;
                            }
                            // Broken package, search for next magic marker
                            qCWarning(clientLog)
                                << "Magic marker was not found, buffer content:" << buffer;
                            // If we don't find a magic marker, the rest of the buffer is trash.
                            buffer.clear();
                        } else {
                            buffer.remove(0, start + offsetMagicCode.size());
                            state = 1;
                        }
                        markerOffset = 0;
                    }

                    if (state == 1) {
                        if (buffer.size() < 4)
                            return false; // wait for more data
                        QDataStream ds(buffer);
                        ds >> packetSize;
                        // Socket-data packets can carry up to 32 KiB of payload plus
                        // CBOR map overhead; file-read results can be larger still.
                        // 64 MiB is a safe upper bound that allows all legitimate
                        // traffic while still catching obviously corrupted size fields.
                        static constexpr qint32 maxPacketSize = 64 * 1024 * 1024;
                        if (packetSize > 0 && packetSize <= maxPacketSize) {
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
                            auto result = p->readPacket(reader);
                            QTC_CHECK_RESULT(result);
                        }
                    }
                    return !buffer.isEmpty();
                };

            connect(
                d->process,
                &Process::readyReadStandardError,
                d->process,
                [p, buffer = QByteArray(), stateMachine]() mutable {
                    buffer.append(p->process->readAllRawStandardError());
                    while (stateMachine(buffer)) {}
                });

            connect(d->process, &Process::readyReadStandardOutput, d->process, [p] {
                qCWarning(clientLog).noquote() << p->process->readAllStandardOutput();
            });

            d->process->start();

            if (!d->process)
                return ResultError(Tr::tr("Cannot start bridge process."));

            if (!d->process->waitForStarted())
                return ResultError(
                    Tr::tr("Cannot start bridge process: %1").arg(d->process->errorString()));
            // waitForStarted() is also satisfied by a done signal arriving instead
            // of a started one, so it says nothing on its own about a bridge that
            // exited immediately - which is what one that cannot be executed does.
            if (!d->process->isRunning()) {
                return ResultError(
                    Tr::tr("Bridge process exited immediately: %1")
                        .arg(d->process->errorString()));
            }
            return ResultOk;
        },
        Qt::BlockingQueuedConnection,
        &result);

    return result;
}

enum class Errors {
    Handle,
    DontHandle,
};

template<class R>
static Utils::Result<QFuture<R>> createJob(
    Internal::ClientPrivate *d,
    QCborMap args,
    const std::function<JobResult(QVariantMap map, QPromise<R> &promise)> &resultFunc,
    Errors handleErrors = Errors::Handle)
{
    if (!d->process || !d->process->isRunning())
        return ResultError(Tr::tr("The bridge process is not running."));

    std::shared_ptr<QPromise<R>> promise = std::make_shared<QPromise<R>>();
    QFuture<R> future = promise->future();

    promise->start();

    auto j = d->jobs.writeLocked();
    int id = j->nextId++;
    auto handler = [handleErrors, promise, resultFunc](QVariantMap map) {
        QString type = map.value("Type").toString();

        if (handleErrors == Errors::Handle && type == "error") {
            const QString err = map.value("Error", QString{}).toString();
            const QString errType = map.value("ErrorType", QString{}).toString();
            const int errNo = map.value("Errno", -1).toInt();

            if (errType == "Errno") {
                promise->setException(
                    std::make_exception_ptr(std::system_error(errNo, std::generic_category())));
                promise->finish();
            } else if (errType == "NormalExit") {
                promise->setException(std::make_exception_ptr(std::runtime_error("NormalExit")));
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
    };
    j->map.insert(id,
                  std::make_shared<std::function<JobResult(QVariantMap)>>(std::move(handler)));

    args.insert(QString("Id"), id);

    QMetaObject::invokeMethod(
        d->process,
        [d, args]() {
            QTC_ASSERT(d->process, return);
            d->process->writeRaw(args.toCborValue().toCbor());
        },
        Qt::QueuedConnection);

    // Proactively tell the server to abort the job the moment the consumer
    // cancels the future, independent of any traffic. Reacting to canceled only
    // while a packet is being processed would never fire for a job that streams
    // nothing (e.g. a find whose name filters exclude everything), leaving the
    // server to walk the whole tree to completion. The job is kept registered
    // until its terminal packet arrives, so in-flight packets never hit an empty
    // job map.
    //
    // The watcher is created in the bridge thread (next to d->process) so its
    // signals are delivered there and writeRaw runs on the right thread. It is
    // parented to d->process and deletes itself once the future finishes, so it
    // never outlives the client.
    QMetaObject::invokeMethod(
        d->process,
        [d, id, future]() {
            QTC_ASSERT(d->process, return);
            auto *watcher = new QFutureWatcher<R>(d->process);
            QObject::connect(watcher, &QFutureWatcherBase::canceled, d->process, [d, id] {
                QTC_ASSERT(d->process, return);
                const QCborMap cancel{{"Type", "cancel"}, {"Id", id}};
                d->process->writeRaw(cancel.toCborValue().toCbor());
            });
            QObject::connect(watcher, &QFutureWatcherBase::finished, watcher, [watcher] {
                watcher->deleteLater();
            });
            watcher->setFuture(future);
        },
        Qt::QueuedConnection);

    return future;
}

static Utils::Result<QFuture<void>> createVoidJob(
    Internal::ClientPrivate *d, const QCborMap &args, const QString &resulttype)
{
    return createJob<void>(d, args, [resulttype](QVariantMap map, QPromise<void> &promise) {
        ASSERT_TYPE(resulttype);
        promise.finish();
        return JobResult::Done;
    });
}

Result<QFuture<Client::ExecResult>> Client::execute(
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

Result<QFuture<Client::FindData>> Client::find(
    const QString &directory, const Utils::FileFilter &filter)
{
    // TODO: golang's walkDir does not support automatically following symlinks.
    if ((filter.iteratorFlags & DirIteratorFlag::FollowSymlinks) != DirIteratorFlag{})
        return ResultError(Tr::tr("FollowSymlinks is not supported."));

    QCborMap findArgs{
        {"Type", "find"},
        {"Find",
         QCborMap{
             {"Directory", directory},
             {"FileFilters", static_cast<int>(filter.fileFilters)},
             {"NameFilters", QCborArray::fromStringList(filter.nameFilters)},
             {"IteratorFlags", static_cast<int>(filter.iteratorFlags)}}}};

    return createJob<FindData>(
        d.get(),
        findArgs,
        [hasEntries = false,
         cache = QList<FindData>()](QVariantMap map, QPromise<FindData> &promise) mutable {
            QString type = map.value("Type").toString();

            // After cancellation the server has been told to stop (by the
            // future watcher in createJob). Discard whatever is still streaming
            // in and finish the job when its terminal "findend"/"error" packet
            // arrives.
            if (promise.isCanceled())
                return type == "finddata" ? JobResult::Continue : JobResult::Done;

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
                    promise.addResults(cache);
                    cache.clear();
                }
                return JobResult::Continue;
            } else if (type == "error") {
                hasEntries = true;
                promise.addResult(make_unexpected(map.value("Error", QString{}).toString()));
                return JobResult::Done;
            }

            if (cache.size() > 0)
                promise.addResults(cache);
            else if (!hasEntries)
                promise.addResult(make_unexpected(std::nullopt));

            return JobResult::Done;
        },
        Errors::DontHandle);
}

Utils::Result<QFuture<QString>> Client::readlink(const QString &path)
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

Utils::Result<QFuture<QString>> Client::fileId(const QString &path)
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

Utils::Result<QFuture<quint64>> Client::freeSpace(const QString &path)
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

Utils::Result<QFuture<QByteArray>> Client::readFile(
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

Utils::Result<QFuture<qint64>> Client::writeFile(
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

Utils::Result<QFuture<void>> Client::removeFile(const QString &path)
{
    return createVoidJob(d.get(), QCborMap{{"Type", "remove"}, {"Path", path}}, "removeresult");
}

Utils::Result<QFuture<void>> Client::removeRecursively(const QString &path)
{
    return createVoidJob(d.get(), QCborMap{{"Type", "removeall"}, {"Path", path}}, "removeallresult");
}

Utils::Result<QFuture<void>> Client::ensureExistingFile(const QString &path)
{
    return createVoidJob(
        d.get(),
        QCborMap{{"Type", "ensureexistingfile"}, {"Path", path}},
        "ensureexistingfileresult");
}

Utils::Result<QFuture<void>> Client::createDir(const QString &path)
{
    return createVoidJob(d.get(), QCborMap{{"Type", "createdir"}, {"Path", path}}, "createdirresult");
}

Utils::Result<QFuture<void>> Client::copyFile(const QString &source, const QString &target)
{
    return createVoidJob(
        d.get(),
        QCborMap{
            {"Type", "copyfile"},
            {"CopyFile", QCborMap{{"Source", source}, {"Target", target}}},
        },
        "copyfileresult");
}

Utils::Result<QFuture<void>> Client::createSymLink(const QString &source, const QString &symLink)
{
    return createVoidJob(
        d.get(),
        QCborMap{
                 {"Type", "createsymlink"},
                 {"CreateSymLink", QCborMap{{"Source", source}, {"SymLink", symLink}}},
                 },
        "createsymlinkresult");
}

Utils::Result<QFuture<void>> Client::renameFile(const QString &source, const QString &target)
{
    return createVoidJob(
        d.get(),
        QCborMap{
            {"Type", "renamefile"},
            {"RenameFile", QCborMap{{"Source", source}, {"Target", target}}},
        },
        "renamefileresult");
}

Utils::Result<QFuture<FilePath>> Client::createTempFile(const QString &path)
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

Utils::Result<QFuture<FilePath>> Client::createTempDir(const QString &path)
{
    return createJob<FilePath>(
        d.get(),
        QCborMap{{"Type", "createtempdir"}, {"Path", path}},
        [](QVariantMap map, QPromise<FilePath> &promise) {
            ASSERT_TYPE("createtempdirresult");

            promise.addResult(FilePath::fromUserInput(map.value("Path").toString()));

            return JobResult::Done;
        });
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

Utils::Result<QFuture<void>> Client::setPermissions(
    const QString &path, QFile::Permissions perms)
{
    int p = toUnixChmod(perms);

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
        QTC_ASSERT(d->process, return);
        QCborMap stopWatch{{"Type", "stopwatch"}, {"Id", id}};
        d->watchers.remove(id);
        d->process->writeRaw(stopWatch.toCborValue().toCbor());
    });
}

Utils::Result<std::unique_ptr<FilePathWatcher>> Client::watch(const FilePath &path)
{
    auto jobResult = createJob<GoFilePathWatcher::Watch>(
        d.get(),
        QCborMap{{"Type", "watch"}, {"Path", path.nativePath()}},
        [this, path](QVariantMap map, QPromise<GoFilePathWatcher::Watch> &promise) {
            ASSERT_TYPE("addwatchresult");

            auto watchPromise = std::make_shared<QPromise<FilePath>>();
            QFuture<FilePath> watchFuture = watchPromise->future();
            watchPromise->start();
            auto watcherId = map.value("Id").toInt();
            // Report back the exact FilePath that was watched, so the change
            // notifications carry the same path the caller passed in.
            d->watchers.insert(watcherId, {std::move(watchPromise), path});

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
        return ResultError(jobResult.error());

    try {
        return std::make_unique<GoFilePathWatcher>(jobResult->result());
    } catch (const std::exception &e) {
        return ResultError(QString::fromUtf8(e.what()));
    }
}

Utils::Result<QFuture<void>> Client::signalProcess(int pid, Utils::ControlSignal signal)
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
        return ResultError(Tr::tr("The KickOff signal is not supported."));
    case ControlSignal::CloseWriteChannel:
        return ResultError(Tr::tr("The CloseWriteChannel signal is not supported."));
    }

    return createVoidJob(
        d.get(),
        QCborMap{{"Type", "signal"}, {"signal", QCborMap{{"Pid", pid}, {"Signal", signalString}}}},
        "signalsuccess");
}

Result<QFuture<QString>> Client::owner(const QString &path)
{
    return createJob<QString>(
        d.get(),
        QCborMap{{"Type", "owner"}, {"Path", path}},
        [](QVariantMap map, QPromise<QString> &promise) {
            ASSERT_TYPE("ownerresult");

            promise.addResult(map.value("Owner").toString());

            return JobResult::Done;
        });
}

Result<QFuture<uint>> Client::ownerId(const QString &path)
{
    return createJob<uint>(
        d.get(),
        QCborMap{{"Type", "ownerid"}, {"Path", path}},
        [](QVariantMap map, QPromise<uint> &promise) {
            ASSERT_TYPE("owneridresult");

            promise.addResult(uint(map.value("OwnerId").toInt()));

            return JobResult::Done;
        });
}

Result<QFuture<QString>> Client::group(const QString &path)
{
    return createJob<QString>(
        d.get(),
        QCborMap{{"Type", "group"}, {"Path", path}},
        [](QVariantMap map, QPromise<QString> &promise) {
            ASSERT_TYPE("groupresult");

            promise.addResult(map.value("Group").toString());

            return JobResult::Done;
        });
}

Result<QFuture<uint>> Client::groupId(const QString &path)
{
    return createJob<uint>(
        d.get(),
        QCborMap{{"Type", "groupid"}, {"Path", path}},
        [](QVariantMap map, QPromise<uint> &promise) {
            ASSERT_TYPE("groupidresult");

            promise.addResult(uint(map.value("GroupId").toInt()));

            return JobResult::Done;
        });
}

Result<QFuture<bool>> Client::isSameFile(const QString &path1, const QString &path2)
{
    return createJob<bool>(
        d.get(),
        QCborMap{{"Type", "issamefile"}, {"IsSameFile", QCborMap{{"Path1", path1}, {"Path2", path2}}}},
        [](QVariantMap map, QPromise<bool> &promise) {
            ASSERT_TYPE("issamefileresult");

            promise.addResult(map.value("Result").toBool());

            return JobResult::Done;
        });
}

Utils::Result<Client::SocketServerForward> Client::forwardSocketServer()
{
    auto jobResult = createJob<SocketServerForward>(
        d.get(),
        QCborMap{{"Type", "forwardlocalsocketserver"}},
        [this](QVariantMap map, QPromise<SocketServerForward> &promise) {
            ASSERT_TYPE("forwardlocalsocketserverready");

            const auto id = map.value("Id").toInt();
            const QString remotePath = map.value("Path").toString();

            auto eventPromise = std::make_shared<QPromise<SocketServerEvent>>();
            eventPromise->start();
            QFuture<SocketServerEvent> eventFuture = eventPromise->future();
            d->socketServerForwards.insert(id, std::move(eventPromise));

            promise.addResult(SocketServerForward{id, remotePath, eventFuture});
            return JobResult::Done;
        });

    if (!jobResult)
        return ResultError(jobResult.error());

    try {
        return jobResult->result();
    } catch (const std::exception &e) {
        return ResultError(QString::fromUtf8(e.what()));
    }
}

void Client::sendSocketData(int id, int connId, const QByteArray &data)
{
    QMetaObject::invokeMethod(d->process, [this, id, connId, data]() {
        QTC_ASSERT(d->process, return);
        QCborMap msg{
            {"Type", "socketdata"},
            {"Id", id},
            {"ConnId", connId},
            {"SocketData", QCborMap{{"Data", data}}}};
        d->process->writeRaw(msg.toCborValue().toCbor());
    });
}

void Client::sendSocketClose(int id, int connId)
{
    QMetaObject::invokeMethod(d->process, [this, id, connId]() {
        QTC_ASSERT(d->process, return);
        QCborMap msg{{"Type", "socketclose"}, {"Id", id}, {"ConnId", connId}};
        d->process->writeRaw(msg.toCborValue().toCbor());
    });
}

void Client::sendSocketStopForward(int id)
{
    QMetaObject::invokeMethod(d->process, [this, id]() {
        QTC_ASSERT(d->process, return);
        QCborMap msg{{"Type", "stopforwardserver"}, {"Id", id}};
        d->process->writeRaw(msg.toCborValue().toCbor());
        // Do NOT erase socketServerForwards[id] here. Go will send a
        // "forwardserverstopped" packet only after all in-flight socketdata /
        // socketclose packets have been written to the output channel, so
        // handleSocketResults will erase the entry when that ack arrives.
    });
}


Utils::Result<QFuture<Client::Stat>> Client::stat(const QString &path)
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

Result<QFuture<bool>> Client::is(const QString &path, Is is)
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

Result<FilePath> Client::getCmdBridgePath(
    OsType osType, OsArch osArch, const FilePath &libExecPath)
{
    static const QMap<OsType, QString> typeToString = {
        {OsType::OsTypeWindows, QStringLiteral("windows")},
        {OsType::OsTypeLinux, QStringLiteral("linux")},
        {OsType::OsTypeMac, QStringLiteral("darwin")},
        {OsType::OsTypeFreeBSD, QStringLiteral("freebsd")},
        {OsType::OsTypeOpenBSD, QStringLiteral("openbsd")},
        {OsType::OsTypeNetBSD, QStringLiteral("netbsd")},
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

    QString cmdBridgeName = QStringLiteral("cmdbridge-%1-%2").arg(type, arch);

    if (osType == OsType::OsTypeWindows)
        cmdBridgeName += QStringLiteral(".exe");

    const FilePath result = libExecPath.resolvePath(cmdBridgeName);
    if (result.exists())
        return result;

    return ResultError(
        QString(Tr::tr("No command bridge available for architecture \"%1-%2\".")).arg(type, arch));
}

} // namespace CmdBridge
