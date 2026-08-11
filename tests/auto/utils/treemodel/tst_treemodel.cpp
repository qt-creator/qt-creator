// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/treemodel.h>

#include <QSortFilterProxyModel>
#include <QTest>

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
    void testRemoveRows();
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

void tst_TreeModel::testRemoveRows()
{
    TreeModel<> m;
    TreeItem *r = m.rootItem();
    for (int i = 0; i < 5; ++i)                     // item0 .. item4
        r->appendChild(createItem(QString("item%1").arg(i)));
    QCOMPARE(m.rowCount(), 5);

    // Direct removal of a middle range.
    QVERIFY(m.removeRows(1, 2));                     // drops item1, item2
    QCOMPARE(m.rowCount(), 3);
    QCOMPARE(m.index(0, 0).data().toString(), QString("item0"));
    QCOMPARE(m.index(1, 0).data().toString(), QString("item3"));
    QCOMPARE(m.index(2, 0).data().toString(), QString("item4"));

    // Out-of-range and empty requests are rejected without changing anything.
    QVERIFY(!m.removeRows(2, 5));
    QVERIFY(!m.removeRows(-1, 1));
    QVERIFY(!m.removeRows(0, 0));
    QCOMPARE(m.rowCount(), 3);

    // Removal through a QSortFilterProxyModel must reach the source model.
    // The Valgrind suppression dialog relies on exactly this path
    // (QTCREATORBUG-18041).
    QSortFilterProxyModel proxy;
    proxy.setSourceModel(&m);
    QCOMPARE(proxy.rowCount(), 3);
    QVERIFY(proxy.removeRow(0));                     // drops item0 via the proxy
    QCOMPARE(proxy.rowCount(), 2);
    QCOMPARE(m.rowCount(), 2);
    QCOMPARE(m.index(0, 0).data().toString(), QString("item3"));
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

QTEST_GUILESS_MAIN(tst_TreeModel)

#include "tst_treemodel.moc"
