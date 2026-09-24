// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "remoterun_test.h"

#include "linuxdevice.h"
#include "remotelinux_constants.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/sshparameters.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runcontrol.h>

#include <utils/outputformat.h>
#include <utils/result.h>

#include <QSignalSpy>
#include <QTest>

using namespace ProjectExplorer;
using namespace Utils;

namespace Remote::Internal {

enum { Timeout = 30000 };

void RemoteRunTest::initTestCase()
{
    const SshParameters params = SshTest::getParameters();
    if (!SshTest::checkParameters(params)) {
        SshTest::printSetupHelp();
        QSKIP("Set the QTC_SSH_TEST_* variables to a reachable Linux host, or follow the "
              "setup help above.");
    }

    const LinuxDevice::Ptr device = LinuxDevice::create();
    device->setupId(IDevice::ManuallyAdded);
    device->setType(Constants::GenericLinuxOsType);
    device->setDisplayName("Remote run test device");
    device->sshParametersAspectContainer().setSshParameters(params);
    DeviceManager::addDevice(device);
    m_device = device;

    bool done = false;
    QString error;
    device->tryToConnect(Continuation<>(this, [&done, &error](const Result<> &res) {
        if (!res)
            error = res.error();
        done = true;
    }));
    QTRY_VERIFY_WITH_TIMEOUT(done, Timeout);
    QVERIFY2(error.isEmpty(), qPrintable(error));
}

void RemoteRunTest::cleanupTestCase()
{
    if (m_device)
        DeviceManager::removeDevice(m_device->id());
}

// Writing to the application's standard input has to survive the trip through the SSH
// client: what is written here reaches the remote command, and closing the write channel
// gives it an EOF rather than leaving it hanging.
void RemoteRunTest::testStandardInput()
{
    QVERIFY(m_device);

    RunControl runControl(ProjectExplorer::Constants::NORMAL_RUN_MODE);
    runControl.setDeviceForTest(m_device);
    runControl.setCommandLine({m_device->filePath("/bin/cat"), {}});
    runControl.setRunRecipe(runControl.processRecipe(runControl.processTask()));

    QString output;
    QString messages;
    connect(&runControl, &RunControl::appendMessage,
            [&output, &messages](const QString &msg, OutputFormat format) {
        messages += msg;
        if (format == StdOutFormat)
            output += msg;
    });

    QSignalSpy startedSpy(&runControl, &RunControl::started);
    QSignalSpy stoppedSpy(&runControl, &RunControl::stopped);
    runControl.initiateStart();
    QTRY_VERIFY2_WITH_TIMEOUT(!startedSpy.isEmpty(), qPrintable(messages), Timeout);
    QVERIFY(runControl.acceptsStandardInput());

    runControl.writeStandardInput("hello\n");
    QTRY_VERIFY2_WITH_TIMEOUT(output.contains("hello"), qPrintable(messages), Timeout);
    QVERIFY(runControl.acceptsStandardInput());

    runControl.closeStandardInput();
    QVERIFY(!runControl.acceptsStandardInput());
    QTRY_VERIFY2_WITH_TIMEOUT(!stoppedSpy.isEmpty(), qPrintable(messages), Timeout);
}

} // Remote::Internal
