/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <utils/objectpool.h>

#include <QtTest>

//TESTED_COMPONENT=src/libs/utils

using namespace Utils;

class tst_ObjectPool : public QObject
{
    Q_OBJECT

private slots:
    void testSize();
};

void tst_ObjectPool::testSize()
{
    QObject parent;

    QPointer<QObject> object1 = new QObject;
    object1->setObjectName("object1");

    QPointer<QObject> object2 = new QObject(&parent);
    object2->setObjectName("object2");

    QPointer<QObject> object3 = new QObject;
    object3->setObjectName("object3");

    QPointer<QObject> object4 = new QObject(&parent);
    object4->setObjectName("object4");

    {
        ObjectPool<QObject> pool;
        QCOMPARE(pool.size(), 0);

        pool.addObject(object1.data());
        QCOMPARE(pool.size(), 1);

        pool.addObject(object2.data());
        QCOMPARE(pool.size(), 2);

        pool.addObject(object3.data());
        QCOMPARE(pool.size(), 3);

        pool.addObject(object4.data());
        QCOMPARE(pool.size(), 4);

        delete object1;
        QCOMPARE(pool.size(), 3);
        QCOMPARE(parent.children().size(), 2);

        delete object2;
        QCOMPARE(pool.size(), 2);
        QCOMPARE(parent.children().size(), 1);
    }

    QCOMPARE(parent.children().size(), 1);
    QCOMPARE(object3.isNull(), true);
    QCOMPARE(object4.isNull(), false);

    delete object4;
    QCOMPARE(parent.children().size(), 0);
    QCOMPARE(object3.isNull(), true);
    QCOMPARE(object4.isNull(), true);
}


QTEST_MAIN(tst_ObjectPool)

#include "tst_objectpool.moc"
