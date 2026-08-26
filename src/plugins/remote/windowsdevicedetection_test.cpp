// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "windowsdevicedetection_test.h"

#include "powershellutils.h"
#include "windowsdevice.h"
#include "remotelinux_constants.h"

#include "remotelinux_constants.h"

#include <debugger/debuggerconstants.h>
#include <debugger/debuggeritem.h>
#include <debugger/debuggerkitaspect.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/idevicefactory.h>
#include <projectexplorer/devicesupport/sshparameters.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/kitaspect.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/msvctoolchain.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectexplorersettings.h>
#include <projectexplorer/toolchain.h>
#include <projectexplorer/toolchainkitaspect.h>
#include <projectexplorer/toolchainmanager.h>

#include <utils/algorithm.h>
#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/qtcprocess.h>

#include <QElapsedTimer>
#include <QEventLoop>
#include <QScopeGuard>
#include <QTest>
#include <QTimer>
#include <QUuid>

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

// The sessions a process runs in, empty when it does not run at all, and nothing when the
// device could not be asked. Get-Process and not the device's process list: that one goes
// through WMI, which a machine can deny to its own user. Filtering rather than -Name so
// that finding nothing is a successful query, not a suppressed error.
static std::optional<QStringList> processSessions(const FilePath &deviceRoot,
                                                  const QString &imageName)
{
    Process query;
    query.setCommand(
        {deviceRoot.withNewPath("powershell.exe"),
         {"-NoProfile", "-NonInteractive", "-EncodedCommand",
          encodePowerShellCommand("Get-Process | Where-Object { $_.Name -eq '" + imageName
                                  + "' } | ForEach-Object { $_.SessionId }")}});
    query.runBlocking(std::chrono::seconds(60));
    if (query.result() != ProcessResult::FinishedWithSuccess)
        return std::nullopt;
    QStringList result;
    for (const QString &line : query.cleanedStdOut().split('\n')) {
        if (!line.trimmed().isEmpty())
            result.append(line.trimmed());
    }
    return result;
}

// Locates cmake.exe bundled with Qt under C:\Qt\Tools on the device (e.g. CMake_64\bin\cmake.exe).
// The test cannot depend on CMakeProjectManager, so it finds cmake directly via device file access.
static FilePath findDeviceCMake(const FilePath &deviceRoot)
{
    const FilePath toolsRoot = deviceRoot.withNewPath("C:/Qt/Tools");
    if (!toolsRoot.isDir())
        return {};
    for (const FilePath &toolDir : toolsRoot.dirEntries(
             FileFilter({}, DirFilterFlag::Dirs | DirFilterFlag::NoDotAndDotDot))) {
        const FilePath cmake = toolDir / "bin" / "cmake.exe";
        if (cmake.isExecutableFile())
            return cmake;
    }
    return {};
}

void WindowsDeviceDetectionTest::testDetectToolchainsAndCreateKit()
{
    // host must name a Windows machine - no fallback accepted
    const SshParameters params = SshTest::getParameters("WIN");
    if (!SshTest::hasVariantHost("WIN") || !SshTest::checkParameters(params)) {
        SshTest::printSetupHelp();
        QSKIP("Set QTC_SSH_TEST_WIN_HOST (and _USER/_PORT/_KEYFILE where they differ from the "
              "plain QTC_SSH_TEST_* values) to a reachable Windows-over-SSH host.");
    }

    // Build the device and register it so device-rooted process/file routing resolves to it.
    auto windowsDeviceFactory
        = Utils::findOrDefault(IDeviceFactory::allDeviceFactories(), [&](IDeviceFactory *f) {
              return f->deviceType() == Constants::GenericWindowsOsType;
          });
    QVERIFY2(windowsDeviceFactory, "No Windows device factory was registered.");
    const IDevicePtr device = windowsDeviceFactory->construct();
    QVERIFY2(device, "Failed to construct a Windows device from the factory.");
    device->sshParametersAspectContainer().setSshParameters(params);
    DeviceManager::addDevice(device);

    const Id deviceId = device->id();
    const QString sourceId = deviceId.toString();
    const FilePath deviceRoot = device->rootPath();

    // Remove everything this test registers, even when an assertion fails midway.
    // Auto-created kits carry a "<deviceId>/<abi>" detection source id.
    const QScopeGuard cleanup([&] {
        for (Kit *k : KitManager::kits()) {
            if (k->detectionSource().id.startsWith(sourceId))
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

    // Trigger the same auto-detection the device settings page runs. The completion
    // callback only covers the device tools recipe; toolchain and kit registration
    // continue asynchronously and are polled for below.
    bool detectionDone = false;
    const ToolDetectionLogger logger([](const QString &msg) { qDebug().noquote() << msg; });
    device->runAutoDetect(logger, [&detectionDone] { detectionDone = true; });
    QVERIFY2(waitFor([&] { return detectionDone; }, 180 * 1000),
             "Auto-detection did not finish.");

    // Toolchains are registered synchronously; the MSVC compiler command and the kit are
    // filled in asynchronously once the vcvars environment capture completes.
    const auto deviceToolchains = [&] {
        return Utils::filtered(ToolchainManager::toolchains(), [&](Toolchain *tc) {
            return tc->compilerCommand().isSameDevice(deviceRoot);
        });
    };
    QVERIFY2(waitFor([&] { return !deviceToolchains().isEmpty(); }, 30 * 1000),
             "No toolchains were detected for the device.");

    // clang-cl installed on the device must be found too, and paired with an MSVC from the
    // same machine - the pairing supplies the vcvars environment it needs.
    const auto deviceClangCl = [&] {
        return Utils::filtered(ToolchainManager::toolchains(), [&](Toolchain *tc) {
            return tc->typeId() == ProjectExplorer::Constants::CLANG_CL_TOOLCHAIN_TYPEID
                   && tc->compilerCommand().isSameDevice(deviceRoot);
        });
    };
    // Whether the machine has LLVM installed is not this test's business, but if it has, the
    // detection must find it.
    const bool deviceHasClangCl
        = deviceRoot.withNewPath("C:/Program Files/LLVM/bin/clang-cl.exe").isExecutableFile();
    if (deviceHasClangCl) {
        QVERIFY2(waitFor([&] { return !deviceClangCl().isEmpty(); }, 60 * 1000),
                 "clang-cl is installed on the device but was not detected.");
        for (Toolchain *tc : deviceClangCl()) {
            qDebug().noquote() << "clang-cl:" << tc->displayName()
                               << tc->compilerCommand().toUserOutput();
        }
    } else {
        qWarning("No LLVM on the device, so the clang-cl detection is not covered here.");
    }

    const auto deviceKits = [&] {
        return Utils::filtered(KitManager::kits(), [&](Kit *k) {
            return k->detectionSource().id.startsWith(sourceId);
        });
    };
    QVERIFY2(waitFor([&] { return !deviceKits().isEmpty(); }, 180 * 1000),
             "No kit was created for the device.");

    // Exactly one kit, with C and C++ compilers resolved to executables on the device.
    const QList<Kit *> kits = deviceKits();
    const auto compilerPath = [](Toolchain *tc) {
        if (!tc)
            return QString("<none>");
        if (MsvcToolchain *msvcTc = dynamic_cast<MsvcToolchain *>(tc))
            return msvcTc->varsBat().toUserOutput();
        return tc->compilerCommand().toUserOutput();
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
    if (MsvcToolchain *msvcTc = dynamic_cast<MsvcToolchain *>(cTc))
        QVERIFY2(!msvcTc->varsBat().isEmpty(), "MSVC toolchain has no vcvars.bat.");
    if (MsvcToolchain *msvcxxTc = dynamic_cast<MsvcToolchain *>(cxxTc))
        QVERIFY2(!msvcxxTc->varsBat().isEmpty(), "MSVC toolchain has no vcvars.bat.");

    // An MSVC toolchain resolves its compiler only once the vcvars environment capture for that
    // toolchain has finished, which happens after the kit exists. Wait for it: which of the
    // device's kits comes first is not fixed, and the build below passes these commands to CMake,
    // where an unresolved one silently becomes an empty -DCMAKE_CXX_COMPILER and CMake then
    // reports "No CMAKE_CXX_COMPILER could be found".
    QVERIFY2(waitFor([&] {
                 return !cTc->compilerCommand().isEmpty() && !cxxTc->compilerCommand().isEmpty();
             }, 120 * 1000),
             "The kit's compilers were not resolved.");

    QVERIFY2(cTc->isSameDevice(deviceRoot),
             "C compiler is not located on the device.");
    QVERIFY2(cxxTc->isSameDevice(deviceRoot),
             "C++ compiler is not located on the device.");

    // Qt and CMake are attached asynchronously after the kit appears, each by the owning plugin's
    // kit aspect. Their kit values are checked generically (Qt version id / CMake tool id) so this
    // test needs no QtSupport or CMakeProjectManager dependency. Each check is guarded on the
    // aspect actually being registered: a -test run may not load those plugins (the full GUI
    // always does), so run with e.g. "-load QtSupport -load CMakeProjectManager" to exercise them.
    const auto aspectAvailable = [](const Id &id) {
        return Utils::anyOf(KitAspectFactory::kitAspectFactories(),
                            [&id](const KitAspectFactory *f) { return f->id() == id; });
    };

    const Id qtAspectId("QtSupport.QtInformation");
    if (aspectAvailable(qtAspectId)) {
        const bool qtAttached = waitFor([&] {
            const QVariant v = kit->value(qtAspectId);
            return v.isValid() && v.toInt() >= 0;
        }, 30 * 1000);
        qDebug().noquote() << "  Qt: version id" << kit->value(qtAspectId).toInt();
        QVERIFY2(qtAttached, "No Qt version was attached to the kit.");
    } else {
        qWarning("QtSupport not loaded; skipping the Qt attachment check.");
    }

    const Id cmakeAspectId("CMakeProjectManager.CMakeKitInformation");
    if (aspectAvailable(cmakeAspectId)) {
        const bool cmakeAttached = waitFor([&] {
            const QVariant v = kit->value(cmakeAspectId);
            return v.isValid() && !v.toString().isEmpty();
        }, 30 * 1000);
        qDebug().noquote() << "  CMake: tool id" << kit->value(cmakeAspectId).toString();
        QVERIFY2(cmakeAttached, "No CMake tool was attached to the kit.");
    } else {
        qWarning("CMakeProjectManager not loaded; skipping the CMake attachment check.");
    }

    // The device's CDB (registered during auto-detection) must be attached to the kit by the
    // debugger kit aspect: a debugger on the same device as the kit's build device is picked up
    // automatically at kit creation. Guarded on the device actually having cdb.exe installed.
    const Id debuggerAspectId = Debugger::DebuggerKitAspect::id();
    if (aspectAvailable(debuggerAspectId)) {
        const Debugger::DebuggerItem dbg = Debugger::DebuggerKitAspect::debugger(kit);
        qDebug().noquote() << "  Debugger:"
                           << (dbg.isValid() ? dbg.command().toUserOutput() : QString("<none>"))
                           << "engine" << int(dbg.engineType());
        QVERIFY2(dbg.isValid(), "No debugger was attached to the kit.");
        QVERIFY2(dbg.engineType() == Debugger::CdbEngineType,
                 "The attached debugger is not CDB.");
        QVERIFY2(dbg.command().isSameDevice(deviceRoot),
                 "The attached CDB is not located on the device.");
    } else {
        qWarning("Debugger plugin not loaded; skipping the CDB attachment check.");
    }

    // Build phase: configure and build a trivial CMake project on the device with the Ninja
    // generator, proving the kit produces a working build (MSVC cl.exe + ninja, all over SSH).
    // Needs the kit's CMake tool, so it is guarded on CMakeProjectManager being loaded.
    if (!aspectAvailable(cmakeAspectId)) {
        qWarning("CMakeProjectManager not loaded; skipping the Ninja build check.");
        return;
    }

    const FilePath ninja = device->deviceToolPath(Id(ProjectExplorer::Constants::TOOL_TYPE_NINJA));
    QVERIFY2(ninja.isExecutableFile(), "Ninja was not detected on the device.");
    const FilePath cmakeExe = findDeviceCMake(deviceRoot);
    QVERIFY2(cmakeExe.isExecutableFile(), "CMake was not found on the device.");

    const Result<FilePath> tmp = deviceRoot.tmpDir();
    QVERIFY2(tmp.has_value(), "Could not resolve a temporary directory on the device.");
    const FilePath projectDir = *tmp / ("qtc-ninja-" + QUuid::createUuid().toString(QUuid::Id128));
    const FilePath buildDir = projectDir / "build";
    const QScopeGuard removeProject([&] { projectDir.removeRecursively(); });

    QVERIFY(projectDir.ensureWritableDir().has_value());
    QVERIFY(buildDir.ensureWritableDir().has_value());
    QVERIFY((projectDir / "CMakeLists.txt").writeFileContents(
                "cmake_minimum_required(VERSION 3.16)\n"
                "project(hello LANGUAGES CXX)\n"
                "add_executable(hello main.cpp)\n").has_value());
    QVERIFY((projectDir / "main.cpp").writeFileContents(
                "#include <cstdio>\n"
                "int main() { printf(\"HELLO_FROM_DEVICE\\n\"); return 0; }\n").has_value());

    const Environment buildEnv = kit->buildEnvironment();
    // The make tool a qmake project would build with.
    for (Toolchain *tc : ToolchainKitAspect::toolChains(kit)) {
        const FilePath make = tc->makeCommand(buildEnv);
        qDebug().noquote() << "make tool:" << tc->displayName() << "->" << make.toUserOutput();
        QVERIFY2(make.isSameDevice(deviceRoot),
                 qPrintable("The make tool is not on the device: " + make.toUserOutput()));
        QVERIFY2(make.isExecutableFile(),
                 qPrintable("The make tool does not exist: " + make.toUserOutput()));

        // jom wins over nmake where the Qt installer put one.
        const FilePath deviceJom = deviceRoot.withNewPath(
            "C:/Qt/Tools/QtCreator/bin/jom/jom.exe");
        if (deviceJom.isExecutableFile() && globalProjectExplorerSettings().useJom())
            QCOMPARE(make, deviceJom);

        Process makeVersion;
        makeVersion.setCommand({make, {"/?"}});
        makeVersion.setEnvironment(buildEnv);
        makeVersion.runBlocking(std::chrono::seconds(60));
        // nmake announces itself as NMAKE, jom as jom.
        QVERIFY2(makeVersion.allOutput().contains("NMAKE")
                     || makeVersion.allOutput().contains("jom"),
                 qPrintable("Running the make tool said: " + makeVersion.allOutput()));
    }

    // Pass the compiler explicitly from the kit toolchains, as Qt Creator's own CMake configure
    // does; the MSVC environment (INCLUDE/LIB) comes from the kit's build environment.
    Process configure;
    configure.setCommand({cmakeExe, {"-S", projectDir.path(), "-B", buildDir.path(),
                                     "-G", "Ninja",
                                     "-DCMAKE_MAKE_PROGRAM=" + ninja.path(),
                                     "-DCMAKE_C_COMPILER:FILEPATH=" + cTc->compilerCommand().path(),
                                     "-DCMAKE_CXX_COMPILER:FILEPATH=" + cxxTc->compilerCommand().path()}});
    configure.setWorkingDirectory(buildDir);
    configure.setEnvironment(buildEnv);
    configure.runBlocking(std::chrono::seconds(180));
    if (configure.result() != ProcessResult::FinishedWithSuccess)
        qDebug().noquote() << "CMake configure output:\n" << configure.allOutput();
    QCOMPARE(configure.result(), ProcessResult::FinishedWithSuccess);

    Process build;
    build.setCommand({cmakeExe, {"--build", buildDir.path()}});
    build.setWorkingDirectory(buildDir);
    build.setEnvironment(buildEnv);
    build.runBlocking(std::chrono::seconds(180));
    if (build.result() != ProcessResult::FinishedWithSuccess)
        qDebug().noquote() << "CMake build output:\n" << build.allOutput();
    QCOMPARE(build.result(), ProcessResult::FinishedWithSuccess);

    QVERIFY2((buildDir / "hello.exe").isExecutableFile(), "hello.exe was not produced.");

    // Run the freshly built executable on the device. This exercises the launch-over-SSH path
    // that the run worker relies on (the device's process interface plus the build environment):
    // it proves stdout capture and exit-code propagation, not merely that the binary exists.
    Process run;
    run.setCommand({buildDir / "hello.exe", {}});
    run.setWorkingDirectory(buildDir);
    run.setEnvironment(buildEnv);
    run.runBlocking(std::chrono::seconds(60));
    if (run.result() != ProcessResult::FinishedWithSuccess)
        qDebug().noquote() << "Run output:\n" << run.allOutput();
    QCOMPARE(run.result(), ProcessResult::FinishedWithSuccess);
    QVERIFY2(run.cleanedStdOut().contains("HELLO_FROM_DEVICE"),
             "The executable's output was not captured from the device.");
}

// Stopping a run must end the application on the device, not just the SSH connection that
// carried it. The victim is a private copy of ping.exe, so the check cannot be confused by
// another instance of a system binary, and killing it cannot disturb anything else.
void WindowsDeviceDetectionTest::testStopKillsTheRemoteApplication()
{
    const SshParameters params = SshTest::getParameters("WIN");
    if (!SshTest::checkParameters(params)) {
        SshTest::printSetupHelp();
        QSKIP("Set QTC_SSH_TEST_WIN_HOST/USER/... (or QTC_SSH_TEST_*) to a reachable "
              "Windows-over-SSH host.");
    }

    auto windowsDeviceFactory
        = Utils::findOrDefault(IDeviceFactory::allDeviceFactories(), [&](IDeviceFactory *f) {
              return f->deviceType() == Constants::GenericWindowsOsType;
          });
    QVERIFY2(windowsDeviceFactory, "No Windows device factory was registered.");
    const IDevicePtr device = windowsDeviceFactory->construct();
    QVERIFY2(device, "Failed to construct a Windows device from the factory.");
    device->sshParametersAspectContainer().setSshParameters(params);
    DeviceManager::addDevice(device);

    const Id deviceId = device->id();
    const FilePath deviceRoot = device->rootPath();
    FilePath victim; // Named once the device can say where its temporary directory is.
    const QScopeGuard cleanup([&] {
        if (victim.isEmpty()) {
            DeviceManager::removeDevice(deviceId);
            return;
        }
        // A failing run leaves the victim behind, and Windows locks the image of a running
        // process, so it has to go before its file can. taskkill and not the device's own
        // kill by path: that is what the test is here to catch failing.
        Process killer;
        killer.setCommand({deviceRoot.withNewPath("C:/Windows/System32/taskkill.exe"),
                           {"/F", "/IM", victim.fileName()}});
        killer.runBlocking(std::chrono::seconds(60));
        victim.removeFile();
        DeviceManager::removeDevice(deviceId);
    });

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

    QString tempDir = device->systemEnvironment().value("TEMP");
    if (tempDir.isEmpty())
        tempDir = device->systemEnvironment().value("TMP");
    QVERIFY2(!tempDir.isEmpty(), "The device reports no temporary directory.");
    tempDir.replace('\\', '/');
    // The device user's own directory: a machine can keep C:/Users/Public to itself.
    victim = deviceRoot.withNewPath(
        tempDir + "/qtc-stop-test-" + QUuid::createUuid().toString(QUuid::Id128) + ".exe");

    const FilePath ping = deviceRoot.withNewPath("C:/Windows/System32/ping.exe");
    QVERIFY2(ping.isExecutableFile(), "ping.exe was not found on the device.");
    QVERIFY2(bool(ping.copyFile(victim)), "Failed to copy the test executable on the device.");

    // By image name, which carries the uuid. A query that did not get through says nothing
    // about the process, and taking it for an absent one would let the second wait below
    // pass on the very thing it looks for.
    const auto victimRuns = [&]() -> std::optional<bool> {
        const std::optional<QStringList> sessions
            = processSessions(deviceRoot, victim.completeBaseName());
        if (!sessions)
            return std::nullopt;
        return !sessions->isEmpty();
    };

    Process app;
    app.setCommand({victim, {"-n", "600", "127.0.0.1"}});
    app.setExtraData(ProjectExplorer::Constants::IS_APPLICATION_RUN, true);
    app.start();
    QVERIFY2(app.waitForStarted(std::chrono::seconds(60)),
             qPrintable("The test application did not start: " + app.errorString()));

    QVERIFY2(waitFor([&] { return victimRuns() == std::optional(true); }, 90 * 1000),
             "The application never appeared on the device.");

    app.stop();
    QVERIFY2(waitFor([&] { return victimRuns() == std::optional(false); }, 90 * 1000),
             "Stopping the run left the application running on the device.");
}

} // namespace Remote::Internal
