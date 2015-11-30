/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef SFTPINCOMINGPACKET_P_H
#define SFTPINCOMINGPACKET_P_H

#include "sftppacket_p.h"

namespace QSsh {
namespace Internal {

struct SftpHandleResponse {
    quint32 requestId;
    QByteArray handle;
};

struct SftpStatusResponse {
    quint32 requestId;
    SftpStatusCode status;
    QString errorString;
    QByteArray language;
};

struct SftpFileAttributes {
    bool sizePresent;
    bool timesPresent;
    bool uidAndGidPresent;
    bool permissionsPresent;
    quint64 size;
    quint32 uid;
    quint32 gid;
    quint32 permissions;
    quint32 atime;
    quint32 mtime;
};

struct SftpFile {
    QString fileName;
    QString longName; // Not present in later RFCs, so we don't expose this to the user.
    SftpFileAttributes attributes;
};

struct SftpNameResponse {
    quint32 requestId;
    QList<SftpFile> files;
};

struct SftpDataResponse {
    quint32 requestId;
    QByteArray data;
};

struct SftpAttrsResponse {
    quint32 requestId;
    SftpFileAttributes attrs;
};

class SftpIncomingPacket : public AbstractSftpPacket
{
public:
    SftpIncomingPacket();

    void consumeData(QByteArray &data);
    void clear();
    bool isComplete() const;
    quint32 extractServerVersion() const;
    SftpHandleResponse asHandleResponse() const;
    SftpStatusResponse asStatusResponse() const;
    SftpNameResponse asNameResponse() const;
    SftpDataResponse asDataResponse() const;
    SftpAttrsResponse asAttrsResponse() const;

private:
    void moveFirstBytes(QByteArray &target, QByteArray &source, int n);

    SftpFileAttributes asFileAttributes(quint32 &offset) const;
    SftpFile asFile(quint32 &offset) const;

    quint32 m_length;
};

} // namespace Internal
} // namespace QSsh

#endif // SFTPINCOMINGPACKET_P_H
