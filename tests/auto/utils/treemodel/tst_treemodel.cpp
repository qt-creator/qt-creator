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

#include <utils/treemodel.h>

#include <QtTest>

#include <type_traits>

//TESTED_COMPONENT=src/libs/utils/treemodel

using namespace Utils;

class tst_TreeModel : public QObject
{
    Q_OBJECT

private slots:
    void testTypes();
    void testIteration();
    void testMixed();
};

static int countLevelItems(TreeItem *base, int level)
{
    int n = 0;
    int bl = base->level();
    base->forAllChildren([level, bl, &n](TreeItem *item) {
        if (item->level() == bl + level)
            ++n;
    });
    return n;
}

static TreeItem *createItem(const QString &name)
{
    return new StaticTreeItem(name);
}

void tst_TreeModel::testIteration()
{
    TreeModel<> m;
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

    QCOMPARE(r->childCount(), 3);
    QCOMPARE(countLevelItems(r, 1), 3);
    QCOMPARE(countLevelItems(r, 2), 6);
    QCOMPARE(countLevelItems(r, 3), 0);
    QCOMPARE(countLevelItems(group0, 1), 0);
    QCOMPARE(countLevelItems(group1, 1), 3);
    QCOMPARE(countLevelItems(group1, 2), 0);
    QCOMPARE(countLevelItems(group2, 1), 3);
    QCOMPARE(countLevelItems(group2, 2), 0);
}

struct ItemA : public TreeItem {};
struct ItemB : public TreeItem {};

void tst_TreeModel::testMixed()
{
    TreeModel<TreeItem, ItemA, ItemB> m;
    TreeItem *r = m.rootItem();
    TreeItem *ra;
    r->appendChild(new ItemA);
    r->appendChild(ra = new ItemA);
    ra->appendChild(new ItemB);
    ra->appendChild(new ItemB);

    int n = 0;
    m.forItemsAtLevel<1>([&n](ItemA *) { ++n; });
    QCOMPARE(n, 2);

    n = 0;
    m.forItemsAtLevel<2>([&n](ItemB *) { ++n; });
    QCOMPARE(n, 2);
}

void tst_TreeModel::testTypes()
{
    struct A {};
    struct B {};
    struct C {};

    static_assert(std::is_same<Internal::SelectType<0, A>::Type, A>::value, "");
    static_assert(std::is_same<Internal::SelectType<0>::Type, TreeItem>::value, "");
    static_assert(std::is_same<Internal::SelectType<1>::Type, TreeItem>::value, "");
    static_assert(std::is_same<Internal::SelectType<0, A, B, C>::Type, A>::value, "");
    static_assert(std::is_same<Internal::SelectType<1, A, B, C>::Type, B>::value, "");
    static_assert(std::is_same<Internal::SelectType<2, A, B, C>::Type, C>::value, "");
    static_assert(std::is_same<Internal::SelectType<3, A, B, C>::Type, TreeItem>::value, "");
}

QTEST_MAIN(tst_TreeModel)

#include "tst_treemodel.moc"
