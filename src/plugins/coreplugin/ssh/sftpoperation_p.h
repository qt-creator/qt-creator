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

#ifndef SFTPOPERATION_P_H
#define SFTPOPERATION_P_H

#include "sftpdefs.h"

#include <QtCore/QByteArray>
#include <QtCore/QMap>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE
class QFile;
QT_END_NAMESPACE

namespace Core {
namespace Internal {

class SftpOutgoingPacket;

struct AbstractSftpOperation
{
    typedef QSharedPointer<AbstractSftpOperation> Ptr;
    enum Type {
        ListDir, MakeDir, RmDir, Rm, Rename, CreateFile, Download, Upload
    };

    AbstractSftpOperation(SftpJobId jobId);
    virtual ~AbstractSftpOperation();
    virtual Type type() const=0;
    virtual SftpOutgoingPacket &initialPacket(SftpOutgoingPacket &packet)=0;

    const SftpJobId jobId;

private:
    AbstractSftpOperation(const AbstractSftpOperation &);
    AbstractSftpOperation &operator=(const AbstractSftpOperation &);
};

struct SftpMakeDir : public AbstractSftpOperation
{
    typedef QSharedPointer<SftpMakeDir> Ptr;

    SftpMakeDir(SftpJobId jobId, const QString &path);
    virtual Type type() const { return MakeDir; }
    virtual SftpOutgoingPacket &initialPacket(SftpOutgoingPacket &packet);

    const QString remoteDir;
};

struct SftpRmDir : public AbstractSftpOperation
{
    typedef QSharedPointer<SftpRmDir> Ptr;

    SftpRmDir(SftpJobId jobId, const QString &path);
    virtual Type type() const { return RmDir; }
    virtual SftpOutgoingPacket &initialPacket(SftpOutgoingPacket &packet);

    const QString remoteDir;
};

struct SftpRm : public AbstractSftpOperation
{
    typedef QSharedPointer<SftpRm> Ptr;

    SftpRm(SftpJobId jobId, const QString &path);
    virtual Type type() const { return Rm; }
    virtual SftpOutgoingPacket &initialPacket(SftpOutgoingPacket &packet);

    const QString remoteFile;
};

struct SftpRename : public AbstractSftpOperation
{
    typedef QSharedPointer<SftpRename> Ptr;

    SftpRename(SftpJobId jobId, const QString &oldPath, const QString &newPath);
    virtual Type type() const { return Rename; }
    virtual SftpOutgoingPacket &initialPacket(SftpOutgoingPacket &packet);

    const QString oldPath;
    const QString newPath;
};


struct AbstractSftpOperationWithHandle : public AbstractSftpOperation
{
    typedef QSharedPointer<AbstractSftpOperationWithHandle> Ptr;
    enum State { Inactive, OpenRequested, Open, CloseRequested };

    AbstractSftpOperationWithHandle(SftpJobId jobId, const QString &remotePath);
    ~AbstractSftpOperationWithHandle();

    const QString remotePath;
    QByteArray remoteHandle;
    State state;
    bool hasError;
};


struct SftpListDir : public AbstractSftpOperationWithHandle
{
    typedef QSharedPointer<SftpListDir> Ptr;

    SftpListDir(SftpJobId jobId, const QString &path);
    virtual Type type() const { return ListDir; }
    virtual SftpOutgoingPacket &initialPacket(SftpOutgoingPacket &packet);
};


struct SftpCreateFile : public AbstractSftpOperationWithHandle
{
    typedef QSharedPointer<SftpCreateFile> Ptr;

    SftpCreateFile(SftpJobId jobId, const QString &path, SftpOverwriteMode mode);
    virtual Type type() const { return CreateFile; }
    virtual SftpOutgoingPacket &initialPacket(SftpOutgoingPacket &packet);

    const SftpOverwriteMode mode;
};

struct AbstractSftpTransfer : public AbstractSftpOperationWithHandle
{
    typedef QSharedPointer<AbstractSftpTransfer> Ptr;

    AbstractSftpTransfer(SftpJobId jobId, const QString &remotePath,
        const QSharedPointer<QFile> &localFile);
    void calculateInFlightCount(quint32 chunkSize);

    static const int MaxInFlightCount;

    const QSharedPointer<QFile> localFile;
    quint64 fileSize;
    quint64 offset;
    int inFlightCount;
    bool statRequested;
};

struct SftpDownload : public AbstractSftpTransfer
{
    typedef QSharedPointer<SftpDownload> Ptr;
    SftpDownload(SftpJobId jobId, const QString &remotePath,
        const QSharedPointer<QFile> &localFile);
    virtual Type type() const { return Download; }
    virtual SftpOutgoingPacket &initialPacket(SftpOutgoingPacket &packet);

    QMap<quint32, quint32> offsets;
    SftpJobId eofId;
};

struct SftpUpload : public AbstractSftpTransfer
{
    typedef QSharedPointer<SftpUpload> Ptr;

    SftpUpload(SftpJobId jobId, const QString &remotePath,
        const QSharedPointer<QFile> &localFile, SftpOverwriteMode mode);
    virtual Type type() const { return Upload; }
    virtual SftpOutgoingPacket &initialPacket(SftpOutgoingPacket &packet);

    SftpOverwriteMode mode;
};

} // namespace Internal
} // namespace Core

#endif // SFTPOPERATION_P_H
