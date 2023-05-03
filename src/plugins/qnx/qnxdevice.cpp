// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qnxdevice.h"

#include "qnxconstants.h"
#include "qnxdeployqtlibrariesdialog.h"
#include "qnxdevicetester.h"
#include "qnxtr.h"

#include <coreplugin/icore.h>

#include <projectexplorer/devicesupport/sshparameters.h>

#include <remotelinux/genericlinuxdeviceconfigurationwizardpages.h>
#include <remotelinux/remotelinuxsignaloperation.h>
#include <remotelinux/linuxdevice.h>

#include <utils/port.h>
#include <utils/portlist.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/wizard.h>

using namespace ProjectExplorer;
using namespace RemoteLinux;
using namespace Utils;

namespace Qnx::Internal {

static QString signalProcessByNameQnxCommandLine(const QString &filePath, int sig)
{
    QString executable = filePath;
    return QString::fromLatin1("for PID in $(ps -f -o pid,comm | grep %1 | awk '/%1/ {print $1}'); "
        "do "
            "kill -%2 $PID; "
        "done").arg(executable.replace(QLatin1String("/"), QLatin1String("\\/"))).arg(sig);
}

class QnxDeviceProcessSignalOperation : public RemoteLinuxSignalOperation
{
public:
    explicit QnxDeviceProcessSignalOperation(const IDeviceConstPtr &device)
        : RemoteLinuxSignalOperation(device)
    {}

    QString killProcessByNameCommandLine(const QString &filePath) const override
    {
        return QString::fromLatin1("%1; %2").arg(signalProcessByNameQnxCommandLine(filePath, 15),
                                                 signalProcessByNameQnxCommandLine(filePath, 9));
    }

    QString interruptProcessByNameCommandLine(const QString &filePath) const override
    {
        return signalProcessByNameQnxCommandLine(filePath, 2);
    }
};

class QnxDevice final : public LinuxDevice
{
public:
    QnxDevice()
    {
        setDisplayType(Tr::tr("QNX"));
        setDefaultDisplayName(Tr::tr("QNX Device"));
        setOsType(OsTypeOtherUnix);
        setupId(IDevice::ManuallyAdded);
        setType(Constants::QNX_QNX_OS_TYPE);
        setMachineType(IDevice::Hardware);
        SshParameters sshParams;
        sshParams.timeout = 10;
        setSshParameters(sshParams);
        setFreePorts(PortList::fromString("10000-10100"));

        addDeviceAction({Tr::tr("Deploy Qt libraries..."), [](const IDevice::Ptr &device, QWidget *parent) {
            QnxDeployQtLibrariesDialog dialog(device, parent);
            dialog.exec();
        }});
    }

    DeviceProcessSignalOperation::Ptr signalOperation() const final
    {
        return DeviceProcessSignalOperation::Ptr(new QnxDeviceProcessSignalOperation(sharedFromThis()));
    }

    DeviceTester *createDeviceTester() const final { return new QnxDeviceTester; }
};

class QnxDeviceWizard : public Wizard
{
public:
    QnxDeviceWizard() : Wizard(Core::ICore::dialogParent())
    {
        setWindowTitle(Tr::tr("New QNX Device Configuration Setup"));

        addPage(&m_setupPage);
        addPage(&m_keyDeploymentPage);
        addPage(&m_finalPage);
        m_finalPage.setCommitPage(true);

        m_device.reset(new QnxDevice);

        m_setupPage.setDevice(m_device);
        m_keyDeploymentPage.setDevice(m_device);
    }

    IDevice::Ptr device() const { return m_device; }

private:
    GenericLinuxDeviceConfigurationWizardSetupPage m_setupPage;
    GenericLinuxDeviceConfigurationWizardKeyDeploymentPage m_keyDeploymentPage;
    GenericLinuxDeviceConfigurationWizardFinalPage m_finalPage;

    LinuxDevice::Ptr m_device;
};

// Factory

QnxDeviceFactory::QnxDeviceFactory() : IDeviceFactory(Constants::QNX_QNX_OS_TYPE)
{
    setDisplayName(Tr::tr("QNX Device"));
    setCombinedIcon(":/qnx/images/qnxdevicesmall.png",
                    ":/qnx/images/qnxdevice.png");
    setQuickCreationAllowed(true);
    setConstructionFunction([] { return IDevice::Ptr(new QnxDevice); });
    setCreator([] {
        QnxDeviceWizard wizard;
        if (wizard.exec() != QDialog::Accepted)
            return IDevice::Ptr();
        return wizard.device();
    });
}

} // Qnx::Internal
