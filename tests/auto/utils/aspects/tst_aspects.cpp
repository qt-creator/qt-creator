// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/aspects.h>
#include <utils/layoutbuilder.h>

#include <QDoubleSpinBox>
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

QTEST_MAIN(tst_Aspects)

#include "tst_aspects.moc"
