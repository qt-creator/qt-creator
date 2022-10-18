// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "filetransfer.h"

#include "devicemanager.h"
#include "idevice.h"

#include <utils/processinterface.h>
#include <utils/qtcassert.h>

#include <QProcess>

using namespace Utils;

namespace ProjectExplorer {

FileTransferDirection FileToTransfer::direction() const
{
    if (m_source.needsDevice() == m_target.needsDevice())
        return FileTransferDirection::Invalid;
    return m_source.needsDevice() ? FileTransferDirection::Download : FileTransferDirection::Upload;
}

QString FileTransferSetupData::defaultRsyncFlags()
{
    return "-av";
}

static FileTransferDirection transferDirection(const FilesToTransfer &files)
{
    if (files.isEmpty())
        return FileTransferDirection::Invalid;

    const FileTransferDirection direction = files.first().direction();
    for (const FileToTransfer &file : files) {
        if (file.direction() != direction)
            return FileTransferDirection::Invalid;
    }
    return direction;
}

static const FilePath &remoteFile(FileTransferDirection direction, const FileToTransfer &file)
{
    return direction == FileTransferDirection::Upload ? file.m_target : file.m_source;
}

static IDeviceConstPtr matchedDevice(FileTransferDirection direction, const FilesToTransfer &files)
{
    if (files.isEmpty())
        return {};
    const FilePath &filePath = remoteFile(direction, files.first());
    for (const FileToTransfer &file : files) {
        if (!filePath.isSameDevice(remoteFile(direction, file)))
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
    void test(const ProjectExplorer::IDeviceConstPtr &onDevice);
    void start();
    void stop();

    FileTransferSetupData m_setup;

signals:
    void progress(const QString &progressMessage);
    void done(const ProcessResultData &resultData);

private:
    void startFailed(const QString &errorString);
    void run(const FileTransferSetupData &setup, const IDeviceConstPtr &device);

    std::unique_ptr<FileTransferInterface> m_transfer;
};

void FileTransferPrivate::test(const IDeviceConstPtr &onDevice)
{
    if (!onDevice)
        return startFailed(tr("No device set for test transfer."));

    run({{}, m_setup.m_method, m_setup.m_rsyncFlags}, onDevice);
}

void FileTransferPrivate::start()
{
    if (m_setup.m_files.isEmpty())
        return startFailed(tr("No files to transfer."));

    const FileTransferDirection direction = transferDirection(m_setup.m_files);

    IDeviceConstPtr device;
    if (direction != FileTransferDirection::Invalid)
        device = matchedDevice(direction, m_setup.m_files);

    if (!device) {
        // Fall back to generic copy.
        const FilePath &filePath = m_setup.m_files.first().m_target;
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
    QTC_ASSERT(m_transfer, startFailed(tr("Missing transfer implementation.")); return);

    m_transfer->setParent(this);
    connect(m_transfer.get(), &FileTransferInterface::progress,
            this, &FileTransferPrivate::progress);
    connect(m_transfer.get(), &FileTransferInterface::done,
            this, &FileTransferPrivate::done);
    m_transfer->start();
}

void FileTransferPrivate::startFailed(const QString &errorString)
{
    emit done({0, QProcess::NormalExit, QProcess::FailedToStart, errorString});
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

void FileTransfer::test(const ProjectExplorer::IDeviceConstPtr &onDevice)
{
    d->test(onDevice);
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

QString FileTransfer::transferMethodName(FileTransferMethod method)
{
    switch (method) {
    case FileTransferMethod::Sftp:  return FileTransfer::tr("sftp");
    case FileTransferMethod::Rsync: return FileTransfer::tr("rsync");
    case FileTransferMethod::GenericCopy: return FileTransfer::tr("generic file copy");
    }
    QTC_CHECK(false);
    return {};
}

} // namespace ProjectExplorer

#include "filetransfer.moc"
