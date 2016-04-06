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

#include "remotelinuxenvironmentaspectwidget.h"

#include "remotelinuxrunconfiguration.h"
#include "remotelinuxenvironmentreader.h"

#include <projectexplorer/target.h>
#include <projectexplorer/kitinformation.h>

#include <QCoreApplication>
#include <QMessageBox>
#include <QPushButton>

using namespace ProjectExplorer;
using namespace RemoteLinux::Internal;

namespace {
const QString FetchEnvButtonText
    = QCoreApplication::translate("RemoteLinux::RemoteLinuxEnvironmentAspectWidget",
                                  "Fetch Device Environment");
} // anonymous namespace

namespace RemoteLinux {

RemoteLinuxEnvironmentAspectWidget::RemoteLinuxEnvironmentAspectWidget(RemoteLinuxEnvironmentAspect *aspect) :
    EnvironmentAspectWidget(aspect, new QPushButton)
{
    RunConfiguration *runConfiguration = aspect->runConfiguration();
    Target *target = runConfiguration->target();
    IDevice::ConstPtr device = DeviceKitInformation::device(target->kit());

    deviceEnvReader = new RemoteLinuxEnvironmentReader(device, this);
    connect(target, &ProjectExplorer::Target::kitChanged,
            deviceEnvReader, &RemoteLinuxEnvironmentReader::handleCurrentDeviceConfigChanged);

    QPushButton *button = fetchButton();
    button->setText(FetchEnvButtonText);
    connect(button, &QPushButton::clicked, this, &RemoteLinuxEnvironmentAspectWidget::fetchEnvironment);
    connect(deviceEnvReader, &RemoteLinuxEnvironmentReader::finished,
            this, &RemoteLinuxEnvironmentAspectWidget::fetchEnvironmentFinished);
    connect(deviceEnvReader, &RemoteLinuxEnvironmentReader::error,
            this, &RemoteLinuxEnvironmentAspectWidget::fetchEnvironmentError);
}

RemoteLinuxEnvironmentAspect *RemoteLinuxEnvironmentAspectWidget::aspect() const
{
    return dynamic_cast<RemoteLinuxEnvironmentAspect *>(EnvironmentAspectWidget::aspect());
}

QPushButton *RemoteLinuxEnvironmentAspectWidget::fetchButton() const
{
    return qobject_cast<QPushButton *>(additionalWidget());
}

void RemoteLinuxEnvironmentAspectWidget::fetchEnvironment()
{
    QPushButton *button = fetchButton();
    disconnect(button, &QPushButton::clicked,
               this, &RemoteLinuxEnvironmentAspectWidget::fetchEnvironment);
    connect(button, &QPushButton::clicked,
            this, &RemoteLinuxEnvironmentAspectWidget::stopFetchEnvironment);
    button->setText(tr("Cancel Fetch Operation"));
    deviceEnvReader->start();
}

void RemoteLinuxEnvironmentAspectWidget::fetchEnvironmentFinished()
{
    QPushButton *button = fetchButton();
    disconnect(button, &QPushButton::clicked,
               this, &RemoteLinuxEnvironmentAspectWidget::stopFetchEnvironment);
    connect(button, &QPushButton::clicked,
            this, &RemoteLinuxEnvironmentAspectWidget::fetchEnvironment);
    button->setText(FetchEnvButtonText);
    aspect()->setRemoteEnvironment(deviceEnvReader->remoteEnvironment());
}

void RemoteLinuxEnvironmentAspectWidget::fetchEnvironmentError(const QString &error)
{
    QMessageBox::warning(this, tr("Device Error"),
        tr("Fetching environment failed: %1").arg(error));
}

void RemoteLinuxEnvironmentAspectWidget::stopFetchEnvironment()
{
    deviceEnvReader->stop();
    fetchEnvironmentFinished();
}

// --------------------------------------------------------------------
// RemoteLinuxEnvironmentAspectWidget:
// --------------------------------------------------------------------

} // namespace RemoteLinux
