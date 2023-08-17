// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filestreamer.h"

#include "async.h"
#include "process.h"

#include <solutions/tasking/barrier.h>

#include <QFile>
#include <QMutex>
#include <QMutexLocker>
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
    void setFilePath(const FilePath &filePath) { m_filePath = filePath; }
    void start() {
        QTC_ASSERT(!m_taskTree, return);

        const GroupItem task = m_filePath.needsDevice() ? remoteTask() : localTask();
        m_taskTree.reset(new TaskTree({task}));
        const auto finalize = [this](bool success) {
            m_taskTree.release()->deleteLater();
            emit done(success);
        };
        connect(m_taskTree.get(), &TaskTree::done, this, [=] { finalize(true); });
        connect(m_taskTree.get(), &TaskTree::errorOccurred, this, [=] { finalize(false); });
        m_taskTree->start();
    }

signals:
    void done(bool success);

protected:
    FilePath m_filePath;
    std::unique_ptr<TaskTree> m_taskTree;

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
    GroupItem remoteTask() final {
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
    GroupItem localTask() final {
        const auto setup = [this](Async<QByteArray> &async) {
            async.setConcurrentCallData(localRead, m_filePath);
            Async<QByteArray> *asyncPtr = &async;
            connect(asyncPtr, &AsyncBase::resultReadyAt, this, [=](int index) {
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
        QTC_ASSERT(!m_taskTree, return);
        m_writeData = writeData;
    }
    void write(const QByteArray &newData) {
        QTC_ASSERT(m_taskTree, return);
        QTC_ASSERT(m_writeData.isEmpty(), return);
        QTC_ASSERT(m_writeBuffer, return);
        m_writeBuffer->write(newData);
    }
    void closeWriteChannel() {
        QTC_ASSERT(m_taskTree, return);
        QTC_ASSERT(m_writeData.isEmpty(), return);
        QTC_ASSERT(m_writeBuffer, return);
        m_writeBuffer->closeWriteChannel();
    }

signals:
    void started();

private:
    GroupItem remoteTask() final {
        const auto setup = [this](Process &process) {
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
        const auto finalize = [this](const Process &) {
            delete m_writeBuffer;
            m_writeBuffer = nullptr;
        };
        return ProcessTask(setup, finalize, finalize);
    }
    GroupItem localTask() final {
        const auto setup = [this](Async<void> &async) {
            m_writeBuffer = new WriteBuffer(isBuffered(), &async);
            async.setConcurrentCallData(localWrite, m_filePath, m_writeData, m_writeBuffer);
            emit started();
        };
        const auto finalize = [this](const Async<void> &) {
            delete m_writeBuffer;
            m_writeBuffer = nullptr;
        };
        return AsyncTask<void>(setup, finalize, finalize);
    }

    bool isBuffered() const { return m_writeData.isEmpty(); }
    QByteArray m_writeData;
    WriteBuffer *m_writeBuffer = nullptr;
};

class FileStreamReaderAdapter : public TaskAdapter<FileStreamReader>
{
public:
    FileStreamReaderAdapter() { connect(task(), &FileStreamBase::done, this, &TaskInterface::done); }
    void start() override { task()->start(); }
};

class FileStreamWriterAdapter : public TaskAdapter<FileStreamWriter>
{
public:
    FileStreamWriterAdapter() { connect(task(), &FileStreamBase::done, this, &TaskInterface::done); }
    void start() override { task()->start(); }
};

using FileStreamReaderTask = CustomTask<FileStreamReaderAdapter>;
using FileStreamWriterTask = CustomTask<FileStreamWriterAdapter>;

static Group sameRemoteDeviceTransferTask(const FilePath &source, const FilePath &destination)
{
    QTC_CHECK(source.needsDevice());
    QTC_CHECK(destination.needsDevice());
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
    SingleBarrier writerReadyBarrier;
    TreeStorage<TransferStorage> storage;

    const auto setupReader = [=](FileStreamReader &reader) {
        reader.setFilePath(source);
        QTC_CHECK(storage->writer != nullptr);
        QObject::connect(&reader, &FileStreamReader::readyRead,
                         storage->writer, &FileStreamWriter::write);
    };
    const auto finalizeReader = [=](const FileStreamReader &) {
        if (storage->writer) // writer may be deleted before the reader on TaskTree::stop().
            storage->writer->closeWriteChannel();
    };
    const auto setupWriter = [=](FileStreamWriter &writer) {
        writer.setFilePath(destination);
        QObject::connect(&writer, &FileStreamWriter::started,
                         writerReadyBarrier->barrier(), &Barrier::advance);
        QTC_CHECK(storage->writer == nullptr);
        storage->writer = &writer;
    };

    const Group root {
        Storage(writerReadyBarrier),
        parallel,
        Storage(storage),
        FileStreamWriterTask(setupWriter),
        Group {
            waitForBarrierTask(writerReadyBarrier),
            FileStreamReaderTask(setupReader, finalizeReader, finalizeReader)
        }
    };

    return root;
}

static Group transferTask(const FilePath &source, const FilePath &destination)
{
    if (source.needsDevice() && destination.needsDevice() && source.isSameDevice(destination))
        return sameRemoteDeviceTransferTask(source, destination);
    return interDeviceTransferTask(source, destination);
}

static void transfer(QPromise<void> &promise, const FilePath &source, const FilePath &destination)
{
    if (promise.isCanceled())
        return;

    if (!TaskTree::runBlocking(transferTask(source, destination), promise.future()))
        promise.future().cancel();
}

class FileStreamerPrivate : public QObject
{
public:
    StreamMode m_streamerMode = StreamMode::Transfer;
    FilePath m_source;
    FilePath m_destination;
    QByteArray m_readBuffer;
    QByteArray m_writeBuffer;
    StreamResult m_streamResult = StreamResult::FinishedWithError;
    std::unique_ptr<TaskTree> m_taskTree;

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

StreamResult FileStreamer::result() const
{
    return d->m_streamResult;
}

void FileStreamer::start()
{
    // TODO: Preliminary check if local source exists?
    QTC_ASSERT(!d->m_taskTree, return);
    d->m_taskTree.reset(new TaskTree({d->task()}));
    const auto finalize = [this](bool success) {
        d->m_streamResult = success ? StreamResult::FinishedWithSuccess
                                    : StreamResult::FinishedWithError;
        d->m_taskTree.release()->deleteLater();
        emit done();
    };
    connect(d->m_taskTree.get(), &TaskTree::done, this, [=] { finalize(true); });
    connect(d->m_taskTree.get(), &TaskTree::errorOccurred, this, [=] { finalize(false); });
    d->m_taskTree->start();
}

void FileStreamer::stop()
{
    d->m_taskTree.reset();
}

} // namespace Utils

#include "filestreamer.moc"
