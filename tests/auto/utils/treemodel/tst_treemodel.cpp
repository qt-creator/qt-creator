/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include <utils/treemodel.h>

#include <QtTest>

//TESTED_COMPONENT=src/libs/utils/treemodel

using namespace Utils;

class tst_TreeModel : public QObject
{
    Q_OBJECT

private slots:
    void testIteration();
};

static int countLevelItems(TreeItem *base, int level)
{
    int n = 0;
    foreach (TreeItem *item, UntypedTreeLevelItems(base, level)) {
        Q_UNUSED(item);
        ++n;
    }
    return n;
}

static TreeItem *createItem(const char *name)
{
    return new TreeItem(QStringList(QString::fromLatin1(name)));
}

void tst_TreeModel::testIteration()
{
    TreeModel m;
    TreeItem *r = m.rootItem();
    TreeItem *group0 = createItem("group0");
    TreeItem *group1 = createItem("group1");
    TreeItem *item10 = createItem("item10");
    TreeItem *item11 = createItem("item11");
    TreeItem *item12 = createItem("item12");
    group1->appendChild(item10);
    group1->appendChild(item11);
    TreeItem *group2 = createItem("group2");
    TreeItem *item20 = createItem("item20");
    TreeItem *item21 = createItem("item21");
    TreeItem *item22 = createItem("item22");
    r->appendChild(group0);
    r->appendChild(group1);
    r->appendChild(group2);
    group1->appendChild(item12);
    group2->appendChild(item20);
    group2->appendChild(item21);
    group2->appendChild(item22);

    QCOMPARE(r->rowCount(), 3);
    QCOMPARE(countLevelItems(r, 1), 3);
    QCOMPARE(countLevelItems(r, 2), 6);
    QCOMPARE(countLevelItems(r, 3), 0);
    QCOMPARE(countLevelItems(group0, 1), 0);
    QCOMPARE(countLevelItems(group1, 1), 3);
    QCOMPARE(countLevelItems(group1, 2), 0);
    QCOMPARE(countLevelItems(group2, 1), 3);
    QCOMPARE(countLevelItems(group2, 2), 0);
}


QTEST_MAIN(tst_TreeModel)

#include "tst_treemodel.moc"
