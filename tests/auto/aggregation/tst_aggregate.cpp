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

#include <aggregate.h>

#include <QtTest>

//TESTED_COMPONENT=src/libs/aggregation

class tst_Aggregate : public QObject
{
    Q_OBJECT

private slots:
    void deleteAggregation();
    void queryAggregation();
    void queryAll();
    void parentAggregate();
};

class Interface1 : public QObject
{
    Q_OBJECT
};

class Interface11 : public Interface1
{
    Q_OBJECT
};

class Interface2 : public QObject
{
    Q_OBJECT
};

class Interface3 : public QObject
{
    Q_OBJECT
};

void tst_Aggregate::deleteAggregation()
{
    QPointer<Aggregation::Aggregate> aggregation;
    QPointer<QObject> component1;
    QPointer<QObject> component2;

    aggregation = new Aggregation::Aggregate;
    component1 = new Interface1;
    component2 = new Interface2;
    aggregation->add(component1);
    aggregation->add(component2);
    delete aggregation;
    QVERIFY(aggregation == 0);
    QVERIFY(component1 == 0);
    QVERIFY(component2 == 0);

    aggregation = new Aggregation::Aggregate;
    component1 = new Interface1;
    component2 = new Interface2;
    aggregation->add(component1);
    aggregation->add(component2);
    delete component1;
    QVERIFY(aggregation == 0);
    QVERIFY(component1 == 0);
    QVERIFY(component2 == 0);

    aggregation = new Aggregation::Aggregate;
    component1 = new Interface1;
    component2 = new Interface2;
    aggregation->add(component1);
    aggregation->add(component2);
    delete component2;
    QVERIFY(aggregation == 0);
    QVERIFY(component1 == 0);
    QVERIFY(component2 == 0);

    // if a component doesn't belong to an aggregation, it should simply delete itself
    component1 = new Interface1;
    delete component1;
    QVERIFY(component1 == 0);
}

void tst_Aggregate::queryAggregation()
{
    Aggregation::Aggregate aggregation;
    QObject *aggObject = &aggregation;
    QObject *component1 = new Interface11;
    QObject *component2 = new Interface2;
    aggregation.add(component1);
    aggregation.add(component2);
    QCOMPARE(Aggregation::query<Interface1>(&aggregation), component1);
    QCOMPARE(Aggregation::query<Interface2>(&aggregation), component2);
    QCOMPARE(Aggregation::query<Interface11>(&aggregation), component1);
    QCOMPARE(Aggregation::query<Interface3>(&aggregation), (Interface3 *)0);

    QCOMPARE(Aggregation::query<Interface1>(aggObject), component1);
    QCOMPARE(Aggregation::query<Interface2>(aggObject), component2);
    QCOMPARE(Aggregation::query<Interface11>(aggObject), component1);
    QCOMPARE(Aggregation::query<Interface3>(aggObject), (Interface3 *)0);

    QCOMPARE(Aggregation::query<Interface1>(component1), component1);
    QCOMPARE(Aggregation::query<Interface2>(component1), component2);
    QCOMPARE(Aggregation::query<Interface11>(component1), component1);
    QCOMPARE(Aggregation::query<Interface3>(component1), (Interface3 *)0);

    QCOMPARE(Aggregation::query<Interface1>(component2), component1);
    QCOMPARE(Aggregation::query<Interface2>(component2), component2);
    QCOMPARE(Aggregation::query<Interface11>(component2), component1);
    QCOMPARE(Aggregation::query<Interface3>(component2), (Interface3 *)0);

    // components that don't belong to an aggregation should be query-able to itself only
    QObject *component3 = new Interface3;
    QCOMPARE(Aggregation::query<Interface1>(component3), (Interface1 *)0);
    QCOMPARE(Aggregation::query<Interface2>(component3), (Interface2 *)0);
    QCOMPARE(Aggregation::query<Interface11>(component3), (Interface11 *)0);
    QCOMPARE(Aggregation::query<Interface3>(component3), component3);
    delete component3;
}

void tst_Aggregate::queryAll()
{
    Aggregation::Aggregate aggregation;
    QObject *aggObject = &aggregation;
    Interface1 *component1 = new Interface1;
    Interface11 *component11 = new Interface11;
    Interface2 *component2 = new Interface2;
    aggregation.add(component1);
    aggregation.add(component11);
    aggregation.add(component2);
    QCOMPARE(Aggregation::query_all<Interface1>(&aggregation), QList<Interface1 *>() << component1 << component11);
    QCOMPARE(Aggregation::query_all<Interface11>(&aggregation), QList<Interface11 *>() << component11);
    QCOMPARE(Aggregation::query_all<Interface2>(&aggregation), QList<Interface2 *>() << component2);
    QCOMPARE(Aggregation::query_all<Interface3>(&aggregation), QList<Interface3 *>());

    QCOMPARE(Aggregation::query_all<Interface1>(aggObject), QList<Interface1 *>() << component1 << component11);
    QCOMPARE(Aggregation::query_all<Interface11>(aggObject), QList<Interface11 *>() << component11);
    QCOMPARE(Aggregation::query_all<Interface2>(aggObject), QList<Interface2 *>() << component2);
    QCOMPARE(Aggregation::query_all<Interface3>(aggObject), QList<Interface3 *>());

    QCOMPARE(Aggregation::query_all<Interface1>(component1), QList<Interface1 *>() << component1 << component11);
    QCOMPARE(Aggregation::query_all<Interface11>(component1), QList<Interface11 *>() << component11);
    QCOMPARE(Aggregation::query_all<Interface2>(component1), QList<Interface2 *>() << component2);
    QCOMPARE(Aggregation::query_all<Interface3>(component1), QList<Interface3 *>());

    QCOMPARE(Aggregation::query_all<Interface1>(component11), QList<Interface1 *>() << component1 << component11);
    QCOMPARE(Aggregation::query_all<Interface11>(component11), QList<Interface11 *>() << component11);
    QCOMPARE(Aggregation::query_all<Interface2>(component11), QList<Interface2 *>() << component2);
    QCOMPARE(Aggregation::query_all<Interface3>(component11), QList<Interface3 *>());

    QCOMPARE(Aggregation::query_all<Interface1>(component2), QList<Interface1 *>() << component1 << component11);
    QCOMPARE(Aggregation::query_all<Interface11>(component2), QList<Interface11 *>() << component11);
    QCOMPARE(Aggregation::query_all<Interface2>(component2), QList<Interface2 *>() << component2);
    QCOMPARE(Aggregation::query_all<Interface3>(component2), QList<Interface3 *>());
}

void tst_Aggregate::parentAggregate()
{
    Aggregation::Aggregate aggregation;
    Aggregation::Aggregate aggregation2;
    Interface1 *component1 = new Interface1;
    Interface11 *component11 = new Interface11;
    QObject *component2 = new QObject;
    aggregation.add(component1);
    aggregation.add(component11);
    QCOMPARE(Aggregation::Aggregate::parentAggregate(&aggregation), &aggregation);
    QCOMPARE(Aggregation::Aggregate::parentAggregate(component1), &aggregation);
    QCOMPARE(Aggregation::Aggregate::parentAggregate(component11), &aggregation);
    QCOMPARE(Aggregation::Aggregate::parentAggregate(component2), (Aggregation::Aggregate *)0);
    // test reparenting a component to another aggregate (should warn but not work)
    aggregation2.add(component11);
    QCOMPARE(Aggregation::Aggregate::parentAggregate(component11), &aggregation);
    // test adding an aggregate to an aggregate (should warn but not work)
    aggregation.add(&aggregation2);
    QCOMPARE(Aggregation::Aggregate::parentAggregate(&aggregation2), &aggregation2);
    // test removing an object from an aggregation.
    aggregation.remove(component11);
    QCOMPARE(Aggregation::Aggregate::parentAggregate(component11), (Aggregation::Aggregate *)0);
}

QTEST_MAIN(tst_Aggregate)

#include "tst_aggregate.moc"
