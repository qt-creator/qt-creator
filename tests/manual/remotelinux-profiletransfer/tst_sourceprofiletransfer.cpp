// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Standalone manual test for QTCREATORBUG-34734.
//
// It is self-contained: it builds a small Alpine SSH image whose /etc/profile
// runs busybox resize, starts a container, registers it as a LinuxDevice, then
// writes a binary through the device's file access (dd of=..., which sources the
// profile) and checks it arrives intact. Container is built/started/torn down by
// the test. It links the RemoteLinux plugin but does NOT need a running Qt
// Creator: it drives LinuxDevice::fileAccess() directly, without DeviceManager.
//
// Unlike xterm's resize (which needs /dev/tty and bails when there is none),
// busybox resize works on the standard fds: it writes the size query and read()s
// the reply from stdin, so over a no-tty ssh exec it swallows the head of the
// piped file. Without the fix that truncates the deployed binary.

#include <remotelinux/linuxdevice.h>

#include <projectexplorer/devicesupport/devicemanager.h>
#include <projectexplorer/devicesupport/idevice.h>
#include <projectexplorer/devicesupport/sshparameters.h>

#include <utils/appinfo.h>
#include <utils/devicefileaccess.h>
#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/processreaper.h>
#include <utils/qtcprocess.h>
#include <utils/result.h>
#include <utils/temporarydirectory.h>

#include <QApplication>
#include <QDir>
#include <QEventLoop>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

#include <chrono>

using namespace ProjectExplorer;
using namespace RemoteLinux;
using namespace Utils;
using namespace std::chrono_literals;

static const char s_dockerfile[] = R"(FROM alpine:3.20
RUN apk add --no-cache openssh && ssh-keygen -A && mkdir -p /var/empty
RUN ln -sf /bin/busybox /usr/bin/resize
RUN printf '\nstty columns 200 rows 10 2>/dev/null\nresize\n' >> /etc/profile
ARG PUBKEY
RUN mkdir -p /root/.ssh && chmod 700 /root/.ssh \
    && echo "$PUBKEY" > /root/.ssh/authorized_keys && chmod 600 /root/.ssh/authorized_keys \
    && sed -i 's/^#\?PermitRootLogin.*/PermitRootLogin prohibit-password/' /etc/ssh/sshd_config
EXPOSE 22
CMD ["/usr/sbin/sshd", "-D", "-e"]
)";

struct ToolResult { int exitCode = -1; QString out; QString err; };

static ToolResult run(const FilePath &exe, const QStringList &args,
                      std::chrono::seconds timeout = 60s)
{
    Process p;
    p.setCommand({exe, args});
    p.runBlocking(timeout);
    return {p.exitCode(), p.cleanedStdOut(), p.cleanedStdErr()};
}

static QByteArray makeBlob()
{
    QByteArray data("\x7f""ELF", 4);
    for (int i = 0; i < 28188; ++i)
        data.append(char((i * 7 + 3) % 256));
    return data;
}

class tst_SourceProfileTransfer : public QObject
{
    Q_OBJECT

private slots:
    void profileDoesNotEatTransferData();
};

void tst_SourceProfileTransfer::profileDoesNotEatTransferData()
{
#ifdef Q_OS_WIN
    QSKIP("Needs a POSIX host with docker, ssh and ssh-keygen.");
#else
    const Environment env = Environment::systemEnvironment();
    const FilePath docker = env.searchInPath("docker");
    const FilePath sshKeygen = env.searchInPath("ssh-keygen");
    const FilePath ssh = env.searchInPath("ssh");
    if (docker.isEmpty() || sshKeygen.isEmpty() || ssh.isEmpty())
        QSKIP("This test requires docker, ssh and ssh-keygen in PATH.");
    if (run(docker, {"info"}, 15s).exitCode != 0)
        QSKIP("Docker daemon is not reachable.");

    // --- build context: Dockerfile + a throwaway ssh keypair ---
    QTemporaryDir ctx;
    QVERIFY(ctx.isValid());
    const FilePath ctxDir = FilePath::fromString(ctx.path());
    QVERIFY(ctxDir.pathAppended("Dockerfile").writeFileContents(s_dockerfile));
    const FilePath keyFile = ctxDir.pathAppended("id");
    QVERIFY2(run(sshKeygen, {"-t", "ed25519", "-N", "", "-q", "-f", keyFile.path()}).exitCode == 0,
             "ssh-keygen failed");
    const Result<QByteArray> pubKey = keyFile.stringAppended(".pub").fileContents();
    QVERIFY(pubKey.has_value());

    // --- build image ---
    const QString image = "qtc-profile-repro-test";
    const ToolResult build = run(docker, {"build", "--build-arg",
                                          "PUBKEY=" + QString::fromUtf8(*pubKey).trimmed(),
                                          "-t", image, ctx.path()}, 300s);
    QVERIFY2(build.exitCode == 0, qPrintable("docker build failed:\n" + build.err));

    // --- start container on a random host port ---
    const QString name = QString("qtc-profile-repro-test-%1").arg(QCoreApplication::applicationPid());
    run(docker, {"rm", "-f", name}, 30s); // clean any leftover
    const ToolResult start = run(docker, {"run", "-d", "--name", name, "-p", "127.0.0.1::22", image}, 60s);
    QVERIFY2(start.exitCode == 0, qPrintable("docker run failed:\n" + start.err));
    const QScopeGuard containerGuard([&] { run(docker, {"rm", "-f", name}, 30s); });

    const ToolResult portInfo = run(docker, {"port", name, "22"}, 15s);
    QVERIFY2(portInfo.exitCode == 0, qPrintable("docker port failed:\n" + portInfo.err));
    const int port = portInfo.out.trimmed().section(':', -1).toInt();
    QVERIFY2(port > 0, qPrintable("could not parse mapped port from: " + portInfo.out));

    // --- wait for sshd ---
    const QStringList sshBase = {"-i", keyFile.path(), "-p", QString::number(port),
                                 "-o", "StrictHostKeyChecking=no", "-o", "UserKnownHostsFile=/dev/null",
                                 "-o", "BatchMode=yes", "-o", "ConnectTimeout=2", "root@127.0.0.1"};
    bool sshReady = false;
    for (int i = 0; i < 30 && !sshReady; ++i) {
        if (run(ssh, QStringList(sshBase) << "true", 10s).exitCode == 0)
            sshReady = true;
        else
            QTest::qWait(1000);
    }
    QVERIFY2(sshReady, "sshd in the container did not become reachable");

    // LinuxDevice and the FilePath device hooks rely on a DeviceManager instance.
    // Create the singleton directly - its constructor installs the device-file
    // hooks - so this standalone test needs neither the plugin manager nor a
    // running Qt Creator.
    DeviceManager deviceManager;

    SshParameters params;
    params.setHost("127.0.0.1");
    params.setPort(port);
    params.setUserName("root");
    params.setPrivateKeyFile(keyFile);
    params.setAuthenticationType(SshParameters::AuthenticationTypeSpecificKey);
    params.setHostKeyCheckingMode(SshHostKeyCheckingNone);
    params.setTimeout(20);

    LinuxDevice::Ptr device = LinuxDevice::create();
    device->setupId(IDevice::ManuallyAdded);
    device->sshParametersAspectContainer().setSshParameters(params);
    device->sourceProfile.setValue(true); // default; this is what triggers the bug

    DeviceManager::addDevice(device);
    const QScopeGuard deviceGuard([&] { DeviceManager::removeDevice(device->id()); });

    {
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, &loop, [&loop] { loop.exit(1); });
        timer.start(30 * 1000);
        const auto handler = [&loop](const Result<> &res) { loop.exit(res.has_value() ? 0 : 1); };
        device->tryToConnect(Continuation<>(this, handler));
        QVERIFY2(loop.exec() == 0, "Could not connect to the container device.");
    }

    // QTC_DISABLE_CMDBRIDGE is set in main() before the environment snapshot is
    // taken, so the device uses the dd-over-SSH file access (which sources the
    // profile) rather than cmdbridge (which would bypass it).
    QVERIFY(device->fileAccess());

    const QByteArray src = makeBlob();
    const FilePath remote = device->rootPath().withNewPath("/tmp/qtc-bug-34734.bin");
    const QScopeGuard fileGuard([&] { remote.removeFile(); });

    // writeFileContents() pipes the data into "dd of=...", which sources the
    // profile; the profile's busybox resize shares this stdin.
    const Result<qint64> written = remote.writeFileContents(src);
    QVERIFY2(written.has_value(), written ? "" : qPrintable(written.error()));

    const Result<QByteArray> readBack = remote.fileContents();
    QVERIFY2(readBack.has_value(), readBack ? "" : qPrintable(readBack.error()));

    // Pre-fix, busybox resize ate the leading bytes; with the fix it is intact.
    QCOMPARE(readBack->size(), src.size());
    QCOMPARE(*readBack, src);
#endif
}

int main(int argc, char *argv[])
{
    // Force the dd-over-SSH path
    Environment::modifySystemEnvironment({{"QTC_DISABLE_CMDBRIDGE", "1"}});

    // Device/aspect code touches QtGui (QStyleHints), so a QGuiApplication is
    // required; run headless so no display is needed.
    qputenv("QT_QPA_PLATFORM", "offscreen");

    QApplication app(argc, argv);
    // Keep temp paths short: the SSH ControlMaster socket lives under a
    // QTemporaryDir named after the application, and a Unix domain socket path
    // must stay under ~104 chars (especially with macOS's long /var/folders tmp).
    QCoreApplication::setApplicationName("qspt");

    // Required before using Utils::Process / temporary files.
    TemporaryDirectory::setMasterTemporaryDirectory(QDir::tempPath() + "/qspt-XXXXXX");

    // ICore::libexecPath() (used to locate cmdbridge) reads appInfo().libexec,
    // which is empty in a standalone test. Point it at the build's libexec so the
    // device can find/deploy cmdbridge - it then fails on Alpine's musl and falls
    // back to the dd-over-SSH path, exactly like the affected board.
    AppInfo info = appInfo();
    info.libexec = FilePath::fromUserInput(QTC_TEST_LIBEXEC_PATH);
    Utils::Internal::setAppInfo(info);

    tst_SourceProfileTransfer test;
    const int result = QTest::qExec(&test, argc, argv);
    ProcessReaper::deleteAll();
    return result;
}

#include "tst_sourceprofiletransfer.moc"
