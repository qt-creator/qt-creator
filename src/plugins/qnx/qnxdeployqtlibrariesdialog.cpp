/**************************************************************************
**
** Copyright (C) 2015 BlackBerry Limited. All rights reserved.
**
** Contact: BlackBerry (qt@blackberry.com)
** Contact: KDAB (info@kdab.com)
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

#include "qnxdeployqtlibrariesdialog.h"
#include "ui_qnxdeployqtlibrariesdialog.h"

#include "qnxqtversion.h"

#include <projectexplorer/deployablefile.h>
#include <qtsupport/qtversionmanager.h>
#include <remotelinux/genericdirectuploadservice.h>
#include <ssh/sshremoteprocessrunner.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QMessageBox>

using namespace QtSupport;

namespace Qnx {
namespace Internal {

QnxDeployQtLibrariesDialog::QnxDeployQtLibrariesDialog(const ProjectExplorer::IDevice::ConstPtr &device,
                                                       QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::QnxDeployQtLibrariesDialog),
    m_device(device),
    m_progressCount(0),
    m_state(Inactive)
{
    m_ui->setupUi(this);

    QList<BaseQtVersion*> qtVersions = QtVersionManager::validVersions();
    foreach (BaseQtVersion *qtVersion, qtVersions) {
        QnxQtVersion *qnxQt = dynamic_cast<QnxQtVersion *>(qtVersion);
        if (!qnxQt)
            continue;

        m_ui->qtLibraryCombo->addItem(qnxQt->displayName(), qnxQt->uniqueId());
    }

    m_ui->basePathLabel->setText(QString());
    m_ui->remoteDirectory->setText(QLatin1String("/qt"));

    m_uploadService = new RemoteLinux::GenericDirectUploadService(this);
    m_uploadService->setDevice(m_device);

    connect(m_uploadService, SIGNAL(progressMessage(QString)), this, SLOT(updateProgress(QString)));
    connect(m_uploadService, SIGNAL(progressMessage(QString)),
            m_ui->deployLogWindow, SLOT(appendPlainText(QString)));
    connect(m_uploadService, SIGNAL(errorMessage(QString)),
            m_ui->deployLogWindow, SLOT(appendPlainText(QString)));
    connect(m_uploadService, SIGNAL(warningMessage(QString)),
            m_ui->deployLogWindow, SLOT(appendPlainText(QString)));
    connect(m_uploadService, SIGNAL(stdOutData(QString)),
            m_ui->deployLogWindow, SLOT(appendPlainText(QString)));
    connect(m_uploadService, SIGNAL(stdErrData(QString)),
            m_ui->deployLogWindow, SLOT(appendPlainText(QString)));
    connect(m_uploadService, SIGNAL(finished()), this, SLOT(handleUploadFinished()));

    m_processRunner = new QSsh::SshRemoteProcessRunner(this);
    connect(m_processRunner, SIGNAL(connectionError()),
            this, SLOT(handleRemoteProcessError()));
    connect(m_processRunner, SIGNAL(processClosed(int)),
            this, SLOT(handleRemoteProcessCompleted()));

    connect(m_ui->deployButton, SIGNAL(clicked()), this, SLOT(deployLibraries()));
    connect(m_ui->closeButton, SIGNAL(clicked()), this, SLOT(close()));
}

QnxDeployQtLibrariesDialog::~QnxDeployQtLibrariesDialog()
{
    delete m_ui;
}

int QnxDeployQtLibrariesDialog::execAndDeploy(int qtVersionId, const QString &remoteDirectory)
{
    m_ui->remoteDirectory->setText(remoteDirectory);
    m_ui->qtLibraryCombo->setCurrentIndex(m_ui->qtLibraryCombo->findData(qtVersionId));

    deployLibraries();
    return exec();
}

void QnxDeployQtLibrariesDialog::closeEvent(QCloseEvent *event)
{
    // A disabled Deploy button indicates the upload is still running
    if (!m_ui->deployButton->isEnabled()) {
        int answer = QMessageBox::question(this, windowTitle(),
                                           tr("Closing the dialog will stop the deployment. "
                                              "Are you sure you want to do this?"),
                                           QMessageBox::Yes | QMessageBox::No);
        if (answer == QMessageBox::No)
            event->ignore();
        else if (answer == QMessageBox::Yes)
            m_uploadService->stop();
    }
}

void QnxDeployQtLibrariesDialog::deployLibraries()
{
    QTC_ASSERT(m_state == Inactive, return);

    if (m_ui->remoteDirectory->text().isEmpty()) {
        QMessageBox::warning(this, windowTitle(),
                             tr("Please input a remote directory to deploy to."));
        return;
    }

    QTC_ASSERT(!m_device.isNull(), return);

    m_progressCount = 0;
    m_ui->deployProgress->setValue(0);
    m_ui->remoteDirectory->setEnabled(false);
    m_ui->deployButton->setEnabled(false);
    m_ui->qtLibraryCombo->setEnabled(false);
    m_ui->deployLogWindow->clear();

    checkRemoteDirectoryExistance();
}

void QnxDeployQtLibrariesDialog::startUpload()
{
    QTC_CHECK(m_state == CheckingRemoteDirectory || m_state == RemovingRemoteDirectory);

    m_state = Uploading;

    QList<ProjectExplorer::DeployableFile> filesToUpload = gatherFiles();

    m_ui->deployProgress->setRange(0, filesToUpload.count());

    m_uploadService->setDeployableFiles(filesToUpload);
    m_uploadService->start();
}

void QnxDeployQtLibrariesDialog::updateProgress(const QString &progressMessage)
{
    QTC_CHECK(m_state == Uploading);

    if (!progressMessage.startsWith(QLatin1String("Uploading file")))
        return;

    ++m_progressCount;

    m_ui->deployProgress->setValue(m_progressCount);
}

void QnxDeployQtLibrariesDialog::handleUploadFinished()
{
    m_ui->remoteDirectory->setEnabled(true);
    m_ui->deployButton->setEnabled(true);
    m_ui->qtLibraryCombo->setEnabled(true);

    m_state = Inactive;
}

void QnxDeployQtLibrariesDialog::handleRemoteProcessError()
{
    QTC_CHECK(m_state == CheckingRemoteDirectory || m_state == RemovingRemoteDirectory);

    m_ui->deployLogWindow->appendPlainText(
                tr("Connection failed: %1")
                .arg(m_processRunner->lastConnectionErrorString()));
    handleUploadFinished();
}

void QnxDeployQtLibrariesDialog::handleRemoteProcessCompleted()
{
    QTC_CHECK(m_state == CheckingRemoteDirectory || m_state == RemovingRemoteDirectory);

    if (m_state == CheckingRemoteDirectory) {
        // Directory exists
        if (m_processRunner->processExitCode() == 0) {
            int answer = QMessageBox::question(this, windowTitle(),
                                               tr("The remote directory \"%1\" already exists. "
                                                  "Deploying to that directory will remove any files "
                                                  "already present.\n\n"
                                                  "Are you sure you want to continue?")
                                               .arg(fullRemoteDirectory()),
                                               QMessageBox::Yes | QMessageBox::No);
            if (answer == QMessageBox::Yes)
                removeRemoteDirectory();
            else
                handleUploadFinished();
        } else {
            startUpload();
        }
    } else if (m_state == RemovingRemoteDirectory) {
        QTC_ASSERT(m_processRunner->processExitCode() == 0, return);

        startUpload();
    }
}

QList<ProjectExplorer::DeployableFile> QnxDeployQtLibrariesDialog::gatherFiles()
{
    QList<ProjectExplorer::DeployableFile> result;

    const int qtVersionId =
            m_ui->qtLibraryCombo->itemData(m_ui->qtLibraryCombo->currentIndex()).toInt();


    QnxQtVersion *qtVersion = dynamic_cast<QnxQtVersion *>(QtVersionManager::version(qtVersionId));

    QTC_ASSERT(qtVersion, return result);

    if (Utils::HostOsInfo::isWindowsHost()) {
        result.append(gatherFiles(qtVersion->versionInfo().value(QLatin1String("QT_INSTALL_LIBS")),
                    QString(), QStringList() << QLatin1String("*.so.?")));
        result.append(gatherFiles(qtVersion->versionInfo().value(QLatin1String("QT_INSTALL_LIBS"))
                    + QLatin1String("/fonts")));
    } else {
        result.append(gatherFiles(
                    qtVersion->versionInfo().value(QLatin1String("QT_INSTALL_LIBS"))));
    }

    result.append(gatherFiles(qtVersion->versionInfo().value(QLatin1String("QT_INSTALL_PLUGINS"))));
    result.append(gatherFiles(qtVersion->versionInfo().value(QLatin1String("QT_INSTALL_IMPORTS"))));
    result.append(gatherFiles(qtVersion->versionInfo().value(QLatin1String("QT_INSTALL_QML"))));

    return result;
}

QList<ProjectExplorer::DeployableFile> QnxDeployQtLibrariesDialog::gatherFiles(
        const QString &dirPath, const QString &baseDirPath, const QStringList &nameFilters)
{
    QList<ProjectExplorer::DeployableFile> result;
    if (dirPath.isEmpty())
        return result;

    QDir dir(dirPath);
    QFileInfoList list = dir.entryInfoList(nameFilters,
            QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);

    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);
        if (fileInfo.isDir()) {
            result.append(gatherFiles(fileInfo.absoluteFilePath(), baseDirPath.isEmpty() ?
                                          dirPath : baseDirPath));
        } else {
            QString remoteDir;
            if (baseDirPath.isEmpty()) {
                remoteDir = fullRemoteDirectory() + QLatin1Char('/') +
                        QFileInfo(dirPath).baseName();
            } else {
                QDir baseDir(baseDirPath);
                baseDir.cdUp();
                remoteDir = fullRemoteDirectory() + QLatin1Char('/') +
                        baseDir.relativeFilePath(dirPath);
            }
            result.append(ProjectExplorer::DeployableFile(fileInfo.absoluteFilePath(), remoteDir));
        }
    }

    return result;
}

QString QnxDeployQtLibrariesDialog::fullRemoteDirectory() const
{
    return  m_ui->remoteDirectory->text();
}

void QnxDeployQtLibrariesDialog::checkRemoteDirectoryExistance()
{
    QTC_CHECK(m_state == Inactive);

    m_state = CheckingRemoteDirectory;

    m_ui->deployLogWindow->appendPlainText(tr("Checking existence of \"%1\"")
                                           .arg(fullRemoteDirectory()));

    const QByteArray cmd = "test -d " + fullRemoteDirectory().toLatin1();
    m_processRunner->run(cmd, m_device->sshParameters());
}

void QnxDeployQtLibrariesDialog::removeRemoteDirectory()
{
    QTC_CHECK(m_state == CheckingRemoteDirectory);

    m_state = RemovingRemoteDirectory;

    m_ui->deployLogWindow->appendPlainText(tr("Removing \"%1\"").arg(fullRemoteDirectory()));

    const QByteArray cmd = "rm -rf " + fullRemoteDirectory().toLatin1();
    m_processRunner->run(cmd, m_device->sshParameters());
}

} // namespace Internal
} // namespace Qnx
