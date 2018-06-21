/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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

    QCOMPARE(env.toStringList(), QStringList({"Foo=bar", "Hi=HO", "bar=baz"}));
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

    auto end = env.constEnd();
    auto it = env.constFind(variable);

    QCOMPARE((end != it), contains);

    if (contains)
        QCOMPARE(it.value(), QString("bar"));

}

QTEST_MAIN(tst_Environment)

#include "tst_environment.moc"
