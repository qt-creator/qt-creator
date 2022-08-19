// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "../projectexplorer_export.h"
#include "filetransferinterface.h"
#include "idevicefwd.h"

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

    void test(const ProjectExplorer::IDeviceConstPtr &onDevice);
    void start();
    void stop();

    static QString transferMethodName(FileTransferMethod method);

signals:
    void progress(const QString &progressMessage);
    void done(const Utils::ProcessResultData &resultData);

private:
    FileTransferPrivate *d;
};

} // namespace ProjectExplorer
