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
        QTest::addColumn<OsType>("os");
        QTest::addColumn<std::function<void(MacroExpander &)>>("setup");

        std::function<void(MacroExpander &)> empty;
        std::function<void(MacroExpander &)> file = [](MacroExpander &expander) {
            expander.registerVariable("file", "", [] { return "foo.txt"; });
        };

        std::function<void(MacroExpander &)> withspace = [](MacroExpander &expander) {
            expander.registerVariable("WithSpace", "", [] { return "This has spaces"; });
        };

        QTest::newRow("empty") << QString() << QString() << Utils::OsTypeLinux << empty;
        QTest::newRow("no expansion")
            << QString("foo") << QString("foo") << Utils::OsTypeLinux << empty;

        QTest::newRow("simple-expansion")
            << QString("cat %{file}") << QString("cat foo.txt") << Utils::OsTypeLinux << file;

        QTest::newRow("with-ticks")
            << QString("echo -n 'foo %{file}'") << QString("echo -n 'foo foo.txt'")
            << Utils::OsTypeLinux << file;

        QTest::newRow("with-ticks-env")
            << QString("file=%{file} echo -n 'foo \"$file\"'")
            << QString("file=foo.txt echo -n 'foo \"$file\"'") << Utils::OsTypeLinux << file;

        QTest::newRow("with-spaces")
            << QString("echo %{WithSpace}") << QString("echo 'This has spaces'")
            << Utils::OsTypeLinux << withspace;

        QTest::newRow("with-spaces-pre-quoted")
            << QString("echo 'Some: %{WithSpace}'") << QString("echo 'Some: This has spaces'")
            << Utils::OsTypeLinux << withspace;

        QTest::newRow("with-spaces-nested")
            << QString("sh -c 'echo %{WithSpace}'") << QString("sh -c 'echo This has spaces'")
            << Utils::OsTypeLinux << withspace;

        QTest::newRow("expando-within-shell-substitution")
            << QString("${VAR:-%{file}}") << QString("${VAR:-foo.txt}") << Utils::OsTypeLinux
            << file;
        QTest::newRow("expando-within-shell-substitution-with-space")
            << QString("echo \"Some: ${VAR:-%{WithSpace}}\"")
            << QString("echo \"Some: ${VAR:-This has spaces}\"") << Utils::OsTypeLinux << withspace;

        // Windows tests
        QTest::newRow("with-spaces")
            << QString("echo %{WithSpace}") << QString("echo \"This has spaces\"")
            << Utils::OsTypeWindows << withspace;

        QTest::newRow("with-spaces-manual")
            << QString("echo \"Some: %{WithSpace}\"") << QString("echo \"Some: This has spaces\"")
            << Utils::OsTypeWindows << withspace;

        QTest::newRow("with-spaces-nested")
            << QString("cmd /k \"echo %{WithSpace}\"") << QString("cmd /k \"echo This has spaces\"")
            << Utils::OsTypeWindows << withspace;
    }

    void expandCommandArgs()
    {
        QFETCH(QString, input);
        QFETCH(QString, expected);
        QFETCH(OsType, os);
        QFETCH(std::function<void(MacroExpander &)>, setup);

        MacroExpander expander;

        if (setup)
            setup(expander);

        QCOMPARE(expander.expandProcessArgs(input, os), expected);
    }

    void expandProcessArgs_data()
    {
        QTest::addColumn<QString>("in");
        QTest::addColumn<QString>("out");
        QTest::addColumn<OsType>("os");
        QChar sp(QLatin1Char(' '));

        struct Val
        {
            const char *in;
            const char *out;
            OsType os;
        };

        const std::array vals
            = {Val{"plain", 0, OsTypeWindows},
               Val{"%{a}", "hi", OsTypeWindows},
               Val{"%{aa}", "\"hi ho\"", OsTypeWindows},
               Val{"%{b}", "h\\i", OsTypeWindows},
               Val{"%{c}", "\\hi", OsTypeWindows},
               Val{"%{d}", "hi\\", OsTypeWindows},
               Val{"%{ba}", "\"h\\i ho\"", OsTypeWindows},
               Val{"%{ca}", "\"\\hi ho\"", OsTypeWindows},
               Val{"%{da}", "\"hi ho\\\\\"", OsTypeWindows}, // or "\"hi ho\"\\"
               Val{"%{e}", "\"h\"\\^\"\"i\"", OsTypeWindows},
               Val{"%{f}", "\"\"\\^\"\"hi\"", OsTypeWindows},
               Val{"%{g}", "\"hi\"\\^\"\"\"", OsTypeWindows},
               Val{"%{h}", "\"h\\\\\"\\^\"\"i\"", OsTypeWindows},
               Val{"%{i}", "\"\\\\\"\\^\"\"hi\"", OsTypeWindows},
               Val{"%{j}", "\"hi\\\\\"\\^\"\"\"", OsTypeWindows},
               Val{"%{k}", "\"&special;\"", OsTypeWindows},
               Val{"%{x}", "\\", OsTypeWindows},
               Val{"%{y}", "\"\"\\^\"\"\"", OsTypeWindows},
               Val{"%{z}", "\"\"", OsTypeWindows},

               Val{"quoted", 0, OsTypeWindows},
               Val{"\"%{a}\"", "\"hi\"", OsTypeWindows},
               Val{"\"%{aa}\"", "\"hi ho\"", OsTypeWindows},
               Val{"\"%{b}\"", "\"h\\i\"", OsTypeWindows},
               Val{"\"%{c}\"", "\"\\hi\"", OsTypeWindows},
               Val{"\"%{d}\"", "\"hi\\\\\"", OsTypeWindows},
               Val{"\"%{ba}\"", "\"h\\i ho\"", OsTypeWindows},
               Val{"\"%{ca}\"", "\"\\hi ho\"", OsTypeWindows},
               Val{"\"%{da}\"", "\"hi ho\\\\\"", OsTypeWindows},
               Val{"\"%{e}\"", "\"h\"\\^\"\"i\"", OsTypeWindows},
               Val{"\"%{f}\"", "\"\"\\^\"\"hi\"", OsTypeWindows},
               Val{"\"%{g}\"", "\"hi\"\\^\"\"\"", OsTypeWindows},
               Val{"\"%{h}\"", "\"h\\\\\"\\^\"\"i\"", OsTypeWindows},
               Val{"\"%{i}\"", "\"\\\\\"\\^\"\"hi\"", OsTypeWindows},
               Val{"\"%{j}\"", "\"hi\\\\\"\\^\"\"\"", OsTypeWindows},
               Val{"\"%{k}\"", "\"&special;\"", OsTypeWindows},
               Val{"\"%{x}\"", "\"\\\\\"", OsTypeWindows},
               Val{"\"%{y}\"", "\"\"\\^\"\"\"", OsTypeWindows},
               Val{"\"%{z}\"", "\"\"", OsTypeWindows},

               Val{"leading bs", 0, OsTypeWindows},
               Val{"\\%{a}", "\\hi", OsTypeWindows},
               Val{"\\%{aa}", "\\\\\"hi ho\"", OsTypeWindows},
               Val{"\\%{b}", "\\h\\i", OsTypeWindows},
               Val{"\\%{c}", "\\\\hi", OsTypeWindows},
               Val{"\\%{d}", "\\hi\\", OsTypeWindows},
               Val{"\\%{ba}", "\\\\\"h\\i ho\"", OsTypeWindows},
               Val{"\\%{ca}", "\\\\\"\\hi ho\"", OsTypeWindows},
               Val{"\\%{da}", "\\\\\"hi ho\\\\\"", OsTypeWindows},
               Val{"\\%{e}", "\\\\\"h\"\\^\"\"i\"", OsTypeWindows},
               Val{"\\%{f}", "\\\\\"\"\\^\"\"hi\"", OsTypeWindows},
               Val{"\\%{g}", "\\\\\"hi\"\\^\"\"\"", OsTypeWindows},
               Val{"\\%{h}", "\\\\\"h\\\\\"\\^\"\"i\"", OsTypeWindows},
               Val{"\\%{i}", "\\\\\"\\\\\"\\^\"\"hi\"", OsTypeWindows},
               Val{"\\%{j}", "\\\\\"hi\\\\\"\\^\"\"\"", OsTypeWindows},
               Val{"\\%{x}", "\\\\", OsTypeWindows},
               Val{"\\%{y}", "\\\\\"\"\\^\"\"\"", OsTypeWindows},
               Val{"\\%{z}", "\\", OsTypeWindows},

               Val{"trailing bs", 0, OsTypeWindows},
               Val{"%{a}\\", "hi\\", OsTypeWindows},
               Val{"%{aa}\\", "\"hi ho\"\\", OsTypeWindows},
               Val{"%{b}\\", "h\\i\\", OsTypeWindows},
               Val{"%{c}\\", "\\hi\\", OsTypeWindows},
               Val{"%{d}\\", "hi\\\\", OsTypeWindows},
               Val{"%{ba}\\", "\"h\\i ho\"\\", OsTypeWindows},
               Val{"%{ca}\\", "\"\\hi ho\"\\", OsTypeWindows},
               Val{"%{da}\\", "\"hi ho\\\\\"\\", OsTypeWindows},
               Val{"%{e}\\", "\"h\"\\^\"\"i\"\\", OsTypeWindows},
               Val{"%{f}\\", "\"\"\\^\"\"hi\"\\", OsTypeWindows},
               Val{"%{g}\\", "\"hi\"\\^\"\"\"\\", OsTypeWindows},
               Val{"%{h}\\", "\"h\\\\\"\\^\"\"i\"\\", OsTypeWindows},
               Val{"%{i}\\", "\"\\\\\"\\^\"\"hi\"\\", OsTypeWindows},
               Val{"%{j}\\", "\"hi\\\\\"\\^\"\"\"\\", OsTypeWindows},
               Val{"%{x}\\", "\\\\", OsTypeWindows},
               Val{"%{y}\\", "\"\"\\^\"\"\"\\", OsTypeWindows},
               Val{"%{z}\\", "\\", OsTypeWindows},

               Val{"bs-enclosed", 0, OsTypeWindows},
               Val{"\\%{a}\\", "\\hi\\", OsTypeWindows},
               Val{"\\%{aa}\\", "\\\\\"hi ho\"\\", OsTypeWindows},
               Val{"\\%{b}\\", "\\h\\i\\", OsTypeWindows},
               Val{"\\%{c}\\", "\\\\hi\\", OsTypeWindows},
               Val{"\\%{d}\\", "\\hi\\\\", OsTypeWindows},
               Val{"\\%{ba}\\", "\\\\\"h\\i ho\"\\", OsTypeWindows},
               Val{"\\%{ca}\\", "\\\\\"\\hi ho\"\\", OsTypeWindows},
               Val{"\\%{da}\\", "\\\\\"hi ho\\\\\"\\", OsTypeWindows},
               Val{"\\%{e}\\", "\\\\\"h\"\\^\"\"i\"\\", OsTypeWindows},
               Val{"\\%{f}\\", "\\\\\"\"\\^\"\"hi\"\\", OsTypeWindows},
               Val{"\\%{g}\\", "\\\\\"hi\"\\^\"\"\"\\", OsTypeWindows},
               Val{"\\%{h}\\", "\\\\\"h\\\\\"\\^\"\"i\"\\", OsTypeWindows},
               Val{"\\%{i}\\", "\\\\\"\\\\\"\\^\"\"hi\"\\", OsTypeWindows},
               Val{"\\%{j}\\", "\\\\\"hi\\\\\"\\^\"\"\"\\", OsTypeWindows},
               Val{"\\%{x}\\", "\\\\\\", OsTypeWindows},
               Val{"\\%{y}\\", "\\\\\"\"\\^\"\"\"\\", OsTypeWindows},
               Val{"\\%{z}\\", "\\\\", OsTypeWindows},

               Val{"bs-enclosed and trailing literal quote", 0, OsTypeWindows},
               Val{"\\%{a}\\\\\\^\"", "\\hi\\\\\\^\"", OsTypeWindows},
               Val{"\\%{aa}\\\\\\^\"", "\\\\\"hi ho\"\\\\\\^\"", OsTypeWindows},
               Val{"\\%{b}\\\\\\^\"", "\\h\\i\\\\\\^\"", OsTypeWindows},
               Val{"\\%{c}\\\\\\^\"", "\\\\hi\\\\\\^\"", OsTypeWindows},
               Val{"\\%{d}\\\\\\^\"", "\\hi\\\\\\\\\\^\"", OsTypeWindows},
               Val{"\\%{ba}\\\\\\^\"", "\\\\\"h\\i ho\"\\\\\\^\"", OsTypeWindows},
               Val{"\\%{ca}\\\\\\^\"", "\\\\\"\\hi ho\"\\\\\\^\"", OsTypeWindows},
               Val{"\\%{da}\\\\\\^\"", "\\\\\"hi ho\\\\\"\\\\\\^\"", OsTypeWindows},
               Val{"\\%{e}\\\\\\^\"", "\\\\\"h\"\\^\"\"i\"\\\\\\^\"", OsTypeWindows},
               Val{"\\%{f}\\\\\\^\"", "\\\\\"\"\\^\"\"hi\"\\\\\\^\"", OsTypeWindows},
               Val{"\\%{g}\\\\\\^\"", "\\\\\"hi\"\\^\"\"\"\\\\\\^\"", OsTypeWindows},
               Val{"\\%{h}\\\\\\^\"", "\\\\\"h\\\\\"\\^\"\"i\"\\\\\\^\"", OsTypeWindows},
               Val{"\\%{i}\\\\\\^\"", "\\\\\"\\\\\"\\^\"\"hi\"\\\\\\^\"", OsTypeWindows},
               Val{"\\%{j}\\\\\\^\"", "\\\\\"hi\\\\\"\\^\"\"\"\\\\\\^\"", OsTypeWindows},
               Val{"\\%{x}\\\\\\^\"", "\\\\\\\\\\\\\\^\"", OsTypeWindows},
               Val{"\\%{y}\\\\\\^\"", "\\\\\"\"\\^\"\"\"\\\\\\^\"", OsTypeWindows},
               Val{"\\%{z}\\\\\\^\"", "\\\\\\\\\\^\"", OsTypeWindows},

               Val{"bs-enclosed and trailing unclosed quote", 0, OsTypeWindows},
               Val{"\\%{a}\\\\\"", "\\hi\\\\\"", OsTypeWindows},
               Val{"\\%{aa}\\\\\"", "\\\\\"hi ho\"\\\\\"", OsTypeWindows},
               Val{"\\%{b}\\\\\"", "\\h\\i\\\\\"", OsTypeWindows},
               Val{"\\%{c}\\\\\"", "\\\\hi\\\\\"", OsTypeWindows},
               Val{"\\%{d}\\\\\"", "\\hi\\\\\\\\\"", OsTypeWindows},
               Val{"\\%{ba}\\\\\"", "\\\\\"h\\i ho\"\\\\\"", OsTypeWindows},
               Val{"\\%{ca}\\\\\"", "\\\\\"\\hi ho\"\\\\\"", OsTypeWindows},
               Val{"\\%{da}\\\\\"", "\\\\\"hi ho\\\\\"\\\\\"", OsTypeWindows},
               Val{"\\%{e}\\\\\"", "\\\\\"h\"\\^\"\"i\"\\\\\"", OsTypeWindows},
               Val{"\\%{f}\\\\\"", "\\\\\"\"\\^\"\"hi\"\\\\\"", OsTypeWindows},
               Val{"\\%{g}\\\\\"", "\\\\\"hi\"\\^\"\"\"\\\\\"", OsTypeWindows},
               Val{"\\%{h}\\\\\"", "\\\\\"h\\\\\"\\^\"\"i\"\\\\\"", OsTypeWindows},
               Val{"\\%{i}\\\\\"", "\\\\\"\\\\\"\\^\"\"hi\"\\\\\"", OsTypeWindows},
               Val{"\\%{j}\\\\\"", "\\\\\"hi\\\\\"\\^\"\"\"\\\\\"", OsTypeWindows},
               Val{"\\%{x}\\\\\"", "\\\\\\\\\\\\\"", OsTypeWindows},
               Val{"\\%{y}\\\\\"", "\\\\\"\"\\^\"\"\"\\\\\"", OsTypeWindows},
               Val{"\\%{z}\\\\\"", "\\\\\\\\\"", OsTypeWindows},

               Val{"multi-var", 0, OsTypeWindows},
               Val{"%{x}%{y}%{z}", "\\\\\"\"\\^\"\"\"", OsTypeWindows},
               Val{"%{x}%{z}%{y}%{z}", "\\\\\"\"\\^\"\"\"", OsTypeWindows},
               Val{"%{x}%{z}%{y}", "\\\\\"\"\\^\"\"\"", OsTypeWindows},
               Val{"%{x}\\^\"%{z}", "\\\\\\^\"", OsTypeWindows},
               Val{"%{x}%{z}\\^\"%{z}", "\\\\\\^\"", OsTypeWindows},
               Val{"%{x}%{z}\\^\"", "\\\\\\^\"", OsTypeWindows},
               Val{"%{x}\\%{z}", "\\\\", OsTypeWindows},
               Val{"%{x}%{z}\\%{z}", "\\\\", OsTypeWindows},
               Val{"%{x}%{z}\\", "\\\\", OsTypeWindows},
               Val{"%{aa}%{a}", "\"hi hohi\"", OsTypeWindows},
               Val{"%{aa}%{aa}", "\"hi hohi ho\"", OsTypeWindows},
               Val{"%{aa}:%{aa}", "\"hi ho\":\"hi ho\"", OsTypeWindows},
               Val{"hallo ^|%{aa}^|", "hallo ^|\"hi ho\"^|", OsTypeWindows},

               Val{"quoted multi-var", 0, OsTypeWindows},
               Val{"\"%{x}%{y}%{z}\"", "\"\\\\\"\\^\"\"\"", OsTypeWindows},
               Val{"\"%{x}%{z}%{y}%{z}\"", "\"\\\\\"\\^\"\"\"", OsTypeWindows},
               Val{"\"%{x}%{z}%{y}\"", "\"\\\\\"\\^\"\"\"", OsTypeWindows},
               Val{"\"%{x}\"^\"\"%{z}\"", "\"\\\\\"^\"\"\"", OsTypeWindows},
               Val{"\"%{x}%{z}\"^\"\"%{z}\"", "\"\\\\\"^\"\"\"", OsTypeWindows},
               Val{"\"%{x}%{z}\"^\"\"\"", "\"\\\\\"^\"\"\"", OsTypeWindows},
               Val{"\"%{x}\\%{z}\"", "\"\\\\\\\\\"", OsTypeWindows},
               Val{"\"%{x}%{z}\\%{z}\"", "\"\\\\\\\\\"", OsTypeWindows},
               Val{"\"%{x}%{z}\\\\\"", "\"\\\\\\\\\"", OsTypeWindows},
               Val{"\"%{aa}%{a}\"", "\"hi hohi\"", OsTypeWindows},
               Val{"\"%{aa}%{aa}\"", "\"hi hohi ho\"", OsTypeWindows},
               Val{"\"%{aa}:%{aa}\"", "\"hi ho:hi ho\"", OsTypeWindows},

               Val{"plain", 0, OsTypeLinux},
               Val{"%{a}", "hi", OsTypeLinux},
               Val{"%{b}", "'hi ho'", OsTypeLinux},
               Val{"%{c}", "'&special;'", OsTypeLinux},
               Val{"%{d}", "'h\\i'", OsTypeLinux},
               Val{"%{e}", "'h\"i'", OsTypeLinux},
               Val{"%{f}", "'h'\\''i'", OsTypeLinux},
               Val{"%{z}", "''", OsTypeLinux},

               Val{"single-quoted", 0, OsTypeLinux},
               Val{"'%{a}'", "'hi'", OsTypeLinux},
               Val{"'%{b}'", "'hi ho'", OsTypeLinux},
               Val{"'%{c}'", "'&special;'", OsTypeLinux},
               Val{"'%{d}'", "'h\\i'", OsTypeLinux},
               Val{"'%{e}'", "'h\"i'", OsTypeLinux},
               Val{"'%{f}'", "'h'\\''i'", OsTypeLinux},
               Val{"'%{z}'", "''", OsTypeLinux},

               Val{"double-quoted", 0, OsTypeLinux},
               Val{"\"%{a}\"", "\"hi\"", OsTypeLinux},
               Val{"\"%{b}\"", "\"hi ho\"", OsTypeLinux},
               Val{"\"%{c}\"", "\"&special;\"", OsTypeLinux},
               Val{"\"%{d}\"", "\"h\\\\i\"", OsTypeLinux},
               Val{"\"%{e}\"", "\"h\\\"i\"", OsTypeLinux},
               Val{"\"%{f}\"", "\"h'i\"", OsTypeLinux},
               Val{"\"%{z}\"", "\"\"", OsTypeLinux},

               Val{"complex", 0, OsTypeLinux},
               Val{"echo \"$(echo %{a})\"", "echo \"$(echo hi)\"", OsTypeLinux},
               Val{"echo \"$(echo %{b})\"", "echo \"$(echo 'hi ho')\"", OsTypeLinux},
               Val{"echo \"$(echo \"%{a}\")\"", "echo \"$(echo \"hi\")\"", OsTypeLinux},
               // These make no sense shell-wise, but they test expando nesting
               Val{"echo \"%{echo %{a}}\"", "echo \"%{echo hi}\"", OsTypeLinux},
               Val{"echo \"%{echo %{b}}\"", "echo \"%{echo hi ho}\"", OsTypeLinux},
               Val{"echo \"%{echo \"%{a}\"}\"", "echo \"%{echo \"hi\"}\"", OsTypeLinux}};

        const char *title = 0;
        for (const auto &val : vals) {
            if (!val.out) {
                title = val.in;
            } else {
                QString name
                    = QString("%1: %2 (%3)")
                          .arg(title, val.in, val.os == OsTypeWindows ? "windows" : "linux");
                QTest::newRow(name.toLatin1())
                    << QString::fromLatin1(val.in) << QString::fromLatin1(val.out) << val.os;
                QTest::newRow(("padded " + name).toLatin1())
                    << QString(sp + QString::fromLatin1(val.in) + sp)
                    << QString(sp + QString::fromLatin1(val.out) + sp) << val.os;
            }
        }
    }

    void expandProcessArgs()
    {
        QFETCH(QString, in);
        QFETCH(QString, out);
        QFETCH(OsType, os);

        MacroExpander expander;

        if (os == Utils::OsTypeWindows) {
            expander.registerVariable("aa", "", [] { return "hi ho"; });
            expander.registerVariable("b", "", [] { return "h\\i"; });
            expander.registerVariable("c", "", [] { return "\\hi"; });
            expander.registerVariable("d", "", [] { return "hi\\"; });
            expander.registerVariable("ba", "", [] { return "h\\i ho"; });
            expander.registerVariable("ca", "", [] { return "\\hi ho"; });
            expander.registerVariable("da", "", [] { return "hi ho\\"; });
            expander.registerVariable("e", "", [] { return "h\"i"; });
            expander.registerVariable("f", "", [] { return "\"hi"; });
            expander.registerVariable("g", "", [] { return "hi\""; });
            expander.registerVariable("h", "", [] { return "h\\\"i"; });
            expander.registerVariable("i", "", [] { return "\\\"hi"; });
            expander.registerVariable("j", "", [] { return "hi\\\""; });
            expander.registerVariable("k", "", [] { return "&special;"; });
            expander.registerVariable("x", "", [] { return "\\"; });
            expander.registerVariable("y", "", [] { return "\""; });
        } else {
            expander.registerVariable("b", "", [] { return "hi ho"; });
            expander.registerVariable("c", "", [] { return "&special;"; });
            expander.registerVariable("d", "", [] { return "h\\i"; });
            expander.registerVariable("e", "", [] { return "h\"i"; });
            expander.registerVariable("f", "", [] { return "h'i"; });
        }
        expander.registerVariable("a", "", [] { return "hi"; });
        expander.registerVariable("z", "", [] { return ""; });

        QCOMPARE(expander.expandProcessArgs(in, os), out);
    }

    void testProcessArgsFails()
    {
        MacroExpander expander;

        expander.registerVariable("z", "", [] { return ""; });
        expander.registerVariable("file", "", [] { return "foo.txt"; });

        QVERIFY(!expander.expandProcessArgs("\\%{z}%{z}", OsTypeLinux));
        QVERIFY(!expander.expandProcessArgs("echo \\%{file}", OsTypeLinux));

        QVERIFY(!expander.expandProcessArgs("^%{z}%{z}", OsTypeWindows));
    }

    void testMacroExpander_data()
    {
        QTest::addColumn<QString>("in");
        QTest::addColumn<QString>("out");

        struct Val
        {
            const char *in;
            const char *out;
        };

        const std::array vals = {
            Val{"text", "text"},
            Val{"%{a}", "hi"},
            Val{"%%{a}", "%hi"},
            Val{"%%%{a}", "%%hi"},
            Val{"%{b}", "%{b}"},
            Val{"pre%{a}", "prehi"},
            Val{"%{a}post", "hipost"},
            Val{"pre%{a}post", "prehipost"},
            Val{"%{a}%{a}", "hihi"},
            Val{"%{a}text%{a}", "hitexthi"},
            Val{"%{foo}%{a}text%{a}", "ahitexthi"},
            Val{"%{}{a}", "%{a}"},
            Val{"%{}", "%"},
            Val{"test%{}", "test%"},
            Val{"%{}test", "%test"},
            Val{"%{abc", "%{abc"},
            Val{"%{%{a}", "%{hi"},
            Val{"%{%{a}}", "ho"},
            Val{"%{%{a}}}post", "ho}post"},
            Val{"%{hi%{a}}", "bar"},
            Val{"%{hi%{%{foo}}}", "bar"},
            Val{"%{hihi/b/c}", "car"},
            Val{"%{hihi/a/}", "br"}, // empty replacement
            Val{"%{hihi/b}", "bar"}, // incomplete substitution
            Val{"%{hihi/./c}", "car"},
            Val{"%{hihi//./c}", "ccc"},
            Val{"%{hihi/(.)(.)r/\\2\\1c}", "abc"}, // no escape for capture groups
            Val{"%{hihi/b/c/d}", "c/dar"},
            Val{"%{hihi/a/e{\\}e}", "be{}er"},   // escape closing brace
            Val{"%{JS:with \\} inside}", "yay"}, // escape closing brace also in JS:
            Val{"%{JS:literal%\\{}", "hurray"},
            Val{"%{slash/o\\/b/ol's c}", "fool's car"},
            Val{"%{sl\\/sh/(.)(a)(.)/\\2\\1\\3as}", "salsash"}, // escape in variable name
            Val{"%{JS:foo/b/c}", "%{JS:foo/b/c}"}, // No replacement for JS (all considered varName)
            Val{"%{%{a}%{a}/b/c}", "car"},
            Val{"%{nonsense:-sense}", "sense"},
        };

        for (const auto &val : vals)
            QTest::newRow(val.in) << QString::fromLatin1(val.in) << QString::fromLatin1(val.out);
    }

    void testMacroExpander()
    {
        QFETCH(QString, in);
        QFETCH(QString, out);

        MacroExpander expander;
        expander.registerVariable("foo", "", [] { return "a"; });
        expander.registerVariable("a", "", [] { return "hi"; });
        expander.registerVariable("hi", "", [] { return "ho"; });
        expander.registerVariable("hihi", "", [] { return "bar"; });
        expander.registerVariable("slash", "", [] { return "foo/bar"; });
        expander.registerVariable("sl/sh", "", [] { return "slash"; });
        expander.registerVariable("JS:foor", "", [] { return "bar"; });
        expander.registerVariable("JS:with } inside", "", [] { return "yay"; });
        expander.registerVariable("JS:literal%{", "", [] { return "hurray"; });

        QCOMPARE(expander.expand(in), out);
    }
};

QTEST_GUILESS_MAIN(tst_expander)

#include "tst_expander.moc"
