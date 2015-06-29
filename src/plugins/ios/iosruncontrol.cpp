/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "iosruncontrol.h"

#include "iosrunconfiguration.h"
#include "iosrunner.h"

#include <projectexplorer/projectexplorerconstants.h>

using namespace ProjectExplorer;

namespace Ios {
namespace Internal {

IosRunControl::IosRunControl(IosRunConfiguration *rc)
    : RunControl(rc, ProjectExplorer::Constants::NORMAL_RUN_MODE)
    , m_runner(new IosRunner(this, rc, false, false))
    , m_running(false)
{
    setIcon(QLatin1String(ProjectExplorer::Constants::ICON_DEBUG_SMALL));
}

IosRunControl::~IosRunControl()
{
    stop();
}

void IosRunControl::start()
{
    m_running = true;
    emit started();
    disconnect(m_runner, 0, this, 0);

    connect(m_runner, SIGNAL(errorMsg(QString)),
        SLOT(handleRemoteErrorOutput(QString)));
    connect(m_runner, SIGNAL(appOutput(QString)),
        SLOT(handleRemoteOutput(QString)));
    connect(m_runner, SIGNAL(finished(bool)),
        SLOT(handleRemoteProcessFinished(bool)));
    appendMessage(tr("Starting remote process.") + QLatin1Char('\n'), Utils::NormalMessageFormat);
    m_runner->start();
}

RunControl::StopResult IosRunControl::stop()
{
    m_runner->stop();
    return StoppedSynchronously;
}

void IosRunControl::handleRemoteProcessFinished(bool cleanEnd)
{
    if (!cleanEnd)
        appendMessage(tr("Run ended with error."), Utils::ErrorMessageFormat);
    else
        appendMessage(tr("Run ended."), Utils::NormalMessageFormat);
    disconnect(m_runner, 0, this, 0);
    m_running = false;
    emit finished();
}

void IosRunControl::handleRemoteOutput(const QString &output)
{
    appendMessage(output, Utils::StdOutFormatSameLine);
}

void IosRunControl::handleRemoteErrorOutput(const QString &output)
{
    appendMessage(output, Utils::StdErrFormat);
}

bool IosRunControl::isRunning() const
{
    return m_running;
}

QString IosRunControl::displayName() const
{
    return m_runner->displayName();
}

} // namespace Internal
} // namespace Ios
