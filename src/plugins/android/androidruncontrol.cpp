/**************************************************************************
**
** Copyright (c) 2013 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidruncontrol.h"

#include "androiddeploystep.h"
#include "androidglobal.h"
#include "androidrunconfiguration.h"
#include "androidrunner.h"

#include <projectexplorer/projectexplorerconstants.h>

using namespace ProjectExplorer;

namespace Android {
namespace Internal {

AndroidRunControl::AndroidRunControl(AndroidRunConfiguration *rc)
    : RunControl(rc, NormalRunMode)
    , m_runner(new AndroidRunner(this, rc, false))
    , m_running(false)
{
}

AndroidRunControl::~AndroidRunControl()
{
    stop();
}

void AndroidRunControl::start()
{
    m_running = true;
    emit started();
    disconnect(m_runner, 0, this, 0);

    connect(m_runner, SIGNAL(remoteErrorOutput(QByteArray)),
        SLOT(handleRemoteErrorOutput(QByteArray)));
    connect(m_runner, SIGNAL(remoteOutput(QByteArray)),
        SLOT(handleRemoteOutput(QByteArray)));
    connect(m_runner, SIGNAL(remoteProcessFinished(QString)),
        SLOT(handleRemoteProcessFinished(QString)));
    appendMessage(tr("Starting remote process."), Utils::NormalMessageFormat);
    m_runner->start();
}

RunControl::StopResult AndroidRunControl::stop()
{
    m_runner->stop();
    return StoppedSynchronously;
}

void AndroidRunControl::handleRemoteProcessFinished(const QString &error)
{
    appendMessage(error, Utils::ErrorMessageFormat);
    disconnect(m_runner, 0, this, 0);
    m_running = false;
    emit finished();
}

void AndroidRunControl::handleRemoteOutput(const QByteArray &output)
{
    appendMessage(QString::fromUtf8(output), Utils::StdOutFormatSameLine);
}

void AndroidRunControl::handleRemoteErrorOutput(const QByteArray &output)
{
    appendMessage(QString::fromUtf8(output), Utils::StdErrFormatSameLine);
}

bool AndroidRunControl::isRunning() const
{
    return m_running;
}

QString AndroidRunControl::displayName() const
{
    return m_runner->displayName();
}

QIcon AndroidRunControl::icon() const
{
    return QIcon(QLatin1String(ProjectExplorer::Constants::ICON_DEBUG_SMALL));
}

} // namespace Internal
} // namespace Qt4ProjectManager
