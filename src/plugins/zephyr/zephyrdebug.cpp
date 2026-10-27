// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "zephyrdebug.h"

#include "westbuildstep.h"
#include "zephyrconstants.h"
#include "zephyrrun.h"
#include "zephyrtr.h"

#include <debugger/debuggerengine.h>
#include <debugger/debuggerruncontrol.h>

#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runcontrol.h>

#include <utils/qtcprocess.h>

#include <QtTaskTree/QBarrier>

#include <QRegularExpression>

#ifdef WITH_TESTS
#include <projectexplorer/devicesupport/devicekitaspects.h>
#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevicefactory.h>
#include <projectexplorer/kitmanager.h>

#include <QLoggingCategory>
#include <QTemporaryDir>
#include <QTest>
#endif

using namespace Debugger;
using namespace ProjectExplorer;
using namespace QtTaskTree;
using namespace Utils;

namespace Zephyr::Internal {

struct RunnerConfig
{
    FilePath elfFile;
    FilePath gdb;
    int gdbPort = 1234;
};

// The list of "  <runner>:" below "args:", one "    - <arg>" per line.
static QStringList runnerArguments(const QString &yaml, const QString &runner)
{
    QStringList result;
    bool inArgs = false;
    bool inRunner = false;
    for (const QString &line : yaml.split('\n')) {
        // A runner without arguments is followed by an empty line.
        if (line.trimmed().isEmpty() || line.startsWith('#'))
            continue;
        if (!line.startsWith(' ')) {
            inArgs = line.startsWith("args:");
            inRunner = false;
        } else if (inArgs && !line.startsWith("   ")) {
            inRunner = line.trimmed() == runner + ':';
        } else if (inRunner) {
            const QString item = line.trimmed();
            if (item.startsWith("- "))
                result.append(item.mid(2).trimmed());
        }
    }
    return result;
}

static std::optional<int> gdbPortFromArguments(const QStringList &args)
{
    static constexpr QLatin1StringView prefix("--gdb-port=");
    for (int i = 0; i < args.size(); ++i) {
        QString value;
        if (args.at(i).startsWith(prefix))
            value = args.at(i).mid(prefix.size());
        else if (args.at(i) == "--gdb-port" && i + 1 < args.size())
            value = args.at(i + 1);
        bool ok = false;
        const int port = value.toInt(&ok);
        if (ok && port > 0)
            return port;
    }
    return {};
}

static RunnerConfig parseRunnerConfig(const QString &yaml, const FilePath &buildDir)
{
    RunnerConfig result;
    result.elfFile = buildDir / "zephyr/zephyr.elf";

    static const QRegularExpression debugRunnerRe(R"(^debug-runner:\s*(\S+))",
                                           QRegularExpression::MultilineOption);
    const QString runner = debugRunnerRe.match(yaml).captured(1);

    static const QRegularExpression elfRe(R"(^\s+elf_file:\s*(\S+))",
                                          QRegularExpression::MultilineOption);

    const QString elfName = elfRe.match(yaml).captured(1);
    if (!elfName.isEmpty())
        result.elfFile = buildDir / "zephyr" / elfName;

    static const QRegularExpression gdbRe(R"(^\s+gdb:\s*(\S.*?)\s*$)",
                                          QRegularExpression::MultilineOption);
    const QString gdb = gdbRe.match(yaml).captured(1);
    if (!gdb.isEmpty())
        result.gdb = buildDir.withNewMappedPath(FilePath::fromUserInput(gdb));

    if (const std::optional<int> port = gdbPortFromArguments(runnerArguments(yaml, runner)))
        result.gdbPort = *port;
    else if (runner == "qemu")
        result.gdbPort = 1234;
    else if (runner == "jlink" || runner == "nrfjprog" || runner == "nrfutil")
        result.gdbPort = 2331;
    else
        result.gdbPort = 3333; // openocd, pyocd, linkserver, ...

    return result;
}

// GdbEngine needs Python, which the SDK's plain gdb is built without.
static bool gdbHasPython(const QString &configuration)
{
    return configuration.contains("--with-python");
}

// A debugger in a container takes "localhost" for the host. The debug server runs on the
// build device, so a debugger there reaches it under 127.0.0.1.
static QString remoteChannel(const FilePath &debugger, const FilePath &buildDevicePath, int port)
{
    const bool nextToServer = debugger.isSameDevice(buildDevicePath);
    return QLatin1String(nextToServer ?"127.0.0.1:" : "localhost:") + QString::number(port);
}

static RunnerConfig readRunnerConfig(const FilePath &buildDir)
{
    const FilePath yamlFile = buildDir / "zephyr/runners.yaml";
    const QString yaml = QString::fromUtf8(yamlFile.fileContents().value_or(QByteArray{}));
    return parseRunnerConfig(yaml, buildDir);
}

static void killQemuIfRunning(const FilePath &buildDir)
{
    const FilePath pidFile = buildDir / "qemu.pid";
    if (!pidFile.isReadableFile())
        return;
    bool ok = false;
    const qint64 pid = pidFile.fileContents().value_or(QByteArray()).trimmed().toLongLong(&ok);
    if (ok && pid > 0) {
        Process kill;
        if (buildDir.osType() == OsTypeWindows) {
            kill.setCommand({buildDir.withNewPath("C:/Windows/System32/taskkill.exe"),
                             {"/PID", QString::number(pid), "/F"}});
        } else {
            kill.setCommand({buildDir.withNewPath("/bin/kill"), {QString::number(pid)}});
        }
        kill.runBlocking();
    }
    pidFile.removeFile();
}

class ZephyrDebugWorkerFactory final : public RunWorkerFactory
{
public:
    ZephyrDebugWorkerFactory()
    {
        setRecipeProducer([](RunControl *runControl) -> Group {
            const FilePath buildDir = runControl->buildDirectory();
            const QString domain = sysbuildDomain(buildDir);
            const FilePath appBuildDir = domain.isEmpty() ? buildDir : buildDir / domain;
            const RunnerConfig rc = readRunnerConfig(appBuildDir);

            DebuggerRunParameters rp = DebuggerRunParameters::fromRunControl(runControl);
            rp.setStartMode(AttachToRemoteServer);
            rp.setCloseMode(KillAtClose);
            rp.setRemoteChannel("localhost:" + QString::number(rc.gdbPort));
            rp.setSymbolFile(rc.elfFile);
            rp.setUseContinueInsteadOfRun(true);
            rp.setSkipDebugServer(true);

            const FilePath workingDir
                = onBuildDevice(runControl->kit(), westWorkingDirectory(runControl->project()));
            const auto modifier = [buildDir, domain, workingDir,
                                   env = runControl->environment()](Process &process) {
                CommandLine cmd{westExecutable(workingDir, env)};
                cmd.addArg("build");
                cmd.addArg("-d");
                cmd.addArg(buildDir.nativePath());
                cmd.addArg("-t");
                cmd.addArg("debugserver");
                if (!domain.isEmpty())
                    cmd.addArgs({"--domain", domain});
                process.setCommand(cmd);
                process.setWorkingDirectory(workingDir);
                process.setDisableUnixTerminal();
            };

            // The SDK's gdb knows the target architecture, a desktop kit's does not.
            const Storage<FilePath> usableGdb;

            const auto onProbeSetup = [gdb = rc.gdb](Process &process) {
                if (gdb.isEmpty())
                    return SetupResult::StopWithSuccess;
                process.setCommand({gdb, {"--configuration"}});
                return SetupResult::Continue;
            };
            const auto onProbeDone = [runControl, usableGdb, gdb = rc.gdb](
                                         const Process &process, DoneWith result) {
                QString message;
                if (result != DoneWith::Success)
                    message = Tr::tr("Cannot run %1, using the kit's debugger instead.");
                else if (!gdbHasPython(process.allOutput()))
                    message = Tr::tr("%1 has no Python support, using the kit's debugger instead.");
                else
                    *usableGdb = gdb;
                if (!message.isEmpty())
                    runControl->postMessage(message.arg(gdb.toUserOutput()), NormalMessageFormat);
                return DoneResult::Success;
            };
            const auto applyUsableGdb = [runControl, usableGdb, workingDir, port = rc.gdbPort](
                                            DebuggerRunParameters &rp) {
                if (!usableGdb->isEmpty())
                    rp.overrideKitDebugger(*usableGdb, GdbEngineType);
                const FilePath gdb = rp.debugger().command.executable();
                if (!gdb.isSameDevice(workingDir)) {
                    runControl->postMessage(
                        Tr::tr("The debug server runs on the build device, but %1 does not. "
                               "It reaches port %2 only if both share a network. Otherwise, "
                               "use a debugger on the build device.")
                            .arg(gdb.toUserOutput())
                            .arg(port),
                        ErrorMessageFormat);
                }
                rp.setRemoteChannel(remoteChannel(gdb, workingDir, port));
            };

            return Group {
                usableGdb,
                onGroupSetup([appBuildDir] {
                    killQemuIfRunning(appBuildDir);
                    return SetupResult::Continue;
                }),
                ProcessTask(onProbeSetup, onProbeDone).withTimeout(std::chrono::seconds(10))
                    || DoneResult::Success,
                When(runControl->processTaskWithModifier(modifier), &Process::started) >> Do {
                    debuggerRecipe(runControl, rp, applyUsableGdb)
                }
            };
        });
        addSupportedRunMode(ProjectExplorer::Constants::DEBUG_RUN_MODE);
        addSupportedRunConfig(Constants::ZEPHYR_RUN_CONFIG_ID);
    }
};

void setupZephyrDebug()
{
    static ZephyrDebugWorkerFactory theDebugWorkerFactory;
}

#ifdef WITH_TESTS

class ZephyrRunnerConfigTest final : public QObject
{
    Q_OBJECT

private slots:
    void testRunnerConfig_data();
    void testRunnerConfig();
    void testGdbHasPython_data();
    void testGdbHasPython();
    void testRemoteChannel_data();
    void testRemoteChannel();
    void testSysbuildDomain();
    void testZephyrTarget();
};

void ZephyrRunnerConfigTest::testRunnerConfig_data()
{
    QTest::addColumn<QString>("yaml");
    QTest::addColumn<QString>("gdb");
    QTest::addColumn<int>("gdbPort");

    QTest::newRow("qemu, no gdb") << R"(runners:
- qemu
debug-runner: qemu
config:
  elf_file: zephyr.elf
  openocd: /usr/bin/openocd
args:
  qemu:
    []
)" << "" << 1234;

    QTest::newRow("jlink default port") << R"(debug-runner: jlink
config:
  elf_file: zephyr.elf
  # Host tools:
  gdb: /sdk/arm-zephyr-eabi/bin/arm-zephyr-eabi-gdb
args:
  jlink:
    - --dt-flash=y
    - --device=nRF52840_xxAA
)" << "/sdk/arm-zephyr-eabi/bin/arm-zephyr-eabi-gdb" << 2331;

    QTest::newRow("port of the debug runner") << R"(debug-runner: openocd
config:
  gdb: /sdk/gdb
args:
  jlink:
    - --gdb-port=2000
  openocd:
    - --cmd-load
    - --gdb-port=4444
  pyocd:
    - --gdb-port=5555
)" << "/sdk/gdb" << 4444;

    QTest::newRow("runner without arguments before") << R"(debug-runner: jlink
config:
  gdb: /sdk/gdb
# Comment
args:
  nrfjprog:
    []

  jlink:
    - --dt-flash=y
    - --gdb-port=2332
)" << "/sdk/gdb" << 2332;
}

void ZephyrRunnerConfigTest::testRunnerConfig()
{
    QFETCH(QString, yaml);
    QFETCH(QString, gdb);
    QFETCH(int, gdbPort);

    const RunnerConfig rc = parseRunnerConfig(yaml, FilePath::fromString("/build"));
    QCOMPARE(rc.gdb, FilePath::fromUserInput(gdb));
    QCOMPARE(rc.gdbPort, gdbPort);
    QCOMPARE(rc.elfFile, FilePath::fromString("/build/zephyr/zephyr.elf"));
}

void ZephyrRunnerConfigTest::testGdbHasPython_data()
{
    QTest::addColumn<QString>("configuration");
    QTest::addColumn<bool>("hasPython");

    QTest::newRow("with") << R"(This GDB was configured as follows:
   configure --host=x86_64-pc-linux-gnu --target=arm-zephyr-eabi
             --with-python=/usr
             --without-debuginfod
)" << true;
    QTest::newRow("without") << R"(This GDB was configured as follows:
   configure --host=x86_64-pc-linux-gnu --target=arm-zephyr-eabi
             --without-python
             --without-debuginfod
)" << false;
    QTest::newRow("failed to start") << "" << false;
}

void ZephyrRunnerConfigTest::testGdbHasPython()
{
    QFETCH(QString, configuration);
    QFETCH(bool, hasPython);

    QCOMPARE(gdbHasPython(configuration), hasPython);
}

void ZephyrRunnerConfigTest::testRemoteChannel_data()
{
    QTest::addColumn<QString>("debugger");
    QTest::addColumn<QString>("buildDevicePath");
    QTest::addColumn<QString>("channel");

    QTest::newRow("local") << "/sdk/gdb" << "/home/me/ws" << "127.0.0.1:3333";
    QTest::newRow("on the build device")
        << "docker://abc/usr/bin/gdb" << "docker://abc/home/me/ws" << "127.0.0.1:3333";
    QTest::newRow("in a container, built on the host")
        << "docker://abc/usr/bin/gdb" << "/home/me/ws" << "localhost:3333";
    QTest::newRow("in another container")
        << "docker://abc/usr/bin/gdb" << "docker://def/home/me/ws" << "localhost:3333";
}

void ZephyrRunnerConfigTest::testRemoteChannel()
{
    QFETCH(QString, debugger);
    QFETCH(QString, buildDevicePath);
    QFETCH(QString, channel);

    QCOMPARE(remoteChannel(FilePath::fromString(debugger),
                           FilePath::fromString(buildDevicePath), 3333),
             channel);
}

void ZephyrRunnerConfigTest::testSysbuildDomain()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const FilePath buildDir = FilePath::fromString(tmp.path());
    QCOMPARE(sysbuildDomain(buildDir), QString());

    QVERIFY_RESULT((buildDir / "domains.yaml").writeFileContents(R"(default: hello_world
build_dir: /build
domains:
  - name: mcuboot
    build_dir: /build/mcuboot
  - name: hello_world
    build_dir: /build/hello_world
flash_order:
  - mcuboot
  - hello_world
)"));
    QCOMPARE(sysbuildDomain(buildDir), QString("hello_world"));
}

void ZephyrRunnerConfigTest::testZephyrTarget()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    const FilePath buildDir = FilePath::fromString(tmp.path());
    QCOMPARE(zephyrTarget(buildDir), ZephyrTarget::Unknown);

    QVERIFY_RESULT((buildDir / "build.ninja").writeFileContents(R"(
build run: phony CMakeFiles/run
build flash: phony zephyr/cmake/flash/CMakeFiles/flash
)"));
    QCOMPARE(zephyrTarget(buildDir), ZephyrTarget::Hardware);

    QVERIFY_RESULT((buildDir / "build.ninja").writeFileContents(R"(
build run: phony zephyr/run
build run_qemu: phony zephyr/run_qemu
)"));
    QCOMPARE(zephyrTarget(buildDir), ZephyrTarget::Emulator);

    QVERIFY_RESULT((buildDir / "zephyr").ensureWritableDir());
    QVERIFY_RESULT((buildDir / "zephyr/runners.yaml").writeFileContents(R"(runners:
- native
flash-runner: native
)"));
    QCOMPARE(zephyrTarget(buildDir), ZephyrTarget::Emulator);

    QVERIFY_RESULT((buildDir / "build.ninja").writeFileContents(R"(
build flash: phony zephyr/cmake/flash/CMakeFiles/flash
build run_nsim: phony zephyr/run_nsim
)"));
    QVERIFY_RESULT((buildDir / "zephyr/runners.yaml").writeFileContents(R"(runners:
- arc-nsim
flash-runner: arc-nsim
debug-runner: arc-nsim
)"));
    QCOMPARE(zephyrTarget(buildDir), ZephyrTarget::Emulator);

    QVERIFY_RESULT((buildDir / "zephyr/runners.yaml").writeFileContents(R"(runners:
- mdb-nsim
flash-runner: mdb-nsim
debug-runner: mdb-nsim
)"));
    QCOMPARE(zephyrTarget(buildDir), ZephyrTarget::Emulator);

    QVERIFY_RESULT((buildDir / "build.ninja").writeFileContents(R"(
build flash: phony zephyr/cmake/flash/CMakeFiles/flash
build run_renode: phony zephyr/run_renode
)"));
    QVERIFY_RESULT((buildDir / "zephyr/runners.yaml").writeFileContents(R"(runners:
- openocd
flash-runner: openocd
debug-runner: openocd
)"));
    QCOMPARE(zephyrTarget(buildDir), ZephyrTarget::Hardware);
}

QObject *createZephyrRunnerConfigTest()
{
    return new ZephyrRunnerConfigTest;
}

// Needs the Docker plugin (-load Docker), a reachable Docker daemon and
// alpine:latest. Skipped otherwise.

class ZephyrDockerTest final : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void init();
    void cleanupTestCase();
    void testMappedPath();
    void testWestOnDevice();
    void testKillQemuOnDevice();

private:
    QString m_skipReason;
    QTemporaryDir m_hostDir;
    IDevicePtr m_device;
    Kit *m_kit = nullptr;
};

static bool setDeviceSetting(const IDevicePtr &device, const Key &key, const QVariant &value)
{
    for (BaseAspect *aspect : device->aspects()) {
        if (aspect->settingsKey() == key) {
            aspect->setVariantValue(value);
            return true;
        }
    }
    return false;
}

// The command bridge cannot read /proc, so ask a shell on the device.
static bool isRunning(const IDevicePtr &device, const QString &pid)
{
    Process check;
    check.setCommand({device->filePath("/bin/sh"),
                      {"-c", "kill -0 \"$0\" && ! grep -q '^State:.*Z' /proc/\"$0\"/status",
                       pid}});
    check.runBlocking();
    return check.result() == ProcessResult::FinishedWithSuccess;
}

void ZephyrDockerTest::initTestCase()
{
    IDeviceFactory *factory = IDeviceFactory::find("DockerDeviceType");
    if (!factory) {
        m_skipReason = "Needs the Docker plugin (-load Docker).";
        return;
    }

    Process imageCheck;
    imageCheck.setCommand({FilePath::fromString("docker").searchInPath(),
                           {"image", "inspect", "alpine:latest", "--format", "{{.Id}}"}});
    imageCheck.runBlocking();
    if (imageCheck.result() != ProcessResult::FinishedWithSuccess) {
        m_skipReason = "Needs a Docker daemon and alpine:latest (docker pull alpine:latest).";
        return;
    }

    QVERIFY(m_hostDir.isValid());
    m_device = factory->construct();
    QVERIFY(m_device);
    QVERIFY(setDeviceSetting(m_device, "DockerDeviceDataRepo", "alpine"));
    QVERIFY(setDeviceSetting(m_device, "DockerDeviceDataTag", "latest"));
    QVERIFY(setDeviceSetting(m_device, "DockerDeviceDataImageId", imageCheck.stdOut().trimmed()));
    QVERIFY(setDeviceSetting(m_device, "DockerDeviceMappedPaths",
                             QStringList(m_hostDir.path() + ":/hostws")));
    DeviceManager::addDevice(m_device);

    m_kit = KitManager::registerKit([this](Kit *k) {
        k->setUnexpandedDisplayName("Zephyr Docker Test");
        BuildDeviceKitAspect::setDeviceId(k, m_device->id());
    });
    QVERIFY(m_kit);
}

// A skip in initTestCase() would also skip the test classes that run after this one.
void ZephyrDockerTest::init()
{
    if (!m_skipReason.isEmpty())
        QSKIP(qPrintable(m_skipReason));
}

void ZephyrDockerTest::cleanupTestCase()
{
    if (m_kit)
        KitManager::deregisterKit(m_kit);
    // Stopping the container ends the command bridge with expected errors.
    QLoggingCategory::setFilterRules("qtc.cmdbridge.client=false");
    if (m_device)
        DeviceManager::removeDevice(m_device->id());
    m_device.reset();
    QLoggingCategory::setFilterRules({});
}

void ZephyrDockerTest::testMappedPath()
{
    const FilePath hostWs = FilePath::fromString(m_hostDir.path()) / "ws";
    QVERIFY_RESULT(hostWs.ensureWritableDir());
    QVERIFY_RESULT((hostWs / "marker").writeFileContents("from the host"));

    const FilePath onDevice = onBuildDevice(m_kit, hostWs);
    QCOMPARE(onDevice, m_device->filePath("/hostws/ws"));
    QCOMPARE((onDevice / "marker").fileContents().value_or(QByteArray()), "from the host");
}

void ZephyrDockerTest::testWestOnDevice()
{
    const FilePath hostDir = FilePath::fromString(m_hostDir.path());
    const FilePath west = hostDir / "bin/west";
    QVERIFY_RESULT(west.parentDir().ensureWritableDir());
    QVERIFY_RESULT(west.writeFileContents("#!/bin/sh\n"));
    QVERIFY_RESULT(west.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));

    Environment env;
    env.set("PATH", "/hostws/bin");
    QCOMPARE(westExecutable(onBuildDevice(m_kit, hostDir), env),
             m_device->filePath("/hostws/bin/west"));
}

void ZephyrDockerTest::testKillQemuOnDevice()
{
    const FilePath hostBuild = FilePath::fromString(m_hostDir.path()) / "build";
    QVERIFY_RESULT(hostBuild.ensureWritableDir());

    Process qemu;
    qemu.setCommand({m_device->filePath("/bin/sh"),
                     {"-c", "sleep 1000 >/dev/null 2>&1 & echo $! > /hostws/build/qemu.pid"}});
    qemu.runBlocking();
    QCOMPARE(qemu.result(), ProcessResult::FinishedWithSuccess);

    const FilePath pidFile = hostBuild / "qemu.pid";
    const QString pid = QString::fromUtf8(pidFile.fileContents().value_or(QByteArray()).trimmed());
    QVERIFY(pid.toInt() > 0);
    QVERIFY(isRunning(m_device, pid));

    killQemuIfRunning(onBuildDevice(m_kit, hostBuild));

    QTRY_VERIFY2(!isRunning(m_device, pid), "the process in the container survived");
    QVERIFY(!pidFile.exists());
}

QObject *createZephyrDockerTest()
{
    return new ZephyrDockerTest;
}

#endif // WITH_TESTS

} // namespace Zephyr::Internal

#ifdef WITH_TESTS
#include "zephyrdebug.moc"
#endif
