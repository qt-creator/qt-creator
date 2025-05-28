// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filestreamer.h"

#include "async.h"
#include "qtcprocess.h"

#include <solutions/tasking/barrier.h>
#include <solutions/tasking/networkquery.h>
#include <solutions/tasking/tasktreerunner.h>

#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QWaitCondition>

namespace Utils {

using namespace Tasking;

// TODO: Adjust according to time spent on single buffer read so that it's not more than ~50 ms
// in case of local read / write. Should it be adjusted dynamically / automatically?
static const qint64 s_bufferSize = 0x1 << 20; // 1048576

class FileStreamBase : public QObject
{
    Q_OBJECT

public:
    FileStreamBase()
    {
        connect(&m_taskTreeRunner, &TaskTreeRunner::done, this, [this](DoneWith result) {
            emit done(toDoneResult(result == DoneWith::Success));
        });
    }
    void setFilePath(const FilePath &filePath) { m_filePath = filePath; }
    void start() {
        QTC_ASSERT(!m_taskTreeRunner.isRunning(), return);
        m_taskTreeRunner.start({m_filePath.isLocal() ? localTask() : remoteTask()});
    }

signals:
    void done(DoneResult result);

protected:
    FilePath m_filePath;
    TaskTreeRunner m_taskTreeRunner;

private:
    virtual GroupItem remoteTask() = 0;
    virtual GroupItem localTask() = 0;
};

static void localRead(QPromise<QByteArray> &promise, const FilePath &filePath)
{
    if (promise.isCanceled())
        return;

    QFile file(filePath.path());
    if (!file.exists()) {
        promise.future().cancel();
        return;
    }

    if (!file.open(QFile::ReadOnly)) {
        promise.future().cancel();
        return;
    }

    while (int chunkSize = qMin(s_bufferSize, file.bytesAvailable())) {
        if (promise.isCanceled())
            return;
        promise.addResult(file.read(chunkSize));
    }
}

class FileStreamReader : public FileStreamBase
{
    Q_OBJECT

signals:
    void readyRead(const QByteArray &newData);

private:
    GroupItem remoteTask() final
    {
        const QStringView scheme = m_filePath.scheme();
        if (scheme == u"http" || scheme == u"https") {
            const auto onQuerySetup = [this](NetworkQuery &query) {
                const QUrl url = m_filePath.toUrl();
                query.setRequest(QNetworkRequest(url));
                query.setNetworkAccessManager(new QNetworkAccessManager(&query));
                connect(&query, &NetworkQuery::started, this, [this, &query] {
                    connect(query.reply(), &QNetworkReply::readyRead, this, [this, &query] {
                        const QByteArray response = query.reply()->readAll();
                        emit readyRead(response);
                    });
                });
            };
            return NetworkQueryTask{onQuerySetup};
        }

        const auto setup = [this](Process &process) {
            const QStringList args = {"if=" + m_filePath.path()};
            const FilePath dd = m_filePath.withNewPath("dd");
            process.setCommand({dd, args, OsType::OsTypeLinux});
            Process *processPtr = &process;
            connect(processPtr, &Process::readyReadStandardOutput, this, [this, processPtr] {
                emit readyRead(processPtr->readAllRawStandardOutput());
            });
        };
        return ProcessTask(setup);
    }

    GroupItem localTask() final
    {
        const auto setup = [this](Async<QByteArray> &async) {
            async.setConcurrentCallData(localRead, m_filePath);
            Async<QByteArray> *asyncPtr = &async;
            connect(asyncPtr, &AsyncBase::resultReadyAt, this, [this, asyncPtr](int index) {
                emit readyRead(asyncPtr->resultAt(index));
            });
        };
        return AsyncTask<QByteArray>(setup);
    }
};

class WriteBuffer : public QObject
{
    Q_OBJECT

public:
    WriteBuffer(bool isConcurrent, QObject *parent)
        : QObject(parent)
        , m_isConcurrent(isConcurrent) {}
    struct Data {
        QByteArray m_writeData;
        bool m_closeWriteChannel = false;
        bool m_canceled = false;
        bool hasNewData() const { return m_closeWriteChannel || !m_writeData.isEmpty(); }
    };

    void write(const QByteArray &newData) {
        if (m_isConcurrent) {
            QMutexLocker locker(&m_mutex);
            QTC_ASSERT(!m_data.m_closeWriteChannel, return);
            QTC_ASSERT(!m_data.m_canceled, return);
            m_data.m_writeData += newData;
            m_waitCondition.wakeOne();
            return;
        }
        emit writeRequested(newData);
    }
    void closeWriteChannel() {
        if (m_isConcurrent) {
            QMutexLocker locker(&m_mutex);
            QTC_ASSERT(!m_data.m_canceled, return);
            m_data.m_closeWriteChannel = true;
            m_waitCondition.wakeOne();
            return;
        }
        emit closeWriteChannelRequested();
    }
    void cancel() {
        if (m_isConcurrent) {
            QMutexLocker locker(&m_mutex);
            m_data.m_canceled = true;
            m_waitCondition.wakeOne();
            return;
        }
        emit closeWriteChannelRequested();
    }
    Data waitForData() {
        QTC_ASSERT(m_isConcurrent, return {});
        QMutexLocker locker(&m_mutex);
        if (!m_data.hasNewData())
            m_waitCondition.wait(&m_mutex);
        return std::exchange(m_data, {});
    }

signals:
    void writeRequested(const QByteArray &newData);
    void closeWriteChannelRequested();

private:
    QMutex m_mutex;
    QWaitCondition m_waitCondition;
    Data m_data;
    bool m_isConcurrent = false; // Depends on whether FileStreamWriter::m_writeData is empty or not
};

static void localWrite(QPromise<void> &promise, const FilePath &filePath,
                       const QByteArray &initialData, WriteBuffer *buffer)
{
    if (promise.isCanceled())
        return;

    QFile file(filePath.path());

    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        promise.future().cancel();
        return;
    }

    if (!initialData.isEmpty()) {
        const qint64 res = file.write(initialData);
        if (res != initialData.size())
            promise.future().cancel();
        return;
    }

    while (true) {
        if (promise.isCanceled()) {
            promise.future().cancel();
            return;
        }
        const WriteBuffer::Data data = buffer->waitForData();
        if (data.m_canceled || promise.isCanceled()) {
            promise.future().cancel();
            return;
        }
        if (!data.m_writeData.isEmpty()) {
            // TODO: Write in chunks of s_bufferSize and check for promise.isCanceled()
            const qint64 res = file.write(data.m_writeData);
            if (res != data.m_writeData.size()) {
                promise.future().cancel();
                return;
            }
        }
        if (data.m_closeWriteChannel)
            return;
    }
}

class FileStreamWriter : public FileStreamBase
{
    Q_OBJECT

public:
    ~FileStreamWriter() { // TODO: should d'tor remove unfinished file write leftovers?
        if (m_writeBuffer && isBuffered())
            m_writeBuffer->cancel();
        // m_writeBuffer is a child of either Process or Async<void>. So, if m_writeBuffer
        // is still alive now (in case when TaskTree::stop() was called), the FileStreamBase
        // d'tor is going to delete m_writeBuffer later. In case of Async<void>, coming from
        // localTask(), the d'tor of Async<void>, run by FileStreamBase, busy waits for the
        // already canceled here WriteBuffer to finish before deleting WriteBuffer child.
    }

    void setWriteData(const QByteArray &writeData) {
        QTC_ASSERT(!m_taskTreeRunner.isRunning(), return);
        m_writeData = writeData;
    }
    void write(const QByteArray &newData) {
        QTC_ASSERT(m_taskTreeRunner.isRunning(), return);
        QTC_ASSERT(m_writeData.isEmpty(), return);
        QTC_ASSERT(m_writeBuffer, return);
        m_writeBuffer->write(newData);
    }
    void closeWriteChannel() {
        QTC_ASSERT(m_taskTreeRunner.isRunning(), return);
        QTC_ASSERT(m_writeData.isEmpty(), return);
        QTC_ASSERT(m_writeBuffer, return);
        m_writeBuffer->closeWriteChannel();
    }

signals:
    void started();

private:
    GroupItem remoteTask() final
    {
        const auto onSetup = [this](Process &process) {
            m_writeBuffer = new WriteBuffer(false, &process);
            connect(m_writeBuffer, &WriteBuffer::writeRequested, &process, &Process::writeRaw);
            connect(m_writeBuffer, &WriteBuffer::closeWriteChannelRequested,
                    &process, &Process::closeWriteChannel);
            const QStringList args = {"of=" + m_filePath.path()};
            const FilePath dd = m_filePath.withNewPath("dd");
            process.setCommand({dd, args, OsType::OsTypeLinux});
            if (isBuffered())
                process.setProcessMode(ProcessMode::Writer);
            else
                process.setWriteData(m_writeData);
            connect(&process, &Process::started, this, [this] { emit started(); });
        };
        const auto onDone = [this] {
            delete m_writeBuffer;
            m_writeBuffer = nullptr;
        };
        return ProcessTask(onSetup, onDone);
    }
    GroupItem localTask() final {
        const auto onSetup = [this](Async<void> &async) {
            m_writeBuffer = new WriteBuffer(isBuffered(), &async);
            async.setConcurrentCallData(localWrite, m_filePath, m_writeData, m_writeBuffer);
            emit started();
        };
        const auto onDone = [this] {
            delete m_writeBuffer;
            m_writeBuffer = nullptr;
        };
        return AsyncTask<void>(onSetup, onDone);
    }

    bool isBuffered() const { return m_writeData.isEmpty(); }
    QByteArray m_writeData;
    WriteBuffer *m_writeBuffer = nullptr;
};

using FileStreamReaderTask = SimpleCustomTask<FileStreamReader>;
using FileStreamWriterTask = SimpleCustomTask<FileStreamWriter>;

static Group sameRemoteDeviceTransferTask(const FilePath &source, const FilePath &destination)
{
    QTC_CHECK(!source.isLocal());
    QTC_CHECK(!destination.isLocal());
    QTC_CHECK(source.isSameDevice(destination));

    const auto setup = [source, destination](Process &process) {
        const QStringList args = {source.path(), destination.path()};
        const FilePath cp = source.withNewPath("cp");
        process.setCommand({cp, args, OsType::OsTypeLinux});
    };
    return {ProcessTask(setup)};
}

static Group interDeviceTransferTask(const FilePath &source, const FilePath &destination)
{
    struct TransferStorage { QPointer<FileStreamWriter> writer; };
    Storage<TransferStorage> storage;

    const auto onWriterSetup = [storage, destination](FileStreamWriter &writer) {
        writer.setFilePath(destination);
        QTC_CHECK(storage->writer == nullptr);
        storage->writer = &writer;
    };

    const auto onReaderSetup = [storage, source](FileStreamReader &reader) {
        reader.setFilePath(source);
        QTC_CHECK(storage->writer != nullptr);
        QObject::connect(&reader, &FileStreamReader::readyRead,
                         storage->writer, &FileStreamWriter::write);
    };
    const auto onReaderDone = [storage] {
        if (storage->writer) // writer may be deleted before the reader on TaskTree::stop().
            storage->writer->closeWriteChannel();
    };

    return {
        storage,
        When (FileStreamWriterTask(onWriterSetup), &FileStreamWriter::started) >> Do {
            FileStreamReaderTask(onReaderSetup, onReaderDone)
        }
    };
}

static Group transferTask(const FilePath &source, const FilePath &destination)
{
    if (!source.isLocal() && !destination.isLocal() && source.isSameDevice(destination))
        return sameRemoteDeviceTransferTask(source, destination);
    return interDeviceTransferTask(source, destination);
}

static void transfer(QPromise<void> &promise, const FilePath &source, const FilePath &destination)
{
    if (promise.isCanceled())
        return;

    if (TaskTree::runBlocking(transferTask(source, destination), promise.future()) != DoneWith::Success)
        promise.future().cancel(); // TODO: Is this needed?
}

class FileStreamerPrivate : public QObject
{
public:
    StreamMode m_streamerMode = StreamMode::Transfer;
    FilePath m_source;
    FilePath m_destination;
    QByteArray m_readBuffer;
    QByteArray m_writeBuffer;
    DoneResult m_streamResult = DoneResult::Error;
    TaskTreeRunner m_taskTreeRunner;

    GroupItem task() {
        if (m_streamerMode == StreamMode::Reader)
            return readerTask();
        if (m_streamerMode == StreamMode::Writer)
            return writerTask();
        return transferTask();
    }

private:
    GroupItem readerTask() {
        const auto setup = [this](FileStreamReader &reader) {
            m_readBuffer.clear();
            reader.setFilePath(m_source);
            connect(&reader, &FileStreamReader::readyRead, this, [this](const QByteArray &data) {
                m_readBuffer += data;
            });
        };
        return FileStreamReaderTask(setup);
    }
    GroupItem writerTask() {
        const auto setup = [this](FileStreamWriter &writer) {
            writer.setFilePath(m_destination);
            writer.setWriteData(m_writeBuffer);
        };
        return FileStreamWriterTask(setup);
    }
    GroupItem transferTask() {
        const auto setup = [this](Async<void> &async) {
            async.setConcurrentCallData(transfer, m_source, m_destination);
        };
        return AsyncTask<void>(setup);
    }
};

FileStreamer::FileStreamer(QObject *parent)
    : QObject(parent)
    , d(new FileStreamerPrivate)
{
    connect(&d->m_taskTreeRunner, &TaskTreeRunner::done, this, [this](DoneWith result) {
        d->m_streamResult = toDoneResult(result == DoneWith::Success);
        emit done(d->m_streamResult);
    });
}

FileStreamer::~FileStreamer()
{
    delete d;
}

void FileStreamer::setSource(const FilePath &source)
{
    d->m_source = source;
}

void FileStreamer::setDestination(const FilePath &destination)
{
    d->m_destination = destination;
}

void FileStreamer::setStreamMode(StreamMode mode)
{
    d->m_streamerMode = mode;
}

QByteArray FileStreamer::readData() const
{
    return d->m_readBuffer;
}

void FileStreamer::setWriteData(const QByteArray &writeData)
{
    d->m_writeBuffer = writeData;
}

DoneResult FileStreamer::result() const
{
    return d->m_streamResult;
}

void FileStreamer::start()
{
    // TODO: Preliminary check if local source exists?
    QTC_ASSERT(!d->m_taskTreeRunner.isRunning(), return);
    d->m_taskTreeRunner.start({d->task()});
}

void FileStreamer::stop()
{
    d->m_taskTreeRunner.reset();
}

} // namespace Utils

#include "filestreamer.moc"
