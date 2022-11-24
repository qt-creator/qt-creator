// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include <utils/commandline.h>
#include <utils/hostosinfo.h>

#include <QObject>
#include <QtTest>

using namespace Utils;

class tst_CommandLine : public QObject
{
    Q_OBJECT
private:
private slots:
    void initTestCase() {}
    void cleanupTestCase() {}

    void testAnd()
    {
        CommandLine cmd("echo", {"foo"});
        CommandLine cmd2("echo", {"bar", "blizz"});

        cmd.addCommandLineWithAnd(cmd2);

        const QString actual = cmd.toUserOutput();
        const QString wanted = "echo foo && echo bar blizz";

        QCOMPARE(actual, wanted);
    }

    void testAndComplex()
    {
        if (HostOsInfo::isWindowsHost())
            QSKIP("CommandLine does not produce useful escaping on windows.");

        CommandLine cmd("/tmp/space path/\"echo", {"foo", "long with space"});
        CommandLine cmd2("/tmp/space \"path/echo", {"bar\"", "blizz is 'great"});

        cmd.addCommandLineWithAnd(cmd2);

        const QString actual = cmd.toUserOutput();
        const QString wanted =
                "/tmp/space path/\"echo foo 'long with space' && '/tmp/space \"path/echo' "
                "'bar\"' 'blizz is '\\''great'";

        QCOMPARE(actual, wanted);
    }

    void testAndAdd()
    {
        if (HostOsInfo::isWindowsHost())
            QSKIP("CommandLine does not produce useful escaping on windows.");

        CommandLine cmd("/tmp/space path/\"echo", {"foo", "long with space"});
        CommandLine cmd2("/tmp/space \"path/echo", {"bar\"", "blizz is 'great"});
        cmd.addCommandLineWithAnd(cmd2);

        CommandLine shell("bash", {"-c"});
        shell.addCommandLineAsSingleArg(cmd);

        const QString actual = shell.toUserOutput();
        const QString wanted =
                 "bash -c ''\\''/tmp/space path/\"echo'\\'' foo '\\''long with space'\\'' && "
                 "'\\''/tmp/space \"path/echo'\\'' '\\''bar\"'\\'' '\\''blizz is "
                 "'\\''\\'\\'''\\''great'\\'''";

        QCOMPARE(actual, wanted);
    }
};

QTEST_GUILESS_MAIN(tst_CommandLine)

#include "tst_commandline.moc"
