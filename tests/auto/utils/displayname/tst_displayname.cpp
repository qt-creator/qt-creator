// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/displayname.h>

#include <QTest>

using namespace Utils;

const Key nameKey = "Name";

class tst_DisplayName : public QObject
{
    Q_OBJECT

private slots:
    void defaultValue();
    void emptyDefaultValueIsIgnored();
    void serialization_data();
    void serialization();
};

void tst_DisplayName::defaultValue()
{
    DisplayName name;
    QVERIFY(name.setDefaultValue("theDefault"));
    QCOMPARE(name.value(), "theDefault");
    QVERIFY(name.usesDefaultValue());

    QVERIFY(name.setValue("explicit"));
    QCOMPARE(name.value(), "explicit");
    QVERIFY(!name.usesDefaultValue());

    // setting the value to the default falls back to the default again
    QVERIFY(name.setValue("theDefault"));
    QVERIFY(name.usesDefaultValue());
}

void tst_DisplayName::emptyDefaultValueIsIgnored()
{
    DisplayName name;
    name.setDefaultValue("theDefault");

    QVERIFY(!name.setDefaultValue({}));
    QCOMPARE(name.defaultValue(), "theDefault");
    QCOMPARE(name.value(), "theDefault");
}

void tst_DisplayName::serialization_data()
{
    QTest::addColumn<bool>("forceSerialization");
    QTest::addColumn<QString>("explicitValue");

    QTest::newRow("default name") << false << QString();
    QTest::newRow("explicit name") << false << QString("explicit");
    QTest::newRow("default name, forced") << true << QString();
    QTest::newRow("explicit name, forced") << true << QString("explicit");
}

void tst_DisplayName::serialization()
{
    QFETCH(bool, forceSerialization);
    QFETCH(QString, explicitValue);

    DisplayName name;
    if (forceSerialization)
        name.forceSerialization();
    name.setDefaultValue("theDefault");
    if (!explicitValue.isEmpty())
        name.setValue(explicitValue);
    const QString expectedValue = name.value();

    Store store;
    name.toMap(store, nameKey);

    QCOMPARE(store.contains(nameKey), forceSerialization || !explicitValue.isEmpty());
    QCOMPARE(store.size(), store.contains(nameKey) ? 1 : 0);

    DisplayName restored;
    restored.fromMap(store, nameKey);

    // no default is stored, so a name that relies on it is empty until the owner supplies it
    QCOMPARE(restored.value().isEmpty(), explicitValue.isEmpty());
    restored.setDefaultValue("theDefault");

    QCOMPARE(restored.value(), expectedValue);
    QCOMPARE(restored.usesDefaultValue(), explicitValue.isEmpty());
}

QTEST_GUILESS_MAIN(tst_DisplayName)

#include "tst_displayname.moc"
