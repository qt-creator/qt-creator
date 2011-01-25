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

#include "tcftrkdevice.h"
#include "tcftrkmessage.h"

#include "qt4buildconfiguration.h"
#include "qt4symbiantarget.h"
#include "qt4target.h"
#include "qtoutputformatter.h"
#include "virtualserialdevice.h"

#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

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
using namespace tcftrk;

enum { debug = 0 };

CodaRunControl::CodaRunControl(RunConfiguration *runConfiguration, const QString &mode) :
    S60RunControlBase(runConfiguration, mode),
    m_tcfTrkDevice(0),
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
    if (m_tcfTrkDevice)
        m_tcfTrkDevice->deleteLater();
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
    QTC_ASSERT(!m_tcfTrkDevice, return false);

    m_tcfTrkDevice = new TcfTrkDevice;
    if (debug)
        m_tcfTrkDevice->setVerbose(1);

    connect(m_tcfTrkDevice, SIGNAL(error(QString)), this, SLOT(slotError(QString)));
    connect(m_tcfTrkDevice, SIGNAL(logMessage(QString)), this, SLOT(slotTrkLogMessage(QString)));
    connect(m_tcfTrkDevice, SIGNAL(tcfEvent(tcftrk::TcfTrkEvent)), this, SLOT(slotTcftrkEvent(tcftrk::TcfTrkEvent)));
    connect(m_tcfTrkDevice, SIGNAL(serialPong(QString)), this, SLOT(slotSerialPong(QString)));

    if (m_serialPort.length()) {
        const QSharedPointer<SymbianUtils::VirtualSerialDevice> serialDevice(new SymbianUtils::VirtualSerialDevice(m_serialPort));
        appendMessage(tr("Conecting to '%2'...").arg(m_serialPort), NormalMessageFormat);
        m_tcfTrkDevice->setSerialFrame(true);
        m_tcfTrkDevice->setDevice(serialDevice);
        bool ok = serialDevice->open(QIODevice::ReadWrite);
        if (!ok) {
            appendMessage(tr("Couldn't open serial device: %1").arg(serialDevice->errorString()), ErrorMessageFormat);
            return false;
        }
        m_state = StateConnecting;
        m_tcfTrkDevice->sendSerialPing(false);
    } else {
        const QSharedPointer<QTcpSocket> tcfTrkSocket(new QTcpSocket);
        m_tcfTrkDevice->setDevice(tcfTrkSocket);
        tcfTrkSocket->connectToHost(m_address, m_port);
        m_state = StateConnecting;
        appendMessage(tr("Connecting to %1:%2...").arg(m_address).arg(m_port), NormalMessageFormat);
        QTimer::singleShot(4000, this, SLOT(checkForTimeout()));
    }
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
        m_tcfTrkDevice->sendRunControlTerminateCommand(TcfTrkCallback(),
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
    if (debug) {
        qDebug("CODA log: %s", qPrintable(log.size()>200?log.left(200).append(QLatin1String(" ...")): log));
    }
}

void CodaRunControl::slotSerialPong(const QString &message)
{
    if (debug)
        qDebug() << "CODA serial pong:" << message;
}

void CodaRunControl::slotTcftrkEvent(const TcfTrkEvent &event)
{
    if (debug)
        qDebug() << "CODA event:" << "Type:" << event.type() << "Message:" << event.toString();

    switch (event.type()) {
    case TcfTrkEvent::LocatorHello: { // Commands accepted now
        m_state = StateConnected;
        appendMessage(tr("Connected."), NormalMessageFormat);
        setProgress(maxProgress()*0.80);
        initCommunication();
    }
    break;
    case TcfTrkEvent::RunControlContextRemoved:
        handleContextRemoved(event);
        break;
    case TcfTrkEvent::RunControlContextAdded:
        m_state = StateProcessRunning;
        reportLaunchFinished();
        handleContextAdded(event);
        break;
    case TcfTrkEvent::RunControlSuspended:
        handleContextSuspended(event);
        break;
    case TcfTrkEvent::RunControlModuleLoadSuspended:
        handleModuleLoadSuspended(event);
        break;
    case TcfTrkEvent::LoggingWriteEvent:
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
    m_tcfTrkDevice->sendLoggingAddListenerCommand(TcfTrkCallback(this, &CodaRunControl::handleAddListener));
}

void CodaRunControl::handleContextRemoved(const TcfTrkEvent &event)
{
    const QVector<QByteArray> removedItems
            = static_cast<const TcfTrkRunControlContextRemovedEvent &>(event).ids();
    if (!m_runningProcessId.isEmpty()
            && removedItems.contains(m_runningProcessId.toAscii())) {
        appendMessage(tr("Process has finished."), NormalMessageFormat);
        finishRunControl();
    }
}

void CodaRunControl::handleContextAdded(const TcfTrkEvent &event)
{
    typedef TcfTrkRunControlContextAddedEvent TcfAddedEvent;

    const TcfAddedEvent &me = static_cast<const TcfAddedEvent &>(event);
    foreach (const RunControlContext &context, me.contexts()) {
        if (context.parentId == "root") //is the created context a process
            m_runningProcessId = QLatin1String(context.id);
    }
}

void CodaRunControl::handleContextSuspended(const TcfTrkEvent &event)
{
    typedef TcfTrkRunControlContextSuspendedEvent TcfSuspendEvent;

    const TcfSuspendEvent &me = static_cast<const TcfSuspendEvent &>(event);

    switch (me.reason()) {
    case TcfSuspendEvent::Crash:
        appendMessage(tr("Process has crashed: %1").arg(QString::fromLatin1(me.message())), ErrorMessageFormat);
        m_tcfTrkDevice->sendRunControlResumeCommand(TcfTrkCallback(), me.id()); //TODO: Should I resume automaticly
        break;
    default:
        if (debug)
            qDebug() << "Context suspend not handled:" << "Reason:" << me.reason() << "Message:" << me.message();
        break;
    }
}

void CodaRunControl::handleModuleLoadSuspended(const TcfTrkEvent &event)
{
    // Debug mode start: Continue:
    typedef TcfTrkRunControlModuleLoadContextSuspendedEvent TcfModuleLoadSuspendedEvent;

    const TcfModuleLoadSuspendedEvent &me = static_cast<const TcfModuleLoadSuspendedEvent &>(event);
    if (me.info().requireResume)
        m_tcfTrkDevice->sendRunControlResumeCommand(TcfTrkCallback(), me.id());
}

void CodaRunControl::handleLogging(const TcfTrkEvent &event)
{
    const TcfTrkLoggingWriteEvent &me = static_cast<const TcfTrkLoggingWriteEvent &>(event);
    appendMessage(me.message(), StdOutFormat);
}

void CodaRunControl::handleAddListener(const TcfTrkCommandResult &result)
{
    Q_UNUSED(result)
    m_tcfTrkDevice->sendSymbianOsDataFindProcessesCommand(TcfTrkCallback(this, &CodaRunControl::handleFindProcesses), executableName().toLatin1(), "0");
}

void CodaRunControl::handleFindProcesses(const TcfTrkCommandResult &result)
{
    if (result.values.size() && result.values.at(0).type() == JsonValue::Array && result.values.at(0).children().count()) {
        //there are processes running. Cannot run mine
        appendMessage(tr("The process is already running on the device. Please first close it."), ErrorMessageFormat);
        finishRunControl();
    } else {
        setProgress(maxProgress()*0.90);
        m_tcfTrkDevice->sendProcessStartCommand(TcfTrkCallback(this, &CodaRunControl::handleCreateProcess),
                                                executableName(), executableUid(), commandLineArguments().split(" "), QString(), true);
        appendMessage(tr("Launching: %1").arg(executableName()), NormalMessageFormat);
    }
}

void CodaRunControl::handleCreateProcess(const TcfTrkCommandResult &result)
{
    const bool ok = result.type == TcfTrkCommandResult::SuccessReply;
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
    if (m_tcfTrkDevice)
        m_tcfTrkDevice->deleteLater();
    m_tcfTrkDevice = 0;
    m_state = StateUninit;
    emit finished();
}

QMessageBox *CodaRunControl::createCodaWaitingMessageBox(QWidget *parent)
{
    const QString title  = tr("Waiting for CODA");
    const QString text = tr("Qt Creator is waiting for the CODA application to connect. "
                            "Please make sure the application is running on "
                            "your mobile phone and the right IP address and port are "
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
