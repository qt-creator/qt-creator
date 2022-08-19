// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "remotelinuxenvironmentaspectwidget.h"

#include "linuxdevice.h"
#include "remotelinuxenvironmentaspect.h"
#include "remotelinuxenvironmentreader.h"
#include "remotelinuxtr.h"

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

namespace RemoteLinux {

const QString FetchEnvButtonText = Tr::tr("Fetch Device Environment");

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
            = [target](const Environment &env) {
        IDevice::ConstPtr device = DeviceKitAspect::device(target->kit());
        if (!device) {
            QMessageBox::critical(Core::ICore::dialogParent(),
                                  Tr::tr("Cannot Open Terminal"),
                                  Tr::tr("Cannot open remote terminal: Current kit has no device."));
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
    m_fetchButton->setText(Tr::tr("Cancel Fetch Operation"));
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
    QMessageBox::warning(this, Tr::tr("Device Error"),
        Tr::tr("Fetching environment failed: %1").arg(error));
}

void RemoteLinuxEnvironmentAspectWidget::stopFetchEnvironment()
{
    m_deviceEnvReader->stop();
    fetchEnvironmentFinished();
}

} // RemoteLinux
