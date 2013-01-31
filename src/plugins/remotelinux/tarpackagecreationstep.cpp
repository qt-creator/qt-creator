/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/
#include "tarpackagecreationstep.h"

#include "remotelinuxdeployconfiguration.h"

#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <cstring>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace {

class CreateTarStepWidget : public SimpleBuildStepConfigWidget
{
    Q_OBJECT
public:
    CreateTarStepWidget(TarPackageCreationStep *step) : SimpleBuildStepConfigWidget(step)
    {
        connect(step, SIGNAL(packageFilePathChanged()), SIGNAL(updateSummary()));
    }

    QString summaryText() const
    {
        TarPackageCreationStep * const step = static_cast<TarPackageCreationStep *>(this->step());
        if (step->packageFilePath().isEmpty()) {
            return QLatin1String("<font color=\"red\">")
                + tr("Tarball creation not possible.") + QLatin1String("</font>");
        }
        return QLatin1String("<b>") + tr("Create tarball:") + QLatin1String("</b> ")
            + step->packageFilePath();
    }
};


const int TarBlockSize = 512;
struct TarFileHeader {
    char fileName[100];
    char fileMode[8];
    char uid[8];
    char gid[8];
    char length[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char fileNamePrefix[155];
    char padding[12];
};

} // Anonymous namespace.

TarPackageCreationStep::TarPackageCreationStep(BuildStepList *bsl)
    : AbstractPackagingStep(bsl, stepId())
{
    ctor();
}

TarPackageCreationStep::TarPackageCreationStep(BuildStepList *bsl, TarPackageCreationStep *other)
    : AbstractPackagingStep(bsl, other)
{
    ctor();
}

void TarPackageCreationStep::ctor()
{
    setDefaultDisplayName(displayName());
}

bool TarPackageCreationStep::init()
{
    if (!AbstractPackagingStep::init())
        return false;
    m_packagingNeeded = isPackagingNeeded();
    if (m_packagingNeeded)
        m_files = target()->deploymentData().allFiles();
    return true;
}

void TarPackageCreationStep::run(QFutureInterface<bool> &fi)
{
    setPackagingStarted();
    const bool success = doPackage(fi);
    setPackagingFinished(success);
    if (success)
        emit addOutput(tr("Packaging finished successfully."), MessageOutput);
    else
        emit addOutput(tr("Packaging failed."), ErrorMessageOutput);
    fi.reportResult(success);
}

bool TarPackageCreationStep::doPackage(QFutureInterface<bool> &fi)
{
    emit addOutput(tr("Creating tarball..."), MessageOutput);
    if (!m_packagingNeeded) {
        emit addOutput(tr("Tarball up to date, skipping packaging."), MessageOutput);
        return true;
    }

    // TODO: Optimization: Only package changed files
    QFile tarFile(cachedPackageFilePath());

    if (!tarFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        raiseError(tr("Error: tar file %1 cannot be opened (%2).")
            .arg(QDir::toNativeSeparators(cachedPackageFilePath()), tarFile.errorString()));
        return false;
    }

    foreach (const DeployableFile &d, m_files) {
        if (d.remoteDirectory().isEmpty()) {
            emit addOutput(tr("No remote path specified for file '%1', skipping.")
                .arg(d.localFilePath().toUserOutput()), ErrorMessageOutput);
            continue;
        }
        QFileInfo fileInfo = d.localFilePath().toFileInfo();
        if (!appendFile(tarFile, fileInfo, d.remoteDirectory() + QLatin1Char('/')
                + fileInfo.fileName(), fi)) {
            return false;
        }
    }

    const QByteArray eofIndicator(2*sizeof(TarFileHeader), 0);
    if (tarFile.write(eofIndicator) != eofIndicator.length()) {
        raiseError(tr("Error writing tar file '%1': %2.")
            .arg(QDir::toNativeSeparators(tarFile.fileName()), tarFile.errorString()));
        return false;
    }

    return true;
}

bool TarPackageCreationStep::appendFile(QFile &tarFile, const QFileInfo &fileInfo,
    const QString &remoteFilePath, const QFutureInterface<bool> &fi)
{
    if (!writeHeader(tarFile, fileInfo, remoteFilePath))
        return false;
    if (fileInfo.isDir()) {
        QDir dir(fileInfo.absoluteFilePath());
        foreach (const QString &fileName,
                 dir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot)) {
            const QString thisLocalFilePath = dir.path() + QLatin1Char('/') + fileName;
            const QString thisRemoteFilePath  = remoteFilePath + QLatin1Char('/') + fileName;
            if (!appendFile(tarFile, QFileInfo(thisLocalFilePath), thisRemoteFilePath, fi))
                return false;
        }
        return true;
    }

    const QString nativePath = QDir::toNativeSeparators(fileInfo.filePath());
    QFile file(fileInfo.filePath());
    if (!file.open(QIODevice::ReadOnly)) {
        raiseError(tr("Error reading file '%1': %2.").arg(nativePath, file.errorString()));
        return false;
    }

    const int chunkSize = 1024*1024;

    emit addOutput(tr("Adding file '%1' to tarball...").arg(nativePath), MessageOutput);

    // TODO: Wasteful. Work with fixed-size buffer.
    while (!file.atEnd() && !file.error() != QFile::NoError && !tarFile.error() != QFile::NoError) {
        const QByteArray data = file.read(chunkSize);
        tarFile.write(data);
        if (fi.isCanceled())
            return false;
    }
    if (file.error() != QFile::NoError) {
        raiseError(tr("Error reading file '%1': %2.").arg(nativePath, file.errorString()));
        return false;
    }

    const int blockModulo = file.size() % TarBlockSize;
    if (blockModulo != 0)
        tarFile.write(QByteArray(TarBlockSize - blockModulo, 0));

    if (tarFile.error() != QFile::NoError) {
        raiseError(tr("Error writing tar file '%1': %2.")
            .arg(QDir::toNativeSeparators(tarFile.fileName()), tarFile.errorString()));
        return false;
    }
    return true;
}

bool TarPackageCreationStep::writeHeader(QFile &tarFile, const QFileInfo &fileInfo,
    const QString &remoteFilePath)
{
    TarFileHeader header;
    std::memset(&header, '\0', sizeof header);
    const QByteArray &filePath = remoteFilePath.toUtf8();
    const int maxFilePathLength = sizeof header.fileNamePrefix + sizeof header.fileName;
    if (filePath.count() > maxFilePathLength) {
        raiseError(tr("Cannot add file '%1' to tar-archive: path too long.")
            .arg(QDir::toNativeSeparators(remoteFilePath)));
        return false;
    }

    const int fileNameBytesToWrite = qMin<int>(filePath.length(), sizeof header.fileName);
    const int fileNameOffset = filePath.length() - fileNameBytesToWrite;
    std::memcpy(&header.fileName, filePath.data() + fileNameOffset, fileNameBytesToWrite);
    if (fileNameOffset > 0)
        std::memcpy(&header.fileNamePrefix, filePath.data(), fileNameOffset);
    int permissions = (0400 * fileInfo.permission(QFile::ReadOwner))
        | (0200 * fileInfo.permission(QFile::WriteOwner))
        | (0100 * fileInfo.permission(QFile::ExeOwner))
        | (040 * fileInfo.permission(QFile::ReadGroup))
        | (020 * fileInfo.permission(QFile::WriteGroup))
        | (010 * fileInfo.permission(QFile::ExeGroup))
        | (04 * fileInfo.permission(QFile::ReadOther))
        | (02 * fileInfo.permission(QFile::WriteOther))
        | (01 * fileInfo.permission(QFile::ExeOther));
    const QByteArray permissionString = QString::fromLatin1("%1").arg(permissions,
        sizeof header.fileMode - 1, 8, QLatin1Char('0')).toLatin1();
    std::memcpy(&header.fileMode, permissionString.data(), permissionString.length());
    const QByteArray uidString = QString::fromLatin1("%1").arg(fileInfo.ownerId(),
        sizeof header.uid - 1, 8, QLatin1Char('0')).toLatin1();
    std::memcpy(&header.uid, uidString.data(), uidString.length());
    const QByteArray gidString = QString::fromLatin1("%1").arg(fileInfo.groupId(),
        sizeof header.gid - 1, 8, QLatin1Char('0')).toLatin1();
    std::memcpy(&header.gid, gidString.data(), gidString.length());
    const QByteArray sizeString = QString::fromLatin1("%1").arg(fileInfo.size(),
        sizeof header.length - 1, 8, QLatin1Char('0')).toLatin1();
    std::memcpy(&header.length, sizeString.data(), sizeString.length());
    const QByteArray mtimeString = QString::fromLatin1("%1").arg(fileInfo.lastModified().toTime_t(),
        sizeof header.mtime - 1, 8, QLatin1Char('0')).toLatin1();
    std::memcpy(&header.mtime, mtimeString.data(), mtimeString.length());
    if (fileInfo.isDir())
        header.typeflag = '5';
    std::memcpy(&header.magic, "ustar", sizeof "ustar");
    std::memcpy(&header.version, "00", 2);
    const QByteArray &owner = fileInfo.owner().toUtf8();
    std::memcpy(&header.uname, owner.data(), qMin<int>(owner.length(), sizeof header.uname - 1));
    const QByteArray &group = fileInfo.group().toUtf8();
    std::memcpy(&header.gname, group.data(), qMin<int>(group.length(), sizeof header.gname - 1));
    std::memset(&header.chksum, ' ', sizeof header.chksum);
    quint64 checksum = 0;
    for (size_t i = 0; i < sizeof header; ++i)
        checksum += reinterpret_cast<char *>(&header)[i];
    const QByteArray checksumString = QString::fromLatin1("%1").arg(checksum,
        sizeof header.chksum - 1, 8, QLatin1Char('0')).toLatin1();
    std::memcpy(&header.chksum, checksumString.data(), checksumString.length());
    header.chksum[sizeof header.chksum-1] = 0;
    if (!tarFile.write(reinterpret_cast<char *>(&header), sizeof header)) {
        raiseError(tr("Error writing tar file '%1': %2")
           .arg(QDir::toNativeSeparators(cachedPackageFilePath()), tarFile.errorString()));
        return false;
    }
    return true;
}

QString TarPackageCreationStep::packageFileName() const
{
    return project()->displayName() + QLatin1String(".tar");
}

BuildStepConfigWidget *TarPackageCreationStep::createConfigWidget()
{
    return new CreateTarStepWidget(this);
}

Core::Id TarPackageCreationStep::stepId()
{
    return Core::Id("MaemoTarPackageCreationStep");
}

QString TarPackageCreationStep::displayName()
{
    return tr("Create tarball");
}

} // namespace RemoteLinux

#include "tarpackagecreationstep.moc"
