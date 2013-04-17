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

#include <find/treeviewfind.h>

#include <QtTest>

#include <QTreeWidget>

class tst_treeviewfind : public QObject
{
    Q_OBJECT

private slots:
    void wrapping();
    void columns();
};


void tst_treeviewfind::wrapping()
{
    // set up tree
    //   search for FOO in
    //   * HEADER1
    //     * FOO1
    //   * HEADER2
    //     * A
    //   * HEADER3
    //     * FOO2
    QTreeWidget *tree = new QTreeWidget;
    tree->setColumnCount(1);
    QList<QTreeWidgetItem *> toplevelitems;
    QTreeWidgetItem *item;
    item = new QTreeWidgetItem((QTreeWidget *)0, QStringList() << QLatin1String("HEADER1"));
    item->addChild(new QTreeWidgetItem((QTreeWidget *)0, QStringList() << QLatin1String("FOO1")));
    toplevelitems << item;
    item = new QTreeWidgetItem((QTreeWidget *)0, QStringList() << QLatin1String("HEADER2"));
    item->addChild(new QTreeWidgetItem((QTreeWidget *)0, QStringList() << QLatin1String("A")));
    toplevelitems << item;
    item = new QTreeWidgetItem((QTreeWidget *)0, QStringList() << QLatin1String("HEADER3"));
    item->addChild(new QTreeWidgetItem((QTreeWidget *)0, QStringList() << QLatin1String("FOO2")));
    toplevelitems << item;
    tree->addTopLevelItems(toplevelitems);

    // set up
    Find::TreeViewFind *findSupport = new Find::TreeViewFind(tree);
    tree->setCurrentItem(toplevelitems.at(2)->child(0));
    QCOMPARE(tree->currentItem()->text(0), QString::fromLatin1("FOO2"));

    // forward
    findSupport->findStep(QLatin1String("FOO"), 0);
    QCOMPARE(tree->currentItem(), toplevelitems.at(0)->child(0));

    // backward
    tree->setCurrentItem(toplevelitems.at(0)->child(0));
    QCOMPARE(tree->currentItem()->text(0), QString::fromLatin1("FOO1"));
    findSupport->findStep(QLatin1String("FOO"), Find::FindBackward);
    QCOMPARE(tree->currentItem(), toplevelitems.at(2)->child(0));

    // clean up
    delete findSupport;
    delete tree;
}

void tst_treeviewfind::columns()
{
    // set up tree
    //   search for FOO in
    //   * HEADER1 | HEADER1
    //     * FOO1  | A
    //   * HEADER2 | FOOHEADER2
    //     * FOO2  | FOO3
    //   * HEADER3 | HEADER2
    //     * A     | FOO4
    QTreeWidget *tree = new QTreeWidget;
    tree->setColumnCount(2);
    QList<QTreeWidgetItem *> toplevelitems;
    QTreeWidgetItem *item;
    item = new QTreeWidgetItem((QTreeWidget *)0, QStringList() << QLatin1String("HEADER1") << QLatin1String("HEADER1"));
    item->addChild(new QTreeWidgetItem((QTreeWidget *)0, QStringList() << QLatin1String("FOO1") << QLatin1String("A")));
    toplevelitems << item;
    item = new QTreeWidgetItem((QTreeWidget *)0, QStringList() << QLatin1String("HEADER2") << QLatin1String("FOOHEADER2"));
    item->addChild(new QTreeWidgetItem((QTreeWidget *)0, QStringList() << QLatin1String("FOO2") << QLatin1String("FOO3")));
    toplevelitems << item;
    item = new QTreeWidgetItem((QTreeWidget *)0, QStringList() << QLatin1String("HEADER3") << QLatin1String("HEADER3"));
    item->addChild(new QTreeWidgetItem((QTreeWidget *)0, QStringList() << QLatin1String("A") << QLatin1String("FOO4")));
    toplevelitems << item;
    tree->addTopLevelItems(toplevelitems);

    // set up
    Find::TreeViewFind *findSupport = new Find::TreeViewFind(tree);
    tree->setCurrentItem(toplevelitems.at(0));
    QCOMPARE(tree->currentItem()->text(0), QString::fromLatin1("HEADER1"));

    // find in first column
    findSupport->findStep(QLatin1String("FOO"), 0);
    QCOMPARE(tree->currentItem(), toplevelitems.at(0)->child(0));
    // find in second column of node with children
    findSupport->findStep(QLatin1String("FOO"), 0);
    QCOMPARE(tree->currentItem(), toplevelitems.at(1));
    // again find in first column
    findSupport->findStep(QLatin1String("FOO"), 0);
    QCOMPARE(tree->currentItem(), toplevelitems.at(1)->child(0));
    // don't stay in item if multiple columns match, and find in second column
    findSupport->findStep(QLatin1String("FOO"), 0);
    QCOMPARE(tree->currentItem(), toplevelitems.at(2)->child(0));
    // wrap
    findSupport->findStep(QLatin1String("FOO"), 0);
    QCOMPARE(tree->currentItem(), toplevelitems.at(0)->child(0));

    // backwards
    tree->setCurrentItem(toplevelitems.at(2)->child(0));
    QCOMPARE(tree->currentItem()->text(0), QString::fromLatin1("A"));
    findSupport->findStep(QLatin1String("FOO"), Find::FindBackward);
    QCOMPARE(tree->currentItem(), toplevelitems.at(1)->child(0));
    findSupport->findStep(QLatin1String("FOO"), Find::FindBackward);
    QCOMPARE(tree->currentItem(), toplevelitems.at(1));
    findSupport->findStep(QLatin1String("FOO"), Find::FindBackward);
    QCOMPARE(tree->currentItem(), toplevelitems.at(0)->child(0));
    findSupport->findStep(QLatin1String("FOO"), Find::FindBackward);
    QCOMPARE(tree->currentItem(), toplevelitems.at(2)->child(0));

    // clean up
    delete findSupport;
    delete tree;
}

QTEST_MAIN(tst_treeviewfind)

#include "tst_treeviewfind.moc"
