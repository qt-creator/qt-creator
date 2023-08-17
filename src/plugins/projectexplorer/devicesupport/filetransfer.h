// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../projectexplorer_export.h"
#include "filetransferinterface.h"
#include "idevicefwd.h"

#include <solutions/tasking/tasktree.h>

namespace Utils { class ProcessResultData; }

namespace ProjectExplorer {

class FileTransferPrivate;

class PROJECTEXPLORER_EXPORT FileTransfer : public QObject
{
    Q_OBJECT

public:
    FileTransfer();
    ~FileTransfer();

    void setFilesToTransfer(const FilesToTransfer &files);
    void setTransferMethod(FileTransferMethod method);
    void setRsyncFlags(const QString &flags);

    FileTransferMethod transferMethod() const;

    void setTestDevice(const ProjectExplorer::IDeviceConstPtr &device);
    void test();
    void start();
    void stop();

    Utils::ProcessResultData resultData() const;

    static QString transferMethodName(FileTransferMethod method);

signals:
    void progress(const QString &progressMessage);
    void done(const Utils::ProcessResultData &resultData);

private:
    FileTransferPrivate *d;
};

class PROJECTEXPLORER_EXPORT FileTransferTaskAdapter : public Tasking::TaskAdapter<FileTransfer>
{
public:
    FileTransferTaskAdapter();
    void start() override { task()->start(); }
};

class PROJECTEXPLORER_EXPORT FileTransferTestTaskAdapter : public FileTransferTaskAdapter
{
public:
    void start() final { task()->test(); }
};

using FileTransferTask = Tasking::CustomTask<FileTransferTaskAdapter>;
using FileTransferTestTask = Tasking::CustomTask<FileTransferTestTaskAdapter>;

} // namespace ProjectExplorer
