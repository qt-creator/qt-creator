// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/layoutbuilder.h>

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QApplication>
#include <QGridLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QRegularExpression>
#include <QTest>

#include <memory>
#include <optional>

using namespace Layouting;

class tst_LayoutBuilder : public QObject
{
    Q_OBJECT

private slots:
    void blueprintEmergesTwice();
    void initializerListEndsRow();
    void stepByStepForm();
    void stretchOnBlueprint();
    void setterArgumentIsCopied();
    void bindToIsSetOnEmerge();
    void builderWithProductAppliesItems();
    void applyToExistingWidget();
    void setterOnBlueprint();
    void spanAllEmergesTwice();
    void showOnBlueprint();
    void existingWidgetIsSingleUse();
    void existingDialogButtonIsSingleUse();
    void deletedExistingWidgetIsSkipped();
};

void tst_LayoutBuilder::blueprintEmergesTwice()
{
    const Grid grid{QString("a"), QString("b"), br, QString("c")};
    const std::unique_ptr<QWidget> w1(grid.emerge());
    const std::unique_ptr<QWidget> w2(grid.emerge());
    auto g1 = qobject_cast<QGridLayout *>(w1->layout());
    auto g2 = qobject_cast<QGridLayout *>(w2->layout());
    QVERIFY(g1 && g2);
    QVERIFY(g1 != g2);
    QCOMPARE(g1->rowCount(), 2);
    QCOMPARE(g2->rowCount(), 2);
    QCOMPARE(g2->count(), 3);
}

void tst_LayoutBuilder::initializerListEndsRow()
{
    Grid grid{QString("a"), QString("b")};
    grid.addItems({QString("c")});
    const std::unique_ptr<QWidget> w(grid.emerge());
    auto g = qobject_cast<QGridLayout *>(w->layout());
    QVERIFY(g);
    QCOMPARE(g->rowCount(), 2);
}

void tst_LayoutBuilder::stepByStepForm()
{
    Form form;
    form.setNoMargins();
    form.addItems({QString("a"), QString("b")});
    form.flush();
    form.addItems({QString("c"), QString("d")});
    form.flush();
    QWidget w;
    form.attachTo(&w);
    auto fl = qobject_cast<QFormLayout *>(w.layout());
    QVERIFY(fl);
    QCOMPARE(fl->rowCount(), 2);
    QVERIFY(fl->contentsMargins().isNull());
}

void tst_LayoutBuilder::stretchOnBlueprint()
{
    Row row;
    row.addItem(QString("a"));
    st(&row);
    QWidget w;
    row.attachTo(&w);
    QVERIFY(w.layout());
    QCOMPARE(w.layout()->count(), 2);
    QVERIFY(w.layout()->itemAt(1)->spacerItem());
}

void tst_LayoutBuilder::setterArgumentIsCopied()
{
    std::optional<Column> column;
    {
        const QString local = QString("text").repeated(8);
        column.emplace(Column{Label{text(local)}});
    }
    const std::unique_ptr<QWidget> w(column->emerge());
    auto label = w->findChild<QLabel *>();
    QVERIFY(label);
    QCOMPARE(label->text(), QString("text").repeated(8));
}

void tst_LayoutBuilder::bindToIsSetOnEmerge()
{
    QLabel *label = nullptr;
    const Column column{Label{bindTo(&label), text("x")}};
    QVERIFY(!label);
    const std::unique_ptr<QWidget> w(column.emerge());
    QVERIFY(label);
    QCOMPARE(w->findChild<QLabel *>(), label);
    QCOMPARE(label->text(), QString("x"));
}

void tst_LayoutBuilder::builderWithProductAppliesItems()
{
    const ListWidget list{toolTip("tip")};
    const std::unique_ptr<QWidget> w(list.emerge());
    QVERIFY(qobject_cast<QListWidget *>(w.get()));
    QCOMPARE(w->toolTip(), QString("tip"));
}

void tst_LayoutBuilder::applyToExistingWidget()
{
    Label label({});
    label.setText("x");
    QLabel existing;
    label.applyTo(&existing);
    QCOMPARE(existing.text(), QString("x"));
}

void tst_LayoutBuilder::setterOnBlueprint()
{
    Label label({});
    label.setText("x");
    const std::unique_ptr<QWidget> w1(label.emerge());
    const std::unique_ptr<QWidget> w2(label.emerge());
    QCOMPARE(qobject_cast<QLabel *>(w1.get())->text(), QString("x"));
    QCOMPARE(qobject_cast<QLabel *>(w2.get())->text(), QString("x"));
}

void tst_LayoutBuilder::spanAllEmergesTwice()
{
    const Grid grid{SpanAll(2, QString("a")), br, QString("b"), QString("c")};
    for (int i = 0; i < 2; ++i) {
        const std::unique_ptr<QWidget> w(grid.emerge());
        auto g = qobject_cast<QGridLayout *>(w->layout());
        QVERIFY(g);
        int row, column, rowSpan, columnSpan;
        g->getItemPosition(0, &row, &column, &rowSpan, &columnSpan);
        QCOMPARE(columnSpan, 2);
        g->getItemPosition(2, &row, &column, &rowSpan, &columnSpan);
        QCOMPARE(row, 1);
        QCOMPARE(column, 1);
    }
}

void tst_LayoutBuilder::showOnBlueprint()
{
    Label{text("shown")}.show();
    QLabel *shown = nullptr;
    for (QWidget *w : QApplication::topLevelWidgets()) {
        if (auto label = qobject_cast<QLabel *>(w); label && label->text() == "shown")
            shown = label;
    }
    QVERIFY(shown);
    QVERIFY(shown->isVisible());
    delete shown;
}

void tst_LayoutBuilder::existingWidgetIsSingleUse()
{
    auto existing = new QLabel;
    const Column column{existing, QString("own")};
    const std::unique_ptr<QWidget> w1(column.emerge());
    QCOMPARE(existing->parentWidget(), w1.get());
    QTest::ignoreMessage(QtDebugMsg, QRegularExpression("SOFT ASSERT"));
    const std::unique_ptr<QWidget> w2(column.emerge());
    QCOMPARE(existing->parentWidget(), w1.get());
    QCOMPARE(w2->findChildren<QLabel *>().size(), 1);
}

void tst_LayoutBuilder::existingDialogButtonIsSingleUse()
{
    auto button = new QPushButton;
    DialogButtonBox box{};
    box.addButton(QDialogButtonBox::AcceptRole, button);
    const std::unique_ptr<QWidget> w1(box.emerge());
    QTest::ignoreMessage(QtDebugMsg, QRegularExpression("SOFT ASSERT"));
    const std::unique_ptr<QWidget> w2(box.emerge());
    auto b1 = qobject_cast<QDialogButtonBox *>(w1.get());
    auto b2 = qobject_cast<QDialogButtonBox *>(w2.get());
    QVERIFY(b1 && b2);
    QVERIFY(b1->buttons().contains(button));
    QVERIFY(b2->buttons().isEmpty());
}

void tst_LayoutBuilder::deletedExistingWidgetIsSkipped()
{
    auto existing = new QLabel;
    const Column column{existing, QString("own")};
    delete existing;
    const std::unique_ptr<QWidget> w(column.emerge());
    QCOMPARE(w->findChildren<QLabel *>().size(), 1);
}

QTEST_MAIN(tst_LayoutBuilder)

#include "tst_layoutbuilder.moc"
