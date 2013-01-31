/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "localapplicationruncontrol.h"
#include "localapplicationrunconfiguration.h"
#include "projectexplorerconstants.h"

#include <utils/outputformat.h>
#include <utils/qtcassert.h>
#include <utils/environment.h>

#include <QLabel>
#include <QIcon>
#include <QDir>

namespace ProjectExplorer {
namespace Internal {

LocalApplicationRunControlFactory::LocalApplicationRunControlFactory()
{
}

LocalApplicationRunControlFactory::~LocalApplicationRunControlFactory()
{
}

bool LocalApplicationRunControlFactory::canRun(RunConfiguration *runConfiguration, RunMode mode) const
{
    return mode == NormalRunMode && qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration);
}

QString LocalApplicationRunControlFactory::displayName() const
{
    return tr("Run");
}

RunControl *LocalApplicationRunControlFactory::create(RunConfiguration *runConfiguration, RunMode mode, QString *errorMessage)
{
    QTC_ASSERT(canRun(runConfiguration, mode), return 0);
    LocalApplicationRunConfiguration *localRunConfiguration = qobject_cast<LocalApplicationRunConfiguration *>(runConfiguration);
    // Force the dialog about executables at this point and fail if there is none
    if (!localRunConfiguration->ensureConfigured(errorMessage))
        return 0;
    return new LocalApplicationRunControl(localRunConfiguration, mode);
}

// ApplicationRunControl

LocalApplicationRunControl::LocalApplicationRunControl(LocalApplicationRunConfiguration *rc, RunMode mode)
    : RunControl(rc, mode), m_running(false)
{
    Utils::Environment env = rc->environment();
    QString dir = rc->workingDirectory();
    m_applicationLauncher.setEnvironment(env);
    m_applicationLauncher.setWorkingDirectory(dir);

    m_executable = rc->executable();
    m_runMode = static_cast<ApplicationLauncher::Mode>(rc->runMode());
    m_commandLineArguments = rc->commandLineArguments();

    connect(&m_applicationLauncher, SIGNAL(appendMessage(QString,Utils::OutputFormat)),
            this, SLOT(slotAppendMessage(QString,Utils::OutputFormat)));
    connect(&m_applicationLauncher, SIGNAL(processStarted()),
            this, SLOT(processStarted()));
    connect(&m_applicationLauncher, SIGNAL(processExited(int)),
            this, SLOT(processExited(int)));
    connect(&m_applicationLauncher, SIGNAL(bringToForegroundRequested(qint64)),
            this, SLOT(bringApplicationToForeground(qint64)));
}

LocalApplicationRunControl::~LocalApplicationRunControl()
{
}

void LocalApplicationRunControl::start()
{
    emit started();
    if (m_executable.isEmpty()) {
        appendMessage(tr("No executable specified.\n"), Utils::ErrorMessageFormat);
        emit finished();
    }  else {
        m_running = true;
        QString msg = tr("Starting %1...\n").arg(QDir::toNativeSeparators(m_executable));
        appendMessage(msg, Utils::NormalMessageFormat);
        m_applicationLauncher.start(m_runMode, m_executable, m_commandLineArguments);
        setApplicationProcessHandle(ProcessHandle(m_applicationLauncher.applicationPID()));
    }
}

LocalApplicationRunControl::StopResult LocalApplicationRunControl::stop()
{
    m_applicationLauncher.stop();
    return StoppedSynchronously;
}

bool LocalApplicationRunControl::isRunning() const
{
    return m_running;
}

QIcon LocalApplicationRunControl::icon() const
{
    return QIcon(QLatin1String(ProjectExplorer::Constants::ICON_RUN_SMALL));
}

void LocalApplicationRunControl::slotAppendMessage(const QString &err,
                                                   Utils::OutputFormat format)
{
    appendMessage(err, format);
}

void LocalApplicationRunControl::processStarted()
{
    // Console processes only know their pid after being started
    setApplicationProcessHandle(ProcessHandle(m_applicationLauncher.applicationPID()));
}

void LocalApplicationRunControl::processExited(int exitCode)
{
    m_running = false;
    setApplicationProcessHandle(ProcessHandle());
    QString msg = tr("%1 exited with code %2\n")
        .arg(QDir::toNativeSeparators(m_executable)).arg(exitCode);
    appendMessage(msg, Utils::NormalMessageFormat);
    emit finished();
}

} // namespace Internal
} // namespace ProjectExplorer
