// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "flatpakapi_test.h"

#include "flatpakapi.h"
#include "flatpakdevice.h"
#include "remotelinux_constants.h"

#include <projectexplorer/devicesupport/devicemanager.h>

#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>

#include <QDir>
#include <QFile>
#include <QProcess>
#include <QScopeGuard>
#include <QTemporaryDir>
#include <QTest>

using namespace ProjectExplorer;
using namespace Utils;

namespace Remote::Internal {

void FlatpakApiTest::testHostPathToSandboxPath_data()
{
    QTest::addColumn<QString>("hostPath");
    QTest::addColumn<QString>("sandboxPath");

    QTest::newRow("home") << "/home/u/src" << "/home/u/src";
    QTest::newRow("opt") << "/opt/Qt/6.10.0/gcc_64/bin/qmake"
                         << "/opt/Qt/6.10.0/gcc_64/bin/qmake";
    QTest::newRow("usr") << "/usr/bin/gcc" << "/run/host/usr/bin/gcc";
    QTest::newRow("usr itself") << "/usr" << "/run/host/usr";
    QTest::newRow("etc") << "/etc/os-release" << "/run/host/etc/os-release";
    QTest::newRow("lib64") << "/lib64/ld-linux-x86-64.so.2"
                           << "/run/host/lib64/ld-linux-x86-64.so.2";
    QTest::newRow("usrlocal is not usr") << "/usrlocal/x" << "/usrlocal/x";
    QTest::newRow("tmp") << "/tmp/x" << "";
    QTest::newRow("var tmp") << "/var/tmp/x" << "";
    QTest::newRow("var home") << "/var/home/u/src" << "/var/home/u/src";
    QTest::newRow("var homeless") << "/var/homeless" << "";
    QTest::newRow("app") << "/app/bin/x" << "";
    QTest::newRow("proc") << "/proc/1/maps" << "";
    QTest::newRow("run") << "/run/user/1000" << "";
    QTest::newRow("run media") << "/run/media/u/disk/src" << "/run/media/u/disk/src";
    QTest::newRow("run mediate") << "/run/mediate" << "";
    QTest::newRow("root") << "/" << "";
    QTest::newRow("relative") << "src/main.cpp" << "";
}

void FlatpakApiTest::testHostPathToSandboxPath()
{
    QFETCH(QString, hostPath);
    QFETCH(QString, sandboxPath);
    QCOMPARE(hostPathToSandboxPath(hostPath), sandboxPath);
}

void FlatpakApiTest::testSandboxPathToHostPath_data()
{
    QTest::addColumn<QString>("sandboxPath");
    QTest::addColumn<QString>("hostPath");

    QTest::newRow("home") << "/home/u/src" << "/home/u/src";
    QTest::newRow("run host usr") << "/run/host/usr/bin/gcc" << "/usr/bin/gcc";
    QTest::newRow("run host etc") << "/run/host/etc/os-release" << "/etc/os-release";
    QTest::newRow("run host other") << "/run/host/fonts" << "";
    QTest::newRow("run host itself") << "/run/host" << "";
    // The runtime's, not the host's.
    QTest::newRow("usr") << "/usr/bin/gcc" << "";
    QTest::newRow("app") << "/app/libexec/qtcreator/cmdbridge" << "";
    QTest::newRow("tmp") << "/tmp/x" << "";
    QTest::newRow("var home") << "/var/home/u" << "/var/home/u";
    QTest::newRow("var data") << "/var/data/x" << "";
    QTest::newRow("run media") << "/run/media/u/disk" << "/run/media/u/disk";
    QTest::newRow("root") << "/" << "";
}

void FlatpakApiTest::testSandboxPathToHostPath()
{
    QFETCH(QString, sandboxPath);
    QFETCH(QString, hostPath);
    QCOMPARE(sandboxPathToHostPath(sandboxPath), hostPath);
}

void FlatpakApiTest::testRoundTrip_data()
{
    QTest::addColumn<QString>("hostPath");

    QTest::newRow("home") << "/home/u/src/main.cpp";
    QTest::newRow("usr") << "/usr/include/stdio.h";
    QTest::newRow("var home") << "/var/home/u";
}

void FlatpakApiTest::testRoundTrip()
{
    QFETCH(QString, hostPath);
    QCOMPARE(sandboxPathToHostPath(hostPathToSandboxPath(hostPath)), hostPath);
}

void FlatpakApiTest::testHostCommandLine_data()
{
    QTest::addColumn<QString>("workingDirectory");
    QTest::addColumn<bool>("withEnvironment");
    QTest::addColumn<QString>("markerTemplate");
    QTest::addColumn<QString>("arguments");

    QTest::newRow("plain") << QString() << false << QString()
                           << "--host /bin/sh -c 'exec /bin/ls -l'";
    QTest::newRow("directory") << "/home/u" << false << QString()
                               << "--host --directory=/home/u /bin/sh -c 'exec /bin/ls -l'";
    QTest::newRow("environment") << QString() << true << QString()
                                 << "--host env PATH=/usr/bin /bin/sh -c 'exec /bin/ls -l'";
    QTest::newRow("marker")
        << QString() << false << "__PID_%1__"
        << "--host /bin/sh -c 'type /bin/ls >/dev/null && echo __PID_$$__ && exec /bin/ls -l'";
}

void FlatpakApiTest::testHostCommandLine()
{
    if (!HostOsInfo::isAnyUnixHost())
        QSKIP("The outer quoting is the host's, and Flatpak only exists on Linux.");

    QFETCH(QString, workingDirectory);
    QFETCH(bool, withEnvironment);
    QFETCH(QString, markerTemplate);
    QFETCH(QString, arguments);

    std::optional<Environment> environment;
    if (withEnvironment) {
        environment = Environment(OsTypeLinux);
        environment->set("PATH", "/usr/bin");
    }

    const CommandLine cmd = hostCommandLine(
        FilePath::fromString("/usr/bin/flatpak-spawn"),
        workingDirectory,
        CommandLine{FilePath::fromString("/bin/ls"), {"-l"}, OsTypeLinux},
        environment,
        markerTemplate);

    QCOMPARE(cmd.executable(), FilePath::fromString("/usr/bin/flatpak-spawn"));
    QCOMPARE(cmd.arguments(), arguments);
}

// Runs the command line for real, with a script that does what
// flatpak-spawn does with the options used here, so that an argument the
// inner shell would take apart is seen to survive both quotings.
void FlatpakApiTest::testHostCommandLineRuns()
{
    if (!HostOsInfo::isLinuxHost())
        QSKIP("Flatpak only exists on Linux.");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString launcherPath = dir.filePath("fake-flatpak-spawn");
    QFile launcher(launcherPath);
    QVERIFY(launcher.open(QIODevice::WriteOnly));
    launcher.write(
        "#!/bin/sh\n"
        "[ \"$1\" = --host ] || exit 99\n"
        "shift\n"
        "export FOO=x\n"
        "case \"$1\" in --directory=*) cd \"${1#--directory=}\" || exit 98; shift;; esac\n"
        "exec \"$@\"\n");
    launcher.close();
    QVERIFY(launcher.setPermissions(launcher.permissions() | QFileDevice::ExeOwner));

    Environment environment(OsTypeLinux);
    environment.set("PATH", "/usr/bin:/bin");
    environment.set("HOME", "/nowhere");

    const CommandLine cmd = hostCommandLine(
        FilePath::fromString(launcherPath),
        dir.path(),
        CommandLine{
            FilePath::fromString("/bin/sh"),
            {"-c",
             "pwd; printf '%s|' \"$@\"; printf '\\n%s' \"${FOO-unset}\"",
             "sh",
             "a b",
             "$HOME",
             "it's"},
            OsTypeLinux},
        environment,
        "MARK %1",
        {"FOO"});

    QProcess process;
    process.start(cmd.executable().path(), cmd.splitArguments());
    QVERIFY(process.waitForStarted());
    // Every step execs, so the id the marker reports is the one started.
    const qint64 pid = process.processId();
    QVERIFY(process.waitForFinished());
    QCOMPARE(process.exitCode(), 0);

    const QStringList lines = QString::fromUtf8(process.readAllStandardOutput()).split('\n');
    QCOMPARE(lines.size(), 4);
    QVERIFY(lines.at(0).startsWith("MARK "));
    bool isNumber = false;
    QCOMPARE(lines.at(0).mid(5).toLongLong(&isNumber), pid);
    QVERIFY(isNumber);
    QCOMPARE(lines.at(1), QDir(dir.path()).canonicalPath());
    QCOMPARE(lines.at(2), QString("a b|$HOME|it's|"));
    QCOMPARE(lines.at(3), QString("unset"));
}

void FlatpakApiTest::testDetectHostEnvironment()
{
    if (!HostOsInfo::isLinuxHost())
        QSKIP("Flatpak only exists on Linux.");

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    const QString launcherPath = dir.filePath("fake-flatpak-spawn");
    QFile launcher(launcherPath);
    QVERIFY(launcher.open(QIODevice::WriteOnly));
    launcher.write(
        "#!/bin/sh\n"
        "[ \"$1\" = --host ] || exit 99\n"
        "shift\n"
        "export MULTI=\"$(printf 'first\\n    second=2\\nthird')\"\n"
        "exec \"$@\"\n");
    launcher.close();
    QVERIFY(launcher.setPermissions(launcher.permissions() | QFileDevice::ExeOwner));

    const Result<Environment> environment = detectHostEnvironment(
        FilePath::fromString(launcherPath));
    QVERIFY(environment);
    QCOMPARE(environment->value("MULTI"), QString("first\n    second=2\nthird"));
    QVERIFY(!environment->hasKey("    second"));
}

void FlatpakApiTest::testHandlesFile()
{
    const FlatpakDevice::Ptr device = FlatpakDevice::create();
    device->setupId(IDevice::ManuallyAdded, Id("flatpak.test.handlesFile"));
    DeviceManager::addDevice(device);
    const QScopeGuard cleanup([&] { DeviceManager::removeDevice(device->id()); });

    const FilePath hostFile = FilePath::fromString("flatpak://host/usr/bin/gcc");
    QVERIFY(device->handlesFile(hostFile));
    const IDevice::ConstPtr found = DeviceManager::deviceForPath(hostFile);
    QVERIFY(found);
    QCOMPARE(found->type(), Id(Constants::FlatpakDeviceType));

    const FilePath otherHost = FilePath::fromString("flatpak://other/usr/bin/gcc");
    QVERIFY(!device->handlesFile(otherHost));
    QVERIFY(!DeviceManager::deviceForPath(otherHost));
}

void FlatpakApiTest::testLocalSource()
{
    if (isInsideFlatpakSandbox())
        QSKIP("Whether the host is mounted depends on the permissions of the sandbox.");

    const FlatpakDevice::Ptr device = FlatpakDevice::create();
    const FilePath root = device->rootPath();

    // Mapped below /run/host, which only exists in a sandbox.
    QVERIFY(!device->localSource(root.withNewPath("/usr/include/stdio.h")));

    // Shared at the same path, so it is the path itself.
    const QString home = QDir::homePath() + "/file";
    if (hostPathToSandboxPath(home) != home)
        QSKIP("The home directory is not shared at its own path.");
    const Result<FilePath> source = device->localSource(root.withNewPath(home));
    QVERIFY(source);
    QCOMPARE(source->path(), home);
}

} // namespace Remote::Internal
