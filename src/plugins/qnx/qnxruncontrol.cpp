/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Limited. All rights reserved.
** Contact: KDAB (info@kdab.com)
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

#include "qnxruncontrol.h"
#include "qnxdeviceconfiguration.h"
#include "qnxrunconfiguration.h"
#include "slog2inforunner.h"

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

using namespace Qnx;
using namespace Qnx::Internal;
using namespace RemoteLinux;

QnxRunControl::QnxRunControl(ProjectExplorer::RunConfiguration *runConfig)
    : RemoteLinuxRunControl(runConfig)
    , m_slog2Info(0)
{
    ProjectExplorer::IDevice::ConstPtr dev = ProjectExplorer::DeviceKitInformation::device(runConfig->target()->kit());
    QnxDeviceConfiguration::ConstPtr qnxDevice = dev.dynamicCast<const QnxDeviceConfiguration>();

    QnxRunConfiguration *qnxRunConfig = qobject_cast<QnxRunConfiguration *>(runConfig);
    QTC_CHECK(qnxRunConfig);

    const QString applicationId = Utils::FileName::fromString(qnxRunConfig->remoteExecutableFilePath()).fileName();
    m_slog2Info = new Slog2InfoRunner(applicationId, qnxDevice, this);
    connect(m_slog2Info, SIGNAL(output(QString,Utils::OutputFormat)), this, SLOT(appendMessage(QString,Utils::OutputFormat)));
    connect(this, SIGNAL(started()), m_slog2Info, SLOT(start()));
    if (qnxDevice->qnxVersion() > 0x060500)
        connect(m_slog2Info, SIGNAL(commandMissing()), this, SLOT(printMissingWarning()));
}

ProjectExplorer::RunControl::StopResult QnxRunControl::stop()
{
    m_slog2Info->stop();
    return RemoteLinuxRunControl::stop();
}

void QnxRunControl::printMissingWarning()
{
    appendMessage(tr("Warning: \"slog2info\" is not found on the device, debug output not available."), Utils::ErrorMessageFormat);
}
