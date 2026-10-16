// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/fancytextedit.h>

#include <QAbstractItemView>
#include <QAction>
#include <QCompleter>
#include <QPlainTextEdit>
#include <QSignalSpy>
#include <QStringListModel>
#include <QTest>
#include <QVBoxLayout>

using namespace Utils;

class tst_FancyTextEdit : public QObject
{
    Q_OBJECT

private slots:
    void returnEmitsReturnPressedAndKeepsOneLine();
    void controlReturnInsertsLineBreak();
    void shiftReturnStillReachesItsAction();
    void otherReturnShortcutsStillReachTheirAction();
    void growsWithTheNumberOfLines();
    void stopsGrowingAtMaximumVisibleLines();
    void downOnLastLineOpensCompleter();
    void downOnOtherLinesMovesCursor();
    void completionCanBeMultiLine();
    void returnWhileCompletingEmitsOnce();
    void keysAreNotDroppedAfterReturnOnThePopup();
    void completingEmitsTextChangedOnce();
    void validationSeesAllLinesAndReportsChanges();
};

// The edit that has the keyboard focus is the one inside the widget.
static QWidget *inputOf(FancyTextEdit &edit)
{
    return edit.focusProxy();
}

static void typeLines(FancyTextEdit &edit, const QStringList &lines)
{
    for (int i = 0; i < lines.size(); ++i) {
        if (i > 0)
            QTest::keyClick(inputOf(edit), Qt::Key_Return, Qt::ControlModifier);
        QTest::keyClicks(inputOf(edit), lines.at(i));
    }
}

void tst_FancyTextEdit::returnEmitsReturnPressedAndKeepsOneLine()
{
    FancyTextEdit edit;
    QSignalSpy returnSpy(&edit, &FancyTextEdit::returnPressed);
    QTest::keyClicks(inputOf(edit), "abc");

    QTest::keyClick(inputOf(edit), Qt::Key_Return);

    QCOMPARE(returnSpy.size(), 1);
    QCOMPARE(edit.text(), "abc");
    QCOMPARE(edit.lineCount(), 1);
}

void tst_FancyTextEdit::controlReturnInsertsLineBreak()
{
    FancyTextEdit edit;
    QSignalSpy returnSpy(&edit, &FancyTextEdit::returnPressed);

    typeLines(edit, {"first", "second"});

    QCOMPARE(edit.text(), "first\nsecond");
    QCOMPARE(edit.lineCount(), 2);
    QCOMPARE(returnSpy.size(), 0);
}

void tst_FancyTextEdit::shiftReturnStillReachesItsAction()
{
    QWidget window;
    auto layout = new QVBoxLayout(&window);
    auto edit = new FancyTextEdit;
    layout->addWidget(edit);
    QAction action;
    action.setShortcut(QKeySequence("Shift+Return"));
    action.setShortcutContext(Qt::WidgetWithChildrenShortcut);
    edit->addAction(&action);
    int triggered = 0;
    connect(&action, &QAction::triggered, &window, [&triggered] { ++triggered; });
    QSignalSpy returnSpy(edit, &FancyTextEdit::returnPressed);

    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    inputOf(*edit)->setFocus();
    QTest::keyClicks(inputOf(*edit), "abc");

    QTest::keyClick(inputOf(*edit), Qt::Key_Return, Qt::ShiftModifier);

    QCOMPARE(triggered, 1);
    QCOMPARE(returnSpy.size(), 0);
    QCOMPARE(edit->text(), "abc");
}

void tst_FancyTextEdit::otherReturnShortcutsStillReachTheirAction()
{
    QWidget window;
    auto layout = new QVBoxLayout(&window);
    auto edit = new FancyTextEdit;
    layout->addWidget(edit);
    QAction action;
    action.setShortcut(QKeySequence("Ctrl+Alt+Return"));
    action.setShortcutContext(Qt::WidgetWithChildrenShortcut);
    edit->addAction(&action);
    int triggered = 0;
    connect(&action, &QAction::triggered, &window, [&triggered] { ++triggered; });

    window.show();
    QVERIFY(QTest::qWaitForWindowActive(&window));
    inputOf(*edit)->setFocus();
    QTest::keyClicks(inputOf(*edit), "abc");

    QTest::keyClick(inputOf(*edit), Qt::Key_Return, Qt::ControlModifier | Qt::AltModifier);

    QCOMPARE(triggered, 1);
    QCOMPARE(edit->text(), "abc");
}

void tst_FancyTextEdit::growsWithTheNumberOfLines()
{
    QWidget window;
    auto layout = new QVBoxLayout(&window);
    auto edit = new FancyTextEdit;
    layout->addWidget(edit);
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    const int oneLine = edit->height();
    QCOMPARE(oneLine, edit->sizeHint().height());

    typeLines(*edit, {"a", "b", "c"});

    const int lineHeight = edit->fontMetrics().lineSpacing();
    QTRY_COMPARE(edit->height(), oneLine + 2 * lineHeight);

    edit->setText("single");
    QTRY_COMPARE(edit->height(), oneLine);
}

void tst_FancyTextEdit::stopsGrowingAtMaximumVisibleLines()
{
    FancyTextEdit edit;
    edit.setMaximumVisibleLines(3);
    typeLines(edit, {"a", "b", "c"});
    const int atMaximum = edit.sizeHint().height();
    edit.setText({});

    typeLines(edit, {"a", "b", "c", "d", "e"});

    QCOMPARE(edit.lineCount(), 5);
    QCOMPARE(edit.sizeHint().height(), atMaximum);
}

static QCompleter *historyCompleter(QObject *parent)
{
    auto completer = new QCompleter(new QStringListModel({"foo", "foo\nbar"}, parent), parent);
    completer->setCompletionMode(QCompleter::PopupCompletion);
    return completer;
}

void tst_FancyTextEdit::downOnLastLineOpensCompleter()
{
    FancyTextEdit edit;
    QCompleter *completer = historyCompleter(&edit);
    edit.setSpecialCompleter(completer);
    edit.show();
    QVERIFY(QTest::qWaitForWindowExposed(&edit));
    edit.setText("foo");
    QVERIFY(!completer->popup()->isVisible());

    QTest::keyClick(inputOf(edit), Qt::Key_Down);

    QVERIFY(completer->popup()->isVisible());
}

void tst_FancyTextEdit::downOnOtherLinesMovesCursor()
{
    FancyTextEdit edit;
    QCompleter *completer = historyCompleter(&edit);
    edit.setSpecialCompleter(completer);
    edit.show();
    QVERIFY(QTest::qWaitForWindowExposed(&edit));
    edit.setText("foo\nbar");
    QTest::keyClick(inputOf(edit), Qt::Key_Home, Qt::ControlModifier);

    QTest::keyClick(inputOf(edit), Qt::Key_Down);

    QVERIFY(!completer->popup()->isVisible());
    auto *input = qobject_cast<QPlainTextEdit *>(inputOf(edit));
    QVERIFY(input);
    QCOMPARE(input->textCursor().blockNumber(), 1);
}

void tst_FancyTextEdit::completionCanBeMultiLine()
{
    FancyTextEdit edit;
    QCompleter *completer = historyCompleter(&edit);
    edit.setSpecialCompleter(completer);
    edit.show();
    QVERIFY(QTest::qWaitForWindowExposed(&edit));
    QSignalSpy returnSpy(&edit, &FancyTextEdit::returnPressed);
    edit.setText("foo\n");
    QTest::keyClick(inputOf(edit), Qt::Key_Down);
    QVERIFY(completer->popup()->isVisible());
    QTest::keyClick(completer->popup(), Qt::Key_Down);

    QTest::keyClick(completer->popup(), Qt::Key_Return);

    QCOMPARE(edit.text(), "foo\nbar");
    QCOMPARE(returnSpy.size(), 1);
}

void tst_FancyTextEdit::returnWhileCompletingEmitsOnce()
{
    FancyTextEdit edit;
    QCompleter *completer = historyCompleter(&edit);
    edit.setSpecialCompleter(completer);
    edit.show();
    QVERIFY(QTest::qWaitForWindowExposed(&edit));
    QSignalSpy returnSpy(&edit, &FancyTextEdit::returnPressed);
    edit.setText("foo");
    QTest::keyClick(inputOf(edit), Qt::Key_Down);
    QVERIFY(completer->popup()->isVisible());

    QTest::keyClick(inputOf(edit), Qt::Key_Return);

    QCOMPARE(returnSpy.size(), 1);
}

void tst_FancyTextEdit::keysAreNotDroppedAfterReturnOnThePopup()
{
    FancyTextEdit edit;
    QCompleter *completer = historyCompleter(&edit);
    edit.setSpecialCompleter(completer);
    edit.show();
    QVERIFY(QTest::qWaitForWindowExposed(&edit));
    edit.setText("foo\n");
    QTest::keyClick(inputOf(edit), Qt::Key_Down);
    QVERIFY(completer->popup()->isVisible());
    QTest::keyClick(completer->popup(), Qt::Key_Down);
    QTest::keyClick(completer->popup(), Qt::Key_Return);
    QCOMPARE(edit.text(), "foo\nbar");

    // Typing a term that matches the history shows the popup again.
    edit.setText(QString());
    QTest::keyClick(inputOf(edit), Qt::Key_F);
    QTRY_VERIFY(completer->popup()->isVisible());
    QTest::keyClick(completer->popup(), Qt::Key_O);

    QCOMPARE(edit.text(), "fo");
}

void tst_FancyTextEdit::completingEmitsTextChangedOnce()
{
    FancyTextEdit edit;
    QCompleter *completer = historyCompleter(&edit);
    edit.setSpecialCompleter(completer);
    edit.show();
    QVERIFY(QTest::qWaitForWindowExposed(&edit));
    edit.setText("foo\n");
    QTest::keyClick(inputOf(edit), Qt::Key_Down);
    QVERIFY(completer->popup()->isVisible());
    QTest::keyClick(completer->popup(), Qt::Key_Down);
    QSignalSpy textChangedSpy(&edit, &FancyTextEdit::textChanged);

    QTest::keyClick(completer->popup(), Qt::Key_Return);

    QCOMPARE(edit.text(), "foo\nbar");
    QCOMPARE(textChangedSpy.size(), 1);
}

void tst_FancyTextEdit::validationSeesAllLinesAndReportsChanges()
{
    FancyTextEdit edit;
    // Valid as long as the term has no second line.
    edit.setValidationFunction([](const QString &text) -> Result<> {
        if (text.contains('\n'))
            return ResultError(QString("multi-line"));
        return ResultOk;
    });
    QVERIFY(edit.isValid());
    QSignalSpy validSpy(&edit, &FancyTextEdit::validChanged);

    typeLines(edit, {"first", "second"});

    QVERIFY(!edit.isValid());
    QCOMPARE(validSpy.size(), 1);
    QCOMPARE(validSpy.last().at(0).toBool(), false);
    QCOMPARE(edit.toolTip(), "multi-line");

    edit.setText("one line");

    QVERIFY(edit.isValid());
    QCOMPARE(validSpy.size(), 2);
    QCOMPARE(validSpy.last().at(0).toBool(), true);
}

QTEST_MAIN(tst_FancyTextEdit)

#include "tst_fancytextedit.moc"
