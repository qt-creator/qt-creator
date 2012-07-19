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

#include "s60deploystep.h"

#include "qt4buildconfiguration.h"
#include "qt4project.h"
#include "s60deployconfiguration.h"
#include "s60devicerunconfiguration.h"
#include "symbianidevice.h"
#include "symbianidevicefactory.h"
#include "codadevice.h"
#include "codaruncontrol.h"

#include <coreplugin/icore.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/target.h>
#include <projectexplorer/profileinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <qt4projectmanagerconstants.h>

#include <symbianutils/symbiandevicemanager.h>
#include <utils/qtcassert.h>

#include <QMessageBox>

#include <QTimer>
#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>

#include <QTcpSocket>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager::Internal;

enum { debug = 0 };

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
    m_timeoutTimer(new QTimer(this)),
    m_eventLoop(0),
    m_state(StateUninit),
    m_putWriteOk(false),
    m_putLastChunkSize(0),
    m_putChunkSize(DEFAULT_CHUNK_SIZE),
    m_currentFileIndex(0),
    m_channel(bs->m_channel),
    m_deployCanceled(false),
    m_copyProgress(0)
{
    ctor();
}

S60DeployStep::S60DeployStep(ProjectExplorer::BuildStepList *bc):
    BuildStep(bc, Core::Id(S60_DEPLOY_STEP_ID)), m_timer(0),
    m_timeoutTimer(new QTimer(this)),
    m_eventLoop(0),
    m_state(StateUninit),
    m_putWriteOk(false),
    m_putLastChunkSize(0),
    m_putChunkSize(DEFAULT_CHUNK_SIZE),
    m_currentFileIndex(0),
    m_channel(SymbianIDevice::CommunicationCodaSerialConnection),
    m_deployCanceled(false),
    m_copyProgress(0)
{
    ctor();
}

void S60DeployStep::ctor()
{
    //: Qt4 Deploystep display name
    setDefaultDisplayName(tr("Deploy SIS Package"));
    m_timeoutTimer->setSingleShot(true);
    m_timeoutTimer->setInterval(10000);
    connect(m_timeoutTimer, SIGNAL(timeout()), this, SLOT(timeout()));
}

S60DeployStep::~S60DeployStep()
{
    delete m_timer;
    delete m_eventLoop;
}


bool S60DeployStep::init()
{
    Qt4BuildConfiguration *bc = static_cast<Qt4BuildConfiguration *>(target()->activeBuildConfiguration());
    S60DeployConfiguration *deployConfiguration = static_cast<S60DeployConfiguration *>(bc->target()->activeDeployConfiguration());
    if (!deployConfiguration)
        return false;

    SymbianIDevice::ConstPtr dev = deployConfiguration->device();
    m_serialPortName = dev->serialPortName();
    m_serialPortFriendlyName = SymbianUtils::SymbianDeviceManager::instance()->friendlyNameForPort(m_serialPortName);
    m_packageFileNamesWithTarget = deployConfiguration->packageFileNamesWithTargetInfo();
    m_signedPackages = deployConfiguration->signedPackages();
    m_installationDrive = deployConfiguration->installationDrive();
    m_silentInstall = deployConfiguration->silentInstall();
    m_channel = dev->communicationChannel();

    if (m_signedPackages.isEmpty()) {
        appendMessage(tr("No package has been found. Specify at least one installation package."), true);
        return false;
    }

    if (m_channel == SymbianIDevice::CommunicationCodaTcpConnection) {
        m_address = dev->address();
        m_port = dev->port().toInt();
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
                                       Utils::FileName(), -1,
                                       Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
    emit s60DeploymentFinished(false);
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

    bool serialConnection = m_channel == SymbianIDevice::CommunicationCodaSerialConnection;

    if (serialConnection && m_serialPortName.isEmpty()) {
        errorMessage = tr("No device is connected. Connect a device and try again.");
        reportError(errorMessage);
        return;
    }
    QTC_ASSERT(!m_codaDevice.data(), return);
    if (m_address.isEmpty() && !serialConnection) {
        errorMessage = tr("No address for a device has been defined. Define an address and try again.");
        reportError(errorMessage);
        return;
    }

    // make sure we have the right name of the sis package
    if (processPackageName(errorMessage))
        startDeployment();
    else {
        errorMessage = tr("Failed to find package %1").arg(errorMessage);
        reportError(errorMessage);
        stop();
    }
}

void S60DeployStep::stop()
{
    if (m_codaDevice) {
        switch (state()) {
        case StateSendingData:
            closeFiles();
            break;
        default:
            break; //should also stop the package installation, but CODA does not support it yet
        }
        disconnect(m_codaDevice.data(), 0, this, 0);
        SymbianUtils::SymbianDeviceManager::instance()->releaseCodaDevice(m_codaDevice);
    }
    setState(StateUninit);
    emit s60DeploymentFinished(false);
}

void S60DeployStep::setupConnections()
{
    if (m_channel == SymbianIDevice::CommunicationCodaSerialConnection)
        connect(SymbianUtils::SymbianDeviceManager::instance(), SIGNAL(deviceRemoved(SymbianUtils::SymbianDevice)),
                this, SLOT(deviceRemoved(SymbianUtils::SymbianDevice)));

    connect(m_codaDevice.data(), SIGNAL(error(QString)), this, SLOT(slotError(QString)));
    connect(m_codaDevice.data(), SIGNAL(logMessage(QString)), this, SLOT(slotCodaLogMessage(QString)));
    connect(m_codaDevice.data(), SIGNAL(codaEvent(Coda::CodaEvent)), this, SLOT(slotCodaEvent(Coda::CodaEvent)), Qt::DirectConnection);
    connect(m_codaDevice.data(), SIGNAL(serialPong(QString)), this, SLOT(slotSerialPong(QString)));
    connect(this, SIGNAL(manualInstallation()), this, SLOT(showManualInstallationInfo()));
}

void S60DeployStep::startDeployment()
{
    QTC_ASSERT(!m_codaDevice.data(), return);

    // We need to defer setupConnections() in the case of CommunicationCodaSerialConnection
    //setupConnections();

    if (m_channel == SymbianIDevice::CommunicationCodaSerialConnection) {
        appendMessage(tr("Deploying application to '%1'...").arg(m_serialPortFriendlyName), false);
        m_codaDevice = SymbianUtils::SymbianDeviceManager::instance()->getCodaDevice(m_serialPortName);
        bool ok = m_codaDevice && m_codaDevice->device()->isOpen();
        if (!ok) {
            QString deviceError = tr("No such port");
            if (m_codaDevice)
                deviceError = m_codaDevice->device()->errorString();
            reportError(tr("Could not open serial device: %1").arg(deviceError));
            stop();
            return;
        }
        setupConnections();
        setState(StateConnecting);
        m_codaDevice->sendSerialPing(false);
    } else {
        m_codaDevice = QSharedPointer<Coda::CodaDevice>(new Coda::CodaDevice);
        setupConnections();
        const QSharedPointer<QTcpSocket> codaSocket(new QTcpSocket);
        m_codaDevice->setDevice(codaSocket);
        codaSocket->connectToHost(m_address, m_port);
        setState(StateConnecting);
        appendMessage(tr("Connecting to %1:%2...").arg(m_address).arg(m_port), false);
    }
    QTimer::singleShot(4000, this, SLOT(checkForTimeout()));
}

void S60DeployStep::run(QFutureInterface<bool> &fi)
{
    m_futureInterface = &fi;
    m_deployResult = true;
    m_deployCanceled = false;
    disconnect(this);

    m_futureInterface->setProgressRange(0, 100*m_signedPackages.count());

    connect(this, SIGNAL(s60DeploymentFinished(bool)), this, SLOT(deploymentFinished(bool)));
    connect(this, SIGNAL(finishNow(bool)), this, SLOT(deploymentFinished(bool)), Qt::DirectConnection);
    connect(this, SIGNAL(allFilesSent()), this, SLOT(startInstalling()), Qt::DirectConnection);
    connect(this, SIGNAL(allFilesInstalled()), this, SIGNAL(s60DeploymentFinished()), Qt::DirectConnection);
    connect(this, SIGNAL(copyProgressChanged(int)), this, SLOT(updateProgress(int)));

    start();
    m_timer = new QTimer();
    connect(m_timer, SIGNAL(timeout()), this, SLOT(checkForCancel()), Qt::DirectConnection);
    m_timer->start(500);
    m_eventLoop = new QEventLoop();
    m_eventLoop->exec();
    m_timer->stop();
    delete m_timer;
    m_timer = 0;

    if (m_codaDevice) {
        disconnect(m_codaDevice.data(), 0, this, 0);
        SymbianUtils::SymbianDeviceManager::instance()->releaseCodaDevice(m_codaDevice);
    }

    delete m_eventLoop;
    m_eventLoop = 0;
    fi.reportResult(m_deployResult);
    m_futureInterface = 0;
}

void S60DeployStep::slotWaitingForCodaClosed(int result)
{
    if (result == QMessageBox::Cancel)
        m_deployCanceled = true;
}

void S60DeployStep::slotError(const QString &error)
{
    reportError(tr("Error: %1").arg(error));
}

void S60DeployStep::slotCodaLogMessage(const QString &log)
{
    if (debug > 1)
        qDebug() << "CODA log:" << log;
}

void S60DeployStep::slotSerialPong(const QString &message)
{
    if (debug)
        qDebug() << "CODA serial pong:" << message;
    handleConnected();
}

void S60DeployStep::slotCodaEvent(const Coda::CodaEvent &event)
{
    if (debug)
        qDebug() << "CODA event:" << "Type:" << event.type() << "Message:" << event.toString();

    switch (event.type()) {
    case Coda::CodaEvent::LocatorHello:
        handleConnected();
        break;
    default:
        if (debug)
            qDebug() << "Unhandled event:" << "Type:" << event.type() << "Message:" << event.toString();
        break;
    }
}

void S60DeployStep::handleConnected()
{
    if (state() >= StateConnected)
        return;
    setState(StateConnected);
    emit codaConnected();
    startTransferring();
}

void S60DeployStep::initFileSending()
{
    QTC_ASSERT(m_currentFileIndex < m_signedPackages.count(), return);
    QTC_ASSERT(m_currentFileIndex >= 0, return);
    QTC_ASSERT(m_codaDevice, return);

    const unsigned flags =
            Coda::CodaDevice::FileSystem_TCF_O_WRITE
            |Coda::CodaDevice::FileSystem_TCF_O_CREAT
            |Coda::CodaDevice::FileSystem_TCF_O_TRUNC;
    m_putWriteOk = false;

    QString packageName(QFileInfo(m_signedPackages.at(m_currentFileIndex)).fileName());
    QString remoteFileLocation = QString::fromLatin1("%1:\\Data\\%2").arg(m_installationDrive).arg(packageName);
    m_codaDevice->sendFileSystemOpenCommand(Coda::CodaCallback(this, &S60DeployStep::handleFileSystemOpen),
                                           remoteFileLocation.toAscii(), flags);
    appendMessage(tr("Copying \"%1\"...").arg(packageName), false);
    m_timeoutTimer->start();
}

void S60DeployStep::initFileInstallation()
{
    QTC_ASSERT(m_currentFileIndex < m_signedPackages.count(), return);
    QTC_ASSERT(m_currentFileIndex >= 0, return);

    if (!m_codaDevice)
        return;

    QString packageName(QFileInfo(m_signedPackages.at(m_currentFileIndex)).fileName());
    QString remoteFileLocation = QString::fromLatin1("%1:\\Data\\%2").arg(m_installationDrive).arg(packageName);
    if (m_silentInstall) {
        m_codaDevice->sendSymbianInstallSilentInstallCommand(Coda::CodaCallback(this, &S60DeployStep::handleSymbianInstall),
                                                            remoteFileLocation.toAscii(), QString::fromLatin1("%1:").arg(m_installationDrive).toAscii());
        appendMessage(tr("Installing package \"%1\" on drive %2:...").arg(packageName).arg(m_installationDrive), false);
    } else {
        m_codaDevice->sendSymbianInstallUIInstallCommand(Coda::CodaCallback(this, &S60DeployStep::handleSymbianInstall),
                                                        remoteFileLocation.toAscii());
        appendMessage(tr("Continue the installation on your device."), false);
        emit manualInstallation();
    }
}

void S60DeployStep::startTransferring()
{
    m_currentFileIndex = 0;
    initFileSending();
    setState(StateSendingData);
}

void S60DeployStep::startInstalling()
{
    m_currentFileIndex = 0;
    initFileInstallation();
    setState(StateInstalling);
}

void S60DeployStep::handleFileSystemOpen(const Coda::CodaCommandResult &result)
{
    if (result.type != Coda::CodaCommandResult::SuccessReply) {
        reportError(tr("Could not open remote file: %1").arg(result.errorString()));
        return;
    }

    if (result.values.size() < 1 || result.values.at(0).data().isEmpty()) {
        reportError(tr("Internal error: No filehandle obtained"));
        return;
    }

    m_remoteFileHandle = result.values.at(0).data();

    const QString fileName = m_signedPackages.at(m_currentFileIndex);
    m_putFile.reset(new QFile(fileName));
    if (!m_putFile->open(QIODevice::ReadOnly)) { // Should not fail, was checked before
        reportError(tr("Could not open local file %1: %2").arg(fileName, m_putFile->errorString()));
        return;
    }
    putSendNextChunk();
}

void S60DeployStep::handleSymbianInstall(const Coda::CodaCommandResult &result)
{
    if (result.type == Coda::CodaCommandResult::SuccessReply) {
        appendMessage(tr("Installation has finished"), false);
        if (++m_currentFileIndex >= m_signedPackages.count()) {
            setState(StateFinished);
            emit allFilesInstalled();
        } else
            initFileInstallation();
    } else {
        reportError(tr("Installation failed: %1; "
                       "see %2 for descriptions of the error codes")
                    .arg(result.errorString(),
                         QLatin1String("http://www.developer.nokia.com/Community/Wiki/Symbian_OS_Error_Codes")));
    }
}

void S60DeployStep::putSendNextChunk()
{
    if (!m_codaDevice)
        return;
    QTC_ASSERT(m_putFile, return);

    // Read and send off next chunk
    const quint64 pos = m_putFile->pos();
    const QByteArray data = m_putFile->read(m_putChunkSize);
    const quint64 size = m_putFile->size();
    if (data.isEmpty()) {
        m_putWriteOk = true;
        closeFiles();
        setCopyProgress(100);
    } else {
        m_putLastChunkSize = data.size();
        if (debug > 1)
            qDebug("Writing %llu bytes to remote file '%s' at %llu\n",
                   m_putLastChunkSize,
                   m_remoteFileHandle.constData(), pos);
        m_codaDevice->sendFileSystemWriteCommand(Coda::CodaCallback(this, &S60DeployStep::handleFileSystemWrite),
                                                m_remoteFileHandle, data, unsigned(pos));
        setCopyProgress((100*(m_putLastChunkSize+pos))/size);
        m_timeoutTimer->start();
    }
}

void S60DeployStep::closeFiles()
{
    m_putFile.reset();
    QTC_ASSERT(m_codaDevice, return);

    emit addOutput(QLatin1String("\n"), ProjectExplorer::BuildStep::MessageOutput);
    m_codaDevice->sendFileSystemCloseCommand(Coda::CodaCallback(this, &S60DeployStep::handleFileSystemClose),
                                            m_remoteFileHandle);
}

void S60DeployStep::handleFileSystemWrite(const Coda::CodaCommandResult &result)
{
    m_timeoutTimer->stop();
    // Close remote file even if copy fails
    m_putWriteOk = result;
    if (!m_putWriteOk) {
        QString packageName(QFileInfo(m_signedPackages.at(m_currentFileIndex)).fileName());
        reportError(tr("Could not write to file %1 on device: %2").arg(packageName).arg(result.errorString()));
    }

    if (!m_putWriteOk || m_putLastChunkSize < m_putChunkSize) {
        closeFiles();
    } else {
        putSendNextChunk();
    }
}

void S60DeployStep::handleFileSystemClose(const Coda::CodaCommandResult &result)
{
    if (result.type == Coda::CodaCommandResult::SuccessReply) {
        if (debug)
            qDebug("File closed.\n");
        if (++m_currentFileIndex >= m_signedPackages.count())
            emit allFilesSent();
        else
            initFileSending();
    } else {
        reportError(tr("Failed to close the remote file: %1").arg(result.toString()));
    }
}

void S60DeployStep::checkForTimeout()
{
    if (state() != StateConnecting)
        return;
    QMessageBox *mb = CodaRunControl::createCodaWaitingMessageBox(Core::ICore::mainWindow());
    connect(this, SIGNAL(codaConnected()), mb, SLOT(close()));
    connect(this, SIGNAL(s60DeploymentFinished()), mb, SLOT(close()));
    connect(this, SIGNAL(finishNow()), mb, SLOT(close()));
    connect(mb, SIGNAL(finished(int)), this, SLOT(slotWaitingForCodaClosed(int)));
    mb->open();
}

void S60DeployStep::showManualInstallationInfo()
{
    const QString title  = tr("Installation");
    const QString text = tr("Continue the installation on your device.");
    QMessageBox *mb = new QMessageBox(QMessageBox::Information, title, text,
                                      QMessageBox::Ok, Core::ICore::mainWindow());
    connect(this, SIGNAL(allFilesInstalled()), mb, SLOT(close()));
    connect(this, SIGNAL(s60DeploymentFinished()), mb, SLOT(close()));
    connect(this, SIGNAL(finishNow()), mb, SLOT(close()));
    mb->open();
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
                                           Utils::FileName(), -1,
                                           Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM)));
        emit finishNow(false);
    }
}

void S60DeployStep::deploymentFinished(bool success)
{
    m_deployResult = success;
    if(m_deployResult && m_futureInterface)
        m_futureInterface->setProgressValue(m_futureInterface->progressMaximum());
    if (m_eventLoop)
        m_eventLoop->exit();
}

void S60DeployStep::deviceRemoved(const SymbianUtils::SymbianDevice &device)
{
    if (device.portName() == m_serialPortName)
        reportError(tr("The device '%1' has been disconnected").arg(device.friendlyName()));
}

void S60DeployStep::setCopyProgress(int progress)
{
    if (progress < 0)
        progress = 0;
    else if (progress > 100)
        progress = 100;
    if (copyProgress() == progress)
        return;
    m_copyProgress = progress;
    emit addOutput(QLatin1String("."), ProjectExplorer::BuildStep::MessageOutput, DontAppendNewline);
    emit copyProgressChanged(m_copyProgress);
}

int S60DeployStep::copyProgress() const
{
    return m_copyProgress;
}

void S60DeployStep::updateProgress(int progress)
{
    //This would show the percentage on the Compile output
    //appendMessage(tr("Copy percentage: %1%").arg((m_currentFileIndex*100 + progress) /m_signedPackages.count()), false);
    int copyProgress = ((m_currentFileIndex*100 + progress) /m_signedPackages.count());
    int entireProgress = copyProgress * 0.8; //the copy progress is just 80% of the whole deployment progress
    m_futureInterface->setProgressValueAndText(entireProgress, tr("Copy progress: %1%").arg(copyProgress));
}

void S60DeployStep::timeout()
{
    reportError(tr("A timeout while deploying has occurred. CODA might not be responding. Try reconnecting the device."));
}

BuildStepConfigWidget *S60DeployStep::createConfigWidget()
{
    return new SimpleBuildStepConfigWidget(this);
}

// #pragma mark -- S60DeployStepFactory

S60DeployStepFactory::S60DeployStepFactory(QObject *parent) :
    ProjectExplorer::IBuildStepFactory(parent)
{
}

S60DeployStepFactory::~S60DeployStepFactory()
{
}

bool S60DeployStepFactory::canCreate(ProjectExplorer::BuildStepList *parent, const Core::Id id) const
{
    return canHandle(parent) && id == Core::Id(S60_DEPLOY_STEP_ID);
}

ProjectExplorer::BuildStep *S60DeployStepFactory::create(ProjectExplorer::BuildStepList *parent, const Core::Id id)
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

bool S60DeployStepFactory::canHandle(BuildStepList *parent) const
{
    if (parent->id() != Core::Id(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY))
        return false;
    Core::Id deviceType = ProjectExplorer::DeviceTypeProfileInformation::deviceTypeId(parent->target()->profile());
    if (deviceType != SymbianIDeviceFactory::deviceType())
        return false;
    return qobject_cast<Qt4Project *>(parent->target()->project());
}

bool S60DeployStepFactory::canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const
{
    return canCreate(parent, ProjectExplorer::idFromMap(map));
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

QList<Core::Id> S60DeployStepFactory::availableCreationIds(ProjectExplorer::BuildStepList *parent) const
{
    if (!canHandle(parent))
        return QList<Core::Id>();
    return QList<Core::Id>() << Core::Id(S60_DEPLOY_STEP_ID);
}

QString S60DeployStepFactory::displayNameForId(const Core::Id id) const
{
    if (id == Core::Id(S60_DEPLOY_STEP_ID))
        return tr("Deploy SIS Package");
    return QString();
}
