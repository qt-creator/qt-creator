/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "../../../src/libs/extensionsystem/composite.h"

#include <QtTest/QtTest>

using namespace ExtensionSystem;

class tst_Composite : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();
    void addInterface();
    void removeInterface();
    void constructor();

private:
    Composite *composite;
    QObject *component1;
    QObject *component2;
};

class Interface1 : public Interface
{
    Q_OBJECT
};

class Interface2 : public QObject
{
    Q_OBJECT
};

void tst_Composite::init()
{
    composite = new Composite;
    component1 = new Interface1;
    component2 = new Interface2;
    composite->addInterface(component1);
}

void tst_Composite::cleanup()
{
    delete component1;
    delete component2;
    delete composite;
}

void tst_Composite::addInterface()
{
    QCOMPARE(interface_cast<Interface2>(composite), (void*)0);
    composite->addInterface(component2);
    QCOMPARE(interface_cast<Interface2>(composite), component2);
}

void tst_Composite::removeInterface()
{
    QCOMPARE(interface_cast<Interface1>(composite), component1);
    composite->removeInterface(component1);
    QCOMPARE(interface_cast<Interface1>(composite), (void*)0);
}

void tst_Composite::constructor()
{
    Composite multiple(QSet<QObject*>() << component1 << component2);
    QCOMPARE(interface_cast<Interface1>(&multiple), component1);
    QCOMPARE(interface_cast<Interface2>(&multiple), component2);
}

QTEST_MAIN(tst_Composite)
#include "tst_composite.moc"

