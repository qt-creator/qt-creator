/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "remotelinux_export.h"

#include <projectexplorer/devicesupport/idevicefwd.h>

#include <utils/filepath.h>

namespace Utils { class ProcessResultData; }

namespace RemoteLinux {

enum class TransferDirection {
    Upload,
    Download,
    Invalid
};

class REMOTELINUX_EXPORT FileToTransfer
{
public:
    Utils::FilePath m_source;
    Utils::FilePath m_target;
    TransferDirection transferDirection() const;
};
using FilesToTransfer = QList<FileToTransfer>;

enum class FileTransferMethod {
    Sftp,
    Rsync,
    Default = Sftp
};

class FileTransferPrivate;

class REMOTELINUX_EXPORT FileTransfer : public QObject
{
    Q_OBJECT

public:
    FileTransfer();
    ~FileTransfer();

    void setDevice(const ProjectExplorer::IDeviceConstPtr &device);
    void setTransferMethod(FileTransferMethod method);
    void setFilesToTransfer(const FilesToTransfer &files);
    void setRsyncFlags(const QString &flags);

    FileTransferMethod transferMethod() const;

    void test();
    void start();
    void stop();

    static QString transferMethodName(FileTransferMethod method);
    static QString defaultRsyncFlags();

signals:
    void progress(const QString &progressMessage);
    void done(const Utils::ProcessResultData &resultData);

private:
    FileTransferPrivate *d;
};

} // namespace RemoteLinux
