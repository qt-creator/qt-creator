/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "s60deploystep.h"

#include "qt4buildconfiguration.h"
#include "s60deployconfiguration.h"
#include "s60devicerunconfiguration.h"
#include "s60runconfigbluetoothstarter.h"
#include "tcftrkdevice.h"
#include "trkruncontrol.h"

#include <coreplugin/icore.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/target.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qt4projectmanagerconstants.h>

#include <symbianutils/launcher.h>
#include <symbianutils/symbiandevicemanager.h>
#include <symbianutils/virtualserialdevice.h>
#include <utils/qtcassert.h>

#include <QtGui/QMessageBox>
#include <QtGui/QMainWindow>

#include <QtCore/QTimer>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QEventLoop>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

#include <QtNetwork/QTcpSocket>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager::Internal;

enum {debug = 0};

static const quint64  DEFAULT_CHUNK_SIZE = 40000;

namespace {
const char * const S60_DEPLOY_STEP_ID = "Qt4ProjectManager.S60DeployStep";
}

static inline bool ensureDeleteFile(const QString &fileName, QString *errorMessage)
{
    QFile file(fileName);
    if (file.exists() && !file.remove()) {
        *errorMessage = S60DeployStep::tr("Unable to remove existing file '%1': %2").arg(fileName, file.errorString());
        return false;
    }
    return true;
}

static inline bool renameFile(const QString &sourceName, const QString &targetName,
                              QString *errorMessage)
{
    if (sourceName == targetName)
        return true;
    if (!ensureDeleteFile(targetName, errorMessage))
        return false;
    QFile source(sourceName);
    if (!source.rename(targetName)) {
        *errorMessage = S60DeployStep::tr("Unable to rename file '%1' to '%2': %3")
                .arg(sourceName, targetName, source.errorString());
        return false;
    }
    return true;
}

// #pragma mark -- S60DeployStep

S60DeployStep::S60DeployStep(ProjectExplorer::BuildStepList *bc,
                             S60DeployStep *bs):
    BuildStep(bc, bs), m_timer(0),
    m_releaseDeviceAfterLauncherFinish(bs->m_releaseDeviceAfterLauncherFinish),
    m_handleDeviceRemoval(bs->m_handleDeviceRemoval),
    m_launcher(0),
    m_trkDevice(0),
    m_eventLoop(0),
    m_state(StateUninit),
    m_putWriteOk(false),
    m_putLastChunkSize(0),
    m_putChunkSize(DEFAULT_CHUNK_SIZE),
    m_currentFileIndex(0),
    m_channel(bs->m_channel),
    m_deployCanceled(false)
{
    ctor();
}

S60DeployStep::S60DeployStep(ProjectExplorer::BuildStepList *bc):
    BuildStep(bc, QLatin1String(S60_DEPLOY_STEP_ID)), m_timer(0),
    m_releaseDeviceAfterLauncherFinish(true),
    m_handleDeviceRemoval(true),
    m_launcher(0),
    m_trkDevice(0),
    m_eventLoop(0),
    m_state(StateUninit),
    m_putWriteOk(false),
    m_putLastChunkSize(0),
    m_putChunkSize(DEFAULT_CHUNK_SIZE),
    m_currentFileIndex(0),
    m_channel(S60DeployConfiguration::CommunicationTrkSerialConnection),
    m_deployCanceled(false)
{
    ctor();
}

void S60DeployStep::ctor()
{
    //: Qt4 Deploystep display name
    setDefaultDisplayName(tr("Deploy"));
}

S60DeployStep::~S60DeployStep()
{
    delete m_timer;
    delete m_launcher;
    delete m_trkDevice;
    delete m_eventLoop;
}


bool S60DeployStep::init()
{
    Qt4BuildConfiguration *bc = static_cast<Qt4BuildConfiguration *>(buildConfiguration());
    S60DeployConfiguration *deployConfiguration = static_cast<S60DeployConfiguration *>(bc->target()->activeDeployConfiguration());
    if (!deployConfiguration)
        return false;
    m_serialPortName = deployConfiguration->serialPortName();
    m_serialPortFriendlyName = SymbianUtils::SymbianDeviceManager::instance()->friendlyNameForPort(m_serialPortName);
    m_packageFileNamesWithTarget = deployConfiguration->packageFileNamesWithTargetInfo();
    m_signedPackages = deployConfiguration->signedPackages();
    m_installationDrive = deployConfiguration->installationDrive();
    m_silentInstall = deployConfiguration->silentInstall();

    switch (deployConfiguration->communicationChannel()) {
    case S60DeployConfiguration::CommunicationTrkSerialConnection:
        break;
    case S60DeployConfiguration::CommunicationCodaTcpConnection:
        m_address = deployConfiguration->deviceAddress();
        m_port = deployConfiguration->devicePort().toInt();
    default:
        break;
    }
    m_channel = deployConfiguration->communicationChannel();

    if (m_signedPackages.isEmpty()) {
        appendMessage(tr("No package has been found. Please specify at least one installation package."), true);
        return false;
    }

    if (m_channel == S60DeployConfiguration::CommunicationTrkSerialConnection) {
        QString message;
        if (m_launcher) {
            trk::Launcher::releaseToDeviceManager(m_launcher);
            delete m_launcher;
            m_launcher = 0;
        }

        m_launcher = trk::Launcher::acquireFromDeviceManager(m_serialPortName, this, &message);
        if (!message.isEmpty() || !m_launcher) {
            if (m_launcher)
                trk::Launcher::releaseToDeviceManager(m_launcher);
            delete m_launcher;
            m_launcher = 0;
            appendMessage(message, true);
            return true;
        }
        // Prompt the user to start up the Bluetooth connection
        const trk::PromptStartCommunicationResult src =
                S60RunConfigBluetoothStarter::startCommunication(m_launcher->trkDevice(),
                                                                 0, &message);
        if (src != trk::PromptStartCommunicationConnected) {
            if (!message.isEmpty())
                trk::Launcher::releaseToDeviceManager(m_launcher);
            delete m_launcher;
            m_launcher = 0;
            appendMessage(message, true);
            return false;
        }
    }
    return true;
}

QVariantMap S60DeployStep::toMap() const
{
    return BuildStep::toMap();
}

bool S60DeployStep::fromMap(const QVariantMap &map)
{
    return BuildStep::fromMap(map);
}

void S60DeployStep::appendMessage(const QString &error, bool isError)
{
    emit addOutput(error, isError?ProjectExplorer::BuildStep::ErrorMessageOutput:
                                  ProjectExplorer::BuildStep::MessageOutput);
}

void S60DeployStep::reportError(const QString &error)
{
    emit addOutput(error, ProjectExplorer::BuildStep::ErrorMessageOutput);
    emit addTask(ProjectExplorer::Task(ProjectExplorer::Task::Error,
                                       error,
                                       QString(), -1,
                                       ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    emit finished(false);
}

bool S60DeployStep::processPackageName(QString &errorMessage)
{
    for (int i = 0; i < m_signedPackages.count(); ++i) {
        QFileInfo packageInfo(m_signedPackages.at(i));
        // support for 4.6.1 and pre, where make sis creates 'targetname_armX_udeb.sis' instead of 'targetname.sis'
        QFileInfo packageWithTargetInfo(m_packageFileNamesWithTarget.at(i));
        // does the 4.6.1 version exist?
        if (packageWithTargetInfo.exists() && packageWithTargetInfo.isFile()) {
            // is the 4.6.1 version newer? (to guard against behavior change Qt Creator 1.3 --> 2.0)
            if (!packageInfo.exists() || packageInfo.lastModified() < packageWithTargetInfo.lastModified()) { //TODO change the QtCore
                // the 'targetname_armX_udeb.sis' crap exists and is new, rename it
                appendMessage(tr("Renaming new package '%1' to '%2'")
                              .arg(QDir::toNativeSeparators(m_packageFileNamesWithTarget.at(i)),
                                   QDir::toNativeSeparators(m_signedPackages.at(i))), false);
                return renameFile(m_packageFileNamesWithTarget.at(i), m_signedPackages.at(i), &errorMessage);
            } else {
                // the 'targetname_armX_udeb.sis' crap exists but is old, remove it
                appendMessage(tr("Removing old package '%1'")
                              .arg(QDir::toNativeSeparators(m_packageFileNamesWithTarget.at(i))),
                              false);
                ensureDeleteFile(m_packageFileNamesWithTarget.at(i), &errorMessage);
            }
        }
        if (!packageInfo.exists() || !packageInfo.isFile()) {
            errorMessage = tr("'%1': Package file not found").arg(m_signedPackages.at(i));
            return false;
        }
    }
    return true;
}

void S60DeployStep::start()
{
    QString errorMessage;

    bool serialConnection = (m_channel == S60DeployConfiguration::CommunicationTrkSerialConnection
            || m_channel == S60DeployConfiguration::CommunicationCodaSerialConnection);
    bool trkClient = m_channel == S60DeployConfiguration::CommunicationTrkSerialConnection;

    if (serialConnection && m_serialPortName.isEmpty()
            || (trkClient && !m_launcher)) {
        errorMessage = tr("No device is connected. Please connect a device and try again.");
        reportError(errorMessage);
        return;
    }
    if (!trkClient) {
        QTC_ASSERT(!m_trkDevice, return);
        m_trkDevice = new tcftrk::TcfTrkDevice;
        if (m_address.isEmpty() && !serialConnection) {
            errorMessage = tr("No address for a device has been defined. Please define an address and try again.");
            reportError(errorMessage);
            return;
        }
    }

    // make sure we have the right name of the sis package
    if (processPackageName(errorMessage)) {
        startDeployment();
    } else {
        errorMessage = tr("Failed to find package %1").arg(errorMessage);
        reportError(errorMessage);
        stop();
    }
}

void S60DeployStep::stop()
{
    if (m_channel == S60DeployConfiguration::CommunicationTrkSerialConnection) {
        if (m_launcher)
            m_launcher->terminate();
    } else {
        if (m_trkDevice) {
            delete m_trkDevice;
            m_trkDevice = 0;
        }
    }
    emit finished(false);
}

void S60DeployStep::setupConnections()
{
    if (m_channel == S60DeployConfiguration::CommunicationTrkSerialConnection) {
        connect(SymbianUtils::SymbianDeviceManager::instance(), SIGNAL(deviceRemoved(SymbianUtils::SymbianDevice)),
                this, SLOT(deviceRemoved(SymbianUtils::SymbianDevice)));
        connect(m_launcher, SIGNAL(finished()), this, SLOT(launcherFinished()));

        connect(m_launcher, SIGNAL(canNotConnect(QString)), this, SLOT(connectFailed(QString)));
        connect(m_launcher, SIGNAL(copyingStarted(QString)), this, SLOT(printCopyingNotice(QString)));
        connect(m_launcher, SIGNAL(canNotCreateFile(QString,QString)), this, SLOT(createFileFailed(QString,QString)));
        connect(m_launcher, SIGNAL(canNotWriteFile(QString,QString)), this, SLOT(writeFileFailed(QString,QString)));
        connect(m_launcher, SIGNAL(canNotCloseFile(QString,QString)), this, SLOT(closeFileFailed(QString,QString)));
        connect(m_launcher, SIGNAL(installingStarted(QString)), this, SLOT(printInstallingNotice(QString)));
        connect(m_launcher, SIGNAL(canNotInstall(QString,QString)), this, SLOT(installFailed(QString,QString)));
        connect(m_launcher, SIGNAL(installingFinished()), this, SLOT(printInstallingFinished()));
        connect(m_launcher, SIGNAL(stateChanged(int)), this, SLOT(slotLauncherStateChanged(int)));
    } else {
        connect(m_trkDevice, SIGNAL(error(QString)), this, SLOT(slotError(QString)));
        connect(m_trkDevice, SIGNAL(logMessage(QString)), this, SLOT(slotTrkLogMessage(QString)));
        connect(m_trkDevice, SIGNAL(tcfEvent(tcftrk::TcfTrkEvent)), this, SLOT(slotTcftrkEvent(tcftrk::TcfTrkEvent)), Qt::DirectConnection);
        connect(m_trkDevice, SIGNAL(serialPong(QString)), this, SLOT(slotSerialPong(QString)));
        connect(this, SIGNAL(manualInstallation()), this, SLOT(showManualInstallationInfo()));
    }
}

void S60DeployStep::startDeployment()
{
    if (m_channel == S60DeployConfiguration::CommunicationTrkSerialConnection) {
        QTC_ASSERT(m_launcher, return);
    } else {
        QTC_ASSERT(m_trkDevice, return);
    }

    setupConnections();

    if (m_channel == S60DeployConfiguration::CommunicationTrkSerialConnection) {
        QStringList copyDst;
        foreach (const QString &signedPackage, m_signedPackages)
            copyDst << QString::fromLatin1("%1:\\Data\\%2").arg(m_installationDrive).arg(QFileInfo(signedPackage).fileName());

        m_launcher->setCopyFileNames(m_signedPackages, copyDst);
        m_launcher->setInstallFileNames(copyDst);
        m_launcher->setInstallationDrive(m_installationDrive);
        m_launcher->setInstallationMode(m_silentInstall?trk::Launcher::InstallationModeSilentAndUser:
                                                        trk::Launcher::InstallationModeUser);
        m_launcher->addStartupActions(trk::Launcher::ActionCopyInstall);

        // TODO readd information about packages? msgListFile(m_signedPackage)
        appendMessage(tr("Deploying application to '%2'...").arg(m_serialPortFriendlyName), false);

        QString errorMessage;
        if (!m_launcher->startServer(&errorMessage)) {
            errorMessage = tr("Could not connect to phone on port '%1': %2\n"
                              "Check if the phone is connected and App TRK is running.").arg(m_serialPortName, errorMessage);
            reportError(errorMessage);
            stop();
        }
    } else if (m_channel == S60DeployConfiguration::CommunicationCodaSerialConnection) {
        const QSharedPointer<SymbianUtils::VirtualSerialDevice> serialDevice(new SymbianUtils::VirtualSerialDevice(m_serialPortName));
        appendMessage(tr("Deploying application to '%1'...").arg(m_serialPortFriendlyName), false);
        m_trkDevice->setSerialFrame(true);
        m_trkDevice->setDevice(serialDevice);
        bool ok = serialDevice->open(QIODevice::ReadWrite);
        if (!ok) {
            reportError(tr("Couldn't open serial device: %1").arg(serialDevice->errorString()));
            stop();
            return;
        }
        m_state = StateConnecting;
        m_trkDevice->sendSerialPing(false);
    } else {
        const QSharedPointer<QTcpSocket> tcfTrkSocket(new QTcpSocket);
        m_trkDevice->setDevice(tcfTrkSocket);
        tcfTrkSocket->connectToHost(m_address, m_port);
        m_state = StateConnecting;
        appendMessage(tr("Connecting to %1:%2...").arg(m_address).arg(m_port), false);
        QTimer::singleShot(4000, this, SLOT(checkForTimeout()));
    }
}

void S60DeployStep::run(QFutureInterface<bool> &fi)
{
    m_futureInterface = &fi;
    m_deployResult = true;
    m_deployCanceled = false;
    disconnect(this);

    if (m_channel == S60DeployConfiguration::CommunicationTrkSerialConnection) {
        connect(this, SIGNAL(finished(bool)), this, SLOT(launcherFinished(bool)));
        connect(this, SIGNAL(finishNow(bool)), this, SLOT(launcherFinished(bool)), Qt::DirectConnection);
    } else {
        connect(this, SIGNAL(finished(bool)), this, SLOT(deploymentFinished(bool)));
        connect(this, SIGNAL(finishNow(bool)), this, SLOT(deploymentFinished(bool)), Qt::DirectConnection);
        connect(this, SIGNAL(allFilesSent()), this, SLOT(startInstalling()), Qt::DirectConnection);
        connect(this, SIGNAL(allFilesInstalled()), this, SIGNAL(finished()), Qt::DirectConnection);
    }

    start();
    m_timer = new QTimer();
    connect(m_timer, SIGNAL(timeout()), this, SLOT(checkForCancel()), Qt::DirectConnection);
    m_timer->start(500);
    m_eventLoop = new QEventLoop();
    m_eventLoop->exec();
    m_timer->stop();
    delete m_timer;
    m_timer = 0;

    delete m_trkDevice;
    m_trkDevice = 0;

    delete m_eventLoop;
    m_eventLoop = 0;
    fi.reportResult(m_deployResult);
    m_futureInterface = 0;
}

void S60DeployStep::slotError(const QString &error)
{
    reportError(tr("Error: %1").arg(error));
}

void S60DeployStep::slotTrkLogMessage(const QString &log)
{
    if (debug)
        qDebug() << "CODA log:" << log;
}

void S60DeployStep::slotSerialPong(const QString &message)
{
    if (debug)
        qDebug() << "CODA serial pong:" << message;
}

void S60DeployStep::slotTcftrkEvent (const tcftrk::TcfTrkEvent &event)
{
    if (debug)
        qDebug() << "CODA event:" << "Type:" << event.type() << "Message:" << event.toString();

    switch (event.type()) {
    case tcftrk::TcfTrkEvent::LocatorHello: {// Commands accepted now
        m_state = StateConnected;
        emit tcpConnected();
        startTransferring();
        break;
    }
    default:
        if (debug)
            qDebug() << "Unhandled event:" << "Type:" << event.type() << "Message:" << event.toString();
        break;
    }
}

void S60DeployStep::initFileSending()
{
    QTC_ASSERT(m_currentFileIndex < m_signedPackages.count(), return);
    QTC_ASSERT(m_currentFileIndex >= 0, return);

    const unsigned flags =
            tcftrk::TcfTrkDevice::FileSystem_TCF_O_WRITE
            |tcftrk::TcfTrkDevice::FileSystem_TCF_O_CREAT
            |tcftrk::TcfTrkDevice::FileSystem_TCF_O_TRUNC;
    m_putWriteOk = false;

    QString packageName(QFileInfo(m_signedPackages.at(m_currentFileIndex)).fileName());
    QString remoteFileLocation = QString::fromLatin1("%1:\\Data\\%2").arg(m_installationDrive).arg(packageName);
    m_trkDevice->sendFileSystemOpenCommand(tcftrk::TcfTrkCallback(this, &S60DeployStep::handleFileSystemOpen),
                                           remoteFileLocation.toAscii(), flags);
    appendMessage(tr("Copying \"%1\"...").arg(packageName), false);
}

void S60DeployStep::initFileInstallation()
{
    QTC_ASSERT(m_currentFileIndex < m_signedPackages.count(), return);
    QTC_ASSERT(m_currentFileIndex >= 0, return);

    QString packageName(QFileInfo(m_signedPackages.at(m_currentFileIndex)).fileName());
    QString remoteFileLocation = QString::fromLatin1("%1:\\Data\\%2").arg(m_installationDrive).arg(packageName);
    if (m_silentInstall) {
        m_trkDevice->sendSymbianInstallSilentInstallCommand(tcftrk::TcfTrkCallback(this, &S60DeployStep::handleSymbianInstall),
                                                            remoteFileLocation.toAscii(), QString::fromLatin1("%1:").arg(m_installationDrive).toAscii());
        appendMessage(tr("Installing package \"%1\" on drive %2:...").arg(packageName).arg(m_installationDrive), false);
    } else {
        m_trkDevice->sendSymbianInstallUIInstallCommand(tcftrk::TcfTrkCallback(this, &S60DeployStep::handleSymbianInstall),
                                                        remoteFileLocation.toAscii());
        appendMessage(tr("Please continue the installation on your device."), false);
        emit manualInstallation();
    }
}

void S60DeployStep::startTransferring()
{
    m_currentFileIndex = 0;
    initFileSending();
    m_state = StateSendingData;
}

void S60DeployStep::startInstalling()
{
    m_currentFileIndex = 0;
    initFileInstallation();
    m_state = StateInstalling;
}

void S60DeployStep::handleFileSystemOpen(const tcftrk::TcfTrkCommandResult &result)
{
    if (result.type != tcftrk::TcfTrkCommandResult::SuccessReply) {
        reportError(tr("Open remote file failed: %1").arg(result.errorString()));
        return;
    }

    if (result.values.size() < 1 || result.values.at(0).data().isEmpty()) {
        reportError(tr("Internal error: No filehandle obtained"));
        return;
    }

    m_remoteFileHandle = result.values.at(0).data();

    m_putFile.reset(new QFile(m_signedPackages.at(m_currentFileIndex)));
    if (!m_putFile->open(QIODevice::ReadOnly)) { // Should not fail, was checked before
        reportError(tr("Open local file failed: %1").arg(m_putFile->errorString()));
        return;
    }
    putSendNextChunk();
}

void S60DeployStep::handleSymbianInstall(const tcftrk::TcfTrkCommandResult &result)
{
    if (result.type == tcftrk::TcfTrkCommandResult::SuccessReply) {
        appendMessage(tr("Installation has finished"), false);
        if (++m_currentFileIndex >= m_signedPackages.count())
            emit allFilesInstalled();
        else
            initFileInstallation();
    } else {
        reportError(tr("Installation failed: %1").arg(result.errorString()));
    }
}

void S60DeployStep::putSendNextChunk()
{
    // Read and send off next chunk
    const quint64 pos = m_putFile->pos();
    const QByteArray data = m_putFile->read(m_putChunkSize);
    if (data.isEmpty()) {
        m_putWriteOk = true;
        closeRemoteFile();
    } else {
        m_putLastChunkSize = data.size();
        if (debug)
            qDebug("Writing %llu bytes to remote file '%s' at %llu\n",
                   m_putLastChunkSize,
                   m_remoteFileHandle.constData(), pos);
        m_trkDevice->sendFileSystemWriteCommand(tcftrk::TcfTrkCallback(this, &S60DeployStep::handleFileSystemWrite),
                                                m_remoteFileHandle, data, unsigned(pos));
    }
}

void S60DeployStep::closeRemoteFile()
{
    m_trkDevice->sendFileSystemCloseCommand(tcftrk::TcfTrkCallback(this, &S60DeployStep::handleFileSystemClose),
                                            m_remoteFileHandle);
}

void S60DeployStep::handleFileSystemWrite(const tcftrk::TcfTrkCommandResult &result)
{
    // Close remote file even if copy fails
    m_putWriteOk = result;
    if (!m_putWriteOk) {
        QString packageName(QFileInfo(m_signedPackages.at(m_currentFileIndex)).fileName());
        reportError(tr("Could not write to file %1 on device: %2").arg(packageName).arg(result.errorString()));
    }

    if (!m_putWriteOk || m_putLastChunkSize < m_putChunkSize) {
        closeRemoteFile();
    } else {
        putSendNextChunk();
    }
}

void S60DeployStep::handleFileSystemClose(const tcftrk::TcfTrkCommandResult &result)
{
    if (result.type == tcftrk::TcfTrkCommandResult::SuccessReply) {
        if (debug)
            qDebug("File closed.\n");
        if (++m_currentFileIndex >= m_signedPackages.count())
            emit allFilesSent();
        else
            initFileSending();
    } else {
        reportError(tr("File close failed: %1").arg(result.toString()));
    }
}

void S60DeployStep::checkForTimeout()
{
    if (m_state != StateConnecting)
        return;

    const QString title  = tr("Waiting for CODA");
    const QString text = tr("Qt Creator is waiting for the CODA application to connect. "
                            "Please make sure the application is running on "
                            "your mobile phone and the right IP address and port are "
                            "configured in the project settings.");
    QMessageBox *mb = new QMessageBox(QMessageBox::Information, title, text,
                                      QMessageBox::Cancel, Core::ICore::instance()->mainWindow());
    connect(this, SIGNAL(tcpConnected()), mb, SLOT(close()));
    connect(this, SIGNAL(finished()), mb, SLOT(close()));
    connect(this, SIGNAL(finishNow()), mb, SLOT(close()));
    connect(mb, SIGNAL(finished(int)), this, SLOT(slotWaitingForTckTrkClosed(int)));
    mb->open();
}

void S60DeployStep::showManualInstallationInfo()
{
    const QString title  = tr("Installation");
    const QString text = tr("Please continue the installation on your device.");
    QMessageBox *mb = new QMessageBox(QMessageBox::Information, title, text,
                                      QMessageBox::Ok, Core::ICore::instance()->mainWindow());
    connect(this, SIGNAL(allFilesInstalled()), mb, SLOT(close()));
    connect(this, SIGNAL(finished()), mb, SLOT(close()));
    connect(this, SIGNAL(finishNow()), mb, SLOT(close()));
    mb->open();
}

void S60DeployStep::slotWaitingForTckTrkClosed(int result)
{
    if (result == QMessageBox::Cancel)
        m_deployCanceled = true;
}

void S60DeployStep::setReleaseDeviceAfterLauncherFinish(bool v)
{
    m_releaseDeviceAfterLauncherFinish = v;
}

void S60DeployStep::slotLauncherStateChanged(int s)
{
    if (s == trk::Launcher::WaitingForTrk) {
        QMessageBox *mb = TrkRunControl::createTrkWaitingMessageBox(m_launcher->trkServerName(),
                                                                          Core::ICore::instance()->mainWindow());
        connect(m_launcher, SIGNAL(stateChanged(int)), mb, SLOT(close()));
        connect(mb, SIGNAL(finished(int)), this, SLOT(slotWaitingForTrkClosed()));
        mb->open();
    }
}

void S60DeployStep::slotWaitingForTrkClosed()
{
    if (m_launcher && m_launcher->state() == trk::Launcher::WaitingForTrk)
        m_deployCanceled = true;
}

void S60DeployStep::createFileFailed(const QString &filename, const QString &errorMessage)
{
    reportError(tr("Could not create file %1 on device: %2").arg(filename, errorMessage));
}

void S60DeployStep::writeFileFailed(const QString &filename, const QString &errorMessage)
{
    reportError(tr("Could not write to file %1 on device: %2").arg(filename, errorMessage));
}

void S60DeployStep::closeFileFailed(const QString &filename, const QString &errorMessage)
{
    const QString msg = tr("Could not close file %1 on device: %2. It will be closed when App TRK is closed.");
    reportError( msg.arg(filename, errorMessage));
}

void S60DeployStep::connectFailed(const QString &errorMessage)
{
    reportError(tr("Could not connect to App TRK on device: %1. Restarting App TRK might help.").arg(errorMessage));
}

void S60DeployStep::printCopyingNotice(const QString &fileName)
{
    appendMessage(tr("Copying \"%1\"...").arg(fileName), false);
}

void S60DeployStep::printInstallingNotice(const QString &packageName)
{
    appendMessage(tr("Installing package \"%1\" on drive %2:...").arg(packageName).arg(m_installationDrive), false);
}

void S60DeployStep::printInstallingFinished()
{
    appendMessage(tr("Installation has finished"), false);
}

void S60DeployStep::installFailed(const QString &filename, const QString &errorMessage)
{
    reportError(tr("Could not install from package %1 on device: %2").arg(filename, errorMessage));
}

void S60DeployStep::checkForCancel()
{
    if ((m_futureInterface->isCanceled() || m_deployCanceled) && m_timer->isActive()) {
        m_timer->stop();
        stop();
        QString canceledText(tr("Deployment has been cancelled."));
        appendMessage(canceledText, true);
        emit addTask(ProjectExplorer::Task(ProjectExplorer::Task::Error,
                                           canceledText,
                                           QString(), -1,
                                           ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
        emit finishNow(false);
    }
}

void S60DeployStep::launcherFinished(bool success)
{
    m_deployResult = success;
    if (m_releaseDeviceAfterLauncherFinish && m_launcher) {
        m_handleDeviceRemoval = false;
        trk::Launcher::releaseToDeviceManager(m_launcher);
    }
    if (m_launcher)
        m_launcher->deleteLater();
    m_launcher = 0;
    if (m_eventLoop)
        m_eventLoop->exit();
}

void S60DeployStep::deploymentFinished(bool success)
{
    m_deployResult = success;
    if (m_eventLoop)
        m_eventLoop->exit();
}

void S60DeployStep::deviceRemoved(const SymbianUtils::SymbianDevice &d)
{
    if (m_handleDeviceRemoval && d.portName() == m_serialPortName)
        reportError(tr("The device '%1' has been disconnected").arg(d.friendlyName()));
}

// #pragma mark -- S60DeployStepWidget

BuildStepConfigWidget *S60DeployStep::createConfigWidget()
{
    return new S60DeployStepWidget();
}

S60DeployStepWidget::S60DeployStepWidget() : ProjectExplorer::BuildStepConfigWidget()
{
}

void S60DeployStepWidget::init()
{
}

QString S60DeployStepWidget::summaryText() const
{
    return QString("<b>%1</b>").arg(displayName());
}

QString S60DeployStepWidget::displayName() const
{
    return tr("Deploy SIS Package");
}

// #pragma mark -- S60DeployStepFactory

S60DeployStepFactory::S60DeployStepFactory(QObject *parent) :
    ProjectExplorer::IBuildStepFactory(parent)
{
}

S60DeployStepFactory::~S60DeployStepFactory()
{
}

bool S60DeployStepFactory::canCreate(ProjectExplorer::BuildStepList *parent, const QString &id) const
{
    if (parent->id() != QLatin1String(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY))
        return false;
    if (parent->target()->id() != QLatin1String(Constants::S60_DEVICE_TARGET_ID))
        return false;
    return (id == QLatin1String(S60_DEPLOY_STEP_ID));
}

ProjectExplorer::BuildStep *S60DeployStepFactory::create(ProjectExplorer::BuildStepList *parent, const QString &id)
{
    if (!canCreate(parent, id))
        return 0;
    return new S60DeployStep(parent);
}

bool S60DeployStepFactory::canClone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *source) const
{
    if (!canCreate(parent, source->id()))
        return false;
    if (!qobject_cast<S60DeployStep *>(source))
        return false;
    return true;
}

ProjectExplorer::BuildStep *S60DeployStepFactory::clone(ProjectExplorer::BuildStepList *parent, ProjectExplorer::BuildStep *source)
{
    if (!canClone(parent, source))
        return 0;
    return new S60DeployStep(parent, static_cast<S60DeployStep *>(source));
}

bool S60DeployStepFactory::canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const
{
    QString id(ProjectExplorer::idFromMap(map));
    return canCreate(parent, id);
}

ProjectExplorer::BuildStep *S60DeployStepFactory::restore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map)
{
    if (!canRestore(parent, map))
        return 0;
    S60DeployStep *bs = new S60DeployStep(parent);
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

QStringList S60DeployStepFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if (parent->id() == QLatin1String(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
            && parent->target()->id() == QLatin1String(Constants::S60_DEVICE_TARGET_ID))
        return QStringList() << QLatin1String(S60_DEPLOY_STEP_ID);
    return QStringList();
}

QString S60DeployStepFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(S60_DEPLOY_STEP_ID))
        return tr("Deploy SIS Package");
    return QString();
}
