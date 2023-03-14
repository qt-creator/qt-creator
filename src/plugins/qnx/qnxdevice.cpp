// Copyright (C) 2016 BlackBerry Limited. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qnxdevice.h"

#include "qnxconstants.h"
#include "qnxdeployqtlibrariesdialog.h"
#include "qnxdevicetester.h"
#include "qnxdevicewizard.h"
#include "qnxtr.h"

#include <remotelinux/remotelinuxsignaloperation.h>

#include <utils/port.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QRegularExpression>

using namespace ProjectExplorer;
using namespace RemoteLinux;
using namespace Utils;

namespace Qnx::Internal {

const char QnxVersionKey[] = "QnxVersion";

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

QnxDevice::QnxDevice()
{
    setDisplayType(Tr::tr("QNX"));
    setDefaultDisplayName(Tr::tr("QNX Device"));
    setOsType(OsTypeOtherUnix);

    addDeviceAction({Tr::tr("Deploy Qt libraries..."), [](const IDevice::Ptr &device, QWidget *parent) {
        QnxDeployQtLibrariesDialog dialog(device, parent);
        dialog.exec();
    }});
}

int QnxDevice::qnxVersion() const
{
    if (m_versionNumber == 0)
        updateVersionNumber();

    return m_versionNumber;
}

void QnxDevice::updateVersionNumber() const
{
    QtcProcess versionNumberProcess;

    versionNumberProcess.setCommand({filePath("uname"), {"-r"}});
    versionNumberProcess.runBlocking(EventLoopMode::On);

    QByteArray output = versionNumberProcess.readAllRawStandardOutput();
    QString versionMessage = QString::fromLatin1(output);
    const QRegularExpression versionNumberRegExp("(\\d+)\\.(\\d+)\\.(\\d+)");
    const QRegularExpressionMatch match = versionNumberRegExp.match(versionMessage);
    if (match.hasMatch()) {
        int major = match.captured(1).toInt();
        int minor = match.captured(2).toInt();
        int patch = match.captured(3).toInt();
        m_versionNumber = (major << 16)|(minor<<8)|(patch);
    }
}

void QnxDevice::fromMap(const QVariantMap &map)
{
    m_versionNumber = map.value(QLatin1String(QnxVersionKey), 0).toInt();
    LinuxDevice::fromMap(map);
}

QVariantMap QnxDevice::toMap() const
{
    QVariantMap map(LinuxDevice::toMap());
    map.insert(QLatin1String(QnxVersionKey), m_versionNumber);
    return map;
}

PortsGatheringMethod QnxDevice::portsGatheringMethod() const
{
    return {
        // TODO: The command is probably needlessly complicated because the parsing method
        // used to be fixed. These two can now be matched to each other.
        [this](QAbstractSocket::NetworkLayerProtocol protocol) -> CommandLine {
            Q_UNUSED(protocol)
            return {filePath("netstat"), {"-na"}};
        },

        &Port::parseFromNetstatOutput
    };
}

DeviceTester *QnxDevice::createDeviceTester() const
{
    return new QnxDeviceTester;
}

DeviceProcessSignalOperation::Ptr QnxDevice::signalOperation() const
{
    return DeviceProcessSignalOperation::Ptr(new QnxDeviceProcessSignalOperation(sharedFromThis()));
}

// Factory

QnxDeviceFactory::QnxDeviceFactory() : IDeviceFactory(Constants::QNX_QNX_OS_TYPE)
{
    setDisplayName(Tr::tr("QNX Device"));
    setCombinedIcon(":/qnx/images/qnxdevicesmall.png",
                    ":/qnx/images/qnxdevice.png");
    setConstructionFunction(&QnxDevice::create);
    setCreator(&runDeviceWizard);
}

} // Qnx::Internal
