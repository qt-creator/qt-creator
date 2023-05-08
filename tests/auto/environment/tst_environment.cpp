// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/environment.h>

#include <QtTest>

using namespace Utils;

Q_DECLARE_METATYPE(Utils::OsType)

class tst_Environment : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void environment_data();
    void environment();

    void environmentSetup_data();
    void environmentSetup();

    void environmentSetWindows();
    void environmentSetWindowsFuzzy();
    void environmentSetUnix();

    void environmentSetNewWindows();
    void environmentSetNewUnix();

    void environmentUnsetWindows();
    void environmentUnsetWindowsFuzzy();
    void environmentUnsetUnix();

    void environmentUnsetUnknownWindows();
    void environmentUnsetUnknownUnix();

    void expansion_data();
    void expansion();

    void incrementalChanges();

    void pathChanges_data();
    void pathChanges();

    void find_data();
    void find();

private:
    Environment env;
};

void tst_Environment::initTestCase()
{
    env.set("word", "hi");
}

void tst_Environment::environment_data()
{
    QTest::addColumn<QString>("in");
    QTest::addColumn<QString>("out");

    static const struct {
        const char * const in;
        const char * const out;
    } vals[] = {
#ifdef Q_OS_WIN
        {"", ""},
        {"hi", "hi"},
        {"hi%", "hi%"},
        {"hi% ho", "hi% ho"},
        {"hi%here ho", "hi%here ho"},
        {"hi%here%ho", "hi%here%ho"},
        {"hi%word%", "hihi"},
        {"hi%foo%word%", "hi%foohi"},
        {"%word%word%ho", "hiword%ho"},
        {"hi%word%x%word%ho", "hihixhiho"},
        {"hi%word%xx%word%ho", "hihixxhiho"},
        {"hi%word%%word%ho", "hihihiho"},
#else
        {"", ""},
        {"hi", "hi"},
        {"hi$", "hi$"},
        {"hi${", "hi${"},
        {"hi${word", "hi${word"},
        {"hi${word}", "hihi"},
        {"hi${word}ho", "hihiho"},
        {"hi$wo", "hi$wo"},
        {"hi$word", "hihi"},
        {"hi$word ho", "hihi ho"},
        {"$word", "hi"},
        {"hi${word}$word", "hihihi"},
#endif
    };

    for (unsigned i = 0; i < sizeof(vals)/sizeof(vals[0]); i++)
        QTest::newRow(vals[i].in) << QString::fromLatin1(vals[i].in)
                                  << QString::fromLatin1(vals[i].out);
}

void tst_Environment::environment()
{
    QFETCH(QString, in);
    QFETCH(QString, out);

    QCOMPARE(env.expandVariables(in), out);
}

void tst_Environment::environmentSetup_data()
{
    QTest::addColumn<int>("osType");
    QTest::addColumn<QStringList>("in");
    QTest::addColumn<QStringList>("out");

    QTest::newRow("EmptyWin")
            << static_cast<int>(Utils::OsTypeWindows) << QStringList() << QStringList();
    QTest::newRow("EmptyLinux")
            << static_cast<int>(Utils::OsTypeLinux) << QStringList() << QStringList();

    QTest::newRow("SimpleWin")
            << static_cast<int>(Utils::OsTypeWindows) << QStringList({"Foo=bar"}) << QStringList({"Foo=bar"});
    QTest::newRow("EmptyLinux")
            << static_cast<int>(Utils::OsTypeLinux) << QStringList({"Foo=bar"}) << QStringList({"Foo=bar"});

    QTest::newRow("MultiWin")
            << static_cast<int>(Utils::OsTypeWindows)
            << QStringList({"Foo=bar", "Hi=HO"}) << QStringList({"Foo=bar", "Hi=HO"});
    QTest::newRow("MultiLinux")
            << static_cast<int>(Utils::OsTypeLinux)
            << QStringList({"Foo=bar", "Hi=HO"}) << QStringList({"Foo=bar", "Hi=HO"});

    QTest::newRow("DuplicateWin")
            << static_cast<int>(Utils::OsTypeWindows)
            << QStringList({"Foo=bar", "FOO=HO"}) << QStringList({"Foo=HO"});
    QTest::newRow("DuplicateLinux")
            << static_cast<int>(Utils::OsTypeLinux)
            << QStringList({"Foo=bar", "FOO=HO"}) << QStringList({"FOO=HO", "Foo=bar"});
}

void tst_Environment::environmentSetup()
{
    QFETCH(int, osType);
    QFETCH(QStringList, in);
    QFETCH(QStringList, out);

    Environment env(in, static_cast<Utils::OsType>(osType));

    QCOMPARE(env.toStringList(), out);
}

void tst_Environment::environmentSetWindows()
{
    Environment env(QStringList({"Foo=bar", "Hi=HO"}), Utils::OsTypeWindows);

    env.set("Foo", "baz");

    QCOMPARE(env.toStringList(), QStringList({"Foo=baz", "Hi=HO"}));
}

void tst_Environment::environmentSetWindowsFuzzy()
{
    Environment env(QStringList({"Foo=bar", "Hi=HO"}), Utils::OsTypeWindows);

    env.set("FOO", "baz");

    QCOMPARE(env.toStringList(), QStringList({"Foo=baz", "Hi=HO"}));
}

void tst_Environment::environmentSetUnix()
{
    Environment env(QStringList({"Foo=bar", "Hi=HO"}), Utils::OsTypeLinux);

    env.set("Foo", "baz");

    QCOMPARE(env.toStringList(), QStringList({"Foo=baz", "Hi=HO"}));
}

void tst_Environment::environmentSetNewWindows()
{
    Environment env(QStringList({"Foo=bar", "Hi=HO"}), Utils::OsTypeWindows);

    env.set("bar", "baz");

    QCOMPARE(env.toStringList(), QStringList({"bar=baz", "Foo=bar", "Hi=HO"}));
}

void tst_Environment::environmentSetNewUnix()
{
    Environment env(QStringList({"Foo=bar", "Hi=HO"}), Utils::OsTypeLinux);

    env.set("bar", "baz");

    QCOMPARE(env.toStringList(), QStringList({"Foo=bar", "Hi=HO", "bar=baz"}));
}

void tst_Environment::environmentUnsetWindows()
{
    Environment env(QStringList({"Foo=bar", "Hi=HO"}), Utils::OsTypeWindows);

    env.unset("Foo");

    QCOMPARE(env.toStringList(), QStringList({"Hi=HO"}));
}

void tst_Environment::environmentUnsetWindowsFuzzy()
{
    Environment env(QStringList({"Foo=bar", "Hi=HO"}), Utils::OsTypeWindows);

    env.unset("FOO");

    QCOMPARE(env.toStringList(), QStringList({"Hi=HO"}));
}

void tst_Environment::environmentUnsetUnix()
{
    Environment env(QStringList({"Foo=bar", "Hi=HO"}), Utils::OsTypeLinux);

    env.unset("Foo");

    QCOMPARE(env.toStringList(), QStringList({"Hi=HO"}));
}

void tst_Environment::environmentUnsetUnknownWindows()
{
    Environment env(QStringList({"Foo=bar", "Hi=HO"}), Utils::OsTypeWindows);

    env.unset("baz");

    QCOMPARE(env.toStringList(), QStringList({"Foo=bar", "Hi=HO"}));
}

void tst_Environment::environmentUnsetUnknownUnix()
{
    Environment env(QStringList({"Foo=bar", "Hi=HO"}), Utils::OsTypeLinux);

    env.unset("baz");

    QCOMPARE(env.toStringList(), QStringList({"Foo=bar", "Hi=HO"}));
}

void tst_Environment::expansion_data()
{
    QTest::addColumn<Utils::OsType>("osType");
    QTest::addColumn<QString>("eu");
    QTest::addColumn<QString>("ew");

    QTest::newRow("win") << Utils::OsTypeWindows << "${v}" << "blubb";
    QTest::newRow("lin") << Utils::OsTypeLinux << "blubb" << "%v%";
}

void tst_Environment::expansion()
{
    QFETCH(Utils::OsType, osType);
    QFETCH(QString, eu);
    QFETCH(QString, ew);

    const Environment env(QStringList{"eu=${v}", "ew=%v%", "v=blubb"}, osType);
    QCOMPARE(env.expandedValueForKey("eu"), eu);
    QCOMPARE(env.expandedValueForKey("ew"), ew);
}

void tst_Environment::incrementalChanges()
{
    const Environment origEnv({{"VAR1", "VALUE1"}, {"VAR2", "VALUE2"}, {"PATH", "/usr/bin"}});
    const NameValueItems changes({
        {"VAR1", QString(), NameValueItem::Unset},
        {"VAR2", "VALUE2", NameValueItem::SetDisabled},
        {"PATH", "/usr/local/bin", NameValueItem::Append},
        {"PATH", "/tmp", NameValueItem::Prepend}});

    // Check values after change application.
    Environment newEnv = origEnv;
    newEnv.modify(changes);
    QVERIFY(!newEnv.hasKey("VAR1"));
    QCOMPARE(newEnv.value("VAR2"), QString());
    Environment::FindResult res = newEnv.find("VAR2");
    QCOMPARE(res->value, "VALUE2");
    QVERIFY(!res->enabled);
    const QChar sep = HostOsInfo::pathListSeparator();
    QCOMPARE(newEnv.value("PATH"),
             QString("/tmp").append(sep).append("/usr/bin").append(sep).append("/usr/local/bin"));

    // Check apply/diff round-trips.
    const NameValueItems diff = origEnv.diff(newEnv);
    const NameValueItems reverseDiff = newEnv.diff(origEnv);
    Environment newEnv2 = origEnv;
    newEnv2.modify(diff);
    QCOMPARE(newEnv, newEnv2);
    newEnv2.modify(reverseDiff);
    QCOMPARE(newEnv2, origEnv);

    // Check conversion round-trips.
    QCOMPARE(NameValueItem::fromStringList(NameValueItem::toStringList(changes)), changes);
    QCOMPARE(NameValueItem::fromStringList(NameValueItem::toStringList(diff)), diff);
    QCOMPARE(NameValueItem::fromStringList(NameValueItem::toStringList(reverseDiff)), reverseDiff);
    QCOMPARE(NameValueItem::itemsFromVariantList(NameValueItem::toVariantList(changes)), changes);
    QCOMPARE(NameValueItem::itemsFromVariantList(NameValueItem::toVariantList(diff)), diff);
    QCOMPARE(NameValueItem::itemsFromVariantList(NameValueItem::toVariantList(reverseDiff)),
             reverseDiff);
}

void tst_Environment::pathChanges_data()
{
    const Environment origEnvLinux({"PATH=/bin:/usr/bin", "VAR=VALUE"}, OsTypeLinux);
    const Environment origEnvWin({"PATH=C:\\Windows\\System32;D:\\gnu\\bin", "VAR=VALUE"}, OsTypeWindows);

    QTest::addColumn<Environment>("environment");
    QTest::addColumn<bool>("prepend"); // if false => append
    QTest::addColumn<QString>("variable");
    QTest::addColumn<QString>("value");
    QTest::addColumn<Environment>("expected");

    QTest::newRow("appendOrSetPath existingLeading Unix")
        << origEnvLinux << false << "PATH" << "/bin"
        << Environment({"PATH=/bin:/usr/bin:/bin", "VAR=VALUE"}, OsTypeLinux);
    QTest::newRow("appendOrSetPath existingLeading Win")
        << origEnvWin << false << "PATH" << "C:\\Windows\\System32"
        << Environment({"PATH=C:\\Windows\\System32;D:\\gnu\\bin;C:\\Windows\\System32",
                        "VAR=VALUE"}, OsTypeWindows);
    QTest::newRow("appendOrSetPath existingTrailing Unix")
        << origEnvLinux << false << "PATH" << "/usr/bin"
        << Environment({"PATH=/bin:/usr/bin", "VAR=VALUE"}, OsTypeLinux);
    QTest::newRow("appendOrSetPath existingTrailing Win")
        << origEnvWin << false << "PATH" << "D:\\gnu\\bin"
        << Environment({"PATH=C:\\Windows\\System32;D:\\gnu\\bin",
                        "VAR=VALUE"}, OsTypeWindows);
    QTest::newRow("prependOrSetPath existingLeading Unix")
        << origEnvLinux << true << "PATH" << "/bin"
        << Environment({"PATH=/bin:/usr/bin", "VAR=VALUE"}, OsTypeLinux);
    QTest::newRow("prependOrSetPath existingLeading Win")
        << origEnvWin << true << "PATH" << "C:\\Windows\\System32"
        << Environment({"PATH=C:\\Windows\\System32;D:\\gnu\\bin",
                        "VAR=VALUE"}, OsTypeWindows);
    QTest::newRow("prependOrSetPath existingTrailing Unix")
        << origEnvLinux << true << "PATH" << "/usr/bin"
        << Environment({"PATH=/usr/bin:/bin:/usr/bin", "VAR=VALUE"}, OsTypeLinux);
    QTest::newRow("prependOrSetPath existingTrailing Win")
        << origEnvWin << true << "PATH" << "D:\\gnu\\bin"
        << Environment({"PATH=D:\\gnu\\bin;C:\\Windows\\System32;D:\\gnu\\bin",
                        "VAR=VALUE"}, OsTypeWindows);

    QTest::newRow("appendOrSetPath non-existing Unix")
        << origEnvLinux << false << "PATH" << "/opt"
        << Environment({"PATH=/bin:/usr/bin:/opt", "VAR=VALUE"}, OsTypeLinux);
    QTest::newRow("appendOrSetPath non-existing Win")
        << origEnvWin << false << "PATH" << "C:\\Windows"
        << Environment({"PATH=C:\\Windows\\System32;D:\\gnu\\bin;C:\\Windows",
                        "VAR=VALUE"}, OsTypeWindows);
    QTest::newRow("prependOrSetPath non-existing half-matching Unix")
        << origEnvLinux << true << "PATH" << "/bi"
        << Environment({"PATH=/bi:/bin:/usr/bin", "VAR=VALUE"}, OsTypeLinux);
    QTest::newRow("prependOrSetPath non-existing half-matching Win")
        << origEnvWin << true << "PATH" << "C:\\Windows"
        << Environment({"PATH=C:\\Windows;C:\\Windows\\System32;D:\\gnu\\bin",
                        "VAR=VALUE"}, OsTypeWindows);
}

void tst_Environment::pathChanges()
{
    QFETCH(Environment, environment);
    QFETCH(bool, prepend);
    QFETCH(QString, variable);
    QFETCH(QString, value);
    QFETCH(Environment, expected);

    const QString sep = OsSpecificAspects::pathListSeparator(environment.osType());

    if (prepend)
        environment.prependOrSet(variable, value, sep);
    else
        environment.appendOrSet(variable, value, sep);

    qDebug() << "Actual  :" << environment.toStringList();
    qDebug() << "Expected:" << expected.toStringList();
    QCOMPARE(environment, expected);
}

void tst_Environment::find_data()
{
    QTest::addColumn<Utils::OsType>("osType");
    QTest::addColumn<bool>("contains");
    QTest::addColumn<QString>("variable");


    QTest::newRow("win") << Utils::OsTypeWindows << true << "foo";
    QTest::newRow("win") << Utils::OsTypeWindows << true << "Foo";
    QTest::newRow("lin") << Utils::OsTypeLinux << true << "Foo";
    QTest::newRow("lin") << Utils::OsTypeLinux << false << "foo";
}

void tst_Environment::find()
{
    QFETCH(Utils::OsType, osType);
    QFETCH(bool, contains);
    QFETCH(QString, variable);


    Environment env(QStringList({"Foo=bar", "Hi=HO"}), osType);

    Environment::FindResult res = env.find(variable);

    QCOMPARE(bool(res), contains);

    if (contains)
        QCOMPARE(res->value, QString("bar"));

}

QTEST_GUILESS_MAIN(tst_Environment)

#include "tst_environment.moc"
