// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest>

#include <utils/environment.h>
#include <utils/filepath.h>
#include <utils/hostosinfo.h>

#include "../wslapi.h"

using namespace Utils;
using namespace Wsl::Internal;

// The translations a WSL device is built on, checked without a WSL device:
// they are pure, so a host that has no WSL at all still covers them.
class WslApiTest : public QObject
{
    Q_OBJECT

private slots:
    void testDecodeWslOutput_data()
    {
        QTest::addColumn<QByteArray>("output");
        QTest::addColumn<QString>("decoded");

        QTest::newRow("empty") << QByteArray() << QString();
        QTest::newRow("utf8") << QByteArray("Ubuntu-24.04\r\n") << QString("Ubuntu-24.04\r\n");
        QTest::newRow("utf16le")
            << QByteArray("U\0b\0u\0n\0t\0u\0\r\0\n\0", 16) << QString("Ubuntu\r\n");
        // A single character per line is the case a "is the second byte zero"
        // test would get wrong for UTF-8.
        QTest::newRow("utf8 single char") << QByteArray("A\n") << QString("A\n");
        QTest::newRow("utf16le non ascii")
            << QByteArray("\xe4\x00\n\x00", 4) << QString::fromUtf8("\xc3\xa4\n");
        // A wsl.exe that ignores WSL_UTF8 and has nothing to say answers
        // with the byte order mark and nothing else, which carries no zero
        // byte for the test to find.
        QTest::newRow("utf16le byte order mark only")
            << QByteArray("\xFF\xFE", 2) << QString();
        // The mark is not whitespace, so one left in the answer survives
        // every trim the callers do.
        QTest::newRow("utf16le byte order mark")
            << QByteArray("\xFF\xFE" "U\0b\0u\0n\0t\0u\0", 14) << QString("Ubuntu");
        QTest::newRow("utf8 byte order mark")
            << QByteArray("\xEF\xBB\xBF" "Ubuntu\r\n") << QString("Ubuntu\r\n");
    }

    void testDecodeWslOutput()
    {
        QFETCH(QByteArray, output);
        QFETCH(QString, decoded);

        QCOMPARE(decodeWslOutput(output), decoded);
    }

    void testParseDistributionList_data()
    {
        QTest::addColumn<QString>("output");
        QTest::addColumn<QStringList>("distributions");

        QTest::newRow("empty") << QString() << QStringList();
        QTest::newRow("one") << "Ubuntu-24.04\r\n" << QStringList{"Ubuntu-24.04"};
        QTest::newRow("several")
            << "Ubuntu-24.04\r\nDebian\r\nkali-linux\r\n"
            << QStringList{"Ubuntu-24.04", "Debian", "kali-linux"};
        QTest::newRow("blank lines") << "Ubuntu\r\n\r\nDebian\r\n"
                                     << QStringList{"Ubuntu", "Debian"};
        QTest::newRow("name with spaces")
            << "My Distro\r\n" << QStringList{"My Distro"};
    }

    void testParseDistributionList()
    {
        QFETCH(QString, output);
        QFETCH(QStringList, distributions);

        QCOMPARE(parseDistributionList(output), distributions);
    }

    void testParseMountRoot_data()
    {
        QTest::addColumn<QString>("wslConf");
        QTest::addColumn<QString>("root");

        QTest::newRow("empty") << QString() << "/mnt";
        QTest::newRow("no automount section")
            << "[network]\nhostname = foo\n" << "/mnt";
        QTest::newRow("no root entry") << "[automount]\nenabled = true\n" << "/mnt";
        QTest::newRow("plain") << "[automount]\nroot = /drives\n" << "/drives";
        QTest::newRow("no spaces") << "[automount]\nroot=/drives\n" << "/drives";
        QTest::newRow("trailing slash") << "[automount]\nroot = /drives/\n" << "/drives";
        QTest::newRow("quoted") << "[automount]\nroot = \"/drives\"\n" << "/drives";
        QTest::newRow("comments")
            << "# a comment\n[automount]\n; another\nroot = /drives\n" << "/drives";
        // An entry outside the section it belongs to is not the one asked for.
        QTest::newRow("root in another section")
            << "[network]\nroot = /wrong\n[automount]\nroot = /drives\n" << "/drives";
        QTest::newRow("only in another section") << "[network]\nroot = /wrong\n" << "/mnt";
        QTest::newRow("case insensitive")
            << "[AutoMount]\nRoot = /drives\n" << "/drives";
        // Mounting at "/" is what WSL itself allows, and the trailing slash
        // strip must not eat it.
        QTest::newRow("root is slash") << "[automount]\nroot = /\n" << "/";
    }

    void testParseMountRoot()
    {
        QFETCH(QString, wslConf);
        QFETCH(QString, root);

        QCOMPARE(parseMountRoot(wslConf), root);
    }

    void testHostDrivePathToWslPath_data()
    {
        QTest::addColumn<QString>("mountRoot");
        QTest::addColumn<QString>("hostPath");
        QTest::addColumn<QString>("wslPath");

        QTest::newRow("drive") << "/mnt" << "C:/src/app" << "/mnt/c/src/app";
        QTest::newRow("lowercase drive") << "/mnt" << "c:/src" << "/mnt/c/src";
        QTest::newRow("drive root") << "/mnt" << "C:/" << "/mnt/c/";
        QTest::newRow("bare drive") << "/mnt" << "C:" << "/mnt/c";
        QTest::newRow("other automount root") << "/drives" << "D:/data" << "/drives/d/data";
        // Nothing to map means the path stays as it is, which is what
        // DeviceFileAccess expects of a mapping that cannot be made.
        QTest::newRow("already a unix path") << "/mnt" << "/home/user" << "/home/user";
        QTest::newRow("empty") << "/mnt" << QString() << QString();
    }

    void testHostDrivePathToWslPath()
    {
        QFETCH(QString, mountRoot);
        QFETCH(QString, hostPath);
        QFETCH(QString, wslPath);

        QCOMPARE(hostDrivePathToWslPath(mountRoot, hostPath), wslPath);
    }

    void testNormalizeMountRoot()
    {
        QCOMPARE(normalizeMountRoot("/mnt"), QString("/mnt"));
        QCOMPARE(normalizeMountRoot("/mnt/"), QString("/mnt"));
        QCOMPARE(normalizeMountRoot("/mnt///"), QString("/mnt"));
        // "Ask the distribution" stays unanswered.
        QCOMPARE(normalizeMountRoot(QString()), QString());
        // The one root that is a separator itself.
        QCOMPARE(normalizeMountRoot("/"), QString("/"));
    }

    // A root typed with a trailing separator names the same directory, and one
    // that is nothing but a separator is what WSL itself allows. Both used to
    // leave the translations with a doubled separator to look for, which
    // matches nothing.
    void testOddMountRoots_data()
    {
        QTest::addColumn<QString>("mountRoot");
        QTest::addColumn<QString>("hostPath");
        QTest::addColumn<QString>("wslPath");
        QTest::addColumn<QString>("windowsPathEntry");

        QTest::newRow("trailing separator")
            << "/mnt/" << "C:/src" << "/mnt/c/src" << "/mnt/c/Windows/system32";
        QTest::newRow("root is slash") << "/" << "C:/src" << "/c/src" << "/c/Windows/system32";
    }

    void testOddMountRoots()
    {
        QFETCH(QString, mountRoot);
        QFETCH(QString, hostPath);
        QFETCH(QString, wslPath);
        QFETCH(QString, windowsPathEntry);

        QCOMPARE(hostDrivePathToWslPath(mountRoot, hostPath), wslPath);
        QCOMPARE(wslPathToHostPath(mountRoot, "Ubuntu", wslPath), hostPath);

        Environment environment{OsTypeLinux};
        environment.set("PATH", "/usr/bin:" + windowsPathEntry);
        QCOMPARE(withoutWindowsPath(environment, mountRoot).value("PATH"), QString("/usr/bin"));
    }

    void testHostPathToWslPath_data()
    {
        QTest::addColumn<QString>("hostPath");
        QTest::addColumn<QString>("wslPath");

        QTest::newRow("drive") << "C:/src/app" << "/mnt/c/src/app";
        QTest::newRow("share") << "//wsl.localhost/Ubuntu/home/user" << "/home/user";
        QTest::newRow("share root") << "//wsl.localhost/Ubuntu" << "/";
        QTest::newRow("dollar share") << "//wsl$/Ubuntu/home/user" << "/home/user";
        // A share of another distribution is not a path this one can see, and
        // neither is one on another device.
        QTest::newRow("other distribution")
            << "//wsl.localhost/Debian/home/user" << QString();
        QTest::newRow("other unc host") << "//server/share/dir" << QString();
        // "Ubuntu-24.04" must not match the prefix of "Ubuntu".
        QTest::newRow("distribution is a prefix")
            << "//wsl.localhost/Ubuntu-24.04/home" << QString();
        QTest::newRow("unix path") << "/home/user" << QString();
    }

    void testHostPathToWslPath()
    {
        QFETCH(QString, hostPath);
        QFETCH(QString, wslPath);

        QCOMPARE(
            hostPathToWslPath("/mnt", "Ubuntu", FilePath::fromUserInput(hostPath)), wslPath);
    }

    void testMappedHostPathToWslPath_data()
    {
        QTest::addColumn<QString>("hostPath");
        QTest::addColumn<QString>("wslPath");

        QTest::newRow("drive") << "C:/src/app" << "/mnt/c/src/app";
        // FilePath::withNewMappedPath() hands the mapping the path without the
        // UNC host, so the distribution's own share arrives as the
        // distribution followed by the path inside it.
        QTest::newRow("decomposed share") << "/Ubuntu/home/user/proj" << "/home/user/proj";
        QTest::newRow("decomposed share root") << "/Ubuntu" << "/";
        // Another distribution's share is nothing this one can map, and
        // "Ubuntu-24.04" must not match the prefix of "Ubuntu".
        QTest::newRow("another distribution") << "/Debian/home/user" << "/Debian/home/user";
        QTest::newRow("distribution is a prefix") << "/Ubuntu-24.04/home" << "/Ubuntu-24.04/home";
        QTest::newRow("path in the distribution") << "/home/user" << "/home/user";
        QTest::newRow("empty") << QString() << QString();
    }

    void testMappedHostPathToWslPath()
    {
        QFETCH(QString, hostPath);
        QFETCH(QString, wslPath);

        QCOMPARE(mappedHostPathToWslPath("/mnt", "Ubuntu", hostPath), wslPath);
    }

    void testWslPathToHostPath_data()
    {
        QTest::addColumn<QString>("wslPath");
        QTest::addColumn<QString>("hostPath");

        QTest::newRow("drive") << "/mnt/c/src/app" << "C:/src/app";
        QTest::newRow("drive root") << "/mnt/c" << "C:/";
        QTest::newRow("uppercase drive") << "/mnt/C/src" << "C:/src";
        // Only a single letter names a drive: "/mnt/cdrom" is an ordinary
        // directory that happens to live under the automount root.
        QTest::newRow("longer name under the root")
            << "/mnt/cdrom/disc" << "//wsl.localhost/Ubuntu/mnt/cdrom/disc";
        QTest::newRow("home") << "/home/user" << "//wsl.localhost/Ubuntu/home/user";
        QTest::newRow("root") << "/" << "//wsl.localhost/Ubuntu/";
    }

    void testWslPathToHostPath()
    {
        QFETCH(QString, wslPath);
        QFETCH(QString, hostPath);

        QCOMPARE(wslPathToHostPath("/mnt", "Ubuntu", wslPath), hostPath);
    }

    void testWslCommandLine_data()
    {
        QTest::addColumn<QString>("userName");
        QTest::addColumn<QString>("workingDirectory");
        QTest::addColumn<bool>("withEnvironment");
        QTest::addColumn<QString>("markerTemplate");
        QTest::addColumn<QString>("arguments");

        QTest::newRow("bare")
            << QString() << QString() << false << QString()
            << "--distribution Ubuntu --exec /bin/sh -c \"exec /bin/ls -l\"";
        QTest::newRow("user")
            << "cristian" << QString() << false << QString()
            << "--distribution Ubuntu --user cristian --exec /bin/sh -c \"exec /bin/ls -l\"";
        QTest::newRow("working directory")
            << QString() << "/home/user" << false << QString()
            << "--distribution Ubuntu --cd /home/user --exec /bin/sh -c \"exec /bin/ls -l\"";
        // Around the shell, so that the environment is the one the shell and
        // everything it starts run with. These are arguments of wsl.exe, so
        // the host quotes them, and wsl.exe hands on what it parses back out.
        QTest::newRow("environment")
            << QString() << QString() << true << QString()
            << "--distribution Ubuntu --exec env \"GREETING=hi\" \"PATH=/usr/bin\" "
               "/bin/sh -c \"exec /bin/ls -l\"";
        // The marker goes out only once the executable is found, so a
        // launcher that cannot see the exec still tells a failed start from a
        // command that ran and failed.
        QTest::newRow("marker")
            << QString() << QString() << false << "__qtc%1qtc__"
            << "--distribution Ubuntu --exec /bin/sh -c "
               "\"type /bin/ls >/dev/null && echo __qtc$$qtc__ && exec /bin/ls -l\"";
        // The test that decides whether the marker goes out has to see the
        // configured PATH, or a command that is only on it never starts.
        QTest::newRow("marker with environment")
            << QString() << QString() << true << "__qtc%1qtc__"
            << "--distribution Ubuntu --exec env \"GREETING=hi\" \"PATH=/usr/bin\" "
               "/bin/sh -c \"type /bin/ls >/dev/null && echo __qtc$$qtc__ && exec /bin/ls -l\"";
    }

    void testWslCommandLine()
    {
        // The script is one argument of wsl.exe, and CommandLine quotes it for
        // the OS of the executable it is for, which is this host. The
        // expectations are written the way Windows quotes, where wsl.exe is.
        if (!HostOsInfo::isWindowsHost())
            QSKIP("The outer quoting is the host's, and wsl.exe only exists on Windows.");

        QFETCH(QString, userName);
        QFETCH(QString, workingDirectory);
        QFETCH(bool, withEnvironment);
        QFETCH(QString, markerTemplate);
        QFETCH(QString, arguments);

        std::optional<Environment> environment;
        if (withEnvironment) {
            environment = Environment(OsTypeLinux);
            environment->set("GREETING", "hi");
            environment->set("PATH", "/usr/bin");
        }

        const CommandLine cmd = wslCommandLine(
            FilePath::fromUserInput("C:/Windows/System32/wsl.exe"),
            "Ubuntu",
            userName,
            workingDirectory,
            CommandLine{FilePath::fromUserInput("/bin/ls"), {"-l"}, OsTypeLinux},
            environment,
            markerTemplate);

        QCOMPARE(cmd.executable(), FilePath::fromUserInput("C:/Windows/System32/wsl.exe"));
        QCOMPARE(cmd.arguments(), arguments);
    }

    // An argument that the inner shell would otherwise take apart has to
    // survive both quotings: the one for wsl.exe and the one for /bin/sh.
    void testWslCommandLineQuoting()
    {
        if (!HostOsInfo::isWindowsHost())
            QSKIP("The outer quoting is the host's, and wsl.exe only exists on Windows.");

        const CommandLine cmd = wslCommandLine(
            FilePath::fromUserInput("C:/Windows/System32/wsl.exe"),
            "Ubuntu",
            {},
            {},
            CommandLine{
                FilePath::fromUserInput("/bin/echo"), {"a b", "$HOME", "it's"}, OsTypeLinux},
            {},
            {});

        QVERIFY(cmd.arguments().contains("'a b'"));
        QVERIFY(cmd.arguments().contains("'$HOME'"));
        QVERIFY(cmd.arguments().contains(R"('it'\''s')"));
        // Unquoted, the inner shell would expand it instead of passing it on.
        QVERIFY(!cmd.arguments().contains(" $HOME "));
    }

    // The executable is quoted for the shell that runs it, which is the
    // distribution's: the host's rules leave a "$" in a path alone, and the
    // inner shell then expands it and starts something else.
    void testWslCommandLineQuotesExecutable()
    {
        if (!HostOsInfo::isWindowsHost())
            QSKIP("The outer quoting is the host's, and wsl.exe only exists on Windows.");

        const CommandLine cmd = wslCommandLine(
            FilePath::fromUserInput("C:/Windows/System32/wsl.exe"),
            "Ubuntu",
            {},
            {},
            CommandLine{FilePath::fromUserInput("/home/u/build-$QTDIR/app"), {}, OsTypeLinux},
            {},
            {});

        QVERIFY(cmd.arguments().contains("'/home/u/build-$QTDIR/app'"));
    }

    void testParseEnvironment()
    {
        const Environment environment = parseEnvironment(
            "PATH=/usr/bin:/bin\r\n"
            "HOME=/home/user\n"
            "EMPTY=\n"
            "WITH_EQUALS=a=b\n"
            "MULTILINE=first\n"
            "second\n");

        QCOMPARE(environment.value("PATH"), QString("/usr/bin:/bin"));
        QCOMPARE(environment.value("HOME"), QString("/home/user"));
        QVERIFY(environment.hasKey("EMPTY"));
        QCOMPARE(environment.value("EMPTY"), QString());
        // Only the first "=" separates; the rest belongs to the value.
        QCOMPARE(environment.value("WITH_EQUALS"), QString("a=b"));
        QCOMPARE(environment.value("MULTILINE"), QString("first\nsecond"));
        // The distribution is Linux, so its names are case sensitive.
        QVERIFY(!environment.hasKey("path"));
    }

    void testWithoutWindowsPath()
    {
        Environment environment{OsTypeLinux};
        environment.set("PATH", "/usr/local/bin:/usr/bin:/mnt/c/Windows/system32:/mnt/d/tools");
        environment.set("HOME", "/home/user");

        const Environment filtered = withoutWindowsPath(environment, "/mnt");

        QCOMPARE(filtered.value("PATH"), QString("/usr/local/bin:/usr/bin"));
        // Only PATH is about reachable programs; the rest is left alone.
        QCOMPARE(filtered.value("HOME"), QString("/home/user"));

        // A directory that merely starts with the same letters is not under
        // the automount root.
        Environment lookalike{OsTypeLinux};
        lookalike.set("PATH", "/mnt/c/tools:/mntx/bin:/mnt:/usr/bin");
        QCOMPARE(
            withoutWindowsPath(lookalike, "/mnt").value("PATH"), QString("/mntx/bin:/mnt:/usr/bin"));

        // Another automount root is respected.
        Environment other{OsTypeLinux};
        other.set("PATH", "/drives/c/tools:/usr/bin");
        QCOMPARE(withoutWindowsPath(other, "/drives").value("PATH"), QString("/usr/bin"));

        // Only a drive is a Windows path. A directory under the automount root
        // that names none is the distribution's own, which is what
        // wslPathToHostPath() makes of it too.
        Environment underRoot{OsTypeLinux};
        underRoot.set("PATH", "/mnt/cdrom/bin:/mnt/c/tools");
        QCOMPARE(withoutWindowsPath(underRoot, "/mnt").value("PATH"), QString("/mnt/cdrom/bin"));
    }

    void testRoundTrip_data()
    {
        QTest::addColumn<QString>("hostPath");

        QTest::newRow("drive") << "C:/src/app";
        QTest::newRow("share") << "//wsl.localhost/Ubuntu/home/user";
    }

    void testRoundTrip()
    {
        QFETCH(QString, hostPath);

        const QString inDistribution
            = hostPathToWslPath("/mnt", "Ubuntu", FilePath::fromUserInput(hostPath));
        QVERIFY(!inDistribution.isEmpty());
        QCOMPARE(
            FilePath::fromUserInput(wslPathToHostPath("/mnt", "Ubuntu", inDistribution)),
            FilePath::fromUserInput(hostPath));
    }
};

QTEST_GUILESS_MAIN(WslApiTest)
#include "tst_wsl_api.moc"
