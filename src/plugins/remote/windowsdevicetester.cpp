// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "windowsdevicetester.h"

#include "remotelinuxtr.h"
#include "windowsdevice.h"

#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace Remote {

WindowsDeviceTester::WindowsDeviceTester(const IDevice::Ptr &device, QObject *parent)
    : DeviceTester(device, parent)
{}

void WindowsDeviceTester::testDevice()
{
    const auto windowsDevice = std::dynamic_pointer_cast<WindowsDevice>(device());
    QTC_ASSERT(windowsDevice, emit finished(TestFailure); return);

    emit progressMessage(Tr::tr("Connecting to device and checking for Windows...") + '\n');
    windowsDevice->tryToConnect(Continuation<>(this, [this](const Result<> &res) {
        if (res) {
            emit progressMessage(Tr::tr("The device is a Windows machine and is reachable.") + '\n');
            emit finished(TestSuccess);
        } else {
            emit errorMessage(res.error() + '\n');
            emit finished(TestFailure);
        }
    }));
}

void WindowsDeviceTester::stopTest()
{
    emit finished(TestFailure);
}

} // namespace Remote
