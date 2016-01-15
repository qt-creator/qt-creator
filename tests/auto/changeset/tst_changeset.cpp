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

#include <utils/changeset.h>

#include <QtTest>

//TESTED_COMPONENT=src/utils/changeset

using Utils::ChangeSet;

class tst_ChangeSet : public QObject
{
    Q_OBJECT

private slots:
    void singleReplace();
    void singleMove();
    void singleInsert();
    void singleRemove();
    void singleFlip();
    void singleCopy();

    void doubleInsert();
    void conflicts();
};


void tst_ChangeSet::singleReplace()
{
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.replace(0, 2, "ghi"));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("ghicdef"));
    }
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.replace(4, 6, "ghi"));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcdghi"));
    }
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.replace(3, 3, "ghi"));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcghidef"));
    }
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.replace(0, 6, ""));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String(""));
    }
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.replace(3, 13, "ghi"));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcghi"));
    }
}

void tst_ChangeSet::singleMove()
{
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.move(0, 2, 4));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("cdabef"));
    }
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.move(4, 6, 0));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("efabcd"));
    }
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.move(3, 13, 0));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("defabc"));
    }
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.move(3, 3, 0));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcdef"));
    }
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.move(0, 1, 10));
        cs.apply(&test);
        // ### maybe this should expand the string or error?
        QCOMPARE(test, QLatin1String("bcdef"));
    }
}

void tst_ChangeSet::singleInsert()
{
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.insert(0, "ghi"));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("ghiabcdef"));
    }
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.insert(6, "ghi"));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcdefghi"));
    }
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.insert(3, ""));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcdef"));
    }
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.insert(7, "g"));
        cs.apply(&test);
        // ### maybe this should expand the string or error?
        QCOMPARE(test, QLatin1String("abcdef"));
    }
}

void tst_ChangeSet::singleRemove()
{
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.remove(0, 1));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("bcdef"));
    }
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.remove(3, 6));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abc"));
    }
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.remove(4, 14));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcd"));
    }
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.remove(2, 2));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcdef"));
    }
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.remove(7, 8));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcdef"));
    }
}

void tst_ChangeSet::singleFlip()
{
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.flip(0, 2, 3, 6));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("defcab"));
    }
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.flip(1, 3, 3, 4));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("adbcef"));
    }
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.flip(3, 3, 4, 4));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcdef"));
    }
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.flip(3, 3, 4, 5));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcedf"));
    }
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.flip(0, 6, 6, 12));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcdef"));
    }
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.flip(0, 6, 7, 10));
        cs.apply(&test);
        // ### maybe this should expand the string or error?
        QCOMPARE(test, QLatin1String(""));
    }
    {
        ChangeSet cs;
        QCOMPARE(cs.flip(0, 3, 1, 4), false);
    }
    {
        ChangeSet cs;
        QCOMPARE(cs.flip(0, 3, 2, 5), false);
    }
    {
        ChangeSet cs;
        QVERIFY(cs.flip(0, 3, 0, 0));
        QString test("abcdef");
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcdef"));
    }
    {
        ChangeSet cs;
        QVERIFY(cs.flip(0, 0, 0, 3));
        QString test("abcdef");
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcdef"));
    }
    {
        ChangeSet cs;
        QVERIFY(cs.flip(0, 3, 3, 3));
        QString test("abcdef");
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcdef"));
    }
}

void tst_ChangeSet::singleCopy()
{
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.copy(0, 2, 4));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcdabef"));
    }
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.copy(1, 3, 3));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcbcdef"));
    }
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.copy(3, 3, 4));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcdef"));
    }
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.copy(0, 6, 6));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcdefabcdef"));
    }
    {
        ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.copy(0, 6, 7));
        cs.apply(&test);
        // ### maybe this should expand the string or error?
        QCOMPARE(test, QLatin1String("abcdef"));
    }
    {
        ChangeSet cs;
        QCOMPARE(cs.copy(0, 3, 1), false);
    }
    {
        ChangeSet cs;
        QCOMPARE(cs.copy(0, 3, 2), false);
    }
    {
        ChangeSet cs;
        QVERIFY(cs.copy(0, 3, 0));
        QString test("abcdef");
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcabcdef"));
    }
    {
        ChangeSet cs;
        QVERIFY(cs.copy(0, 3, 3));
        QString test("abcdef");
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcabcdef"));
    }
}

void tst_ChangeSet::doubleInsert()
{
    {
        ChangeSet cs;
        QVERIFY(cs.insert(1, "01"));
        QVERIFY(cs.insert(1, "234"));
        QString test("abcdef");
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("a01234bcdef"));
    }
    {
        ChangeSet cs;
        QVERIFY(cs.insert(1, "234"));
        QVERIFY(cs.insert(1, "01"));
        QString test("abcdef");
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("a23401bcdef"));
    }
    {
        ChangeSet cs;
        QVERIFY(cs.insert(1, "01"));
        QVERIFY(cs.remove(1, 2));
        QVERIFY(cs.insert(2, "234"));
        QString test("abcdef");
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("a01234cdef"));
    }
}

void tst_ChangeSet::conflicts()
{
    {
        ChangeSet cs;
        QVERIFY(cs.move(1, 4, 5));
        QCOMPARE(cs.replace(0, 2, "abc"), false);
    }
    {
        ChangeSet cs;
        QVERIFY(cs.move(1, 4, 5));
        QCOMPARE(cs.replace(1, 4, "abc"), false);
    }
    {
        ChangeSet cs;
        QVERIFY(cs.move(1, 4, 5));
        QCOMPARE(cs.replace(1, 2, "abc"), false);
    }
    {
        ChangeSet cs;
        QVERIFY(cs.move(1, 4, 5));
        QCOMPARE(cs.replace(2, 2, "abc"), false);
    }
    {
        ChangeSet cs;
        QVERIFY(cs.move(1, 4, 5));
        QCOMPARE(cs.replace(2, 3, "abc"), false);
    }
    {
        ChangeSet cs;
        QVERIFY(cs.move(1, 4, 5));
        QCOMPARE(cs.replace(3, 3, "abc"), false);
    }
    {
        ChangeSet cs;
        QVERIFY(cs.move(1, 4, 5));
        QCOMPARE(cs.replace(3, 4, "abc"), false);
    }
    {
        ChangeSet cs;
        QVERIFY(cs.move(1, 4, 5));
        QCOMPARE(cs.replace(4, 6, "abc"), false);
    }

    {
        ChangeSet cs;
        QVERIFY(cs.move(1, 4, 5));
        QVERIFY(cs.replace(0, 1, "bla"));
        QString test("abcdef");
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("blaebcdf"));
    }
    {
        ChangeSet cs;
        QVERIFY(cs.move(1, 4, 5));
        QVERIFY(cs.replace(4, 5, "bla"));
        QString test("abcdef");
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("ablabcdf"));
    }
    {
        ChangeSet cs;
        QVERIFY(cs.move(1, 4, 5));
        QVERIFY(cs.replace(5, 6, "bla"));
        QString test("abcdef");
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("aebcdbla"));
    }
}

QTEST_MAIN(tst_ChangeSet)

#include "tst_changeset.moc"
