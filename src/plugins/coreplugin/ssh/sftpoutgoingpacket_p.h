/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef SFTPOUTGOINGPACKET_P_H
#define SFTPOUTGOINGPACKET_P_H

#include "sftppacket_p.h"
#include "sftpdefs.h"

namespace Core {
namespace Internal {

class SftpOutgoingPacket : public AbstractSftpPacket
{
public:
    SftpOutgoingPacket();
    SftpOutgoingPacket &generateInit(quint32 version);
    SftpOutgoingPacket &generateOpenDir(const QString &path, quint32 requestId);
    SftpOutgoingPacket &generateReadDir(const QByteArray &handle,
        quint32 requestId);
    SftpOutgoingPacket &generateCloseHandle(const QByteArray &handle,
        quint32 requestId);
    SftpOutgoingPacket &generateMkDir(const QString &path, quint32 requestId);
    SftpOutgoingPacket &generateRmDir(const QString &path, quint32 requestId);
    SftpOutgoingPacket &generateRm(const QString &path, quint32 requestId);
    SftpOutgoingPacket &generateRename(const QString &oldPath,
        const QString &newPath, quint32 requestId);
    SftpOutgoingPacket &generateOpenFileForWriting(const QString &path,
         SftpOverwriteMode mode, quint32 requestId);
    SftpOutgoingPacket &generateOpenFileForReading(const QString &path,
        quint32 requestId);
    SftpOutgoingPacket &generateReadFile(const QByteArray &handle,
        quint64 offset, quint32 length, quint32 requestId);
    SftpOutgoingPacket &generateFstat(const QByteArray &handle,
        quint32 requestId);
    SftpOutgoingPacket &generateWriteFile(const QByteArray &handle,
        quint64 offset, const QByteArray &data, quint32 requestId);

private:
    static QByteArray encodeString(const QString &string);

    enum OpenType { Read, Write };
    SftpOutgoingPacket &generateOpenFile(const QString &path, OpenType openType,
        SftpOverwriteMode mode, quint32 requestId);

    SftpOutgoingPacket &init(SftpPacketType type, quint32 requestId);
    SftpOutgoingPacket &appendInt(quint32 value);
    SftpOutgoingPacket &appendInt64(quint64 value);
    SftpOutgoingPacket &appendString(const QString &string);
    SftpOutgoingPacket &appendString(const QByteArray &string);
    SftpOutgoingPacket &finalize();
};

} // namespace Internal
} // namespace Core

#endif // SFTPOUTGOINGPACKET_P_H
