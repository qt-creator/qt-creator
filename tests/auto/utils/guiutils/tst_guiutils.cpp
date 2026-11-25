// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/checkablemessagebox.h>
#include <utils/guiutils.h>

#include <QApplication>
#include <QGroupBox>
#include <QPlainTextEdit>
#include <QTest>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
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
    void testAttachToGroupBoxTitle();
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

// The widget sits on the line of the title, at the right edge of the editor in the
// box, and follows the editor when the layout changes.
void tst_GuiUtils::testAttachToGroupBoxTitle()
{
    QWidget container;
    auto layout = new QVBoxLayout(&container);
    auto box = new QGroupBox("Description");
    auto boxLayout = new QVBoxLayout(box);
    auto editor = new QPlainTextEdit;
    boxLayout->addWidget(editor);
    layout->addWidget(box);
    auto button = new QToolButton(&container);
    button->setFixedSize(16, 16);
    attachToGroupBoxTitle(button, box, editor);
    container.resize(400, 300);
    container.show();
    QVERIFY(QTest::qWaitForWindowExposed(&container));

    const auto editorRect = [&] {
        return QRect(editor->mapTo(&container, QPoint()), editor->size());
    };
    const auto buttonRect = [&] { return button->geometry(); };
    QTRY_COMPARE(buttonRect().right(), editorRect().right());
    QVERIFY(buttonRect().center().y() < editorRect().top());
    QVERIFY(buttonRect().center().y() >= box->mapTo(&container, QPoint()).y());

    container.resize(600, 300);
    QTRY_COMPARE(buttonRect().right(), editorRect().right());

    // The widget keeps its place when it grows.
    button->setFixedSize(40, 16);
    QTRY_COMPARE(buttonRect().width(), 40);
    QTRY_COMPARE(buttonRect().right(), editorRect().right());

    // The widget lives outside the box, so it can outlive it.
    delete box;
    button->setFixedSize(24, 16);
    QCOMPARE(button->width(), 24);
}

QTEST_MAIN(tst_GuiUtils)

#include "tst_guiutils.moc"
