/****************************************************************************
**
** Copyright (C) Filippo Cucchetto <filippocucchetto@gmail.com>
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "nimruncontrol.h"
#include "nimrunconfiguration.h"

#include <projectexplorer/runnables.h>
#include <utils/qtcprocess.h>
#include <utils/outputformat.h>

#include <QDir>

using namespace ProjectExplorer;
using namespace Utils;

namespace Nim {

NimRunControl::NimRunControl(NimRunConfiguration *rc, Core::Id mode)
    : RunControl(rc, mode)
    , m_running(false)
    , m_runnable(rc->runnable().as<StandardRunnable>())
{
    connect(&m_applicationLauncher, &ApplicationLauncher::appendMessage,
            this, &NimRunControl::slotAppendMessage);
    connect(&m_applicationLauncher, &ApplicationLauncher::processStarted,
            this, &NimRunControl::processStarted);
    connect(&m_applicationLauncher, &ApplicationLauncher::processExited,
            this, &NimRunControl::processExited);
    connect(&m_applicationLauncher, &ApplicationLauncher::bringToForegroundRequested,
            this, &RunControl::bringApplicationToForeground);
}

void NimRunControl::start()
{
    emit started();
    m_running = true;
    m_applicationLauncher.start(m_runnable);
    setApplicationProcessHandle(ProcessHandle(m_applicationLauncher.applicationPID()));
}

ProjectExplorer::RunControl::StopResult NimRunControl::stop()
{
    m_applicationLauncher.stop();
    return StoppedSynchronously;
}

bool NimRunControl::isRunning() const
{
    return m_running;
}

void NimRunControl::processStarted()
{
    // Console processes only know their pid after being started
    setApplicationProcessHandle(ProcessHandle(m_applicationLauncher.applicationPID()));
}

void NimRunControl::processExited(int exitCode, QProcess::ExitStatus status)
{
    m_running = false;
    setApplicationProcessHandle(ProcessHandle());
    QString msg;
    if (status == QProcess::CrashExit) {
        msg = tr("%1 crashed")
                .arg(QDir::toNativeSeparators(m_runnable.executable));
    } else {
        msg = tr("%1 exited with code %2")
                .arg(QDir::toNativeSeparators(m_runnable.executable)).arg(exitCode);
    }
    appendMessage(msg + QLatin1Char('\n'), NormalMessageFormat);
    emit finished();
}

void NimRunControl::slotAppendMessage(const QString &err, OutputFormat format)
{
    appendMessage(err, format);
}

}
