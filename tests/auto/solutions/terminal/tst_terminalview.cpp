// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <solutions/terminal/terminalview.h>

#include <QTest>
#include <QTextDocument>
#include <QToolTip>
#include <QUrl>

#include <memory>

using namespace TerminalSolution;

// The tooltip is handed over as explicit rich text, so that whether it is read
// as markup does not depend on the characters the target happens to carry.
static QString asToolTip(const QString &target)
{
    return QLatin1String("<html>") + target.toHtmlEscaped() + QLatin1String("</html>");
}

class TestView : public TerminalView
{
public:
    std::optional<Link> activated;
    QList<QSize> ptyResizes;

    bool resizePty(QSize newSize) override
    {
        ptyResizes.append(newSize);
        return true;
    }

    // Emulates the file/hash sniffing a host does on plain text.
    std::optional<Link> toLink(const QString &text) override
    {
        if (text == QStringView(u"sniffed"))
            return Link{"sniffed-target"};
        return std::nullopt;
    }

    void linkActivated(const Link &link) override { activated = link; }

    // What a triple click builds: the whole row, ending one past its last cell.
    void selectRow(int row)
    {
        const int width = surface()->liveSize().width();
        setSelection(Selection{surface()->gridToPos({0, row}),
                               surface()->gridToPos({width, row}),
                               true});
    }

    void ctrlHover(QPoint gridPos)
    {
        const QPoint pos = viewportPos(gridPos);

        QMouseEvent move(QEvent::MouseMove,
                         pos,
                         viewport()->mapToGlobal(pos),
                         Qt::NoButton,
                         Qt::NoButton,
                         Qt::ControlModifier);
        QCoreApplication::sendEvent(viewport(), &move);
    }

    void ctrlClick(QPoint gridPos)
    {
        ctrlHover(gridPos);

        const QPoint pos = viewportPos(gridPos);
        QMouseEvent press(QEvent::MouseButtonPress,
                          pos,
                          viewport()->mapToGlobal(pos),
                          Qt::LeftButton,
                          Qt::LeftButton,
                          Qt::ControlModifier);
        QCoreApplication::sendEvent(viewport(), &press);
    }

private:
    QPoint viewportPos(QPoint gridPos) const
    {
        return globalToViewport(gridToGlobal(gridPos).toPoint()) + QPoint(1, 1);
    }
};

class tst_TerminalView : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<TestView> m_view;

private slots:
    void init()
    {
        m_view = std::make_unique<TestView>();
        m_view->resize(800, 600);
        m_view->show();
        QVERIFY(QTest::qWaitForWindowExposed(m_view.get()));
    }

    void cleanup()
    {
        QToolTip::hideText();
        m_view.reset();
    }

    void sniffedLinkIsActivated()
    {
        m_view->writeToTerminal("sniffed", true);
        m_view->ctrlClick({0, 0});

        QVERIFY(m_view->activated);
        QCOMPARE(m_view->activated->text, QString("sniffed-target"));
        QCOMPARE(m_view->activated->isUri, false);
    }

    void hyperlinkIsActivated()
    {
        m_view->writeToTerminal("\x1b]8;;http://example.com\x1b\\This is a link\x1b]8;;\x1b\\",
                                true);

        m_view->ctrlClick({9, 0});

        QVERIFY(m_view->activated);
        QCOMPARE(m_view->activated->text, QString("http://example.com"));
        QCOMPARE(m_view->activated->isUri, true);
    }

    void hyperlinkWinsOverSniffing()
    {
        // Every cell of the link text would be sniffed as a link of its own without
        // the hyperlink taking precedence.
        m_view->writeToTerminal("\x1b]8;;http://example.com\x1b\\sniffed\x1b]8;;\x1b\\", true);

        m_view->ctrlClick({0, 0});

        QVERIFY(m_view->activated);
        QCOMPARE(m_view->activated->text, QString("http://example.com"));
    }

    void aHyperlinkShowsItsTargetAsAToolTip()
    {
        m_view->writeToTerminal("\x1b]8;;http://example.com\x1b\\This is a link\x1b]8;;\x1b\\",
                                true);

        m_view->ctrlHover({9, 0});

        QTRY_VERIFY(QToolTip::isVisible());
        QCOMPARE(QToolTip::text(), asToolTip("http://example.com"));

        m_view->ctrlHover({40, 0});
        QTRY_VERIFY(!QToolTip::isVisible());
    }

    void aHyperlinkWithAnUnparseableUriIsNotALink()
    {
        const QString target = "http://[oops";
        QVERIFY2(!QUrl(target).isValid(), "this test needs a uri QUrl rejects");

        m_view->writeToTerminal("\x1b]8;;" + target.toUtf8()
                                    + "\x1b\\This is a link\x1b]8;;\x1b\\",
                                true);

        m_view->ctrlClick({9, 0});

        // There is no encoded form to put in the tooltip, so the reader would
        // be offered an underlined, clickable target they were never shown.
        // Only activation is checked: hiding the tooltip is what the code did
        // before this too, so that says nothing about which of them ran.
        QVERIFY(!m_view->activated);
    }

    void aSniffedLinkShowsItsTargetAsAToolTip()
    {
        m_view->writeToTerminal("sniffed", true);

        m_view->ctrlHover({0, 0});

        QTRY_VERIFY(QToolTip::isVisible());
        QCOMPARE(QToolTip::text(), asToolTip("sniffed-target"));
    }

    void aSelectionSurvivesOutputBelowIt()
    {
        m_view->writeToTerminal("one\r\ntwo\r\nthree", true);

        m_view->selectRow(0);
        QVERIFY(m_view->selection());

        // Row 1 holds no selected cell: a row selection ends on the first cell
        // of the row below, which is where the selection stops rather than
        // where it reaches.
        m_view->writeToTerminal("\x1b[2;1Hbelow", true);

        QVERIFY2(m_view->selection(), "output on the row below cleared the selection");

        // The control: damage that does reach the selected row clears it, so
        // the check above is not passing because nothing is being cleared.
        m_view->writeToTerminal("\x1b[1;1Hinside", true);

        QVERIFY(!m_view->selection());
    }

    void aClearedScrollbackDropsTheSelection()
    {
        // Fill the scrollback, so that the rows the selection names are
        // measured from a top that clearing is about to move.
        for (int i = 0; i < 40; ++i)
            m_view->writeToTerminal("line\r\n", true);

        const int scrollbackRows = m_view->surface()->fullSize().height()
                                   - m_view->surface()->liveSize().height();
        QVERIFY(scrollbackRows > 0);

        m_view->selectRow(scrollbackRows - 1);
        QVERIFY(m_view->selection().has_value());

        // Clearing reports a size change, not a damaged rectangle, so the
        // overlap test that keeps a selection across a redraw never sees it.
        m_view->surface()->clearAll();

        QTRY_VERIFY(!m_view->selection().has_value());
    }

    void aToolTipSaysWhichFormatItIs()
    {
        m_view->writeToTerminal(
            "\x1b]8;;http://example.com/?a=1&b=2\x1b\\This is a link\x1b]8;;\x1b\\", true);

        m_view->ctrlHover({9, 0});

        QTRY_VERIFY(QToolTip::isVisible());

        // Escaping the target removes every '<', which is the character
        // Qt::mightBeRichText looks for, so a target carrying an '&' and left
        // at Qt::AutoText would reach the reader as "&amp;".
        QVERIFY2(Qt::mightBeRichText(QToolTip::text()),
                 "the tooltip is read as plain text, so its escapes are shown as they are");
        QCOMPARE(QToolTip::text(), asToolTip("http://example.com/?a=1&b=2"));
    }

    void aBurstOfResizesIsCoalesced()
    {
        m_view->ptyResizes.clear();

        int events = 0;
        for (int width = 800; width > 600; width -= 20, ++events)
            m_view->resize(width, 600);

        QTRY_VERIFY(!m_view->ptyResizes.isEmpty());

        QVERIFY2(m_view->ptyResizes.size() < events / 2,
                 qPrintable(QString("%1 events became %2 resizes")
                                .arg(events)
                                .arg(m_view->ptyResizes.size())));

        QCOMPARE(m_view->ptyResizes.last(), m_view->surface()->liveSize());
    }

    void plainTextIsNotALink()
    {
        m_view->writeToTerminal("This is a link", true);

        m_view->ctrlClick({0, 0});

        QVERIFY(!m_view->activated);
    }
};

QTEST_MAIN(tst_TerminalView)

#include "tst_terminalview.moc"
