// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dockerdebuggertest.h"

#include <debugger/debuggerrunconfigurationaspect.h>

#include "dockerapi.h"
#include "dockerdevice.h"
#include "dockersettings.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runcontrol.h>

#include <utils/aspects.h>
#include <utils/portlist.h>
#include <utils/qtcprocess.h>

#include <QtTaskTree/QTaskTree>

#include <QLoggingCategory>
#include <QTest>

using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace Docker::Internal {

// Verifies the fix for QTCREATORBUG-34093, part 2:
// DebuggerRunWorkerFactory now calls requestQmlChannel() for Docker devices
// when QML debugging is enabled. Before the fix no QML channel port was ever
// allocated and the application waited indefinitely for the debugger.

class DockerQmlChannelTest : public QObject
{
    Q_OBJECT

private:
    // Build AspectContainerData with QML debugging enabled and C++/Python disabled.
    static AspectContainerData makeQmlOnlyAspectData()
    {
        AspectContainerData data;
        data.append(Debugger::DebuggerRunConfigurationAspect::Data::createQmlTestData());
        return data;
    }

    // Build AspectContainerData with both C++ and QML debugging enabled.
    static AspectContainerData makeCombinedAspectData()
    {
        AspectContainerData data;
        data.append(Debugger::DebuggerRunConfigurationAspect::Data::createCombinedTestData());
        return data;
    }

private slots:
    void cleanupTestCase()
    {
        for (Kit *k : m_kits)
            KitManager::deregisterKit(k);
        m_kits.clear();
    }

    // Test 1: Docker device + QML debugging -> requestQmlChannel() must be called.
    void testDockerQmlDebuggingRequestsQmlChannel()
    {
        auto device = DockerDevice::create(&dockerSettings());

        Kit *kit = KitManager::registerKit([](Kit *k) {
            k->setUnexpandedDisplayName("Docker_DockerDebuggerTest");
        });
        QVERIFY(kit);
        m_kits.append(kit);

        auto *rc = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        // setKit() sets d->data.kit and calls setDevice(RunDeviceKitAspect::device(kit)).
        // The kit has no device, so setDevice(nullptr) is called first, then
        // setDeviceForTest() overrides it with the Docker device.
        rc->setKit(kit);
        rc->setDeviceForTest(device);
        rc->setRunConfigIdForTest(ProjectExplorer::Constants::CMAKE_RUNCONFIG_ID);
        rc->setAspectDataForTest(makeQmlOnlyAspectData());

        // createRecipe() finds DebuggerRunWorkerFactory and calls its recipe producer.
        // The producer calls requestQmlChannel() as a side effect when the device
        // is DockerDeviceType and isQmlDebugging() is true.
        rc->createRecipe(ProjectExplorer::Constants::DEBUG_RUN_MODE);

        QVERIFY(rc->usesQmlChannel());
        delete rc;
    }

    // Test 2: Docker device + combined C++/QML debugging -> requestQmlChannel() must be called.
    // Regression test for QTCREATORBUG-34093: combined mode must also trigger
    // requestQmlChannel() so that a QML channel port is allocated before
    // fixupParameters() looks at m_qmlServer.
    void testDockerCombinedDebuggingRequestsQmlChannel()
    {
        auto device = DockerDevice::create(&dockerSettings());

        Kit *kit = KitManager::registerKit([](Kit *k) {
            k->setUnexpandedDisplayName("Docker_CombinedDebuggerTest");
        });
        QVERIFY(kit);
        m_kits.append(kit);

        auto *rc = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        rc->setKit(kit);
        rc->setDeviceForTest(device);
        rc->setRunConfigIdForTest(ProjectExplorer::Constants::CMAKE_RUNCONFIG_ID);
        rc->setAspectDataForTest(makeCombinedAspectData());

        rc->createRecipe(ProjectExplorer::Constants::DEBUG_RUN_MODE);

        QVERIFY(rc->usesQmlChannel());
        delete rc;
    }

    // Test 3: Non-Docker device (no device) + QML debugging -> no requestQmlChannel().
    void testNonDockerQmlDebuggingDoesNotRequestQmlChannel()
    {
        Kit *kit = KitManager::registerKit([](Kit *k) {
            k->setUnexpandedDisplayName("Docker_NonDockerDebuggerTest");
        });
        QVERIFY(kit);
        m_kits.append(kit);

        auto *rc = new RunControl(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        rc->setKit(kit);
        // No device set -> condition "device && type != DESKTOP" is false.
        rc->setRunConfigIdForTest(ProjectExplorer::Constants::CMAKE_RUNCONFIG_ID);
        rc->setAspectDataForTest(makeQmlOnlyAspectData());
        rc->createRecipe(ProjectExplorer::Constants::DEBUG_RUN_MODE);

        QVERIFY(!rc->usesQmlChannel());
        delete rc;
    }

private:
    QList<Kit *> m_kits;
};

QObject *createDockerQmlChannelTest()
{
    return new DockerQmlChannelTest;
}

// DockerPortsGatheringTest
//
// QTCREATORBUG-34093, part 1:
// portsGatheringRecipe() reads /proc/net/tcp and /proc/net/tcp6 directly via
// docker exec, bypassing the isReadableDir("/proc/net") probe that returned
// false through the Docker file-access bridge.
//
// Requires a running Docker daemon and alpine:latest; skipped automatically
// when either is unavailable.

class DockerPortsGatheringTest : public QObject
{
    Q_OBJECT

private:
    DockerDevice::Ptr m_device;

private slots:
    void initTestCase()
    {
        if (!DockerApi::instance()->canConnect())
            QSKIP("Docker daemon is not reachable");

        const FilePath dockerBin = dockerSettings().binaryPath.effectiveBinary();
        Process imageCheck;
        imageCheck.setCommand({dockerBin, {"image", "inspect", "alpine:latest",
                                           "--format", "{{.Id}}"}});
        imageCheck.runBlocking();
        if (imageCheck.result() != ProcessResult::FinishedWithSuccess)
            QSKIP("alpine:latest not available; run: docker pull alpine:latest");

        m_device = DockerDevice::create(&dockerSettings());
        m_device->repo.setValue("alpine");
        m_device->tag.setValue("latest");
        m_device->imageId.setValue(imageCheck.stdOut().trimmed());

        DeviceManager::addDevice(m_device);

        const Result<> r = m_device->updateContainerAccess();
        if (!r)
            QSKIP(qPrintable("Failed to start Docker container: " + r.error()));
    }

    void cleanupTestCase()
    {
        // When the container is killed, the bridge process exits with code -1.
        // The CmdBridgeClient logs expected errors during this forced teardown.
        // Suppress the category for cleanup so they do not pollute test output.
        QLoggingCategory::setFilterRules("qtc.cmdbridge.client=false");
        if (m_device) {
            DeviceManager::removeDevice(m_device->id());
            m_device->shutdown();
        }
        m_device.reset();
        QLoggingCategory::setFilterRules("");
    }

    // Verifies that portsGatheringRecipe() succeeds inside a Docker container.
    void testPortsGatheringRecipeSucceeds()
    {
        m_device->setFreePorts(PortList::fromString("10000-10099"));

        // Capture the result inside the tree: storage data is freed when the tree
        // is destroyed on return from runBlocking(), so it must not be accessed after.
        bool portsValid = false;
        QString portsError;
        const Storage<PortsOutputData> output;
        const auto onDone = [&portsValid, &portsError, output] {
            portsValid = bool(*output);
            if (!portsValid)
                portsError = (*output).error();
        };

        const DoneWith doneWith = QTaskTree::runBlocking(
            Group{output, m_device->portsGatheringRecipe(output), onGroupDone(onDone)});

        QCOMPARE(doneWith, DoneWith::Success);
        QVERIFY2(portsValid, qPrintable(portsError));
    }

    // Verifies that a port actively listened on inside the container is detected.
    void testPortsGatheringRecipeFindsListeningPort()
    {
        constexpr quint16 testPort = 10003;
        m_device->setFreePorts(PortList::fromString("10000-10099"));

        // Start nc in the background and poll /proc/net/tcp until the port
        // appears. The shell returns only once the socket is confirmed bound,
        // so no fixed sleep is needed on the host side.
        const QString portHex = QString("%1").arg(testPort, 4, 16, QChar('0')).toUpper();
        Process plant;
        plant.setCommand(CommandLine{
            m_device->filePath("/bin/sh"),
            {"-c", QString("nc -l -p %1 & "
                           "until grep -q ':%2 ' /proc/net/tcp /proc/net/tcp6 2>/dev/null; "
                           "do :; done").arg(testPort).arg(portHex)}});
        plant.runBlocking();

        QList<Port> foundPorts;
        QString portsError;
        const Storage<PortsOutputData> output;
        const auto onDone = [&foundPorts, &portsError, output] {
            if (*output)
                foundPorts = **output;
            else
                portsError = (*output).error();
        };

        const DoneWith doneWith = QTaskTree::runBlocking(
            Group{output, m_device->portsGatheringRecipe(output), onGroupDone(onDone)});

        QCOMPARE(doneWith, DoneWith::Success);
        QVERIFY2(portsError.isEmpty(), qPrintable(portsError));
        QVERIFY2(foundPorts.contains(Port(testPort)),
                 qPrintable(QString("Port %1 not found in /proc/net/tcp output").arg(testPort)));
    }
};

QObject *createDockerPortsGatheringTest()
{
    return new DockerPortsGatheringTest;
}

} // namespace Docker::Internal

#include "dockerdebuggertest.moc"
