/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef SFTPDEFS_H
#define SFTPDEFS_H

#include "ssh_global.h"

#include <QFile>
#include <QString>

namespace QSsh {

typedef quint32 SftpJobId;
QSSH_EXPORT extern const SftpJobId SftpInvalidJob;

enum SftpOverwriteMode {
    SftpOverwriteExisting, SftpAppendToExisting, SftpSkipExisting
};

enum SftpFileType { FileTypeRegular, FileTypeDirectory, FileTypeOther, FileTypeUnknown };

class QSSH_EXPORT SftpFileInfo
{
public:
    SftpFileInfo() : type(FileTypeUnknown), sizeValid(false), permissionsValid(false) { }

    QString name;
    SftpFileType type;
    quint64 size;
    QFile::Permissions permissions;

    // The RFC allows an SFTP server not to support any file attributes beyond the name.
    bool sizeValid;
    bool permissionsValid;
};

} // namespace QSsh

#endif // SFTPDEFS_H
