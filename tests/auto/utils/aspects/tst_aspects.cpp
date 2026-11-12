// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/aspects.h>
#include <utils/layoutbuilder.h>

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
};

static std::unique_ptr<QWidget> createWidget(IntegerAspect &aspect)
{
    using namespace Layouting;
    return std::unique_ptr<QWidget>(Column { aspect }.emerge());
}

static QSpinBox *spinBox(const std::unique_ptr<QWidget> &widget)
{
    return widget->findChild<QSpinBox *>();
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

QTEST_MAIN(tst_Aspects)

#include "tst_aspects.moc"
