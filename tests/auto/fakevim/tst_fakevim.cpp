// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

// Automated checks for FakeVim behavior that depends on a laid-out editor
// viewport (scrolling). The handler is driven on a real, shown QPlainTextEdit
// so that "lines on screen" is meaningful. Assertions use relative invariants
// (the view keeps scrolling, the cursor stays within the scrolloff band) so
// they do not depend on the exact font metrics of the test machine.

#include "fakevimhandler.h"

#include <QPlainTextEdit>
#include <QScopeGuard>
#include <QScrollBar>
#include <QTest>
#include <QTextBlock>

using namespace FakeVim::Internal;

class tst_FakeVim : public QObject
{
    Q_OBJECT

public:
    int topLine() const { return m_editor->cursorForPosition(QPoint(2, 2)).blockNumber(); }
    int cursorLine() const { return m_editor->textCursor().blockNumber(); }
    int linesVisible() const
    {
        const int h = m_editor->cursorRect().height();
        return h > 0 ? m_editor->viewport()->height() / h : 0;
    }
    void keys(const QString &input)
    {
        m_handler->handleInput(input);
        QCoreApplication::processEvents();
    }

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();

    void ctrlEScrollsPastScrollOff();
    void ctrlYScrollsPastScrollOff();
    void ctrlEHonorsCount();
    void ctrlEStableWhenCentered();
    void ctrlEScrollsPastScrollOffWhenWrapped();

private:
    QPlainTextEdit *m_editor = nullptr;
    FakeVimHandler *m_handler = nullptr;
    const int m_scrollOff = 5;
};

void tst_FakeVim::initTestCase()
{
    m_editor = new QPlainTextEdit;
    m_editor->setLineWrapMode(QPlainTextEdit::NoWrap);
    QFont font("Monospace");
    font.setPixelSize(12);
    m_editor->setFont(font);

    QString text;
    for (int i = 0; i < 400; ++i)
        text += QString("line%1\n").arg(i, 3, 10, QChar('0'));
    m_editor->setPlainText(text);
    m_editor->resize(400, 300);
    m_editor->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_editor));

    m_handler = new FakeVimHandler(m_editor);
    m_handler->installEventFilter();
    m_handler->setupWidget();
    m_handler->handleCommand(QString("set scrolloff=%1").arg(m_scrollOff));
    QCoreApplication::processEvents();

    if (linesVisible() < 2 * m_scrollOff + 4)
        QSKIP("Editor viewport is too small for the scrolling tests.");
}

void tst_FakeVim::cleanupTestCase()
{
    delete m_handler;
    delete m_editor;
}

void tst_FakeVim::init()
{
    keys("<ESC>");
    keys("gg");
    QCOMPARE(topLine(), 0);
}

// QTCREATORBUG-34074: Ctrl-E must keep scrolling past the point where the
// cursor reaches "scrolloff" from the top, dragging the cursor along.
void tst_FakeVim::ctrlEScrollsPastScrollOff()
{
    const int visible = linesVisible();

    // Put the cursor below the top scrolloff band so it is initially fixed.
    keys(QString("%1G").arg(visible)); // line 'visible' (1-based)
    const int startCursor = cursorLine();

    int previousTop = topLine();
    int previousCursor = cursorLine();
    bool cursorWasDragged = false;
    // Enough presses that the cursor reaches the boundary and is then dragged.
    for (int i = 0; i < visible + m_scrollOff + 5; ++i) {
        keys("<c-e>");
        const int top = topLine();
        const int cursor = cursorLine();

        QVERIFY2(top >= previousTop, "Ctrl-E must not scroll backwards.");
        QVERIFY2(cursor >= top + m_scrollOff, "Cursor came closer to the top than scrolloff.");
        QVERIFY2(cursor >= previousCursor, "Ctrl-E must not move the cursor up.");
        // The cursor stays on its buffer line while the view scrolls under it;
        // it only starts moving once it reaches the scrolloff band, and is then
        // dragged along pinned exactly at the offset.
        if (cursor > previousCursor) {
            cursorWasDragged = true;
            QCOMPARE(cursor - top, m_scrollOff);
        }
        previousTop = top;
        previousCursor = cursor;
    }

    // The regression: scrolling used to stop at the boundary, leaving the
    // cursor (and the view) frozen. It must have continued and dragged the
    // cursor down instead.
    QVERIFY2(cursorWasDragged, "Scrolling stopped at scrolloff (QTCREATORBUG-34074).");
    QVERIFY(cursorLine() > startCursor);
    QVERIFY(topLine() > 0);
    QCOMPARE(cursorLine() - topLine(), m_scrollOff); // pinned at the offset
}

// Symmetric case for Ctrl-Y (scroll up, cursor pinned at the bottom).
void tst_FakeVim::ctrlYScrollsPastScrollOff()
{
    const int visible = linesVisible();

    keys("200G");            // somewhere in the middle of the document
    keys("zz");              // center the line, so there is room to scroll up
    const int startCursor = cursorLine();
    const int bottomOffset = visible - 1 - m_scrollOff;

    int previousTop = topLine();
    bool cursorWasDragged = false;
    for (int i = 0; i < visible + m_scrollOff + 5; ++i) {
        const int previousCursor = cursorLine();
        keys("<c-y>");

        QVERIFY2(topLine() <= previousTop, "Ctrl-Y must not scroll forwards.");
        QVERIFY2(cursorLine() <= topLine() + bottomOffset,
                 "Cursor came closer to the bottom than scrolloff.");
        if (cursorLine() < previousCursor)
            cursorWasDragged = true;
        previousTop = topLine();
    }

    QVERIFY2(cursorWasDragged, "Scrolling stopped at scrolloff (QTCREATORBUG-34074).");
    QVERIFY(cursorLine() < startCursor);
}

// A repeat count must scroll that many lines.
void tst_FakeVim::ctrlEHonorsCount()
{
    keys("100G");
    keys("zt"); // put the cursor line at the top
    QCoreApplication::processEvents();
    const int top1 = topLine();
    keys("1<c-e>");
    const int step1 = topLine() - top1;

    keys("zt");
    QCoreApplication::processEvents();
    const int top3 = topLine();
    keys("3<c-e>");
    const int step3 = topLine() - top3;

    QVERIFY2(step1 >= 1, "1<c-e> should scroll at least one line.");
    QCOMPARE(step3, 3 * step1);
}

// Regression test for the "Center cursor on scroll" display option
// (QPlainTextEdit::centerOnScroll). With that option the earlier code, which
// scrolled through scrollToLine(), desynchronized and made Ctrl-E jitter the
// view back and forth. It must scroll monotonically and drag the cursor along;
// the view may also scroll past the last line, so the cursor can be dragged on
// toward the end of the document.
void tst_FakeVim::ctrlEStableWhenCentered()
{
    m_editor->setCenterOnScroll(true);
    const QScopeGuard resetCenter([this] { m_editor->setCenterOnScroll(false); });

    const int visible = linesVisible();
    keys("gg");
    keys("200G");
    keys("zt"); // put the cursor line at the top

    int previousTop = topLine();
    int previousCursor = cursorLine();
    bool scrolled = false;
    bool cursorWasDragged = false;
    for (int i = 0; i < visible + m_scrollOff + 10; ++i) {
        keys("<c-e>");
        const int top = topLine();
        const int cursor = cursorLine();

        QVERIFY2(top >= previousTop, "Ctrl-E jittered backwards with centerOnScroll.");
        QVERIFY2(cursor >= previousCursor, "Ctrl-E moved the cursor up with centerOnScroll.");
        QVERIFY2(cursor >= top + m_scrollOff, "Cursor came closer to the top than scrolloff.");
        if (top > previousTop)
            scrolled = true;
        if (cursor > previousCursor) {
            cursorWasDragged = true;
            QCOMPARE(cursor - top, m_scrollOff);
        }
        previousTop = top;
        previousCursor = cursor;
    }
    QVERIFY2(scrolled, "View did not scroll with centerOnScroll.");
    QVERIFY2(cursorWasDragged, "Cursor was not dragged with centerOnScroll.");
}

// Same invariants as ctrlEScrollsPastScrollOff(), but with "wrap" on and long
// lines so that one buffer line spans several display lines. Everything is
// measured in display lines (the scroll bar value is the first fully visible
// display line, the cursor's display line comes from the block layout), so a
// buffer-line-based implementation, or an off-by-one first-visible-line, shows
// up as the cursor slipping inside the scrolloff band.
void tst_FakeVim::ctrlEScrollsPastScrollOffWhenWrapped()
{
    QScrollBar *vbar = m_editor->verticalScrollBar();
    auto cursorDisplayLine = [&] {
        const QTextCursor tc = m_editor->textCursor();
        const QTextBlock b = tc.block();
        const int inBlock = b.layout()->lineForTextPosition(tc.positionInBlock()).lineNumber();
        return b.firstLineNumber() + inBlock;
    };

    m_editor->setLineWrapMode(QPlainTextEdit::WidgetWidth);
    const QScopeGuard restore([this] {
        m_editor->setLineWrapMode(QPlainTextEdit::NoWrap);
    });
    QString wrapped;
    for (int i = 0; i < 80; ++i)
        wrapped += QString("line%1 ").arg(i, 3, 10, QChar('0')) + QString(200, QChar('x')) + '\n';
    m_editor->setPlainText(wrapped);
    QCoreApplication::processEvents();

    keys("gg");
    keys("30G");

    int previousTop = vbar->value();
    int previousCursor = cursorDisplayLine();
    bool cursorWasDragged = false;
    for (int i = 0; i < 30; ++i) {
        keys("<c-e>");
        const int top = vbar->value();           // first fully visible display line
        const int cursor = cursorDisplayLine();   // cursor's display line
        const int row = cursor - top;             // cursor's row on screen

        QVERIFY2(top >= previousTop, "Ctrl-E must not scroll backwards (wrapped).");
        QVERIFY2(row >= m_scrollOff, "Cursor came closer to the top than scrolloff (wrapped).");
        QVERIFY2(cursor >= previousCursor, "Ctrl-E must not move the cursor up (wrapped).");
        if (cursor > previousCursor) {
            cursorWasDragged = true;
            QCOMPARE(row, m_scrollOff); // pinned exactly at the offset, in display lines
        }
        previousTop = top;
        previousCursor = cursor;
    }
    QVERIFY2(cursorWasDragged, "Scrolling stopped at scrolloff with wrap on.");
}

QTEST_MAIN(tst_FakeVim)

#include "tst_fakevim.moc"
