// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <app/app_version.h>

#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/launcherinterface.h>
#include <utils/processinterface.h>
#include <utils/qtcprocess.h>
#include <utils/temporarydirectory.h>

#include <QObject>
#include <QtTest>

#include <iostream>

using namespace Utils;

FilePath self;

class tst_CommandLine : public QObject
{
    Q_OBJECT
private:
    Environment testEnv = Environment::systemEnvironment();
    QString newLine;

    QString run(const CommandLine &cmd)
    {
        QtcProcess p;
        p.setCommand(cmd);
        p.setEnvironment(testEnv);
        p.runBlocking();
        return QString::fromUtf8(p.readAllRawStandardOutput());
    }

private slots:
    void initTestCase()
    {
        TemporaryDirectory::setMasterTemporaryDirectory(
            QDir::tempPath() + "/" + Core::Constants::IDE_CASED_ID + "-XXXXXX");

        const QString libExecPath(qApp->applicationDirPath() + '/'
                                  + QLatin1String(TEST_RELATIVE_LIBEXEC_PATH));
        LauncherInterface::setPathToLauncher(libExecPath);

        testEnv.appendOrSet("TEST_ECHO", "1");

        if (HostOsInfo::isWindowsHost())
            newLine = "\r\n";
        else
            newLine = "\n";
    }

    void cleanupTestCase() { Singleton::deleteAll(); }

    void testSpace()
    {
        CommandLine cmd(self, {"With Space"});
        QString expected = "With Space" + newLine;
        QCOMPARE(run(cmd), expected);
    }

    void testQuote()
    {
        QStringList args = {"\"With <\"Quote\"> % ^^ \"", "Hallo ??"};
        CommandLine cmd(self, args);
        QString expected = args.join(newLine) + newLine;
        QCOMPARE(run(cmd), expected);
    }

    void testAnd()
    {
        QStringList args = {"foo", "bar", "baz"};
        CommandLine cmd(self, {args[0]});
        CommandLine cmd2(self, args.sliced(1));

        cmd.addCommandLineWithAnd(cmd2);

        QString expected = args.join(newLine) + newLine;

        QCOMPARE(run(cmd), expected);
    }

    void testAndComplex()
    {
        QStringList args = {"foo", "long with space", "bar\"", "blizz is 'great"};
        CommandLine cmd(self, args.sliced(0, 2));
        CommandLine cmd2(self, args.sliced(2, 2));

        cmd.addCommandLineWithAnd(cmd2);
        QString expected = args.join(newLine) + newLine;
        QString actual = run(cmd);
        QCOMPARE(actual, expected);
    }

    void testAndAdd()
    {
        if (HostOsInfo::isWindowsHost())
            QSKIP("The test does not yet work on Windows.");

        QStringList args = {"foo", "long with space", "bar", "blizz is great"};

        CommandLine cmd(self, args.sliced(0, 2));
        CommandLine cmd2(self, args.sliced(2, 2));

        cmd.addCommandLineWithAnd(cmd2);

        CommandLine shell;
        if (HostOsInfo::isWindowsHost()) {
            shell.setExecutable(FilePath::fromUserInput(qEnvironmentVariable("COMSPEC")));
            shell.addArgs({"/v:off", "/s", "/c"});
        } else {
            shell.setExecutable(FilePath::fromUserInput("/bin/sh"));
            shell.addArgs({"-c"});
        }

        shell.addCommandLineAsSingleArg(cmd);

        QString expected = args.join(newLine) + newLine;
        QString actual = run(shell);
        QCOMPARE(actual, expected);
    }
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    self = FilePath::fromString(argv[0]);

    if (qEnvironmentVariableIsSet("TEST_ECHO")) {
        for (int i = 1; i < argc; ++i) {
            std::cout << argv[i] << std::endl;
        }
        return 0;
    }

    TESTLIB_SELFCOVERAGE_START(tst_CommandLine)
    QT_PREPEND_NAMESPACE(QTest::Internal::callInitMain)<tst_CommandLine>();

    tst_CommandLine tc;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&tc, argc, argv);
}

#include "tst_commandline.moc"
