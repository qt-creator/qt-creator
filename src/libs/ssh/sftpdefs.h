/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "ssh_global.h"

#include <QFile>
#include <QList>
#include <QString>

#include <memory>

namespace QSsh {

class SftpTransfer;
using SftpTransferPtr = std::unique_ptr<SftpTransfer>;
class SftpSession;
using SftpSessionPtr = std::unique_ptr<SftpSession>;

enum class FileTransferErrorHandling { Abort, Ignore };

class FileToTransfer
{
public:
    FileToTransfer(const QString &source, const QString &target)
        : sourceFile(source), targetFile(target) {}
    QString sourceFile;
    QString targetFile;
};
using FilesToTransfer = QList<FileToTransfer>;

namespace Internal { enum class FileTransferType { Upload, Download }; }

typedef quint32 SftpJobId;
QSSH_EXPORT extern const SftpJobId SftpInvalidJob;

enum SftpOverwriteMode {
    SftpOverwriteExisting, SftpAppendToExisting, SftpSkipExisting
};

enum SftpFileType { FileTypeRegular, FileTypeDirectory, FileTypeOther, FileTypeUnknown };

class QSSH_EXPORT SftpFileInfo
{
public:
    QString name;
    SftpFileType type = FileTypeUnknown;
    quint64 size = 0;
    QFile::Permissions permissions;
};

} // namespace QSsh
