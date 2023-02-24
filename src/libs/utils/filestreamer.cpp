// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filestreamer.h"

#include "asynctask.h"
#include "qtcprocess.h"

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

        const TaskItem task = m_filePath.needsDevice() ? remoteTask() : localTask();
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
    virtual TaskItem remoteTask() = 0;
    virtual TaskItem localTask() = 0;
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
    TaskItem remoteTask() final {
        const auto setup = [this](QtcProcess &process) {
            const QStringList args = {"if=" + m_filePath.path()};
            const FilePath dd = m_filePath.withNewPath("dd");
            process.setCommand({dd, args, OsType::OsTypeLinux});
            QtcProcess *processPtr = &process;
            connect(processPtr, &QtcProcess::readyReadStandardOutput, this, [this, processPtr] {
                emit readyRead(processPtr->readAllRawStandardOutput());
            });
        };
        return Process(setup);
    }
    TaskItem localTask() final {
        const auto setup = [this](AsyncTask<QByteArray> &async) {
            async.setConcurrentCallData(localRead, m_filePath);
            AsyncTask<QByteArray> *asyncPtr = &async;
            connect(asyncPtr, &AsyncTaskBase::resultReadyAt, this, [=](int index) {
                emit readyRead(asyncPtr->resultAt(index));
            });
        };
        return Async<QByteArray>(setup);
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
    TaskItem remoteTask() final {
        const auto setup = [this](QtcProcess &process) {
            m_writeBuffer = new WriteBuffer(false, &process);
            connect(m_writeBuffer, &WriteBuffer::writeRequested, &process, &QtcProcess::writeRaw);
            connect(m_writeBuffer, &WriteBuffer::closeWriteChannelRequested,
                    &process, &QtcProcess::closeWriteChannel);
            const QStringList args = {"of=" + m_filePath.path()};
            const FilePath dd = m_filePath.withNewPath("dd");
            process.setCommand({dd, args, OsType::OsTypeLinux});
            if (isBuffered())
                process.setProcessMode(ProcessMode::Writer);
            else
                process.setWriteData(m_writeData);
            connect(&process, &QtcProcess::started, this, [this] { emit started(); });
        };
        const auto finalize = [this](const QtcProcess &) {
            delete m_writeBuffer;
            m_writeBuffer = nullptr;
        };
        return Process(setup, finalize, finalize);
    }
    TaskItem localTask() final {
        const auto setup = [this](AsyncTask<void> &async) {
            m_writeBuffer = new WriteBuffer(isBuffered(), &async);
            async.setConcurrentCallData(localWrite, m_filePath, m_writeData, m_writeBuffer);
            emit started();
        };
        const auto finalize = [this](const AsyncTask<void> &) {
            delete m_writeBuffer;
            m_writeBuffer = nullptr;
        };
        return Async<void>(setup, finalize, finalize);
    }

    bool isBuffered() const { return m_writeData.isEmpty(); }
    QByteArray m_writeData;
    WriteBuffer *m_writeBuffer = nullptr;
};

class FileStreamReaderAdapter : public Utils::Tasking::TaskAdapter<FileStreamReader>
{
public:
    FileStreamReaderAdapter() { connect(task(), &FileStreamBase::done, this, &TaskInterface::done); }
    void start() override { task()->start(); }
};

class FileStreamWriterAdapter : public Utils::Tasking::TaskAdapter<FileStreamWriter>
{
public:
    FileStreamWriterAdapter() { connect(task(), &FileStreamBase::done, this, &TaskInterface::done); }
    void start() override { task()->start(); }
};

} // namespace Utils

QTC_DECLARE_CUSTOM_TASK(Reader, Utils::FileStreamReaderAdapter);
QTC_DECLARE_CUSTOM_TASK(Writer, Utils::FileStreamWriterAdapter);

namespace Utils {

static Group interDeviceTransfer(const FilePath &source, const FilePath &destination)
{
    struct TransferStorage { QPointer<FileStreamWriter> writer; };
    Condition condition;
    TreeStorage<TransferStorage> storage;

    const auto setupReader = [=](FileStreamReader &reader) {
        reader.setFilePath(source);
        QTC_CHECK(storage->writer != nullptr);
        QObject::connect(&reader, &FileStreamReader::readyRead,
                         storage->writer, &FileStreamWriter::write);
    };
    const auto finalizeReader = [=](const FileStreamReader &) {
        QTC_CHECK(storage->writer != nullptr);
        storage->writer->closeWriteChannel();
    };
    const auto setupWriter = [=](FileStreamWriter &writer) {
        writer.setFilePath(destination);
        ConditionActivator *activator = condition.activator();
        QObject::connect(&writer, &FileStreamWriter::started,
                         &writer, [activator] { activator->activate(); });
        QTC_CHECK(storage->writer == nullptr);
        storage->writer = &writer;
    };

    const Group root {
        parallel,
        Storage(storage),
        Writer(setupWriter),
        Group {
            WaitFor(condition),
            Reader(setupReader, finalizeReader, finalizeReader)
        }
    };

    return root;
}

static void transfer(QPromise<void> &promise, const FilePath &source, const FilePath &destination)
{
    if (promise.isCanceled())
        return;

    std::unique_ptr<TaskTree> taskTree(new TaskTree(interDeviceTransfer(source, destination)));

    QEventLoop eventLoop;
    bool finalized = false;
    const auto finalize = [loop = &eventLoop, &taskTree, &finalized](int exitCode) {
        if (finalized) // finalize only once
            return;
        finalized = true;
        // Give the tree a chance to delete later all tasks that have finished and caused
        // emission of tree's done or errorOccurred signal.
        // TODO: maybe these signals should be sent queued already?
        QMetaObject::invokeMethod(loop, [loop, &taskTree, exitCode] {
            taskTree.reset();
            loop->exit(exitCode);
        }, Qt::QueuedConnection);
    };
    QTimer timer;
    timer.setInterval(50);
    QObject::connect(&timer, &QTimer::timeout, [&promise, finalize] {
        if (promise.isCanceled())
            finalize(2);
    });
    QObject::connect(taskTree.get(), &TaskTree::done, &eventLoop, [=] { finalize(0); });
    QObject::connect(taskTree.get(), &TaskTree::errorOccurred, &eventLoop, [=] { finalize(1); });
    taskTree->start();
    timer.start();
    if (eventLoop.exec())
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

    TaskItem task() {
        if (m_streamerMode == StreamMode::Reader)
            return readerTask();
        if (m_streamerMode == StreamMode::Writer)
            return writerTask();
        return transferTask();
    }

private:
    TaskItem readerTask() {
        const auto setup = [this](FileStreamReader &reader) {
            m_readBuffer.clear();
            reader.setFilePath(m_source);
            connect(&reader, &FileStreamReader::readyRead, this, [this](const QByteArray &data) {
                m_readBuffer += data;
            });
        };
        return Reader(setup);
    }
    TaskItem writerTask() {
        const auto setup = [this](FileStreamWriter &writer) {
            writer.setFilePath(m_destination);
            writer.setWriteData(m_writeBuffer);
        };
        return Writer(setup);
    }
    TaskItem transferTask() {
        const auto setup = [this](AsyncTask<void> &async) {
            async.setConcurrentCallData(transfer, m_source, m_destination);
        };
        return Async<void>(setup);
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
