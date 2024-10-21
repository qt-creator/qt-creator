// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/filepath.h>
#include <utils/hostosinfo.h>
#include <utils/macroexpander.h>

#include <QTest>

using namespace Utils;

class tst_expander : public QObject
{
    Q_OBJECT

private slots:
    void expandFilePath_data()
    {
        QTest::addColumn<FilePath>("input");
        QTest::addColumn<FilePath>("expected");
        QTest::addColumn<std::function<void(MacroExpander &)>>("setup");

        std::function<void(MacroExpander &)> empty;
        std::function<void(MacroExpander &)> home = [](MacroExpander &expander) {
            expander.registerVariable("Home", "", [] { return QDir::homePath(); });
        };
        std::function<void(MacroExpander &)> remotehome = [](MacroExpander &expander) {
            expander.registerVariable("Home", "", [] { return "ssh://127.0.0.1/home"; });
        };
        std::function<void(MacroExpander &)> buildfolderwithspecialchars =
            [](MacroExpander &expander) {
                expander.registerVariable("BuildFolder", "", [] {
                    return "C:/Test/Some.Dots.In.File";
                });
            };

        QTest::newRow("empty") << FilePath() << FilePath() << empty;
        QTest::newRow("no expansion") << FilePath("foo") << FilePath("foo") << empty;
        QTest::newRow("no expansion with slash")
            << FilePath("foo/bar") << FilePath("foo/bar") << empty;

        QTest::newRow("home") << FilePath("%{Home}/foo")
                              << FilePath::fromString(QDir::homePath() + "/foo") << home;

        QTest::newRow("remote-home")
            << FilePath("%{Home}/foo")
            << FilePath::fromParts(QString("ssh"), QString("127.0.0.1"), QString("/home/foo"))
            << remotehome;

        QTest::newRow("colon") << FilePath("foo:bar")
                               << FilePath::fromParts({}, {}, QString("foo:bar")) << empty;

        QTest::newRow("win-dots")
            << FilePath("%{BuildFolder}/Debug/my.file.with.dots.elf")
            << FilePath::fromParts(
                   {}, {}, QString("C:/Test/Some.Dots.In.File/Debug/my.file.with.dots.elf"))
            << buildfolderwithspecialchars;
    }

    void expandFilePath()
    {
        QFETCH(FilePath, input);
        QFETCH(FilePath, expected);
        QFETCH(std::function<void(MacroExpander &)>, setup);

        MacroExpander expander;

        if (setup)
            setup(expander);

        QCOMPARE(expander.expand(input), expected);
    }

    void expandString_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QString>("expected");
        QTest::addColumn<std::function<void(MacroExpander &)>>("setup");

        std::function<void(MacroExpander &)> empty;
        std::function<void(MacroExpander &)> user = [](MacroExpander &expander) {
            expander.registerVariable("UserName", "", [] { return "JohnDoe"; });
            expander.registerVariable("FirstName", "", [] { return "John"; });
            expander.registerVariable("LastName", "", [] { return "Doe"; });
            expander.registerVariable("Email", "", [] { return "john.doe@example.com"; });
        };

        QTest::newRow("empty") << QString() << QString() << empty;
        QTest::newRow("no expansion") << QString("foo") << QString("foo") << empty;

        QTest::newRow("simple-expansion")
            << QString("My name is: %{FirstName}") << QString("My name is: John") << user;

        QTest::newRow("multiple-expansions")
            << QString("My name is: %{FirstName} %{LastName} (%{UserName})")
            << QString("My name is: John Doe (JohnDoe)") << user;

        QTest::newRow("email") << QString("My email is: %{Email}")
                               << QString("My email is: john.doe@example.com") << user;
    }

    void expandString()
    {
        QFETCH(QString, input);
        QFETCH(QString, expected);
        QFETCH(std::function<void(MacroExpander &)>, setup);

        MacroExpander expander;

        if (setup)
            setup(expander);

        QCOMPARE(expander.expand(input), expected);
    }

    void subProvider()
    {
        MacroExpander expander;
        expander.registerVariable("MainVar", "", [] { return "MainValue"; });
        expander.registerSubProvider([] {
            static MacroExpander *sub = new MacroExpander;
            sub->registerVariable("SubVar", "", [] { return "SubValue"; });
            return sub;
        });

        QCOMPARE(expander.expand(QString("%{MainVar} %{SubVar}")), QString("MainValue SubValue"));
        QString resolved;
        expander.resolveMacro("MainVar", &resolved);
        QCOMPARE(resolved, QString("MainValue"));

        expander.resolveMacro("SubVar", &resolved);
        QCOMPARE(resolved, QString("SubValue"));
    }

    void expandByteArray_data()
    {
        QTest::addColumn<QByteArray>("input");
        QTest::addColumn<QByteArray>("expected");
        QTest::addColumn<std::function<void(MacroExpander &)>>("setup");

        std::function<void(MacroExpander &)> empty;
        std::function<void(MacroExpander &)> user = [](MacroExpander &expander) {
            expander.registerVariable("UserName", "", [] { return "JohnDoe"; });
            expander.registerVariable("FirstName", "", [] { return "John"; });
            expander.registerVariable("LastName", "", [] { return "Doe"; });
        };

        QTest::newRow("empty") << QByteArray() << QByteArray() << empty;
        QTest::newRow("no expansion")
            << QByteArray("\0\x10\xff", 3) << QByteArray("\0\x10\xff", 3) << empty;

        QTest::newRow("simple-expansion")
            << QByteArray("My name is: %{FirstName}") << QByteArray("My name is: John") << user;

        QTest::newRow("with-zeroes") << QByteArray("My name is: \0%{FirstName}", 25)
                                     << QByteArray("My name is: \0John", 17) << user;
    }
    void expandByteArray()
    {
        QFETCH(QByteArray, input);
        QFETCH(QByteArray, expected);
        QFETCH(std::function<void(MacroExpander &)>, setup);

        MacroExpander expander;

        if (setup)
            setup(expander);

        QCOMPARE(expander.expand(input), expected);
    }

    void expandCommandArgs_data()
    {
        QTest::addColumn<QString>("input");
        QTest::addColumn<QString>("expected");
        QTest::addColumn<std::function<void(MacroExpander &)>>("setup");

        std::function<void(MacroExpander &)> empty;
        std::function<void(MacroExpander &)> file = [](MacroExpander &expander) {
            expander.registerVariable("file", "", [] { return "foo.txt"; });
        };

        std::function<void(MacroExpander &)> withspace = [](MacroExpander &expander) {
            expander.registerVariable("WithSpace", "", [] { return "This has spaces"; });
        };

        QTest::newRow("empty") << QString() << QString() << empty;
        QTest::newRow("no expansion") << QString("foo") << QString("foo") << empty;

        QTest::newRow("simple-expansion")
            << QString("cat %{file}") << QString("cat foo.txt") << file;

        QTest::newRow("with-ticks")
            << QString("echo -n 'foo %{file}'") << QString("echo -n 'foo foo.txt'") << file;

        QTest::newRow("with-ticks-env") << QString("file=%{file} echo -n 'foo \"$file\"'")
                                        << QString("file=foo.txt echo -n 'foo \"$file\"'") << file;

        if (Utils::HostOsInfo::isWindowsHost()) {
            QTest::newRow("with-spaces")
                << QString("echo %{WithSpace}") << QString("echo \"This has spaces\"") << withspace;

            QTest::newRow("with-spaces-manual")
                << QString("echo \"Some: %{WithSpace}\"")
                << QString("echo \"Some: This has spaces\"") << withspace;

            QTest::newRow("with-spaces-nested")
                << QString("cmd /k \"echo %{WithSpace}\"")
                << QString("cmd /k \"echo This has spaces\"") << withspace;
        } else {
            QTest::newRow("with-spaces")
                << QString("echo %{WithSpace}") << QString("echo 'This has spaces'") << withspace;

            QTest::newRow("with-spaces-pre-quoted")
                << QString("echo 'Some: %{WithSpace}'") << QString("echo 'Some: This has spaces'")
                << withspace;

            QTest::newRow("with-spaces-nested")
                << QString("sh -c 'echo %{WithSpace}'") << QString("sh -c 'echo This has spaces'")
                << withspace;

            // Due to security concerns, backslash-escaping an expando is treated as a quoting error
            QTest::newRow("backslash-escaping")
                << QString("echo \\%{file}") << QString("echo \\%{file}") << file;

            QTest::newRow("expando-within-shell-substitution")
                << QString("${VAR:-%{file}}") << QString("${VAR:-foo.txt}") << file;
            QTest::newRow("expando-within-shell-substitution-with-space")
                << QString("echo \"Some: ${VAR:-%{WithSpace}}\"")
                << QString("echo \"Some: ${VAR:-This has spaces}\"") << withspace;
        }
    }

    void expandCommandArgs()
    {
        QFETCH(QString, input);
        QFETCH(QString, expected);
        QFETCH(std::function<void(MacroExpander &)>, setup);

        MacroExpander expander;

        if (setup)
            setup(expander);

        QCOMPARE(expander.expandProcessArgs(input), expected);
    }
};

QTEST_GUILESS_MAIN(tst_expander)

#include "tst_expander.moc"
