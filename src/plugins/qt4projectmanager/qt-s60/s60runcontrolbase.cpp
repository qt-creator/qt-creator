/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#include "s60runcontrolbase.h"

#include "qt4buildconfiguration.h"
#include "s60deployconfiguration.h"
#include "s60devicerunconfiguration.h"

#include <analyzerbase/analyzerconstants.h>
#include <coreplugin/icore.h>
#include <debugger/debuggerconstants.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtprofileinformation.h>
#include <utils/qtcassert.h>

#include <QDateTime>
#include <QDir>
#include <QFileInfo>

using namespace ProjectExplorer;
using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

const int PROGRESS_MIN = 0;
const int PROGRESS_MAX = 200;

enum { debug = 0 };

// Format information about a file
QString S60RunControlBase::msgListFile(const QString &file)
{
    QString rc;
    const QFileInfo fi(file);
    QTextStream str(&rc);
    if (fi.exists())
        str << fi.size() << ' ' << fi.lastModified().toString(Qt::ISODate) << ' ' << QDir::toNativeSeparators(fi.absoluteFilePath());
    else
        str << "<non-existent> " << QDir::toNativeSeparators(fi.absoluteFilePath());
    return rc;
}

S60RunControlBase::S60RunControlBase(RunConfiguration *runConfiguration,
                                     RunMode mode) :
    RunControl(runConfiguration, mode),
    m_launchProgress(0)
{
    connect(this, SIGNAL(finished()), this, SLOT(reportLaunchFinished()));
    connect(this, SIGNAL(finished()), this, SLOT(handleFinished()));

    const S60DeviceRunConfiguration *s60runConfig = qobject_cast<S60DeviceRunConfiguration *>(runConfiguration);
    QTC_ASSERT(s60runConfig, return);
    const Qt4BuildConfiguration *activeBuildConf = qobject_cast<Qt4BuildConfiguration *>(s60runConfig->target()->activeBuildConfiguration());
    QTC_ASSERT(activeBuildConf, return);
    const S60DeployConfiguration *activeDeployConf = qobject_cast<S60DeployConfiguration *>(s60runConfig->target()->activeDeployConfiguration());
    QTC_ASSERT(activeDeployConf, return);

    m_executableUid = s60runConfig->executableUid();
    m_targetName = s60runConfig->targetName();
    m_commandLineArguments = s60runConfig->commandLineArguments();
    QString qmlArgs = s60runConfig->qmlCommandLineArguments();
    if ((mode == DebugRunMode || mode == QmlProfilerRunMode) && !qmlArgs.isEmpty()) {
        m_commandLineArguments.prepend(QLatin1Char(' '));
        m_commandLineArguments.prepend(qmlArgs);
    }
    if (const QtSupport::BaseQtVersion *qtv = QtSupport::QtProfileInformation::qtVersion(runConfiguration->target()->profile())) {
        m_qtDir = qtv->versionInfo().value(QLatin1String("QT_INSTALL_DATA"));
        m_qtBinPath = qtv->versionInfo().value(QLatin1String("QT_INSTALL_BINS"));
    }
    m_installationDrive = activeDeployConf->installationDrive();
    QTC_ASSERT(!m_qtBinPath.isEmpty(), return);
    m_executableFileName = s60runConfig->localExecutableFileName();
    m_runSmartInstaller = activeDeployConf->runSmartInstaller();

    if (debug)
        qDebug() << "S60RunControlBase::CT" << m_targetName;
}

void S60RunControlBase::start()
{
    QTC_ASSERT(!m_launchProgress, return);

    m_launchProgress = new QFutureInterface<void>;
    Core::ICore::progressManager()->addTask(m_launchProgress->future(),
                                                        tr("Launching"),
                                                        QLatin1String("Symbian.Launch"));
    m_launchProgress->setProgressRange(PROGRESS_MIN, PROGRESS_MAX);
    m_launchProgress->setProgressValue(PROGRESS_MIN);
    m_launchProgress->reportStarted();

    if (m_runSmartInstaller) { //Smart Installer does the running by itself
        cancelProgress();
        appendMessage(tr("Please finalise the installation on your device.\n"), Utils::NormalMessageFormat);
        emit finished();
        return;
    }

    if (!doStart()) {
        emit finished();
        return;
    }
    emit started();
    startLaunching();
}

S60RunControlBase::~S60RunControlBase()
{
    if (m_launchProgress) {
        m_launchProgress->reportFinished();
        delete m_launchProgress;
        m_launchProgress = 0;
    }
}

RunControl::StopResult S60RunControlBase::stop()
{
    doStop();
    return AsynchronousStop;
}

bool S60RunControlBase::promptToStop(bool *optionalPrompt) const
{
    Q_UNUSED(optionalPrompt)
    // We override the settings prompt
    QTC_ASSERT(isRunning(), return true);

    const QString question = tr("<html><head/><body><center><i>%1</i> is still running on the device.</center>"
                                        "<center>Terminating it can leave the target in an inconsistent state.</center>"
                                        "<center>Would you still like to terminate it?</center></body></html>").arg(displayName());
    return showPromptToStopDialog(tr("Application Still Running"), question,
                                  tr("Force Quit"), tr("Keep Running"));
}

void S60RunControlBase::startLaunching()
{
    if (setupLauncher())
        setProgress(maxProgress()*0.30);
    else {
        stop();
        emit finished();
    }
}

void S60RunControlBase::handleFinished()
{
    appendMessage(tr("Finished.\n"), Utils::NormalMessageFormat);
}

void S60RunControlBase::setProgress(int value)
{
    if (m_launchProgress) {
        if (value < PROGRESS_MAX) {
            if (value < PROGRESS_MIN)
                m_launchProgress->setProgressValue(PROGRESS_MIN);
            else
                m_launchProgress->setProgressValue(value);
        } else {
            m_launchProgress->setProgressValue(PROGRESS_MAX);
            m_launchProgress->reportFinished();
            delete m_launchProgress;
            m_launchProgress = 0;
        }
    }
}

void S60RunControlBase::cancelProgress()
{
    if (m_launchProgress) {
        m_launchProgress->reportCanceled();
        m_launchProgress->reportFinished();
    }
    delete m_launchProgress;
    m_launchProgress = 0;
}

int S60RunControlBase::maxProgress() const
{
    return PROGRESS_MAX;
}

void S60RunControlBase::reportLaunchFinished()
{
    setProgress(maxProgress());
}

quint32 S60RunControlBase::executableUid() const
{
    return m_executableUid;
}

QString S60RunControlBase::executableName() const
{
    return QString::fromLatin1("%1.exe").arg(targetName());
}

const QString &S60RunControlBase::targetName() const
{
    return m_targetName;
}

const QString &S60RunControlBase::commandLineArguments() const
{
    return m_commandLineArguments;
}

const QString &S60RunControlBase::executableFileName() const
{
    return m_executableFileName;
}

const QString &S60RunControlBase::qtDir() const
{
    return m_qtDir;
}

const QString &S60RunControlBase::qtBinPath() const
{
    return m_qtBinPath;
}

bool S60RunControlBase::runSmartInstaller() const
{
    return m_runSmartInstaller;
}

char S60RunControlBase::installationDrive() const
{
    return m_installationDrive;
}
