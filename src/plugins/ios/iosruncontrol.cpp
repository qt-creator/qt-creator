/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "iosruncontrol.h"

#include "iosrunconfiguration.h"
#include "iosrunner.h"

#include <utils/utilsicons.h>

#include <projectexplorer/projectexplorerconstants.h>

using namespace ProjectExplorer;

namespace Ios {
namespace Internal {

IosRunControl::IosRunControl(IosRunConfiguration *rc)
    : RunControl(rc, ProjectExplorer::Constants::NORMAL_RUN_MODE)
    , m_runner(new IosRunner(this, rc, false, QmlDebug::NoQmlDebugServices))
{
    setIcon(Utils::Icons::RUN_SMALL_TOOLBAR);
}

IosRunControl::~IosRunControl()
{
    stop();
}

void IosRunControl::start()
{
    reportApplicationStart();
    disconnect(m_runner, 0, this, 0);

    connect(m_runner, &IosRunner::errorMsg,
        this, &IosRunControl::handleRemoteErrorOutput);
    connect(m_runner, &IosRunner::appOutput,
        this, &IosRunControl::handleRemoteOutput);
    connect(m_runner, &IosRunner::finished,
        this, &IosRunControl::handleRemoteProcessFinished);
    appendMessage(tr("Starting remote process.") + QLatin1Char('\n'), Utils::NormalMessageFormat);
    m_runner->start();
}

void IosRunControl::stop()
{
    m_runner->stop();
}

void IosRunControl::handleRemoteProcessFinished(bool cleanEnd)
{
    if (!cleanEnd)
        appendMessage(tr("Run ended with error."), Utils::ErrorMessageFormat);
    else
        appendMessage(tr("Run ended."), Utils::NormalMessageFormat);
    disconnect(m_runner, 0, this, 0);
    reportApplicationStop();
}

void IosRunControl::handleRemoteOutput(const QString &output)
{
    appendMessage(output, Utils::StdOutFormatSameLine);
}

void IosRunControl::handleRemoteErrorOutput(const QString &output)
{
    appendMessage(output, Utils::StdErrFormat);
}

QString IosRunControl::displayName() const
{
    return m_runner->displayName();
}

} // namespace Internal
} // namespace Ios
