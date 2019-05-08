/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "sftptransfer.h"

#include "sshprocess.h"
#include "sshsettings.h"

#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QTemporaryFile>
#include <QTimer>

#include <utils/algorithm.h>
#include <utils/qtcprocess.h>

using namespace Utils;

namespace QSsh {

struct SftpTransfer::SftpTransferPrivate
{
    SshProcess sftpProc;
    FilesToTransfer files;
    Internal::FileTransferType transferType;
    FileTransferErrorHandling errorHandlingMode;
    QStringList connectionArgs;
    QTemporaryFile batchFile;

    QStringList dirsToCreate() const
    {
        QStringList dirs;
        for (const FileToTransfer &f : qAsConst(files)) {
            QString parentDir = QFileInfo(f.targetFile).path();
            while (true) {
                if (dirs.contains(parentDir) || !parentDir.startsWith('/'))
                    break;
                dirs << parentDir;
                parentDir = QFileInfo(parentDir).path();
            }
        }
        sort(dirs, [](const QString &d1, const QString &d2) {
            if (d1 == "/" && d2 != "/")
                return true;
            return d1.count('/') < d2.count('/');
        });
        return dirs;
    }
    QByteArray transferCommand(bool link) const
    {
        QByteArray command;
        switch (transferType) {
        case Internal::FileTransferType::Upload:
            command = link ? "ln -s" : "put";
            break;
        case Internal::FileTransferType::Download:
            command = "get";
            break;
        }
        if (errorHandlingMode == FileTransferErrorHandling::Ignore)
            command.prepend('-');
        return command;
    }
};

SftpTransfer::~SftpTransfer()
{
    delete d;
}

void SftpTransfer::start()
{
    QTimer::singleShot(0, this, &SftpTransfer::doStart);
}

void SftpTransfer::stop()
{
    d->sftpProc.terminate();
}

SftpTransfer::SftpTransfer(const FilesToTransfer &files, Internal::FileTransferType type,
                           FileTransferErrorHandling errorHandlingMode,
                           const QStringList &connectionArgs)
    : d(new SftpTransferPrivate)
{
    d->files = files;
    d->transferType = type;
    d->errorHandlingMode = errorHandlingMode;
    d->connectionArgs = connectionArgs;
    connect(&d->sftpProc, &QProcess::errorOccurred, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
            emitError(tr("sftp failed to start: %1").arg(d->sftpProc.errorString()));
    });
    connect(&d->sftpProc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [this] {
        if (d->sftpProc.exitStatus() != QProcess::NormalExit) {
            emitError(tr("sftp crashed."));
            return;
        }
        if (d->sftpProc.exitCode() != 0) {
            emitError(QString::fromLocal8Bit(d->sftpProc.readAllStandardError()));
            return;
        }
        emit done(QString());
    });
    connect(&d->sftpProc, &QProcess::readyReadStandardOutput, [this] {
        emit progress(QString::fromLocal8Bit(d->sftpProc.readAllStandardOutput()));
    });
}

void SftpTransfer::doStart()
{
    const FileName sftpBinary = SshSettings::sftpFilePath();
    if (!sftpBinary.exists()) {
        emitError(tr("sftp binary \"%1\" does not exist.").arg(sftpBinary.toUserOutput()));
        return;
    }
    if (!d->batchFile.isOpen() && !d->batchFile.open()) {
        emitError(tr("Could not create temporary file: %1").arg(d->batchFile.errorString()));
        return;
    }
    d->batchFile.resize(0);
    for (const QString &dir : d->dirsToCreate()) {
        switch (d->transferType) {
        case Internal::FileTransferType::Upload:
            d->batchFile.write("-mkdir " + QtcProcess::quoteArgUnix(dir).toLocal8Bit() + '\n');
            break;
        case Internal::FileTransferType::Download:
            if (!QDir::root().mkdir(dir)) {
                emitError(tr("Failed to create local directory \"%1\".")
                          .arg(QDir::toNativeSeparators(dir)));
                return;
            }
            break;
        }
    }
    for (const FileToTransfer &f : d->files) {
        QString sourceFileOrLinkTarget;
        bool link = false;
        if (d->transferType == Internal::FileTransferType::Upload) {
            QFileInfo fi(f.sourceFile);
            if (fi.isSymLink()) {
                link = true;
                d->batchFile.write("-rm " + QtcProcess::quoteArgUnix(f.targetFile).toLocal8Bit()
                                   + '\n');
                sourceFileOrLinkTarget = fi.dir().relativeFilePath(fi.symLinkTarget()); // see QTBUG-5817.
            } else {
                sourceFileOrLinkTarget = f.sourceFile;
            }
         }
         d->batchFile.write(d->transferCommand(link) + ' '
                            + QtcProcess::quoteArgUnix(sourceFileOrLinkTarget).toLocal8Bit() + ' '
                            + QtcProcess::quoteArgUnix(f.targetFile).toLocal8Bit() + '\n');
    }
    d->batchFile.flush();
    d->sftpProc.start(sftpBinary.toString(),
                      QStringList{"-b", d->batchFile.fileName()} << d->connectionArgs);
    emit started();
}

void SftpTransfer::emitError(const QString &details)
{
    emit done(tr("File transfer failed: %1").arg(details));
}

} // namespace QSsh
