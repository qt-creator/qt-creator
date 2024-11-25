// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QtTest>

#include <utils/id.h>

QT_BEGIN_NAMESPACE
namespace QTest {
template<>
char *toString(const Utils::Id &id)
{
    return qstrdup(id.name().constData());
}
} // namespace QTest
QT_END_NAMESPACE

namespace Utils {

class tst_id : public QObject
{
    Q_OBJECT

private slots:
    void isValid();
    void withPrefix();
    void withSuffix();
    void compare();
    void suffixAfter();
};

void tst_id::isValid()
{
    QVERIFY(!Id().isValid());
    QVERIFY(!Id::fromSetting("").isValid());
    QVERIFY(Id::fromSetting("a").isValid());
    QVERIFY(Id::generate().isValid());
}

void tst_id::withPrefix()
{
    QCOMPARE(Id("bar").withPrefix("foo"), Id("foobar"));
}

void tst_id::withSuffix()
{
    Id id("foo");
    QCOMPARE(id.withSuffix(42), Id("foo42"));
    QCOMPARE(id.withSuffix('l'), Id("fool"));
    QCOMPARE(id.withSuffix("lish"), Id("foolish"));
    QCOMPARE(id.withSuffix(QStringView(QString("bar"))), Id("foobar"));
}

void tst_id::compare()
{
    Id compId("blubb");
    Id runId = Id::fromSetting("blubb");
    QCOMPARE(compId, runId);
    QCOMPARE(qHash(compId), qHash(runId));
    QCOMPARE(compId.name(), "blubb");
    QCOMPARE(runId, "blubb");
    QVERIFY(compId == runId);
    QVERIFY(compId == "blubb");
    QVERIFY(runId == "blubb");
    QVERIFY(Id("bar") < Id("foo"));
    QVERIFY(Id("foo") > Id("bar"));
    QVERIFY(!(Id("foo") < Id("bar")));
}

void tst_id::suffixAfter()
{
    QCOMPARE(Id("foobar").suffixAfter(Id("foo")), "bar");
}

} // Utils

QTEST_GUILESS_MAIN(Utils::tst_id)

#include "tst_id.moc"
