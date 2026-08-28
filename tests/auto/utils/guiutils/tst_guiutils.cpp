// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/guiutils.h>

#include <QTest>
#include <QWidget>

using namespace Utils;

class tst_GuiUtils : public QObject
{
    Q_OBJECT

private slots:
    void testOnFirstShowRunsOnceOnShow();
    void testOnFirstShowFollowsParent();
    void testOnFirstShowRunsImmediatelyWhenVisible();
};

// QWidget::show() delivers the show event synchronously, so none of these needs
// to wait for anything.

void tst_GuiUtils::testOnFirstShowRunsOnceOnShow()
{
    QWidget widget;
    int calls = 0;
    onFirstShow(&widget, [&calls] { ++calls; });
    QCOMPARE(calls, 0);

    widget.show();
    QCOMPARE(calls, 1);

    widget.hide();
    widget.show();
    QCOMPARE(calls, 1);
}

void tst_GuiUtils::testOnFirstShowFollowsParent()
{
    QWidget parent;
    QWidget *child = new QWidget(&parent);
    int calls = 0;
    onFirstShow(child, [&calls] { ++calls; });

    parent.show();
    QCOMPARE(calls, 1);
}

void tst_GuiUtils::testOnFirstShowRunsImmediatelyWhenVisible()
{
    QWidget widget;
    widget.show();
    QVERIFY(widget.isVisible());

    int calls = 0;
    onFirstShow(&widget, [&calls] { ++calls; });
    QCOMPARE(calls, 1);
}

QTEST_MAIN(tst_GuiUtils)

#include "tst_guiutils.moc"
