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

#include <QtTest>

#include <projectexplorer/gcctoolchain.h>

//////////////// the actual autotest

class tst_ToolChainCache : public QObject
{
    Q_OBJECT

private slots:
    void insertOne();
    void insertOneOne();
    void insertOneTwo();
    void insertOneTwoThree();
    void insertOneTwoOneThree();
};


void tst_ToolChainCache::insertOne()
{
    const QStringList key1 = {"one"};
    const QString value1 = "value1";
    ProjectExplorer::Cache<QStringList, QString, 2> cache;

    cache.insert(key1, value1);

    QVERIFY(bool(cache.check(key1)));
    QCOMPARE(cache.check(key1).value(), value1);
    QVERIFY(!cache.check({"other"}));
}

void tst_ToolChainCache::insertOneOne()
{
    const QStringList key1 = {"one"};
    const QString value1 = "value1";
    ProjectExplorer::Cache<QStringList, QString, 2> cache;

    cache.insert(key1, value1);
    cache.insert(key1, value1);

    QVERIFY(bool(cache.check(key1)));
    QCOMPARE(cache.check(key1).value(), value1);
    QVERIFY(!cache.check({"other"}));
}

void tst_ToolChainCache::insertOneTwo()
{
    const QStringList key1 = {"one"};
    const QString value1 = "value1";
    const QStringList key2 = {"two"};
    const QString value2 = "value2";
    ProjectExplorer::Cache<QStringList, QString, 2> cache;

    cache.insert(key1, value1);
    cache.insert(key2, value2);

    QVERIFY(bool(cache.check(key1)));
    QCOMPARE(cache.check(key1).value(), value1);
    QVERIFY(bool(cache.check(key2)));
    QCOMPARE(cache.check(key2).value(), value2);
    QVERIFY(!cache.check({"other"}));
}

void tst_ToolChainCache::insertOneTwoThree()
{
    const QStringList key1 = {"one"};
    const QString value1 = "value1";
    const QStringList key2 = {"two"};
    const QString value2 = "value2";
    const QStringList key3 = {"three"};
    const QString value3 = "value3";
    ProjectExplorer::Cache<QStringList, QString, 2> cache;

    cache.insert(key1, value1);
    cache.insert(key2, value2);
    cache.insert(key3, value3);

    QVERIFY(!cache.check(key1)); // key1 was evicted
    QVERIFY(bool(cache.check(key2)));
    QCOMPARE(cache.check(key2).value(), value2);
    QVERIFY(bool(cache.check(key3)));
    QCOMPARE(cache.check(key3).value(), value3);
    QVERIFY(!cache.check({"other"}));
}

void tst_ToolChainCache::insertOneTwoOneThree()
{
    const QStringList key1 = {"one"};
    const QString value1 = "value1";
    const QStringList key2 = {"two"};
    const QString value2 = "value2";
    const QStringList key3 = {"three"};
    const QString value3 = "value3";
    ProjectExplorer::Cache<QStringList, QString, 2> cache;

    cache.insert(key1, value1);
    cache.insert(key2, value2);
    cache.insert(key1, value1);
    cache.insert(key3, value3);

    QVERIFY(bool(cache.check(key1)));
    QCOMPARE(cache.check(key1).value(), value1);
    QVERIFY(!cache.check(key2)); // key2 was evicted
    QVERIFY(bool(cache.check(key3)));
    QCOMPARE(cache.check(key3).value(), value3);
    QVERIFY(!cache.check({"other"}));
}

QTEST_MAIN(tst_ToolChainCache)
#include "tst_toolchaincache.moc"
