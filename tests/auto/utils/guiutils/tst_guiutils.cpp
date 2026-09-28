// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/checkablemessagebox.h>
#include <utils/guiutils.h>

#include <QApplication>
#include <QTest>
#include <QTimer>
#include <QWidget>

#include <optional>

using namespace Utils;

class tst_GuiUtils : public QObject
{
    Q_OBJECT

private slots:
    void testOnFirstShowRunsOnceOnShow();
    void testOnFirstShowFollowsParent();
    void testOnFirstShowRunsImmediatelyWhenVisible();
    void testNonInteractiveMessageBoxTakesDefault();
    void testNonInteractiveAsyncMessageBoxTakesDefault();
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

void tst_GuiUtils::testNonInteractiveMessageBoxTakesDefault()
{
    setDialogsInteractive(false);

    // Runs only inside the nested event loop of a dialog that did get shown.
    bool dialogShown = false;
    QTimer closer;
    closer.setSingleShot(true);
    connect(&closer, &QTimer::timeout, [&dialogShown] {
        if (QWidget *dialog = QApplication::activeModalWidget()) {
            dialogShown = true;
            dialog->close();
        }
    });
    closer.start(0);

    const QMessageBox::StandardButton button
        = CheckableMessageBox::question("Title", "Question", {});

    setDialogsInteractive(true);
    QVERIFY(!dialogShown);
    QCOMPARE(button, QMessageBox::No);
}

void tst_GuiUtils::testNonInteractiveAsyncMessageBoxTakesDefault()
{
    setDialogsInteractive(false);

    std::optional<QMessageBox::StandardButton> button;
    CheckableMessageBox::question_async("Title", "Question", {}, this,
                                        [&button](QMessageBox::StandardButton b) { button = b; });

    setDialogsInteractive(true);
    QTRY_VERIFY(button);
    QCOMPARE(*button, QMessageBox::No);
    for (QWidget *widget : QApplication::topLevelWidgets())
        QVERIFY(!qobject_cast<QMessageBox *>(widget));
}

QTEST_MAIN(tst_GuiUtils)

#include "tst_guiutils.moc"
