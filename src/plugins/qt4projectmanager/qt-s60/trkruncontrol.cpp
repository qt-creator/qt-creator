/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "trkruncontrol.h"

#include "s60deployconfiguration.h"
#include "s60devicerunconfiguration.h"
#include "s60runconfigbluetoothstarter.h"

#include "qt4buildconfiguration.h"
#include "qt4symbiantarget.h"
#include "qt4target.h"
#include "qtoutputformatter.h"

#include <symbianutils/bluetoothlistener_gui.h>
#include <symbianutils/launcher.h>
#include <symbianutils/symbiandevicemanager.h>

#include <utils/qtcassert.h>

#include <coreplugin/icore.h>

#include <QtGui/QMessageBox>
#include <QtGui/QMainWindow>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

enum { debug = 0 };

TrkRunControl::TrkRunControl(RunConfiguration *runConfiguration, const QString &mode) :
    S60RunControlBase(runConfiguration, mode),
    m_launcher(0)
{
    const S60DeviceRunConfiguration *s60runConfig = qobject_cast<S60DeviceRunConfiguration *>(runConfiguration);
    QTC_ASSERT(s60runConfig, return);
    const S60DeployConfiguration *activeDeployConf = qobject_cast<S60DeployConfiguration *>(s60runConfig->qt4Target()->activeDeployConfiguration());
    QTC_ASSERT(activeDeployConf, return);

    m_serialPortName = activeDeployConf->serialPortName();
    m_serialPortFriendlyName = SymbianUtils::SymbianDeviceManager::instance()->friendlyNameForPort(m_serialPortName);
}

TrkRunControl::~TrkRunControl()
{
    if (m_launcher)
        m_launcher->deleteLater();
}

bool TrkRunControl::doStart()
{
    if (m_serialPortName.isEmpty()) {
        cancelProgress();
        QString msg = tr("No device is connected. Please connect a device and try again.");
        appendMessage(msg, NormalMessageFormat);
        return false;
    }

    appendMessage(tr("Executable file: %1").arg(msgListFile(executableFileName())),
                  NormalMessageFormat);
    return true;
}

bool TrkRunControl::isRunning() const
{
    return m_launcher && (m_launcher->state() == trk::Launcher::Connecting
                          || m_launcher->state() == trk::Launcher::Connected
                          || m_launcher->state() == trk::Launcher::WaitingForTrk);

}

bool TrkRunControl::setupLauncher()
{
    connect(SymbianUtils::SymbianDeviceManager::instance(), SIGNAL(deviceRemoved(const SymbianUtils::SymbianDevice)),
            this, SLOT(deviceRemoved(SymbianUtils::SymbianDevice)));
    QString errorMessage;
    m_launcher = trk::Launcher::acquireFromDeviceManager(m_serialPortName, 0, &errorMessage);
    if (!m_launcher) {
        appendMessage(errorMessage, ErrorMessageFormat);
        return false;
    }

    connect(m_launcher, SIGNAL(finished()), this, SLOT(launcherFinished()));
    connect(m_launcher, SIGNAL(canNotConnect(QString)), this, SLOT(printConnectFailed(QString)));
    connect(m_launcher, SIGNAL(stateChanged(int)), this, SLOT(slotLauncherStateChanged(int)));
    connect(m_launcher, SIGNAL(processStopped(uint,uint,uint,QString)),
            this, SLOT(processStopped(uint,uint,uint,QString)));

    if (!commandLineArguments().isEmpty())
        m_launcher->setCommandLineArgs(commandLineArguments());

    const QString runFileName = QString::fromLatin1("%1:\\sys\\bin\\%2.exe").arg(installationDrive()).arg(targetName());
    initLauncher(runFileName, m_launcher);
    const trk::PromptStartCommunicationResult src =
            S60RunConfigBluetoothStarter::startCommunication(m_launcher->trkDevice(),
                                                             0, &errorMessage);
    if (src != trk::PromptStartCommunicationConnected)
        return false;

    if (!m_launcher->startServer(&errorMessage)) {
        appendMessage(tr("Could not connect to phone on port '%1': %2\n"
                         "Check if the phone is connected and App TRK is running.").arg(m_serialPortName, errorMessage), ErrorMessageFormat);
        return false;
    }
    return true;
}

void TrkRunControl::doStop()
{
    if (m_launcher)
        m_launcher->terminate();
}

void TrkRunControl::printConnectFailed(const QString &errorMessage)
{
    appendMessage(tr("Could not connect to App TRK on device: %1. Restarting App TRK might help.").arg(errorMessage),
                  ErrorMessageFormat);
}

void TrkRunControl::launcherFinished()
{
    trk::Launcher::releaseToDeviceManager(m_launcher);
    m_launcher->deleteLater();
    m_launcher = 0;
    emit finished();
}

void TrkRunControl::processStopped(uint pc, uint pid, uint tid, const QString &reason)
{
    appendMessage(trk::Launcher::msgStopped(pid, tid, pc, reason), StdOutFormat);
    m_launcher->terminate();
}

QMessageBox *TrkRunControl::createTrkWaitingMessageBox(const QString &port, QWidget *parent)
{
    const QString title  = tr("Waiting for App TRK");
    const QString text = tr("Qt Creator is waiting for the TRK application to connect on %1.<br>"
                            "Please make sure the application is running on "
                            "your mobile phone and the right port is "
                            "configured in the project settings.").arg(port);
    QMessageBox *rc = new QMessageBox(QMessageBox::Information, title, text,
                                      QMessageBox::Cancel, parent);
    return rc;
}

void TrkRunControl::slotLauncherStateChanged(int s)
{
    if (s == trk::Launcher::WaitingForTrk) {
        QMessageBox *mb = TrkRunControl::createTrkWaitingMessageBox(m_launcher->trkServerName(),
                                                                    Core::ICore::instance()->mainWindow());
        connect(m_launcher, SIGNAL(stateChanged(int)), mb, SLOT(close()));
        connect(mb, SIGNAL(finished(int)), this, SLOT(slotWaitingForTrkClosed()));
        mb->open();
    }
}

void TrkRunControl::slotWaitingForTrkClosed()
{
    if (m_launcher && m_launcher->state() == trk::Launcher::WaitingForTrk) {
        stop();
        appendMessage(tr("Canceled."), ErrorMessageFormat);
        emit finished();
    }
}

void TrkRunControl::printApplicationOutput(const QString &output)
{
    printApplicationOutput(output, false);
}

void TrkRunControl::printApplicationOutput(const QString &output, bool onStdErr)
{
    appendMessage(output, onStdErr ? StdErrFormat : StdOutFormat);
}

void TrkRunControl::deviceRemoved(const SymbianUtils::SymbianDevice &d)
{
    if (m_launcher && d.portName() == m_serialPortName) {
        trk::Launcher::releaseToDeviceManager(m_launcher);
        m_launcher->deleteLater();
        m_launcher = 0;
        QString msg = tr("The device '%1' has been disconnected").arg(d.friendlyName());
        appendMessage(msg, ErrorMessageFormat);
        emit finished();
    }
}

void TrkRunControl::initLauncher(const QString &executable, trk::Launcher *launcher)
{
    connect(launcher, SIGNAL(startingApplication()), this, SLOT(printStartingNotice()));
    connect(launcher, SIGNAL(applicationRunning(uint)), this, SLOT(applicationRunNotice(uint)));
    connect(launcher, SIGNAL(canNotRun(QString)), this, SLOT(applicationRunFailedNotice(QString)));
    connect(launcher, SIGNAL(applicationOutputReceived(QString)), this, SLOT(printApplicationOutput(QString)));
    launcher->addStartupActions(trk::Launcher::ActionRun);
    launcher->setFileName(executable);
}

void TrkRunControl::printStartingNotice()
{
    appendMessage(tr("Starting application..."), NormalMessageFormat);
}

void TrkRunControl::applicationRunNotice(uint pid)
{
    appendMessage(tr("Application running with pid %1.").arg(pid), NormalMessageFormat);
    setProgress(maxProgress());
}

void TrkRunControl::applicationRunFailedNotice(const QString &errorMessage)
{
    appendMessage(tr("Could not start application: %1").arg(errorMessage), NormalMessageFormat);
}
