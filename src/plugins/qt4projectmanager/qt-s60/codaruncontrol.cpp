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

#include "codaruncontrol.h"

#include "s60deployconfiguration.h"
#include "s60devicerunconfiguration.h"
#include "symbianidevice.h"

#include "codadevice.h"
#include "codamessage.h"

#include "qt4buildconfiguration.h"
#include "symbiandevicemanager.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <symbianutils/symbiandevicemanager.h>

#include <QDir>
#include <QFileInfo>
#include <QTimer>

#include <QMessageBox>

#include <QTcpSocket>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
using namespace Coda;

enum { debug = 0 };

CodaRunControl::CodaRunControl(RunConfiguration *runConfiguration, RunMode mode) :
    S60RunControlBase(runConfiguration, mode),
    m_port(0),
    m_state(StateUninit),
    m_stopAfterConnect(false)
{
    const S60DeviceRunConfiguration *s60runConfig = qobject_cast<S60DeviceRunConfiguration *>(runConfiguration);
    QTC_ASSERT(s60runConfig, return);
    const S60DeployConfiguration *activeDeployConf = qobject_cast<S60DeployConfiguration *>(s60runConfig->target()->activeDeployConfiguration());
    QTC_ASSERT(activeDeployConf, return);
    QTC_ASSERT(activeDeployConf->device(), return);

    SymbianIDevice::CommunicationChannel channel = activeDeployConf->device()->communicationChannel();
    if (channel == SymbianIDevice::CommunicationCodaTcpConnection) {
        m_address = activeDeployConf->device()->address();
        m_port = activeDeployConf->device()->port().toInt();
    } else if (channel == SymbianIDevice::CommunicationCodaSerialConnection) {
        m_serialPort = activeDeployConf->device()->serialPortName();
    } else {
        QTC_ASSERT(false, return);
    }
}

CodaRunControl::~CodaRunControl()
{
}

bool CodaRunControl::doStart()
{
    if (m_address.isEmpty() && m_serialPort.isEmpty()) {
        cancelProgress();
        QString msg = tr("No device is connected. Please connect a device and try again.\n");
        appendMessage(msg, Utils::NormalMessageFormat);
        return false;
    }
    appendMessage(tr("Executable file: %1\n").arg(msgListFile(executableFileName())),
                  Utils::NormalMessageFormat);
    return true;
}

bool CodaRunControl::isRunning() const
{
    return m_state >= StateConnecting;
}

QIcon CodaRunControl::icon() const
{
    return QIcon(QLatin1String(ProjectExplorer::Constants::ICON_DEBUG_SMALL));
}

bool CodaRunControl::setupLauncher()
{
    QTC_ASSERT(!m_codaDevice, return false);

    if (m_serialPort.length()) {
        // We get the port from SymbianDeviceManager
        appendMessage(tr("Connecting to '%1'...\n").arg(m_serialPort), Utils::NormalMessageFormat);
        m_codaDevice = SymbianUtils::SymbianDeviceManager::instance()->getCodaDevice(m_serialPort);
        if (m_codaDevice.isNull()) {
            appendMessage(tr("Unable to create CODA connection. Please try again.\n"), Utils::ErrorMessageFormat);
            return false;
        }
        if (!m_codaDevice->device()->isOpen()) {
            appendMessage(tr("Could not open serial device: %1\n").arg(m_codaDevice->device()->errorString()), Utils::ErrorMessageFormat);
            return false;
        }
        connect(SymbianUtils::SymbianDeviceManager::instance(), SIGNAL(deviceRemoved(SymbianUtils::SymbianDevice)),
                this, SLOT(deviceRemoved(SymbianUtils::SymbianDevice)));
        m_state = StateConnecting;
        m_codaDevice->sendSerialPing(false);
    } else {
        // For TCP we don't use device manager, we just set it up directly
        m_codaDevice = QSharedPointer<Coda::CodaDevice>(new Coda::CodaDevice, &QObject::deleteLater); // finishRunControl, which deletes m_codaDevice, can get called from within a coda callback, so need to use deleteLater
        const QSharedPointer<QTcpSocket> codaSocket(new QTcpSocket);
        m_codaDevice->setDevice(codaSocket);
        codaSocket->connectToHost(m_address, m_port);
        m_state = StateConnecting;
        appendMessage(tr("Connecting to %1:%2...\n").arg(m_address).arg(m_port), Utils::NormalMessageFormat);
    }

    connect(m_codaDevice.data(), SIGNAL(error(QString)), this, SLOT(slotError(QString)));
    connect(m_codaDevice.data(), SIGNAL(logMessage(QString)), this, SLOT(slotCodaLogMessage(QString)));
    connect(m_codaDevice.data(), SIGNAL(codaEvent(Coda::CodaEvent)), this, SLOT(slotCodaEvent(Coda::CodaEvent)));

    QTimer::singleShot(5000, this, SLOT(checkForTimeout()));
    if (debug)
        m_codaDevice->setVerbose(debug);
    return true;
}

void CodaRunControl::doStop()
{
    switch (m_state) {
    case StateUninit:
    case StateConnecting:
    case StateConnected:
        finishRunControl();
        break;
    case StateProcessRunning:
        QTC_ASSERT(!m_runningProcessId.isEmpty(), return);
        m_codaDevice->sendRunControlTerminateCommand(CodaCallback(),
                                                     m_runningProcessId.toAscii());
        break;
    default:
        if (debug)
            qDebug() << "Unrecognised state while performing shutdown" << m_state;
        break;
    }
}

void CodaRunControl::slotError(const QString &error)
{
    appendMessage(tr("Error: %1\n").arg(error), Utils::ErrorMessageFormat);
    finishRunControl();
}

void CodaRunControl::slotCodaLogMessage(const QString &log)
{
    if (debug > 1)
        qDebug("CODA log: %s", qPrintable(log.size()>200?log.left(200).append(QLatin1String("...")): log));
}

void CodaRunControl::slotCodaEvent(const CodaEvent &event)
{
    if (debug)
        qDebug() << "CODA event:" << "Type:" << event.type() << "Message:" << event.toString();

    switch (event.type()) {
    case CodaEvent::LocatorHello:
        handleConnected(event);
        break;
    case CodaEvent::RunControlContextRemoved:
        handleContextRemoved(event);
        break;
    case CodaEvent::RunControlContextAdded:
        m_state = StateProcessRunning;
        reportLaunchFinished();
        handleContextAdded(event);
        break;
    case CodaEvent::RunControlSuspended:
        handleContextSuspended(event);
        break;
    case CodaEvent::RunControlModuleLoadSuspended:
        handleModuleLoadSuspended(event);
        break;
    case CodaEvent::LoggingWriteEvent:
        handleLogging(event);
        break;
    case CodaEvent::ProcessExitedEvent:
        handleProcessExited(event);
        break;
    default:
        if (debug)
            qDebug() << "CODA event not handled" << event.type();
        break;
    }
}

void CodaRunControl::initCommunication()
{
    m_codaDevice->sendDebugSessionControlSessionStartCommand(CodaCallback(this, &CodaRunControl::handleDebugSessionStarted));
}

void CodaRunControl::handleConnected(const CodaEvent &event)
{
    if (m_state >= StateConnected)
        return;
    m_state = StateConnected;
    appendMessage(tr("Connected.\n"), Utils::NormalMessageFormat);
    setProgress(maxProgress()*0.80);

    m_codaServices = static_cast<const CodaLocatorHelloEvent &>(event).services();

    emit connected();
    if (!m_stopAfterConnect)
        initCommunication();
}

void CodaRunControl::handleContextRemoved(const CodaEvent &event)
{
    const QVector<QByteArray> removedItems
            = static_cast<const CodaRunControlContextRemovedEvent &>(event).ids();
    if (!m_runningProcessId.isEmpty()
            && removedItems.contains(m_runningProcessId.toAscii())) {
        m_codaDevice->sendDebugSessionControlSessionEndCommand(CodaCallback(this, &CodaRunControl::handleDebugSessionEnded));
    }
}

void CodaRunControl::handleContextAdded(const CodaEvent &event)
{
    typedef CodaRunControlContextAddedEvent CodaAddedEvent;

    const CodaAddedEvent &me = static_cast<const CodaAddedEvent &>(event);
    foreach (const RunControlContext &context, me.contexts()) {
        if (context.parentId == "root") //is the created context a process
            m_runningProcessId = QLatin1String(context.id);
    }
}

void CodaRunControl::handleContextSuspended(const CodaEvent &event)
{
    typedef CodaRunControlContextSuspendedEvent CodaSuspendEvent;

    const CodaSuspendEvent &me = static_cast<const CodaSuspendEvent &>(event);

    switch (me.reason()) {
    case CodaSuspendEvent::Other:
    case CodaSuspendEvent::Crash:
        appendMessage(tr("Thread has crashed: %1\n").arg(QString::fromLatin1(me.message())), Utils::ErrorMessageFormat);

        if (me.reason() == CodaSuspendEvent::Crash)
            stop();
        else
            m_codaDevice->sendRunControlResumeCommand(CodaCallback(), me.id()); //TODO: Should I resume automatically
        break;
    default:
        if (debug)
            qDebug() << "Context suspend not handled:" << "Reason:" << me.reason() << "Message:" << me.message();
        break;
    }
}

void CodaRunControl::handleModuleLoadSuspended(const CodaEvent &event)
{
    // Debug mode start: Continue:
    typedef CodaRunControlModuleLoadContextSuspendedEvent CodaModuleLoadSuspendedEvent;

    const CodaModuleLoadSuspendedEvent &me = static_cast<const CodaModuleLoadSuspendedEvent &>(event);
    if (me.info().requireResume)
        m_codaDevice->sendRunControlResumeCommand(CodaCallback(), me.id());
}

void CodaRunControl::handleLogging(const CodaEvent &event)
{
    const CodaLoggingWriteEvent &me = static_cast<const CodaLoggingWriteEvent &>(event);
    appendMessage(QString::fromLatin1(QByteArray(me.message() + '\n')), Utils::StdOutFormat);
}

void CodaRunControl::handleProcessExited(const CodaEvent &event)
{
    Q_UNUSED(event)
    appendMessage(tr("Process has finished.\n"), Utils::NormalMessageFormat);
    m_codaDevice->sendDebugSessionControlSessionEndCommand(CodaCallback(this, &CodaRunControl::handleDebugSessionEnded));
}

void CodaRunControl::handleAddListener(const CodaCommandResult &result)
{
    Q_UNUSED(result)
    m_codaDevice->sendSymbianOsDataFindProcessesCommand(CodaCallback(this, &CodaRunControl::handleFindProcesses),
                                                        QByteArray(),
                                                        QByteArray::number(executableUid(), 16));
}

void CodaRunControl::handleDebugSessionStarted(const CodaCommandResult &result)
{
    Q_UNUSED(result)
    if (m_codaDevice.isNull()) {
        finishRunControl();
        return;
    }
    m_state = StateDebugSessionStarted;
    m_codaDevice->sendLoggingAddListenerCommand(CodaCallback(this, &CodaRunControl::handleAddListener));
}

void CodaRunControl::handleDebugSessionEnded(const CodaCommandResult &result)
{
    Q_UNUSED(result)
    m_state = StateDebugSessionEnded;
    finishRunControl();
}

void CodaRunControl::handleFindProcesses(const CodaCommandResult &result)
{
    if (result.values.size() && result.values.at(0).type() == Json::JsonValue::Array && result.values.at(0).children().count()) {
        //there are processes running. Cannot run mine
        appendMessage(tr("The process is already running on the device. Please first close it.\n"), Utils::ErrorMessageFormat);
        finishRunControl();
    } else {
        setProgress(maxProgress()*0.90);
        m_codaDevice->sendProcessStartCommand(CodaCallback(this, &CodaRunControl::handleCreateProcess),
                                              executableName(),
                                              executableUid(),
                                              commandLineArguments().split(QLatin1Char(' ')),
                                              QString(),
                                              true);
        appendMessage(tr("Launching: %1\n").arg(executableName()), Utils::NormalMessageFormat);
    }
}

void CodaRunControl::handleCreateProcess(const CodaCommandResult &result)
{
    const bool ok = result.type == CodaCommandResult::SuccessReply;
    bool processCreated = false;
    if (ok) {
        if (result.values.size()) {
            Json::JsonValue id = result.values.at(0).findChild("ID");
            if (id.isValid()) {
                m_state = StateProcessRunning;
                m_runningProcessId = QLatin1String(id.data());
                processCreated = true;
            }
        }
    }
    if (processCreated) {
        setProgress(maxProgress());
        appendMessage(tr("Launched.\n"), Utils::NormalMessageFormat);
    } else {
        appendMessage(tr("Launch failed: %1\n").arg(result.toString()), Utils::ErrorMessageFormat);
        finishRunControl();
    }
}

void CodaRunControl::finishRunControl()
{
    m_runningProcessId.clear();
    if (m_codaDevice) {
        disconnect(m_codaDevice.data(), 0, this, 0);
        SymbianUtils::SymbianDeviceManager::instance()->releaseCodaDevice(m_codaDevice);
    }
    m_state = StateUninit;
    emit finished();
}

QMessageBox *CodaRunControl::createCodaWaitingMessageBox(QWidget *parent)
{
    const QString title  = tr("Waiting for CODA");
    const QString text = tr("Qt Creator is waiting for the CODA application to connect.<br>"
                            "Please make sure the application is running on "
                            "your mobile phone and the right IP address and/or port are "
                            "configured in the project settings.");
    QMessageBox *mb = new QMessageBox(QMessageBox::Information, title, text, QMessageBox::Cancel, parent);
    return mb;
}

void CodaRunControl::checkForTimeout()
{
    if (m_state != StateConnecting)
        return;

    QMessageBox *mb = createCodaWaitingMessageBox(Core::ICore::mainWindow());
    connect(this, SIGNAL(finished()), mb, SLOT(close()));
    connect(mb, SIGNAL(finished(int)), this, SLOT(cancelConnection()));
    mb->open();
}

void CodaRunControl::cancelConnection()
{
    if (m_state != StateConnecting)
        return;

    stop();
    appendMessage(tr("Canceled.\n"), Utils::ErrorMessageFormat);
    emit finished();
}

void CodaRunControl::deviceRemoved(const SymbianUtils::SymbianDevice &device)
{
    if (m_codaDevice && device.portName() == m_serialPort) {
        QString msg = tr("The device '%1' has been disconnected.\n").arg(device.friendlyName());
        appendMessage(msg, Utils::ErrorMessageFormat);
        finishRunControl();
    }
}

void CodaRunControl::connect()
{
    m_stopAfterConnect = true;
    start();
}

void CodaRunControl::run()
{
    initCommunication();
}
