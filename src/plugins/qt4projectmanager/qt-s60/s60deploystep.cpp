/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/


#include "S60deploystep.h"

#include "qt4buildconfiguration.h"
#include "s60devicerunconfiguration.h"
#include "symbiandevicemanager.h"
#include "s60runconfigbluetoothstarter.h"

#include <QtGui/QMessageBox>
#include <QtGui/QMainWindow>
#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QDir>

#include <coreplugin/icore.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/target.h>
#include <projectexplorer/ioutputparser.h>
#include <qt4projectmanagerconstants.h>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager::Internal;

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

S60DeployStep::S60DeployStep(ProjectExplorer::BuildConfiguration *bc, const QString &id) :
        BuildStep(bc, id), m_timer(0), m_launcher(0),
        m_releaseDeviceAfterLauncherFinish(true), m_handleDeviceRemoval(true),
        m_eventLoop(0)
{
}

S60DeployStep::S60DeployStep(ProjectExplorer::BuildConfiguration *bc,
                             S60DeployStep *bs):
        BuildStep(bc, bs), m_timer(0), m_launcher(0),
        m_releaseDeviceAfterLauncherFinish(bs->m_releaseDeviceAfterLauncherFinish),
        m_handleDeviceRemoval(bs->m_handleDeviceRemoval),
        m_eventLoop(0)
{
}

S60DeployStep::S60DeployStep(ProjectExplorer::BuildConfiguration *bc):
        BuildStep(bc, QLatin1String(S60_DEPLOY_STEP_ID)), m_timer(0),
        m_launcher(0), m_releaseDeviceAfterLauncherFinish(true),
        m_handleDeviceRemoval(true), m_eventLoop(0)
{
}

S60DeployStep::~S60DeployStep()
{
    delete m_timer;
    delete m_launcher;
    delete m_eventLoop;
}


bool S60DeployStep::init()
{
    Qt4BuildConfiguration *bc = static_cast<Qt4BuildConfiguration *>(buildConfiguration());
    S60DeviceRunConfiguration* runConfiguration = static_cast<S60DeviceRunConfiguration *>(bc->target()->activeRunConfiguration());

    if(!runConfiguration) {
        return false;
    }
    m_serialPortName = runConfiguration->serialPortName();
    m_serialPortFriendlyName = SymbianUtils::SymbianDeviceManager::instance()->friendlyNameForPort(m_serialPortName);
    m_packageFileNameWithTarget = runConfiguration->packageFileNameWithTargetInfo();
    m_signedPackage = runConfiguration->signedPackage();

    setDisplayName(tr("Deploy", "Qt4 DeployStep display name."));
    QString message;
    m_launcher = trk::Launcher::acquireFromDeviceManager(m_serialPortName, this, &message);
    if (!message.isEmpty()) {
        trk::Launcher::releaseToDeviceManager(m_launcher);
        delete m_launcher;
        m_launcher = 0;
        appendMessage(message, true);
        return false;
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

bool S60DeployStep::processPackageName(QString &errorMessage)
{
    QFileInfo packageInfo(m_signedPackage);
    {
        // support for 4.6.1 and pre, where make sis creates 'targetname_armX_udeb.sis' instead of 'targetname.sis'
        QFileInfo packageWithTargetInfo(m_packageFileNameWithTarget);
        // does the 4.6.1 version exist?
        if (packageWithTargetInfo.exists() && packageWithTargetInfo.isFile()) {
            // is the 4.6.1 version newer? (to guard against behavior change Qt Creator 1.3 --> 2.0)
            if (!packageInfo.exists() || packageInfo.lastModified() < packageWithTargetInfo.lastModified()) { //TODO change the QtCore
                // the 'targetname_armX_udeb.sis' crap exists and is new, rename it
                appendMessage(tr("Renaming new package '%1' to '%2'")
                              .arg(QDir::toNativeSeparators(m_packageFileNameWithTarget),
                                   QDir::toNativeSeparators(m_signedPackage)), false);
                return renameFile(m_packageFileNameWithTarget, m_signedPackage, &errorMessage);
            } else {
                // the 'targetname_armX_udeb.sis' crap exists but is old, remove it
                appendMessage(tr("Removing old package '%1'")
                              .arg(QDir::toNativeSeparators(m_packageFileNameWithTarget)),
                              false);
                ensureDeleteFile(m_packageFileNameWithTarget, &errorMessage);
            }
        }
    }
    if (!packageInfo.exists() || !packageInfo.isFile()) {
        errorMessage = tr("Package file not found");
        return false;
    }
    return true;
}

void S60DeployStep::start()
{
    if (m_serialPortName.isEmpty()) {
        appendMessage(tr("There is no device plugged in."), true);
        emit finished();
        return;
    }

    QString errorMessage;

    // make sure we have the right name of the sis package
    if (processPackageName(errorMessage)) {
        startDeployment();
    } else {
        errorMessage = tr("Failed to find package '%1': %2").arg(m_signedPackage, errorMessage);
        appendMessage(errorMessage, true);
        stop();
        emit finished();
    }
}

void S60DeployStep::stop()
{
    if (m_launcher)
        m_launcher->terminate();
    emit finished();
}

void S60DeployStep::setupConnections()
{
    connect(SymbianUtils::SymbianDeviceManager::instance(), SIGNAL(deviceRemoved(SymbianUtils::SymbianDevice)),
            this, SLOT(deviceRemoved(SymbianUtils::SymbianDevice)));
    connect(m_launcher, SIGNAL(finished()), this, SLOT(launcherFinished()));
    connect(m_launcher, SIGNAL(canNotConnect(QString)), this, SLOT(printConnectFailed(QString)));
    connect(m_launcher, SIGNAL(copyingStarted()), this, SLOT(printCopyingNotice()));
    connect(m_launcher, SIGNAL(canNotCreateFile(QString,QString)), this, SLOT(printCreateFileFailed(QString,QString)));
    connect(m_launcher, SIGNAL(canNotWriteFile(QString,QString)), this, SLOT(printWriteFileFailed(QString,QString)));
    connect(m_launcher, SIGNAL(canNotCloseFile(QString,QString)), this, SLOT(printCloseFileFailed(QString,QString)));
    connect(m_launcher, SIGNAL(installingStarted()), this, SLOT(printInstallingNotice()));
    connect(m_launcher, SIGNAL(canNotInstall(QString,QString)), this, SLOT(printInstallFailed(QString,QString)));
    connect(m_launcher, SIGNAL(installingFinished()), this, SLOT(printInstallingFinished()));
    connect(m_launcher, SIGNAL(stateChanged(int)), this, SLOT(slotLauncherStateChanged(int)));
}

void S60DeployStep::startDeployment()
{
    Q_ASSERT(m_launcher);

    QString errorMessage;
    bool success = false;
    do {
        setupConnections();
        //TODO sisx destination and file path user definable
        const QString copyDst = QString::fromLatin1("C:\\Data\\%1").arg(QFileInfo(m_signedPackage).fileName());

        m_launcher->setCopyFileName(m_signedPackage, copyDst);
        m_launcher->setInstallFileName(copyDst);
        m_launcher->addStartupActions(trk::Launcher::ActionCopyInstall);

        appendMessage(tr("Package: %1\nDeploying application to '%2'...").arg(m_signedPackage, m_serialPortFriendlyName), false);

        // Prompt the user to start up the Blue tooth connection
        const trk::PromptStartCommunicationResult src =
                S60RunConfigBluetoothStarter::startCommunication(m_launcher->trkDevice(),
                                                                 0, &errorMessage);
        if (src != trk::PromptStartCommunicationConnected)
            break;
        if (!m_launcher->startServer(&errorMessage)) {
            errorMessage = tr("Could not connect to phone on port '%1': %2\n"
                              "Check if the phone is connected and App TRK is running.").arg(m_serialPortName, errorMessage);
            break;
        }
        success = true;
    } while (false);

    if (!success) {
        if (!errorMessage.isEmpty())
            appendMessage(errorMessage, true);
        stop();
        emit finished();
    }
}

void S60DeployStep::run(QFutureInterface<bool> &fi)
{
    m_futureInterface = &fi;
    m_deployResult = false;
    connect(this, SIGNAL(finished()),
            this, SLOT(launcherFinished()));
    connect(this, SIGNAL(finishNow()),
            this, SLOT(launcherFinished()), Qt::DirectConnection);

    start();
    m_timer = new QTimer();
    connect(m_timer, SIGNAL(timeout()), this, SLOT(checkForCancel()), Qt::DirectConnection);
    m_timer->start(500);
    m_eventLoop = new QEventLoop();
    m_eventLoop->exec();
    m_timer->stop();
    delete m_timer;
    m_timer = 0;

    delete m_eventLoop;
    m_eventLoop = 0;
    fi.reportResult(m_deployResult);
    m_futureInterface = 0;
}

void S60DeployStep::setReleaseDeviceAfterLauncherFinish(bool v)
{
    m_releaseDeviceAfterLauncherFinish = v;
}

void S60DeployStep::slotLauncherStateChanged(int s)
{
    if (s == trk::Launcher::WaitingForTrk) {
        QMessageBox *mb = S60DeviceRunControlBase::createTrkWaitingMessageBox(m_launcher->trkServerName(),
                                                                              Core::ICore::instance()->mainWindow());
        connect(m_launcher, SIGNAL(stateChanged(int)), mb, SLOT(close()));
        connect(mb, SIGNAL(finished(int)), this, SLOT(slotWaitingForTrkClosed()));
        mb->open();
    }
}

void S60DeployStep::slotWaitingForTrkClosed()
{
    if (m_launcher && m_launcher->state() == trk::Launcher::WaitingForTrk) {
        stop();
        appendMessage(tr("Canceled."), true);
        emit finished();
    }
}

void S60DeployStep::printCreateFileFailed(const QString &filename, const QString &errorMessage)
{
    appendMessage(tr("Could not create file %1 on device: %2").arg(filename, errorMessage), true);
}

void S60DeployStep::printWriteFileFailed(const QString &filename, const QString &errorMessage)
{
    appendMessage(tr("Could not write to file %1 on device: %2").arg(filename, errorMessage), true);
}

void S60DeployStep::printCloseFileFailed(const QString &filename, const QString &errorMessage)
{
    const QString msg = tr("Could not close file %1 on device: %2. It will be closed when App TRK is closed.");
    appendMessage( msg.arg(filename, errorMessage), true);
}

void S60DeployStep::printConnectFailed(const QString &errorMessage)
{
    appendMessage(tr("Could not connect to App TRK on device: %1. Restarting App TRK might help.").arg(errorMessage), true);
}

void S60DeployStep::printCopyingNotice()
{
    appendMessage(tr("Copying installation file..."), false);
}

void S60DeployStep::printInstallingNotice()
{
    appendMessage(tr("Installing application..."), false);
}

void S60DeployStep::printInstallingFinished()
{
    appendMessage(tr("Installation has finished"), false);
}

void S60DeployStep::printInstallFailed(const QString &filename, const QString &errorMessage)
{
    appendMessage(tr("Could not install from package %1 on device: %2").arg(filename, errorMessage), true);
}

void S60DeployStep::checkForCancel()
{
    if (m_futureInterface->isCanceled() && m_timer->isActive()) {
        m_timer->stop();
        stop();
        appendMessage(tr("Canceled."), true);
        emit finishNow();
    }
}

void S60DeployStep::launcherFinished()
{
    if (m_releaseDeviceAfterLauncherFinish && m_launcher) {
        m_handleDeviceRemoval = false;
        trk::Launcher::releaseToDeviceManager(m_launcher);
    }
    m_deployResult = true;
    if(m_launcher)
        m_launcher->deleteLater();
    m_launcher = 0;
    if(m_eventLoop)
        m_eventLoop->exit(0);
}

void S60DeployStep::deviceRemoved(const SymbianUtils::SymbianDevice &d)
{
    if (m_handleDeviceRemoval && d.portName() == m_serialPortName) {
        appendMessage(tr("The device '%1' has been disconnected").arg(d.friendlyName()), true);
        emit finished();
    }
}

// #pragma mark -- S60DeployStepWidget

BuildStepConfigWidget *S60DeployStep::createConfigWidget()
{
    return new S60DeployStepWidget();
}

void S60DeployStepWidget::init()
{
}

QString S60DeployStepWidget::summaryText() const
{
    return tr("<b>Deploy SIS Package</b>");
}

QString S60DeployStepWidget::displayName() const
{
    return QString("S60DeployStepWidget::displayName");
}

// #pragma mark -- S60DeployStepFactory

S60DeployStepFactory::S60DeployStepFactory(QObject *parent) :
        ProjectExplorer::IBuildStepFactory(parent)
{
}

S60DeployStepFactory::~S60DeployStepFactory()
{
}

bool S60DeployStepFactory::canCreate(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::BuildStep::Type type, const QString &id) const
{
    if (type != ProjectExplorer::BuildStep::Deploy)
        return false;
    if (parent->target()->id() != QLatin1String(Constants::S60_DEVICE_TARGET_ID))
        return false;
    return (id == QLatin1String(S60_DEPLOY_STEP_ID));
}

ProjectExplorer::BuildStep *S60DeployStepFactory::create(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::BuildStep::Type type, const QString &id)
{
    if (!canCreate(parent, type, id))
        return 0;
    return new S60DeployStep(parent, id);
}

bool S60DeployStepFactory::canClone(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::BuildStep::Type type, ProjectExplorer::BuildStep *source) const
{
    return canCreate(parent, type, source->id());
}

ProjectExplorer::BuildStep *S60DeployStepFactory::clone(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::BuildStep::Type type, ProjectExplorer::BuildStep *source)
{
    if (!canClone(parent, type, source))
        return 0;
    return new S60DeployStep(parent, static_cast<S60DeployStep *>(source));
}

bool S60DeployStepFactory::canRestore(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::BuildStep::Type type, const QVariantMap &map) const
{
    QString id(ProjectExplorer::idFromMap(map));
    return canCreate(parent, type, id);
}

ProjectExplorer::BuildStep *S60DeployStepFactory::restore(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::BuildStep::Type type, const QVariantMap &map)
{
    if (!canRestore(parent, type, map))
        return 0;
    S60DeployStep *bs = new S60DeployStep(parent);
    if (bs->fromMap(map))
        return bs;
    delete bs;
    return 0;
}

QStringList S60DeployStepFactory::availableCreationIds(ProjectExplorer::BuildConfiguration *parent, ProjectExplorer::BuildStep::Type type) const
{
    if (type != ProjectExplorer::BuildStep::Deploy)
        return QStringList();
    if (parent->target()->id() == QLatin1String(Constants::S60_DEVICE_TARGET_ID))
        return QStringList() << QLatin1String(S60_DEPLOY_STEP_ID);
    return QStringList();
}

QString S60DeployStepFactory::displayNameForId(const QString &id) const
{
    if (id == QLatin1String(S60_DEPLOY_STEP_ID))
        return tr("Deploy SIS Package");
    return QString();
}
