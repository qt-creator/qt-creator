// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Tests for the value bindings of the (experimental) layout builder. The point
// of these tests is that the expressions referring to a Bindable work regardless
// of the order in which they appear - reader before or after the source, and
// transform expressions created before the source is set up.

#include "lb.h"

#include <QLabel>
#include <QSpinBox>
#include <QTextEdit>
#include <QTest>

using namespace Layouting;

class tst_Binding : public QObject
{
    Q_OBJECT

private slots:
    void transformedReaderBeforeSource();
    void transformedSourceBeforeReader();
    void transformedMultipleReaders();
    void plainReaderBeforeSource();
    void withBindableScope();
};

void tst_Binding::transformedReaderBeforeSource()
{
    Bindable<int> spinBoxValue;
    Bindable<QString> spinBoxText = spinBoxValue.transformed<QString>(
        [](int v) { return QString("World: %1").arg(v); });

    const std::unique_ptr<QWidget> w(Column {
        Label { text(spinBoxText) },           // reader appears before ...
        SpinBox { onValueChanged(spinBoxValue) }, // ... the source
    }.emerge());

    w->findChild<QSpinBox *>()->setValue(5);
    QCoreApplication::processEvents();
    QCOMPARE(w->findChild<QLabel *>()->text(), QString("World: 5"));
}

void tst_Binding::transformedSourceBeforeReader()
{
    Bindable<int> spinBoxValue;
    Bindable<QString> spinBoxText = spinBoxValue.transformed<QString>(
        [](int v) { return QString("World: %1").arg(v); });

    const std::unique_ptr<QWidget> w(Column {
        SpinBox { onValueChanged(spinBoxValue) }, // source appears before ...
        Label { text(spinBoxText) },           // ... the reader
    }.emerge());

    w->findChild<QSpinBox *>()->setValue(7);
    QCoreApplication::processEvents();
    QCOMPARE(w->findChild<QLabel *>()->text(), QString("World: 7"));
}

void tst_Binding::transformedMultipleReaders()
{
    Bindable<int> spinBoxValue;
    Bindable<QString> spinBoxText = spinBoxValue.transformed<QString>(
        [](int v) { return QString("v=%1").arg(v); });

    const std::unique_ptr<QWidget> w(Column {
        Label { text(spinBoxText) },           // reader before the source
        SpinBox { onValueChanged(spinBoxValue) },
        Label { text(spinBoxText) },           // reader after the source
    }.emerge());

    w->findChild<QSpinBox *>()->setValue(42);
    QCoreApplication::processEvents();
    const QList<QLabel *> labels = w->findChildren<QLabel *>();
    QCOMPARE(labels.size(), 2);
    QCOMPARE(labels.at(0)->text(), QString("v=42"));
    QCOMPARE(labels.at(1)->text(), QString("v=42"));
}

void tst_Binding::plainReaderBeforeSource()
{
    Bindable<QString> content;

    const std::unique_ptr<QWidget> w(Column {
        Label { text(content) },
        TextEdit { onValueChanged(content) },
    }.emerge());

    w->findChild<QTextEdit *>()->setPlainText("hello");
    QCoreApplication::processEvents();
    QCOMPARE(w->findChild<QLabel *>()->text(), QString("hello"));
}

// withBindable() scopes the bindable to the sub-layout, so no separate variable is
// needed to share it between the reader and the source.
void tst_Binding::withBindableScope()
{
    const std::unique_ptr<QWidget> w(withBindable([](Bindable<int> &spinBoxValue) {
        return Column {
            Label { text(spinBoxValue.transformed<QString>(
                [](int v) { return QString("n=%1").arg(v); })) },
            SpinBox { onValueChanged(spinBoxValue) },
        };
    }).emerge());

    w->findChild<QSpinBox *>()->setValue(3);
    QCoreApplication::processEvents();
    QCOMPARE(w->findChild<QLabel *>()->text(), QString("n=3"));
}

QTEST_MAIN(tst_Binding)

#include "tst_lb.moc"
