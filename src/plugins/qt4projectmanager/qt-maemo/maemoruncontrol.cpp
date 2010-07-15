/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include "maemoruncontrol.h"

#include "maemodeploystep.h"
#include "maemoglobal.h"
#include "maemorunconfiguration.h"
#include "maemosshrunner.h"

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/toolchain.h>
#include <utils/qtcassert.h>

#include <QtGui/QMessageBox>

using namespace Core;

namespace Qt4ProjectManager {
namespace Internal {

using ProjectExplorer::RunConfiguration;
using ProjectExplorer::ToolChain;

MaemoRunControl::MaemoRunControl(RunConfiguration *rc)
    : RunControl(rc, ProjectExplorer::Constants::RUNMODE)
    , m_runConfig(qobject_cast<MaemoRunConfiguration *>(rc))
    , m_devConfig(m_runConfig ? m_runConfig->deviceConfig() : MaemoDeviceConfig())
    , m_runner(new MaemoSshRunner(this, m_runConfig))
    , m_running(false)
{
}

MaemoRunControl::~MaemoRunControl()
{
    stop();
}

void MaemoRunControl::start()
{
    if (!m_devConfig.isValid()) {
        handleError(tr("No device configuration set for run configuration."));
    } else {
        emit appendMessage(this, tr("Preparing remote side ..."), false);
        m_running = true;
        m_stopped = false;
        emit started();
        disconnect(m_runner, 0, this, 0);
        connect(m_runner, SIGNAL(error(QString)), this,
            SLOT(handleSshError(QString)));
        connect(m_runner, SIGNAL(readyForExecution()), this,
            SLOT(startExecution()));
        connect(m_runner, SIGNAL(remoteErrorOutput(QByteArray)), this,
            SLOT(handleRemoteErrorOutput(QByteArray)));
        connect(m_runner, SIGNAL(remoteOutput(QByteArray)), this,
            SLOT(handleRemoteOutput(QByteArray)));
        connect(m_runner, SIGNAL(remoteProcessStarted()), this,
            SLOT(handleRemoteProcessStarted()));
        connect(m_runner, SIGNAL(remoteProcessFinished(int)), this,
            SLOT(handleRemoteProcessFinished(int)));
        m_runner->start();
    }
}

void MaemoRunControl::stop()
{
    m_stopped = true;
    if (m_runner) {
        disconnect(m_runner, 0, this, 0);
        m_runner->stop();
    }
    setFinished();
}

void MaemoRunControl::handleSshError(const QString &error)
{
    handleError(error);
    setFinished();
}

void MaemoRunControl::startExecution()
{
    emit appendMessage(this, tr("Starting remote process ..."), false);
    const QString &remoteExe = m_runConfig->remoteExecutableFilePath();
    m_runner->startExecution(QString::fromLocal8Bit("%1 %2 %3")
        .arg(MaemoGlobal::remoteCommandPrefix(remoteExe)).arg(remoteExe)
        .arg(m_runConfig->arguments().join(QLatin1String(" "))).toUtf8());
}

void MaemoRunControl::handleRemoteProcessFinished(int exitCode)
{
    if (m_stopped)
        return;

    emit appendMessage(this,
        tr("Finished running remote process. Exit code was %1.").arg(exitCode),
        false);
    setFinished();
}

void MaemoRunControl::handleRemoteOutput(const QByteArray &output)
{
    emit addToOutputWindowInline(this, QString::fromUtf8(output), false);
}

void MaemoRunControl::handleRemoteErrorOutput(const QByteArray &output)
{
    emit addToOutputWindowInline(this, QString::fromUtf8(output), true);
}

bool MaemoRunControl::isRunning() const
{
    return m_running;
}

void MaemoRunControl::handleError(const QString &errString)
{
    QMessageBox::critical(0, tr("Remote Execution Failure"), errString);
    emit appendMessage(this, errString, true);
    stop();
}

void MaemoRunControl::setFinished()
{
    m_running = false;
    emit finished();
}

} // namespace Internal
} // namespace Qt4ProjectManager
