/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <utils/changeset.h>

#include <QtTest>

//TESTED_COMPONENT=src/utils/changeset

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
        Utils::ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.replace(0, 2, "ghi"));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("ghicdef"));
    }
    {
        Utils::ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.replace(4, 6, "ghi"));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcdghi"));
    }
    {
        Utils::ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.replace(3, 3, "ghi"));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcghidef"));
    }
    {
        Utils::ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.replace(0, 6, ""));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String(""));
    }
    {
        Utils::ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.replace(3, 13, "ghi"));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcghi"));
    }
}

void tst_ChangeSet::singleMove()
{
    {
        Utils::ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.move(0, 2, 4));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("cdabef"));
    }
    {
        Utils::ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.move(4, 6, 0));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("efabcd"));
    }
    {
        Utils::ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.move(3, 13, 0));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("defabc"));
    }
    {
        Utils::ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.move(3, 3, 0));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcdef"));
    }
    {
        Utils::ChangeSet cs;
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
        Utils::ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.insert(0, "ghi"));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("ghiabcdef"));
    }
    {
        Utils::ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.insert(6, "ghi"));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcdefghi"));
    }
    {
        Utils::ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.insert(3, ""));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcdef"));
    }
    {
        Utils::ChangeSet cs;
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
        Utils::ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.remove(0, 1));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("bcdef"));
    }
    {
        Utils::ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.remove(3, 6));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abc"));
    }
    {
        Utils::ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.remove(4, 14));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcd"));
    }
    {
        Utils::ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.remove(2, 2));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcdef"));
    }
    {
        Utils::ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.remove(7, 8));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcdef"));
    }
}

void tst_ChangeSet::singleFlip()
{
    {
        Utils::ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.flip(0, 2, 3, 6));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("defcab"));
    }
    {
        Utils::ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.flip(1, 3, 3, 4));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("adbcef"));
    }
    {
        Utils::ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.flip(3, 3, 4, 4));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcdef"));
    }
    {
        Utils::ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.flip(3, 3, 4, 5));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcedf"));
    }
    {
        Utils::ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.flip(0, 6, 6, 12));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcdef"));
    }
    {
        Utils::ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.flip(0, 6, 7, 10));
        cs.apply(&test);
        // ### maybe this should expand the string or error?
        QCOMPARE(test, QLatin1String(""));
    }
    {
        Utils::ChangeSet cs;
        QCOMPARE(cs.flip(0, 3, 1, 4), false);
    }
    {
        Utils::ChangeSet cs;
        QCOMPARE(cs.flip(0, 3, 2, 5), false);
    }
    {
        Utils::ChangeSet cs;
        QVERIFY(cs.flip(0, 3, 0, 0));
        QString test("abcdef");
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcdef"));
    }
    {
        Utils::ChangeSet cs;
        QVERIFY(cs.flip(0, 0, 0, 3));
        QString test("abcdef");
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcdef"));
    }
    {
        Utils::ChangeSet cs;
        QVERIFY(cs.flip(0, 3, 3, 3));
        QString test("abcdef");
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcdef"));
    }
}

void tst_ChangeSet::singleCopy()
{
    {
        Utils::ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.copy(0, 2, 4));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcdabef"));
    }
    {
        Utils::ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.copy(1, 3, 3));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcbcdef"));
    }
    {
        Utils::ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.copy(3, 3, 4));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcdef"));
    }
    {
        Utils::ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.copy(0, 6, 6));
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcdefabcdef"));
    }
    {
        Utils::ChangeSet cs;
        QString test("abcdef");
        QVERIFY(cs.copy(0, 6, 7));
        cs.apply(&test);
        // ### maybe this should expand the string or error?
        QCOMPARE(test, QLatin1String("abcdef"));
    }
    {
        Utils::ChangeSet cs;
        QCOMPARE(cs.copy(0, 3, 1), false);
    }
    {
        Utils::ChangeSet cs;
        QCOMPARE(cs.copy(0, 3, 2), false);
    }
    {
        Utils::ChangeSet cs;
        QVERIFY(cs.copy(0, 3, 0));
        QString test("abcdef");
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcabcdef"));
    }
    {
        Utils::ChangeSet cs;
        QVERIFY(cs.copy(0, 3, 3));
        QString test("abcdef");
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("abcabcdef"));
    }
}

void tst_ChangeSet::doubleInsert()
{
    {
        Utils::ChangeSet cs;
        QVERIFY(cs.insert(1, "01"));
        QVERIFY(cs.insert(1, "234"));
        QString test("abcdef");
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("a01234bcdef"));
    }
    {
        Utils::ChangeSet cs;
        QVERIFY(cs.insert(1, "234"));
        QVERIFY(cs.insert(1, "01"));
        QString test("abcdef");
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("a23401bcdef"));
    }
    {
        Utils::ChangeSet cs;
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
        Utils::ChangeSet cs;
        QVERIFY(cs.move(1, 4, 5));
        QCOMPARE(cs.replace(0, 2, "abc"), false);
    }
    {
        Utils::ChangeSet cs;
        QVERIFY(cs.move(1, 4, 5));
        QCOMPARE(cs.replace(1, 4, "abc"), false);
    }
    {
        Utils::ChangeSet cs;
        QVERIFY(cs.move(1, 4, 5));
        QCOMPARE(cs.replace(1, 2, "abc"), false);
    }
    {
        Utils::ChangeSet cs;
        QVERIFY(cs.move(1, 4, 5));
        QCOMPARE(cs.replace(2, 2, "abc"), false);
    }
    {
        Utils::ChangeSet cs;
        QVERIFY(cs.move(1, 4, 5));
        QCOMPARE(cs.replace(2, 3, "abc"), false);
    }
    {
        Utils::ChangeSet cs;
        QVERIFY(cs.move(1, 4, 5));
        QCOMPARE(cs.replace(3, 3, "abc"), false);
    }
    {
        Utils::ChangeSet cs;
        QVERIFY(cs.move(1, 4, 5));
        QCOMPARE(cs.replace(3, 4, "abc"), false);
    }
    {
        Utils::ChangeSet cs;
        QVERIFY(cs.move(1, 4, 5));
        QCOMPARE(cs.replace(4, 6, "abc"), false);
    }

    {
        Utils::ChangeSet cs;
        QVERIFY(cs.move(1, 4, 5));
        QVERIFY(cs.replace(0, 1, "bla"));
        QString test("abcdef");
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("blaebcdf"));
    }
    {
        Utils::ChangeSet cs;
        QVERIFY(cs.move(1, 4, 5));
        QVERIFY(cs.replace(4, 5, "bla"));
        QString test("abcdef");
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("ablabcdf"));
    }
    {
        Utils::ChangeSet cs;
        QVERIFY(cs.move(1, 4, 5));
        QVERIFY(cs.replace(5, 6, "bla"));
        QString test("abcdef");
        cs.apply(&test);
        QCOMPARE(test, QLatin1String("aebcdbla"));
    }
}

QTEST_MAIN(tst_ChangeSet)

#include "tst_changeset.moc"
