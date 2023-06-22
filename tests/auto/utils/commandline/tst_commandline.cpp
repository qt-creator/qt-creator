// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <app/app_version.h>

#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/launcherinterface.h>
#include <utils/macroexpander.h>
#include <utils/process.h>
#include <utils/processinterface.h>
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
        Process p;
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

    void testFromUserInput_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QString>("executable");
        QTest::addColumn<QString>("arguments");

        QTest::newRow("empty") << ""
                               << ""
                               << "";
        QTest::newRow("command") << "command"
                                 << "command"
                                 << "";
        QTest::newRow("command-with-args") << "command and args"
                                           << "command"
                                           << "and args";

        if (!HostOsInfo::isWindowsHost()) {
            QTest::newRow("command-with-space-slash") << "command\\ with-space and args"
                                                      << "command with-space"
                                                      << "and args";
            QTest::newRow("command-with-space-single-quote") << "'command with-space' and args"
                                                             << "command with-space"
                                                             << "and args";
        }
        QTest::newRow("command-with-space-double-quote") << "\"command with-space\" and args"
                                                         << "command with-space"
                                                         << "and args";

        QTest::newRow("command-with-space-double-quote-in-name")
            << "\"command\\\"with-quote\" and args"
            << "command\"with-quote"
            << "and args";

        QTest::newRow("inside-space-quoted") << "command\" \"withspace args here"
                                             << "command withspace"
                                             << "args here";
    }

    void testFromUserInput()
    {
        QFETCH(QString, input);
        QFETCH(QString, executable);
        QFETCH(QString, arguments);

        CommandLine cmd = CommandLine::fromUserInput(input);
        QCOMPARE(cmd.executable(), FilePath::fromUserInput(executable));
        QCOMPARE(cmd.arguments(), arguments);
    }

    void testFromInputFails()
    {
        if (HostOsInfo::isWindowsHost())
            QSKIP("The test does not work on Windows.");

        CommandLine cmd = CommandLine::fromUserInput("command\\\\\\ with-space and args");
        QEXPECT_FAIL("",
                     "CommandLine::fromUserInput (and FilePath::fromUserInput) does not handle "
                     "backslashes correctly",
                     Continue);
        QCOMPARE(cmd.executable().fileName(), "command\\ with-space");
        QCOMPARE(cmd.arguments(), "and args");
    }

    void testFromInputWithMacro_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QString>("expectedExecutable");
        QTest::addColumn<QString>("expectedArguments");

        QTest::newRow("simple") << "command %{hello}"
                                << "command"
                                << "hello world";

        QTest::newRow("simple-quoted")
            << "command \"%{hello}\""
            << "command" << (HostOsInfo::isWindowsHost() ? "\"hello world\"" : "'hello world'");

        QTest::newRow("quoted-with-extra")
            << "command \"%{hello}, he said\""
            << "command"
            << (HostOsInfo::isWindowsHost() ? "\"hello world, he said\"" : "'hello world, he said'");

        QTest::newRow("convert-to-quote-win")
            << "command 'this is a test'"
            << "command"
            << (HostOsInfo::isWindowsHost() ? "\"this is a test\"" : "'this is a test'");
    }

    void testFromInputWithMacro()
    {
        QFETCH(QString, input);
        QFETCH(QString, expectedExecutable);
        QFETCH(QString, expectedArguments);

        MacroExpander expander;
        expander.registerVariable("hello", "world var", [] { return "hello world"; });

        CommandLine cmd = CommandLine::fromUserInput(input, &expander);
        QCOMPARE(cmd.executable().toUserOutput(), expectedExecutable);

        if (HostOsInfo::isWindowsHost()) {
            QEXPECT_FAIL("convert-to-quote-win",
                         "Windows should convert single to double quotes",
                         Continue);
        }

        QCOMPARE(cmd.arguments(), expectedArguments);
    }

    void testMultiCommand_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QString>("executable");
        QTest::addColumn<QString>("arguments");

        QTest::newRow("command-and-command") << "command1 && command2"
                                             << "command1"
                                             << "&& command2";

        QTest::newRow("command-and-command-nospace") << "command1&&command2"
                                                     << "command1"
                                                     << "&&command2";

        QTest::newRow("command-semicolon-command") << "command1 ; command2"
                                                   << "command1"
                                                   << "; command2";

        QTest::newRow("command-or-command") << "command1 || command2"
                                            << "command1"
                                            << "|| command2";
    }

    void testMultiCommand()
    {
        QFETCH(QString, input);
        QFETCH(QString, executable);
        QFETCH(QString, arguments);

        CommandLine cmdLine = CommandLine::fromUserInput(input);

        QEXPECT_FAIL(
            "command-and-command-nospace",
            "CommandLine::fromUserInput does not handle multi-command without space correctly",
            Abort);

        QCOMPARE(cmdLine.executable().path(), executable);
        QCOMPARE(cmdLine.arguments(), arguments);
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
