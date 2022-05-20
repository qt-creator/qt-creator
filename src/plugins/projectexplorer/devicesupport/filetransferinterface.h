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

#include "../projectexplorer_export.h"

#include <utils/filepath.h>

namespace Utils { class ProcessResultData; }

namespace ProjectExplorer {

enum class FileTransferDirection {
    Invalid,
    Upload,
    Download
};

enum class FileTransferMethod {
    Sftp,
    Rsync,
    Default = Sftp
};

class PROJECTEXPLORER_EXPORT FileToTransfer
{
public:
    Utils::FilePath m_source;
    Utils::FilePath m_target;

    FileTransferDirection direction() const;
};

using FilesToTransfer = QList<FileToTransfer>;

class PROJECTEXPLORER_EXPORT FileTransferSetupData
{
public:
    FilesToTransfer m_files; // When empty, do test instead of a real transfer
    FileTransferMethod m_method = FileTransferMethod::Default;
    QString m_rsyncFlags = defaultRsyncFlags();

    static QString defaultRsyncFlags();
};

class PROJECTEXPLORER_EXPORT FileTransferInterface : public QObject
{
    Q_OBJECT

signals:
    void progress(const QString &progressMessage);
    void done(const Utils::ProcessResultData &resultData);

protected:
    FileTransferInterface(const FileTransferSetupData &setupData)
        : m_setup(setupData) {}

    void startFailed(const QString &errorString);

    const FileTransferSetupData m_setup;

private:
    FileTransferInterface() = delete;

    virtual void start() = 0;

    friend class FileTransferPrivate;
};

} // namespace ProjectExplorer
