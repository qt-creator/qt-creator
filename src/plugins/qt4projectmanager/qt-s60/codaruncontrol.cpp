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

#include "codaruncontrol.h"

#include "s60deployconfiguration.h"
#include "s60devicerunconfiguration.h"

#include "codadevice.h"
#include "codamessage.h"

#include "qt4buildconfiguration.h"
#include "qt4symbiantarget.h"
#include "qt4target.h"
#include "qtoutputformatter.h"
#include "symbiandevicemanager.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

#include <symbianutils/symbiandevicemanager.h>

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QScopedPointer>
#include <QtCore/QTimer>

#include <QtGui/QMessageBox>
#include <QtGui/QMainWindow>

#include <QtNetwork/QTcpSocket>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;
using namespace Coda;

enum { debug = 0 };

CodaRunControl::CodaRunControl(RunConfiguration *runConfiguration, const QString &mode) :
    S60RunControlBase(runConfiguration, mode),
    m_port(0),
    m_state(StateUninit)
{
    const S60DeviceRunConfiguration *s60runConfig = qobject_cast<S60DeviceRunConfiguration *>(runConfiguration);
    QTC_ASSERT(s60runConfig, return);
    const S60DeployConfiguration *activeDeployConf = qobject_cast<S60DeployConfiguration *>(s60runConfig->qt4Target()->activeDeployConfiguration());
    QTC_ASSERT(activeDeployConf, return);

    S60DeployConfiguration::CommunicationChannel channel = activeDeployConf->communicationChannel();
    if (channel == S60DeployConfiguration::CommunicationCodaTcpConnection) {
        m_address = activeDeployConf->deviceAddress();
        m_port = activeDeployConf->devicePort().toInt();
    } else if (channel == S60DeployConfiguration::CommunicationCodaSerialConnection) {
        m_serialPort = activeDeployConf->serialPortName();
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
        QString msg = tr("No device is connected. Please connect a device and try again.");
        appendMessage(msg, NormalMessageFormat);
        return false;
    }
    appendMessage(tr("Executable file: %1").arg(msgListFile(executableFileName())),
                  NormalMessageFormat);
    return true;
}

bool CodaRunControl::isRunning() const
{
    return m_state >= StateConnecting;
}

bool CodaRunControl::setupLauncher()
{
    QTC_ASSERT(!m_codaDevice, return false);

    if (m_serialPort.length()) {
        // We get the port from SymbianDeviceManager
        appendMessage(tr("Connecting to '%1'...").arg(m_serialPort), NormalMessageFormat);
        m_codaDevice = SymbianUtils::SymbianDeviceManager::instance()->getCodaDevice(m_serialPort);

        bool ok = m_codaDevice && m_codaDevice->device()->isOpen();
        if (!ok) {
            appendMessage(tr("Could not open serial device: %1").arg(m_codaDevice->device()->errorString()), ErrorMessageFormat);
            return false;
        }
        connect(SymbianUtils::SymbianDeviceManager::instance(), SIGNAL(deviceRemoved(const SymbianUtils::SymbianDevice)),
                this, SLOT(deviceRemoved(SymbianUtils::SymbianDevice)));
        connect(m_codaDevice.data(), SIGNAL(error(QString)), this, SLOT(slotError(QString)));
        connect(m_codaDevice.data(), SIGNAL(logMessage(QString)), this, SLOT(slotTrkLogMessage(QString)));
        connect(m_codaDevice.data(), SIGNAL(tcfEvent(Coda::CodaEvent)), this, SLOT(slotCodaEvent(Coda::CodaEvent)));
        connect(m_codaDevice.data(), SIGNAL(serialPong(QString)), this, SLOT(slotSerialPong(QString)));
        m_state = StateConnecting;
        m_codaDevice->sendSerialPing(false);
    } else {
        // For TCP we don't use device manager, we just set it up directly
        m_codaDevice = QSharedPointer<Coda::CodaDevice>(new Coda::CodaDevice);
        connect(m_codaDevice.data(), SIGNAL(error(QString)), this, SLOT(slotError(QString)));
        connect(m_codaDevice.data(), SIGNAL(logMessage(QString)), this, SLOT(slotTrkLogMessage(QString)));
        connect(m_codaDevice.data(), SIGNAL(tcfEvent(Coda::CodaEvent)), this, SLOT(slotCodaEvent(Coda::CodaEvent)));

        const QSharedPointer<QTcpSocket> codaSocket(new QTcpSocket);
        m_codaDevice->setDevice(codaSocket);
        codaSocket->connectToHost(m_address, m_port);
        m_state = StateConnecting;
        appendMessage(tr("Connecting to %1:%2...").arg(m_address).arg(m_port), NormalMessageFormat);

    }
    QTimer::singleShot(4000, this, SLOT(checkForTimeout()));
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
    }
}

void CodaRunControl::slotError(const QString &error)
{
    appendMessage(tr("Error: %1").arg(error), ErrorMessageFormat);
    finishRunControl();
}

void CodaRunControl::slotTrkLogMessage(const QString &log)
{
    if (debug > 1)
        qDebug("CODA log: %s", qPrintable(log.size()>200?log.left(200).append(QLatin1String(" ...")): log));
}

void CodaRunControl::slotSerialPong(const QString &message)
{
    if (debug > 1)
        qDebug() << "CODA serial pong:" << message;
}

void CodaRunControl::slotCodaEvent(const CodaEvent &event)
{
    if (debug)
        qDebug() << "CODA event:" << "Type:" << event.type() << "Message:" << event.toString();

    switch (event.type()) {
    case CodaEvent::LocatorHello: { // Commands accepted now
        m_state = StateConnected;
        appendMessage(tr("Connected."), NormalMessageFormat);
        setProgress(maxProgress()*0.80);
        initCommunication();
    }
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
    default:
        if (debug)
            qDebug() << "CODA event not handled" << event.type();
        break;
    }
}

void CodaRunControl::initCommunication()
{
    m_codaDevice->sendLoggingAddListenerCommand(CodaCallback(this, &CodaRunControl::handleAddListener));
}

void CodaRunControl::handleContextRemoved(const CodaEvent &event)
{
    const QVector<QByteArray> removedItems
            = static_cast<const CodaRunControlContextRemovedEvent &>(event).ids();
    if (!m_runningProcessId.isEmpty()
            && removedItems.contains(m_runningProcessId.toAscii())) {
        appendMessage(tr("Process has finished."), NormalMessageFormat);
        finishRunControl();
    }
}

void CodaRunControl::handleContextAdded(const CodaEvent &event)
{
    typedef CodaRunControlContextAddedEvent TcfAddedEvent;

    const TcfAddedEvent &me = static_cast<const TcfAddedEvent &>(event);
    foreach (const RunControlContext &context, me.contexts()) {
        if (context.parentId == "root") //is the created context a process
            m_runningProcessId = QLatin1String(context.id);
    }
}

void CodaRunControl::handleContextSuspended(const CodaEvent &event)
{
    typedef CodaRunControlContextSuspendedEvent TcfSuspendEvent;

    const TcfSuspendEvent &me = static_cast<const TcfSuspendEvent &>(event);

    switch (me.reason()) {
    case TcfSuspendEvent::Other:
    case TcfSuspendEvent::Crash:
        appendMessage(tr("Thread has crashed: %1").arg(QString::fromLatin1(me.message())), ErrorMessageFormat);

        if (me.reason() == TcfSuspendEvent::Crash)
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
    typedef CodaRunControlModuleLoadContextSuspendedEvent TcfModuleLoadSuspendedEvent;

    const TcfModuleLoadSuspendedEvent &me = static_cast<const TcfModuleLoadSuspendedEvent &>(event);
    if (me.info().requireResume)
        m_codaDevice->sendRunControlResumeCommand(CodaCallback(), me.id());
}

void CodaRunControl::handleLogging(const CodaEvent &event)
{
    const CodaLoggingWriteEvent &me = static_cast<const CodaLoggingWriteEvent &>(event);
    appendMessage(me.message(), StdOutFormat);
}

void CodaRunControl::handleAddListener(const CodaCommandResult &result)
{
    Q_UNUSED(result)
    m_codaDevice->sendSymbianOsDataFindProcessesCommand(CodaCallback(this, &CodaRunControl::handleFindProcesses),
                                                          QByteArray(),
                                                          QByteArray::number(executableUid(), 16));
}

void CodaRunControl::handleFindProcesses(const CodaCommandResult &result)
{
    if (result.values.size() && result.values.at(0).type() == JsonValue::Array && result.values.at(0).children().count()) {
        //there are processes running. Cannot run mine
        appendMessage(tr("The process is already running on the device. Please first close it."), ErrorMessageFormat);
        finishRunControl();
    } else {
        setProgress(maxProgress()*0.90);
        m_codaDevice->sendProcessStartCommand(CodaCallback(this, &CodaRunControl::handleCreateProcess),
                                              executableName(),
                                              executableUid(),
                                              commandLineArguments().split(' '),
                                              QString(),
                                              true);
        appendMessage(tr("Launching: %1").arg(executableName()), NormalMessageFormat);
    }
}

void CodaRunControl::handleCreateProcess(const CodaCommandResult &result)
{
    const bool ok = result.type == CodaCommandResult::SuccessReply;
    if (ok) {
        setProgress(maxProgress());
        appendMessage(tr("Launched."), NormalMessageFormat);
    } else {
        appendMessage(tr("Launch failed: %1").arg(result.toString()), ErrorMessageFormat);
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

    QMessageBox *mb = createCodaWaitingMessageBox(Core::ICore::instance()->mainWindow());
    connect(this, SIGNAL(finished()), mb, SLOT(close()));
    connect(mb, SIGNAL(finished(int)), this, SLOT(cancelConnection()));
    mb->open();
}

void CodaRunControl::cancelConnection()
{
    if (m_state != StateConnecting)
        return;

    stop();
    appendMessage(tr("Canceled."), ErrorMessageFormat);
    emit finished();
}

void CodaRunControl::deviceRemoved(const SymbianUtils::SymbianDevice &device)
{
    if (m_codaDevice && device.portName() == m_serialPort) {
        QString msg = tr("The device '%1' has been disconnected").arg(device.friendlyName());
        appendMessage(msg, ErrorMessageFormat);
        finishRunControl();
    }
}
