/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
** Contact: https://www.qt.io/licensing/
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

#include "androidruncontrol.h"

#include "androidglobal.h"
#include "androidrunconfiguration.h"
#include "androidrunner.h"

#include <utils/utilsicons.h>

#include <projectexplorer/projectexplorerconstants.h>

using namespace ProjectExplorer;

namespace Android {
namespace Internal {

AndroidRunControl::AndroidRunControl(AndroidRunConfiguration *rc)
    : RunControl(rc, ProjectExplorer::Constants::NORMAL_RUN_MODE)
    , m_runner(new AndroidRunner(this, rc, ProjectExplorer::Constants::NORMAL_RUN_MODE))
{
    setRunnable(m_runner->runnable());
    setIcon(Utils::Icons::RUN_SMALL_TOOLBAR);
}

AndroidRunControl::~AndroidRunControl()
{
    stop();
}

void AndroidRunControl::start()
{
    reportApplicationStart();
    disconnect(m_runner, 0, this, 0);

    connect(m_runner, &AndroidRunner::remoteErrorOutput,
        this, &AndroidRunControl::handleRemoteErrorOutput);
    connect(m_runner, &AndroidRunner::remoteOutput,
        this, &AndroidRunControl::handleRemoteOutput);
    connect(m_runner, &AndroidRunner::remoteProcessFinished,
        this, &AndroidRunControl::handleRemoteProcessFinished);
    appendMessage(tr("Starting remote process."), Utils::NormalMessageFormat);
    m_runner->setRunnable(runnable().as<AndroidRunnable>());
    m_runner->start();
}

void AndroidRunControl::stop()
{
    m_runner->stop();
}

void AndroidRunControl::handleRemoteProcessFinished(const QString &error)
{
    appendMessage(error, Utils::ErrorMessageFormat);
    disconnect(m_runner, 0, this, 0);
    reportApplicationStop();
}

void AndroidRunControl::handleRemoteOutput(const QString &output)
{
    appendMessage(output, Utils::StdOutFormatSameLine);
}

void AndroidRunControl::handleRemoteErrorOutput(const QString &output)
{
    appendMessage(output, Utils::StdErrFormatSameLine);
}

QString AndroidRunControl::displayName() const
{
    return m_runner->displayName();
}

} // namespace Internal
} // namespace Android
