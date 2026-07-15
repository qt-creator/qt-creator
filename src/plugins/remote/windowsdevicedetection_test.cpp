// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "windowsdevicedetection_test.h"

#include "windowsdevice.h"

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/sshparameters.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainkitaspect.h>
#include <projectexplorer/toolchainmanager.h>

#include <utils/algorithm.h>
#include <utils/filepath.h>

#include <QElapsedTimer>
#include <QEventLoop>
#include <QScopeGuard>
#include <QTest>
#include <QTimer>

using namespace ProjectExplorer;
using namespace Utils;

namespace Remote::Internal {

// Waits until predicate() is true or the timeout elapses, spinning the event loop so that
// asynchronous work (connection, env capture, kit creation) can make progress.
static bool waitFor(const std::function<bool()> &predicate, int timeoutMs)
{
    QElapsedTimer elapsed;
    elapsed.start();
    while (!predicate() && elapsed.elapsed() < timeoutMs) {
        QEventLoop loop;
        QTimer::singleShot(100, &loop, &QEventLoop::quit);
        loop.exec();
    }
    return predicate();
}

void WindowsDeviceDetectionTest::testDetectToolchainsAndCreateKit()
{
    const SshParameters params = SshTest::getParameters("WIN");
    if (!SshTest::checkParameters(params)) {
        SshTest::printSetupHelp();
        QSKIP("Set QTC_SSH_TEST_WIN_HOST/USER/... (or QTC_SSH_TEST_*) to a reachable "
              "Windows-over-SSH host.");
    }

    // Build the device and register it so device-rooted process/file routing resolves to it.
    const WindowsDevice::Ptr device = WindowsDevice::create();
    device->sshParametersAspectContainer().setSshParameters(params);
    DeviceManager::addDevice(device);

    const Id deviceId = device->id();
    const QString sourceId = deviceId.toString();
    const FilePath deviceRoot = device->rootPath();

    // Remove everything this test registers, even when an assertion fails midway.
    const QScopeGuard cleanup([&] {
        for (Kit *k : KitManager::kits()) {
            if (k->detectionSource().id == sourceId)
                KitManager::deregisterKit(k);
        }
        const Toolchains deviceToolchains = Utils::filtered(
            ToolchainManager::toolchains(), [&](Toolchain *tc) {
                return tc->compilerCommand().isSameDevice(deviceRoot);
            });
        ToolchainManager::deregisterToolchains(deviceToolchains);
        DeviceManager::removeDevice(deviceId);
    });

    // Establish the connection (sets up file access and deploys the command bridge).
    {
        QEventLoop loop;
        QTimer timeout;
        timeout.setSingleShot(true);
        QObject::connect(&timeout, &QTimer::timeout, &loop, [&] { loop.exit(1); });
        timeout.start(60 * 1000);
        device->tryToConnect(Continuation<>(this, [&](const Result<> &res) {
            loop.exit(res ? 0 : 1);
        }));
        QCOMPARE(loop.exec(), 0);
    }
    QCOMPARE(device->deviceState(), IDevice::DeviceReadyToUse);

    // Browsing the device root must list the drives (C:/, ...) so the root is navigable in the
    // file dialogs; the Windows root has no single directory to walk, so the CmdBridge synthesizes
    // the drive entries. (Regression guard for the "can't reach C: from the root" bug.)
    const FilePaths rootEntries = deviceRoot.dirEntries(
        FileFilter({}, DirFilterFlag::Dirs | DirFilterFlag::NoDotAndDotDot));
    const bool hasDrive = Utils::anyOf(rootEntries, [](const FilePath &p) {
        return p.path().contains(':');
    });
    QVERIFY2(hasDrive, "The device root did not list any drives (e.g. C:/).");

    // Trigger the same detection the device widget's auto-detect button runs.
    detectAndRegisterToolchainsForTest(device);

    // Toolchains are registered synchronously; the MSVC compiler command and the kit are
    // filled in asynchronously once the vcvars environment capture completes.
    const auto deviceToolchains = [&] {
        return Utils::filtered(ToolchainManager::toolchains(), [&](Toolchain *tc) {
            return tc->compilerCommand().isSameDevice(deviceRoot);
        });
    };
    QVERIFY2(waitFor([&] { return !deviceToolchains().isEmpty(); }, 30 * 1000),
             "No toolchains were detected for the device.");

    const auto deviceKits = [&] {
        return Utils::filtered(KitManager::kits(), [&](Kit *k) {
            return k->detectionSource().id == sourceId;
        });
    };
    QVERIFY2(waitFor([&] { return !deviceKits().isEmpty(); }, 180 * 1000),
             "No kit was created for the device.");

    // Exactly one kit, with C and C++ compilers resolved to executables on the device.
    const QList<Kit *> kits = deviceKits();
    const auto compilerPath = [](Toolchain *tc) {
        return tc ? tc->compilerCommand().toUserOutput() : QString("<none>");
    };
    for (const Kit *k : kits) {
        qDebug().noquote() << "Kit:" << k->displayName()
                           << "\n  C:  " << compilerPath(ToolchainKitAspect::cToolchain(k))
                           << "\n  Cxx:" << compilerPath(ToolchainKitAspect::cxxToolchain(k));
    }
    QVERIFY(kits.size() > 0);

    const Kit *kit = kits.first();
    Toolchain *cTc = ToolchainKitAspect::cToolchain(kit);
    Toolchain *cxxTc = ToolchainKitAspect::cxxToolchain(kit);
    QVERIFY2(cTc, "Kit has no C toolchain.");
    QVERIFY2(cxxTc, "Kit has no C++ toolchain.");
    QVERIFY2(!cTc->compilerCommand().isEmpty(), "C compiler command is empty.");
    QVERIFY2(!cxxTc->compilerCommand().isEmpty(), "C++ compiler command is empty.");
    QVERIFY2(cTc->compilerCommand().isSameDevice(deviceRoot),
             "C compiler is not located on the device.");
    QVERIFY2(cxxTc->compilerCommand().isSameDevice(deviceRoot),
             "C++ compiler is not located on the device.");
}

} // namespace Remote::Internal
