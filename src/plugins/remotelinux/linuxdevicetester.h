// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "remotelinux_export.h"

#include <projectexplorer/devicesupport/idevice.h>

namespace ProjectExplorer { enum class FileTransferMethod; }
namespace Utils { class ProcessResultData; }

namespace RemoteLinux {

namespace Internal { class GenericLinuxDeviceTesterPrivate; }

class REMOTELINUX_EXPORT GenericLinuxDeviceTester : public ProjectExplorer::DeviceTester
{
    Q_OBJECT

public:
    explicit GenericLinuxDeviceTester(QObject *parent = nullptr);
    ~GenericLinuxDeviceTester() override;

    void testDevice(const ProjectExplorer::IDevice::Ptr &deviceConfiguration) override;
    void stopTest() override;

private:
    void testEcho();
    void handleEchoDone();

    void testUname();
    void handleUnameDone();

    void testPortsGatherer();
    void handlePortsGathererError(const QString &message);
    void handlePortsGathererDone();

    void testFileTransfer(ProjectExplorer::FileTransferMethod method);
    void handleFileTransferDone(const Utils::ProcessResultData &resultData);

    void testCommands();
    void testNextCommand();
    void handleCommandDone();

    void setFinished(ProjectExplorer::DeviceTester::TestResult result);

    std::unique_ptr<Internal::GenericLinuxDeviceTesterPrivate> d;
};

} // namespace RemoteLinux
