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

#include "tarpackagecreationstep.h"

#include "deploymenttimeinfo.h"
#include "remotelinux_constants.h"

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <cstring>

using namespace ProjectExplorer;
using namespace Utils;

namespace RemoteLinux {

const char IgnoreMissingFilesKey[] = "RemoteLinux.TarPackageCreationStep.IgnoreMissingFiles";
const char IncrementalDeploymentKey[] = "RemoteLinux.TarPackageCreationStep.IncrementalDeployment";

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

namespace Internal {

class TarPackageCreationStepPrivate
{
public:
    FilePath m_cachedPackageFilePath;
    bool m_deploymentDataModified = false;
    DeploymentTimeInfo m_deployTimes;
    BoolAspect *m_incrementalDeploymentAspect = nullptr;
    BoolAspect *m_ignoreMissingFilesAspect = nullptr;
    bool m_packagingNeeded = false;
    QList<DeployableFile> m_files;
};

} // namespace Internal

TarPackageCreationStep::TarPackageCreationStep(BuildStepList *bsl, Id id)
    : BuildStep(bsl, id)
    , d(new Internal::TarPackageCreationStepPrivate)
{
    connect(target(), &Target::deploymentDataChanged, this, [this] {
        d->m_deploymentDataModified = true;
    });
    d->m_deploymentDataModified = true;

    d->m_ignoreMissingFilesAspect = addAspect<BoolAspect>();
    d->m_ignoreMissingFilesAspect->setLabel(tr("Ignore missing files"),
                                         BoolAspect::LabelPlacement::AtCheckBox);
    d->m_ignoreMissingFilesAspect->setSettingsKey(IgnoreMissingFilesKey);

    d->m_incrementalDeploymentAspect = addAspect<BoolAspect>();
    d->m_incrementalDeploymentAspect->setLabel(tr("Package modified files only"),
                                            BoolAspect::LabelPlacement::AtCheckBox);
    d->m_incrementalDeploymentAspect->setSettingsKey(IncrementalDeploymentKey);

    setSummaryUpdater([this] {
        FilePath path = packageFilePath();
        if (path.isEmpty())
            return QString("<font color=\"red\">" + tr("Tarball creation not possible.")
                           + "</font>");
        return QString("<b>" + tr("Create tarball:") + "</b> " + path.toUserOutput());
    });
}

TarPackageCreationStep::~TarPackageCreationStep() = default;

Utils::Id TarPackageCreationStep::stepId()
{
    return Constants::TarPackageCreationStepId;
}

QString TarPackageCreationStep::displayName()
{
    return tr("Create tarball");
}

FilePath TarPackageCreationStep::packageFilePath() const
{
    if (buildDirectory().isEmpty())
        return {};
    const QString packageFileName = project()->displayName() + QLatin1String(".tar");
    return buildDirectory().pathAppended(packageFileName);
}

bool TarPackageCreationStep::init()
{
    d->m_cachedPackageFilePath = packageFilePath();
    d->m_packagingNeeded = isPackagingNeeded();
    return true;
}

void TarPackageCreationStep::doRun()
{
    runInThread([this] { return runImpl(); });
}

bool TarPackageCreationStep::fromMap(const QVariantMap &map)
{
    if (!BuildStep::fromMap(map))
        return false;
    d->m_deployTimes.importDeployTimes(map);
    return true;
}

QVariantMap TarPackageCreationStep::toMap() const
{
    QVariantMap map = BuildStep::toMap();
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    map.insert(d->m_deployTimes.exportDeployTimes());
#else
    map.unite(d->m_deployTimes.exportDeployTimes());
#endif
    return map;
}

void TarPackageCreationStep::raiseError(const QString &errorMessage)
{
    emit addTask(DeploymentTask(Task::Error, errorMessage));
    emit addOutput(errorMessage, BuildStep::OutputFormat::Stderr);
}

void TarPackageCreationStep::raiseWarning(const QString &warningMessage)
{
    emit addTask(DeploymentTask(Task::Warning, warningMessage));
    emit addOutput(warningMessage, OutputFormat::ErrorMessage);
}

bool TarPackageCreationStep::isPackagingNeeded() const
{
    const FilePath packagePath = packageFilePath();
    if (!packagePath.exists() || d->m_deploymentDataModified)
        return true;

    const DeploymentData &dd = target()->deploymentData();
    for (int i = 0; i < dd.fileCount(); ++i) {
        if (dd.fileAt(i).localFilePath().isNewerThan(packagePath.lastModified()))
            return true;
    }

    return false;
}

void TarPackageCreationStep::deployFinished(bool success)
{
    disconnect(BuildManager::instance(), &BuildManager::buildQueueFinished,
               this, &TarPackageCreationStep::deployFinished);

    if (!success)
        return;

    const Kit *kit = target()->kit();

    // Store files that have been tar'd and successfully deployed
    const auto files = d->m_files;
    for (const DeployableFile &file : files)
        d->m_deployTimes.saveDeploymentTimeStamp(file, kit, QDateTime());
}

void TarPackageCreationStep::addNeededDeploymentFiles(
        const ProjectExplorer::DeployableFile &deployable,
        const ProjectExplorer::Kit *kit)
{
    const QFileInfo fileInfo = deployable.localFilePath().toFileInfo();
    if (!fileInfo.isDir()) {
        if (d->m_deployTimes.hasLocalFileChanged(deployable, kit))
            d->m_files << deployable;
        return;
    }

    const QStringList files = QDir(deployable.localFilePath().toString())
            .entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

    if (files.isEmpty()) {
        d->m_files << deployable;
        return;
    }

    for (const QString &fileName : files) {
        const FilePath localFilePath = deployable.localFilePath().pathAppended(fileName);

        const QString remoteDir = deployable.remoteDirectory() + '/' + fileInfo.fileName();

        // Recurse through the subdirectories
        addNeededDeploymentFiles(DeployableFile(localFilePath, remoteDir), kit);
    }
}

bool TarPackageCreationStep::runImpl()
{
    const QList<DeployableFile> &files = target()->deploymentData().allFiles();

    if (d->m_incrementalDeploymentAspect->value()) {
        d->m_files.clear();
        for (const DeployableFile &file : files)
            addNeededDeploymentFiles(file, kit());
    } else {
        d->m_files = files;
    }

    const bool success = doPackage();

    if (success) {
        d->m_deploymentDataModified = false;
        emit addOutput(tr("Packaging finished successfully."), OutputFormat::NormalMessage);
    } else {
        emit addOutput(tr("Packaging failed."), OutputFormat::ErrorMessage);
    }

    connect(BuildManager::instance(), &BuildManager::buildQueueFinished,
            this, &TarPackageCreationStep::deployFinished);

    return success;
}

bool TarPackageCreationStep::doPackage()
{
    emit addOutput(tr("Creating tarball..."), OutputFormat::NormalMessage);
    if (!d->m_packagingNeeded) {
        emit addOutput(tr("Tarball up to date, skipping packaging."), OutputFormat::NormalMessage);
        return true;
    }

    // TODO: Optimization: Only package changed files
    const FilePath tarFilePath = d->m_cachedPackageFilePath;
    QFile tarFile(tarFilePath.toString());

    if (!tarFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        raiseError(tr("Error: tar file %1 cannot be opened (%2).")
            .arg(tarFilePath.toUserOutput(), tarFile.errorString()));
        return false;
    }

    for (const DeployableFile &d : qAsConst(d->m_files)) {
        if (d.remoteDirectory().isEmpty()) {
            emit addOutput(tr("No remote path specified for file \"%1\", skipping.")
                .arg(d.localFilePath().toUserOutput()), OutputFormat::ErrorMessage);
            continue;
        }
        QFileInfo fileInfo = d.localFilePath().toFileInfo();
        if (!appendFile(tarFile, fileInfo, d.remoteDirectory() + QLatin1Char('/')
                + fileInfo.fileName())) {
            return false;
        }
    }

    const QByteArray eofIndicator(2*sizeof(TarFileHeader), 0);
    if (tarFile.write(eofIndicator) != eofIndicator.length()) {
        raiseError(tr("Error writing tar file \"%1\": %2.")
            .arg(QDir::toNativeSeparators(tarFile.fileName()), tarFile.errorString()));
        return false;
    }

    return true;
}

static bool setFilePath(TarFileHeader &header, const QByteArray &filePath)
{
    if (filePath.length() <= int(sizeof header.fileName)) {
        std::memcpy(&header.fileName, filePath.data(), filePath.length());
        return true;
    }
    int sepIndex = filePath.indexOf('/');
    while (sepIndex != -1) {
        const int fileNamePart = filePath.length() - sepIndex;
        if (sepIndex <= int(sizeof header.fileNamePrefix)
                &&  fileNamePart <= int(sizeof header.fileName)) {
            std::memcpy(&header.fileNamePrefix, filePath.data(), sepIndex);
            std::memcpy(&header.fileName, filePath.data() + sepIndex + 1, fileNamePart);
            return true;
        }
        sepIndex = filePath.indexOf('/', sepIndex + 1);
    }
    return false;
}

static bool writeHeader(QFile &tarFile, const QFileInfo &fileInfo, const QString &remoteFilePath,
                        const QString &cachedPackageFilePath, QString *errorMessage)
{
    TarFileHeader header;
    std::memset(&header, '\0', sizeof header);
    if (!setFilePath(header, remoteFilePath.toUtf8())) {
        *errorMessage = QCoreApplication::translate("RemoteLinux::TarPackageCreationStep",
            "Cannot add file \"%1\" to tar-archive: path too long.").arg(remoteFilePath);
        return false;
    }
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
    const QByteArray mtimeString = QString::fromLatin1("%1").arg(
                fileInfo.lastModified().toSecsSinceEpoch(),
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
        *errorMessage = QCoreApplication::translate("RemoteLinux::TarPackageCreationStep",
            "Error writing tar file \"%1\": %2").arg(cachedPackageFilePath, tarFile.errorString());
        return false;
    }
    return true;
}

bool TarPackageCreationStep::appendFile(QFile &tarFile, const QFileInfo &fileInfo,
    const QString &remoteFilePath)
{
    QString errorMessage;
    if (!writeHeader(tarFile, fileInfo, remoteFilePath, d->m_cachedPackageFilePath.toUserOutput(),
                     &errorMessage)) {
        raiseError(errorMessage);
        return false;
    }
    if (fileInfo.isDir()) {
        QDir dir(fileInfo.absoluteFilePath());
        foreach (const QString &fileName,
                 dir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot)) {
            const QString thisLocalFilePath = dir.path() + QLatin1Char('/') + fileName;
            const QString thisRemoteFilePath  = remoteFilePath + QLatin1Char('/') + fileName;
            if (!appendFile(tarFile, QFileInfo(thisLocalFilePath), thisRemoteFilePath))
                return false;
        }
        return true;
    }

    const QString nativePath = QDir::toNativeSeparators(fileInfo.filePath());
    QFile file(fileInfo.filePath());
    if (!file.open(QIODevice::ReadOnly)) {
        const QString message = tr("Error reading file \"%1\": %2.")
                                .arg(nativePath, file.errorString());
        if (d->m_ignoreMissingFilesAspect->value()) {
            raiseWarning(message);
            return true;
        } else {
            raiseError(message);
            return false;
        }
    }

    const int chunkSize = 1024*1024;

    emit addOutput(tr("Adding file \"%1\" to tarball...").arg(nativePath),
                   OutputFormat::NormalMessage);

    // TODO: Wasteful. Work with fixed-size buffer.
    while (!file.atEnd() && file.error() == QFile::NoError && tarFile.error() == QFile::NoError) {
        const QByteArray data = file.read(chunkSize);
        tarFile.write(data);
        if (isCanceled())
            return false;
    }
    if (file.error() != QFile::NoError) {
        raiseError(tr("Error reading file \"%1\": %2.").arg(nativePath, file.errorString()));
        return false;
    }

    const int blockModulo = file.size() % TarBlockSize;
    if (blockModulo != 0)
        tarFile.write(QByteArray(TarBlockSize - blockModulo, 0));

    if (tarFile.error() != QFile::NoError) {
        raiseError(tr("Error writing tar file \"%1\": %2.")
            .arg(QDir::toNativeSeparators(tarFile.fileName()), tarFile.errorString()));
        return false;
    }
    return true;
}

} // namespace RemoteLinux
