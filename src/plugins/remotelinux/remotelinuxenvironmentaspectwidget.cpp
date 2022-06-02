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

#include "linuxdevice.h"
#include "remotelinuxenvironmentaspect.h"
#include "remotelinuxenvironmentreader.h"

#include <coreplugin/icore.h>
#include <projectexplorer/environmentwidget.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QMessageBox>
#include <QPushButton>

using namespace ProjectExplorer;
using namespace RemoteLinux::Internal;
using namespace Utils;

namespace {
const QString FetchEnvButtonText
    = QCoreApplication::translate("RemoteLinux::RemoteLinuxEnvironmentAspectWidget",
                                  "Fetch Device Environment");
} // anonymous namespace

namespace RemoteLinux {

RemoteLinuxEnvironmentAspectWidget::RemoteLinuxEnvironmentAspectWidget
        (RemoteLinuxEnvironmentAspect *aspect, Target *target)
    : EnvironmentAspectWidget(aspect)
    , m_fetchButton(new QPushButton(FetchEnvButtonText))
{
    addWidget(m_fetchButton);

    IDevice::ConstPtr device = DeviceKitAspect::device(target->kit());

    m_deviceEnvReader = new RemoteLinuxEnvironmentReader(device, this);
    connect(target, &ProjectExplorer::Target::kitChanged,
            m_deviceEnvReader, &RemoteLinuxEnvironmentReader::handleCurrentDeviceConfigChanged);

    connect(m_fetchButton, &QPushButton::clicked, this, &RemoteLinuxEnvironmentAspectWidget::fetchEnvironment);
    connect(m_deviceEnvReader, &RemoteLinuxEnvironmentReader::finished,
            this, &RemoteLinuxEnvironmentAspectWidget::fetchEnvironmentFinished);
    connect(m_deviceEnvReader, &RemoteLinuxEnvironmentReader::error,
            this, &RemoteLinuxEnvironmentAspectWidget::fetchEnvironmentError);

    const EnvironmentWidget::OpenTerminalFunc openTerminalFunc
            = [target](const Utils::Environment &env) {
        IDevice::ConstPtr device = DeviceKitAspect::device(target->kit());
        if (!device) {
            QMessageBox::critical(Core::ICore::dialogParent(),
                                  tr("Cannot Open Terminal"),
                                  tr("Cannot open remote terminal: Current kit has no device."));
            return;
        }
        const auto linuxDevice = device.dynamicCast<const LinuxDevice>();
        QTC_ASSERT(linuxDevice, return);
        linuxDevice->openTerminal(env, FilePath());
    };
    envWidget()->setOpenTerminalFunc(openTerminalFunc);
}

void RemoteLinuxEnvironmentAspectWidget::fetchEnvironment()
{
    disconnect(m_fetchButton, &QPushButton::clicked,
               this, &RemoteLinuxEnvironmentAspectWidget::fetchEnvironment);
    connect(m_fetchButton, &QPushButton::clicked,
            this, &RemoteLinuxEnvironmentAspectWidget::stopFetchEnvironment);
    m_fetchButton->setText(tr("Cancel Fetch Operation"));
    m_deviceEnvReader->start();
}

void RemoteLinuxEnvironmentAspectWidget::fetchEnvironmentFinished()
{
    disconnect(m_fetchButton, &QPushButton::clicked,
               this, &RemoteLinuxEnvironmentAspectWidget::stopFetchEnvironment);
    connect(m_fetchButton, &QPushButton::clicked,
            this, &RemoteLinuxEnvironmentAspectWidget::fetchEnvironment);
    m_fetchButton->setText(FetchEnvButtonText);
    qobject_cast<RemoteLinuxEnvironmentAspect *>(aspect())->setRemoteEnvironment(
                m_deviceEnvReader->remoteEnvironment());
}

void RemoteLinuxEnvironmentAspectWidget::fetchEnvironmentError(const QString &error)
{
    QMessageBox::warning(this, tr("Device Error"),
        tr("Fetching environment failed: %1").arg(error));
}

void RemoteLinuxEnvironmentAspectWidget::stopFetchEnvironment()
{
    m_deviceEnvReader->stop();
    fetchEnvironmentFinished();
}

// --------------------------------------------------------------------
// RemoteLinuxEnvironmentAspectWidget:
// --------------------------------------------------------------------

} // namespace RemoteLinux
