// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filetransfer.h"

#include "devicemanager.h"
#include "idevice.h"
#include "../projectexplorertr.h"

#include <utils/async.h>
#include <utils/processinterface.h>
#include <utils/qtcassert.h>

#include <QtTaskTree/QSingleTaskTreeRunner>

#include <QProcess>

using namespace QtTaskTree;
using namespace Utils;

namespace ProjectExplorer {

static void createDir(QPromise<Result<>> &promise, const FilePath &pathToCreate)
{
    const Result<> result = pathToCreate.ensureWritableDir();
    promise.addResult(result);
    if (!result)
        promise.future().cancel();
}

static void copyFile(QPromise<Result<>> &promise, const FileToTransfer &file)
{
    const Result<> result = file.m_source.copyFile(file.m_target);
    promise.addResult(result);
    if (!result)
        promise.future().cancel();
}

class GenericTransferImpl : public FileTransferInterface
{
    QSingleTaskTreeRunner m_taskTree;

public:
    GenericTransferImpl(const FileTransferSetupData &setup)
        : FileTransferInterface(setup)
    {}

private:
    void start() final
    {
        QSet<FilePath> allParentDirs;
        for (const FileToTransfer &f : m_setup.m_files)
            allParentDirs.insert(f.m_target.parentDir());
        const ListIterator iteratorParentDirs(QList(allParentDirs.cbegin(), allParentDirs.cend()));

        const auto onCreateDirSetup = [iteratorParentDirs](Async<Result<>> &async) {
            async.setConcurrentCallData(createDir, *iteratorParentDirs);
        };
        const auto onCreateDirDone = [this, iteratorParentDirs](const Async<Result<>> &async) {
            const Result<> result = async.result();
            if (result)
                emit progress(
                    Tr::tr("Created directory: \"%1\".").arg(iteratorParentDirs->toUserOutput())
                    + "\n");
            else
                emit progress(result.error() + "\n");
        };

        const ListIterator iterator(m_setup.m_files);
        const Storage<int> counterStorage;

        const auto onCopySetup = [iterator](Async<Result<>> &async) {
            async.setConcurrentCallData(copyFile, *iterator);
        };
        const auto onCopyDone = [this, iterator, counterStorage](const Async<Result<>> &async) {
            const Result<> result = async.result();
            int &counter = *counterStorage;
            ++counter;
            if (result) {
                emit progress(Tr::tr("Copied %1/%2: \"%3\" -> \"%4\".\n")
                                  .arg(counter)
                                  .arg(m_setup.m_files.size())
                                  .arg(iterator->m_source.toUserOutput())
                                  .arg(iterator->m_target.toUserOutput()));
            } else {
                emit progress(result.error() + "\n");
            }
        };

        const Group recipe {
            For (iteratorParentDirs) >> Do {
                parallelIdealThreadCountLimit,
                AsyncTask<Result<>>(onCreateDirSetup, onCreateDirDone),
            },
            For (iterator) >> Do {
                ParallelLimit(2),
                counterStorage,
                AsyncTask<Result<>>(onCopySetup, onCopyDone),
            },
        };

        m_taskTree.start(recipe, {}, [this](DoneWith result) {
            ProcessResultData resultData;
            if (result != DoneWith::Success) {
                resultData.m_exitCode = -1;
                resultData.m_errorString = Tr::tr("Failed to deploy files.");
            }
            emit done(resultData);
        });
    }
};

FileTransferInterface *createGenericFileTransferInterface(const FileTransferSetupData &setup)
{
    return new GenericTransferImpl(setup);
}

QString FileTransferSetupData::defaultRsyncFlags()
{
    return "-rltv";
}

static IDeviceConstPtr matchedDevice(const FilesToTransfer &files)
{
    if (files.isEmpty())
        return {};
    const FilePath filePath = files.first().m_target;
    for (const FileToTransfer &file : files) {
        if (!filePath.isSameDevice(file.m_target))
            return {};
    }
    return DeviceManager::deviceForPath(filePath);
}

void FileTransferInterface::startFailed(const QString &errorString)
{
    emit done({0, QProcess::NormalExit, QProcess::FailedToStart, errorString});
}

class FileTransferPrivate : public QObject
{
    Q_OBJECT

public:
    IDeviceConstPtr m_testDevice;
    ProcessResultData m_resultData;

    void test();
    void start();
    void stop();

    FileTransferSetupData m_setup;

signals:
    void progress(const QString &progressMessage);
    void done(const ProcessResultData &resultData);

private:
    void emitDone(const ProcessResultData &resultData);
    void startFailed(const QString &errorString);
    void run(const FileTransferSetupData &setup, const IDeviceConstPtr &device);

    std::unique_ptr<FileTransferInterface> m_transfer;
};

void FileTransferPrivate::test()
{
    if (!m_testDevice)
        return startFailed(Tr::tr("No device set for test transfer."));

    run({{}, m_setup.m_method, m_setup.m_rsyncFlags}, m_testDevice);
}

void FileTransferPrivate::start()
{
    if (m_setup.m_files.isEmpty())
        return startFailed(Tr::tr("No files to transfer."));

    IDeviceConstPtr device = matchedDevice(m_setup.m_files);

    if (!device) {
        // Fall back to generic copy.
        const FilePath filePath = m_setup.m_files.first().m_target;
        device = DeviceManager::deviceForPath(filePath);
        m_setup.m_method = FileTransferMethod::GenericCopy;
    }

    run(m_setup, device);
}

void FileTransferPrivate::stop()
{
    if (!m_transfer)
        return;
    m_transfer->disconnect();
    m_transfer.release()->deleteLater();
}

void FileTransferPrivate::run(const FileTransferSetupData &setup, const IDeviceConstPtr &device)
{
    stop();

    m_transfer.reset(device->createFileTransferInterface(setup));
    QTC_ASSERT(m_transfer, startFailed(Tr::tr("Missing transfer implementation.")); return);

    m_transfer->setParent(this);
    connect(m_transfer.get(), &FileTransferInterface::progress,
            this, &FileTransferPrivate::progress);
    connect(m_transfer.get(), &FileTransferInterface::done,
            this, &FileTransferPrivate::emitDone);
    m_transfer->start();
}

void FileTransferPrivate::emitDone(const ProcessResultData &resultData)
{
    m_resultData = resultData;
    emit done(resultData);
}

void FileTransferPrivate::startFailed(const QString &errorString)
{
    emitDone({0, QProcess::NormalExit, QProcess::FailedToStart, errorString});
}

FileTransfer::FileTransfer()
    : d(new FileTransferPrivate)
{
    d->setParent(this);
    connect(d, &FileTransferPrivate::progress, this, &FileTransfer::progress);
    connect(d, &FileTransferPrivate::done, this, &FileTransfer::done);
}

FileTransfer::~FileTransfer()
{
    stop();
    delete d;
}

void FileTransfer::setFilesToTransfer(const FilesToTransfer &files)
{
    d->m_setup.m_files = files;
}

void FileTransfer::setTransferMethod(FileTransferMethod method)
{
    d->m_setup.m_method = method;
}

void FileTransfer::setRsyncFlags(const QString &flags)
{
    d->m_setup.m_rsyncFlags = flags;
}

void FileTransfer::setTestDevice(const ProjectExplorer::IDeviceConstPtr &device)
{
    d->m_testDevice = device;
}

void FileTransfer::test()
{
    d->test();
}

FileTransferMethod FileTransfer::transferMethod() const
{
    return d->m_setup.m_method;
}

void FileTransfer::start()
{
    d->start();
}

void FileTransfer::stop()
{
    d->stop();
}

ProcessResultData FileTransfer::resultData() const
{
    return d->m_resultData;
}

QString FileTransfer::transferMethodName(FileTransferMethod method)
{
    switch (method) {
    case FileTransferMethod::Sftp:  return Tr::tr("sftp");
    case FileTransferMethod::Rsync: return Tr::tr("rsync");
    case FileTransferMethod::GenericCopy: return Tr::tr("generic file copy");
    }
    QTC_CHECK(false);
    return {};
}

static void setupTransfer(FileTransfer *transfer, QTaskInterface *iface)
{
    QObject::connect(transfer, &FileTransfer::done, iface, [iface](const ProcessResultData &result) {
        const bool success = result.m_exitStatus == QProcess::NormalExit
                             && result.m_error == QProcess::UnknownError
                             && result.m_exitCode == 0;
        iface->reportDone(toDoneResult(success));
    }, Qt::SingleShotConnection);
}

void FileTransferTaskAdapter::operator()(FileTransfer *task, QTaskInterface *iface)
{
    setupTransfer(task, iface);
    task->start();
}

void FileTransferTestTaskAdapter::operator()(FileTransfer *task, QTaskInterface *iface)
{
    setupTransfer(task, iface);
    task->test();
}

} // namespace ProjectExplorer

#include "filetransfer.moc"
