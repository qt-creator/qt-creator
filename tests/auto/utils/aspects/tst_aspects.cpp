// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/aspects.h>
#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>

#include <QDoubleSpinBox>
#include <QLabel>
#include <QListWidget>
#include <QSpinBox>
#include <QTest>
#include <QWidget>

#include <memory>

using namespace Utils;

class tst_Aspects : public QObject
{
    Q_OBJECT

private slots:
    void integerAspectUpdatesEverySpinBox();
    void integerAspectTakesEditsFromEverySpinBox();
    void integerAspectKeepsUnscaledValue();
    void doubleAspectUpdatesEverySpinBox();
    void doubleAspectTakesEditsFromEverySpinBox();
    void doubleAspectKeepsUnroundedValue();
    void textDisplayGivesEveryWidgetALabel();
    void textDisplayUpdatesEveryLabel();
    void multiSelectionAspectUpdatesEveryList();
    void multiSelectionAspectTakesEditsFromEveryList();
    void multiSelectionAspectEditKeepsValueOrder();
    void multiSelectionAspectKeepsValueOrder();
};

static std::unique_ptr<QWidget> createWidget(BaseAspect &aspect)
{
    using namespace Layouting;
    return std::unique_ptr<QWidget>(Column { aspect }.emerge());
}

static QSpinBox *spinBox(const std::unique_ptr<QWidget> &widget)
{
    return widget->findChild<QSpinBox *>();
}

// InfoLabel has no Q_OBJECT; the widget holds nothing but the display's label.
static InfoLabel *infoLabel(const std::unique_ptr<QWidget> &widget)
{
    const QList<QLabel *> labels = widget->findChildren<QLabel *>();
    return labels.size() == 1 ? static_cast<InfoLabel *>(labels.first()) : nullptr;
}

static QListWidget *listWidget(const std::unique_ptr<QWidget> &widget)
{
    return widget->findChild<QListWidget *>();
}

static QStringList checkedItems(QListWidget *list)
{
    QStringList checked;
    for (int i = 0; i < list->count(); ++i) {
        if (list->item(i)->checkState() == Qt::Checked)
            checked.append(list->item(i)->text());
    }
    return checked;
}

static QDoubleSpinBox *doubleSpinBox(const std::unique_ptr<QWidget> &widget)
{
    return widget->findChild<QDoubleSpinBox *>();
}

void tst_Aspects::integerAspectUpdatesEverySpinBox()
{
    IntegerAspect aspect;
    aspect.setRange(0, 100);
    const auto first = createWidget(aspect);
    const auto second = createWidget(aspect);
    QVERIFY(spinBox(first));
    QVERIFY(spinBox(second));

    aspect.setValue(42);

    QCOMPARE(spinBox(first)->value(), 42);
    QCOMPARE(spinBox(second)->value(), 42);
}

void tst_Aspects::integerAspectTakesEditsFromEverySpinBox()
{
    IntegerAspect aspect;
    aspect.setRange(0, 100);
    const auto first = createWidget(aspect);
    const auto second = createWidget(aspect);
    QVERIFY(spinBox(first));
    QVERIFY(spinBox(second));

    spinBox(first)->setValue(17);

    QCOMPARE(aspect.value(), 17);
    QCOMPARE(spinBox(second)->value(), 17);
}

void tst_Aspects::integerAspectKeepsUnscaledValue()
{
    IntegerAspect aspect;
    aspect.setRange(0, 10000);
    aspect.setDisplayScaleFactor(1000);
    const auto widget = createWidget(aspect);
    QVERIFY(spinBox(widget));

    aspect.setValue(1500);

    QCOMPARE(spinBox(widget)->value(), 1);
    QCOMPARE(aspect.value(), 1500);
}

void tst_Aspects::doubleAspectUpdatesEverySpinBox()
{
    DoubleAspect aspect;
    aspect.setRange(0, 100);
    const auto first = createWidget(aspect);
    const auto second = createWidget(aspect);
    QVERIFY(doubleSpinBox(first));
    QVERIFY(doubleSpinBox(second));

    aspect.setValue(4.5);

    QCOMPARE(doubleSpinBox(first)->value(), 4.5);
    QCOMPARE(doubleSpinBox(second)->value(), 4.5);
}

void tst_Aspects::doubleAspectTakesEditsFromEverySpinBox()
{
    DoubleAspect aspect;
    aspect.setRange(0, 100);
    const auto first = createWidget(aspect);
    const auto second = createWidget(aspect);
    QVERIFY(doubleSpinBox(first));
    QVERIFY(doubleSpinBox(second));

    doubleSpinBox(first)->setValue(1.5);

    QCOMPARE(aspect.value(), 1.5);
    QCOMPARE(doubleSpinBox(second)->value(), 1.5);
}

void tst_Aspects::doubleAspectKeepsUnroundedValue()
{
    DoubleAspect aspect;
    aspect.setRange(0, 100);
    const auto widget = createWidget(aspect);
    QVERIFY(doubleSpinBox(widget));
    QCOMPARE(doubleSpinBox(widget)->decimals(), 2);

    aspect.setValue(0.125);

    QCOMPARE(aspect.value(), 0.125);
}

void tst_Aspects::textDisplayGivesEveryWidgetALabel()
{
    TextDisplay display(nullptr, "Hello");
    const auto first = createWidget(display);
    const auto second = createWidget(display);

    QVERIFY(infoLabel(first));
    QVERIFY(infoLabel(second));
    QVERIFY(infoLabel(first) != infoLabel(second));
}

void tst_Aspects::textDisplayUpdatesEveryLabel()
{
    TextDisplay display(nullptr, "Hello");
    display.setIconType(InfoLabelType::None);
    const auto first = createWidget(display);
    const auto second = createWidget(display);
    QVERIFY(infoLabel(first));
    QVERIFY(infoLabel(second));

    display.setText("Changed");
    display.setIconType(InfoLabelType::Warning);

    QCOMPARE(infoLabel(first)->text(), "Changed");
    QCOMPARE(infoLabel(second)->text(), "Changed");
    QCOMPARE(infoLabel(first)->type(), InfoLabelType::Warning);
    QCOMPARE(infoLabel(second)->type(), InfoLabelType::Warning);
}

void tst_Aspects::multiSelectionAspectUpdatesEveryList()
{
    MultiSelectionAspect aspect;
    aspect.setAllValues({"a", "b", "c"});
    const auto first = createWidget(aspect);
    const auto second = createWidget(aspect);
    QVERIFY(listWidget(first));
    QVERIFY(listWidget(second));

    aspect.setValue({"a", "c"});

    QCOMPARE(checkedItems(listWidget(first)), QStringList({"a", "c"}));
    QCOMPARE(checkedItems(listWidget(second)), QStringList({"a", "c"}));
}

void tst_Aspects::multiSelectionAspectTakesEditsFromEveryList()
{
    MultiSelectionAspect aspect;
    aspect.setAllValues({"a", "b", "c"});
    const auto first = createWidget(aspect);
    const auto second = createWidget(aspect);
    QVERIFY(listWidget(first));
    QVERIFY(listWidget(second));

    listWidget(first)->item(1)->setCheckState(Qt::Checked);

    QCOMPARE(aspect.value(), QStringList({"b"}));
    QCOMPARE(checkedItems(listWidget(second)), QStringList({"b"}));
}

void tst_Aspects::multiSelectionAspectEditKeepsValueOrder()
{
    MultiSelectionAspect aspect;
    aspect.setAllValues({"a", "b", "c"});
    aspect.setValue({"c", "a", "x"});
    const auto widget = createWidget(aspect);
    QVERIFY(listWidget(widget));

    listWidget(widget)->item(1)->setCheckState(Qt::Checked);
    QCOMPARE(aspect.value(), QStringList({"c", "a", "x", "b"}));

    listWidget(widget)->item(0)->setCheckState(Qt::Unchecked);
    QCOMPARE(aspect.value(), QStringList({"c", "x", "b"}));
}

void tst_Aspects::multiSelectionAspectKeepsValueOrder()
{
    MultiSelectionAspect aspect;
    aspect.setAllValues({"a", "b", "c"});
    const auto widget = createWidget(aspect);
    QVERIFY(listWidget(widget));

    aspect.setValue({"c", "a"});

    QCOMPARE(aspect.value(), QStringList({"c", "a"}));
    QCOMPARE(checkedItems(listWidget(widget)), QStringList({"a", "c"}));
}

QTEST_MAIN(tst_Aspects)

#include "tst_aspects.moc"
