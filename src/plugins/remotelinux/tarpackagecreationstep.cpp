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

#include <projectexplorer/buildmanager.h>
#include <projectexplorer/deploymentdata.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>
#include <ssh/sshconnection.h>
#include <ssh/sshconnectionmanager.h>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <QCheckBox>
#include <QVBoxLayout>

#include <cstring>

using namespace ProjectExplorer;

namespace RemoteLinux {
namespace {
const char IgnoreMissingFilesKey[] = "RemoteLinux.TarPackageCreationStep.IgnoreMissingFiles";
const char IncrementalDeploymentKey[] = "RemoteLinux.TarPackageCreationStep.IncrementalDeployment";

class CreateTarStepWidget : public SimpleBuildStepConfigWidget
{
    Q_OBJECT
public:
    CreateTarStepWidget(TarPackageCreationStep *step) : SimpleBuildStepConfigWidget(step)
    {
        m_ignoreMissingFilesCheckBox.setText(tr("Ignore missing files"));
        m_incrementalDeploymentCheckBox.setText(tr("Package modified files only"));

        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setMargin(0);
        mainLayout->addWidget(&m_incrementalDeploymentCheckBox);
        mainLayout->addWidget(&m_ignoreMissingFilesCheckBox);

        m_ignoreMissingFilesCheckBox.setChecked(step->ignoreMissingFiles());
        m_incrementalDeploymentCheckBox.setChecked(step->isIncrementalDeployment());

        connect(&m_ignoreMissingFilesCheckBox, &QAbstractButton::toggled,
                this, &CreateTarStepWidget::handleIgnoreMissingFilesChanged);

        connect(&m_incrementalDeploymentCheckBox, &QAbstractButton::toggled,
                this, &CreateTarStepWidget::handleIncrementalDeploymentChanged);

        connect(step, &AbstractPackagingStep::packageFilePathChanged,
                this, &BuildStepConfigWidget::updateSummary);
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

    bool showWidget() const { return true; }

private:
    void handleIgnoreMissingFilesChanged(bool ignoreMissingFiles) {
        TarPackageCreationStep *step = qobject_cast<TarPackageCreationStep *>(this->step());
        step->setIgnoreMissingFiles(ignoreMissingFiles);
    }

    void handleIncrementalDeploymentChanged(bool incrementalDeployment) {
        TarPackageCreationStep *step = qobject_cast<TarPackageCreationStep *>(this->step());
        step->setIncrementalDeployment(incrementalDeployment);
    }

    QCheckBox m_ignoreMissingFilesCheckBox;
    QCheckBox m_incrementalDeploymentCheckBox;
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
    m_ignoreMissingFiles = false;
}

bool TarPackageCreationStep::init(QList<const BuildStep *> &earlierSteps)
{
    if (!AbstractPackagingStep::init(earlierSteps))
        return false;

    m_packagingNeeded = isPackagingNeeded();

    return true;
}

void TarPackageCreationStep::run(QFutureInterface<bool> &fi)
{
    setPackagingStarted();

    const QList<DeployableFile> &files = target()->deploymentData().allFiles();

    if (m_incrementalDeployment) {
        m_files.clear();
        for (const DeployableFile &file : files)
            addNeededDeploymentFiles(file, target()->kit());
    } else {
        m_files = files;
    }

    const bool success = doPackage(fi);

    setPackagingFinished(success);
    if (success)
        emit addOutput(tr("Packaging finished successfully."), OutputFormat::NormalMessage);
    else
        emit addOutput(tr("Packaging failed."), OutputFormat::ErrorMessage);

    connect(BuildManager::instance(), &BuildManager::buildQueueFinished,
            this, &TarPackageCreationStep::deployFinished);

    reportRunResult(fi, success);
}

void TarPackageCreationStep::setIgnoreMissingFiles(bool ignoreMissingFiles)
{
    m_ignoreMissingFiles = ignoreMissingFiles;
}

bool TarPackageCreationStep::ignoreMissingFiles() const
{
    return m_ignoreMissingFiles;
}

void TarPackageCreationStep::setIncrementalDeployment(bool incrementalDeployment)
{
    m_incrementalDeployment = incrementalDeployment;
}

bool TarPackageCreationStep::isIncrementalDeployment() const
{
    return m_incrementalDeployment;
}

void TarPackageCreationStep::addNeededDeploymentFiles(
        const ProjectExplorer::DeployableFile &deployable,
        const ProjectExplorer::Kit *kit)
{
    const QFileInfo fileInfo = deployable.localFilePath().toFileInfo();
    if (!fileInfo.isDir()) {
        if (m_deployTimes.hasChangedSinceLastDeployment(deployable, kit))
            m_files << deployable;
        return;
    }

    const QStringList files = QDir(deployable.localFilePath().toString())
            .entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

    if (files.isEmpty()) {
        m_files << deployable;
        return;
    }

    for (const QString &fileName : files) {
        const QString localFilePath = deployable.localFilePath().appendPath(fileName).toString();

        const QString remoteDir = deployable.remoteDirectory() + '/' + fileInfo.fileName();

        // Recurse through the subdirectories
        addNeededDeploymentFiles(DeployableFile(localFilePath, remoteDir), kit);
    }
}

bool TarPackageCreationStep::doPackage(QFutureInterface<bool> &fi)
{
    emit addOutput(tr("Creating tarball..."), OutputFormat::NormalMessage);
    if (!m_packagingNeeded) {
        emit addOutput(tr("Tarball up to date, skipping packaging."), OutputFormat::NormalMessage);
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
            emit addOutput(tr("No remote path specified for file \"%1\", skipping.")
                .arg(d.localFilePath().toUserOutput()), OutputFormat::ErrorMessage);
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
        raiseError(tr("Error writing tar file \"%1\": %2.")
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
        const QString message = tr("Error reading file \"%1\": %2.")
                                .arg(nativePath, file.errorString());
        if (m_ignoreMissingFiles) {
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
        if (fi.isCanceled())
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

bool TarPackageCreationStep::writeHeader(QFile &tarFile, const QFileInfo &fileInfo,
    const QString &remoteFilePath)
{
    TarFileHeader header;
    std::memset(&header, '\0', sizeof header);
    const QByteArray &filePath = remoteFilePath.toUtf8();
    const int maxFilePathLength = sizeof header.fileNamePrefix + sizeof header.fileName;
    if (filePath.count() > maxFilePathLength) {
        raiseError(tr("Cannot add file \"%1\" to tar-archive: path too long.")
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
        raiseError(tr("Error writing tar file \"%1\": %2")
           .arg(QDir::toNativeSeparators(cachedPackageFilePath()), tarFile.errorString()));
        return false;
    }
    return true;
}

void TarPackageCreationStep::deployFinished(bool success)
{
    disconnect(BuildManager::instance(), &BuildManager::buildQueueFinished,
               this, &TarPackageCreationStep::deployFinished);

    if (!success)
        return;

    const Kit *kit = target()->kit();

    // Store files that have been tar'd and successfully deployed
    const auto files = m_files;
    for (const DeployableFile &file : files)
        m_deployTimes.saveDeploymentTimeStamp(file, kit);
}

QString TarPackageCreationStep::packageFileName() const
{
    return project()->displayName() + QLatin1String(".tar");
}

BuildStepConfigWidget *TarPackageCreationStep::createConfigWidget()
{
    return new CreateTarStepWidget(this);
}

bool TarPackageCreationStep::fromMap(const QVariantMap &map)
{
    if (!AbstractPackagingStep::fromMap(map))
        return false;
    setIgnoreMissingFiles(map.value(QLatin1String(IgnoreMissingFilesKey), false).toBool());
    setIncrementalDeployment(map.value(QLatin1String(IncrementalDeploymentKey), false).toBool());
    m_deployTimes.importDeployTimes(map);
    return true;
}

QVariantMap TarPackageCreationStep::toMap() const
{
    QVariantMap map = AbstractPackagingStep::toMap();
    map.insert(QLatin1String(IgnoreMissingFilesKey), ignoreMissingFiles());
    map.insert(QLatin1String(IncrementalDeploymentKey), m_incrementalDeployment);
    map.unite(m_deployTimes.exportDeployTimes());
    return map;
}

Core::Id TarPackageCreationStep::stepId()
{
    return "MaemoTarPackageCreationStep";
}

QString TarPackageCreationStep::displayName()
{
    return tr("Create tarball");
}

} // namespace RemoteLinux

#include "tarpackagecreationstep.moc"
