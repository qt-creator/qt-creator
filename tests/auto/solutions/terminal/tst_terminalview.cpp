// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <solutions/terminal/terminalview.h>

#include <QTest>
#include <QToolTip>

#include <memory>

using namespace TerminalSolution;

class TestView : public TerminalView
{
public:
    std::optional<Link> activated;

    // Emulates the file/hash sniffing a host does on plain text.
    std::optional<Link> toLink(const QString &text) override
    {
        if (text == QStringView(u"sniffed"))
            return Link{"sniffed-target"};
        return std::nullopt;
    }

    void linkActivated(const Link &link) override { activated = link; }

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

        QVERIFY(QToolTip::isVisible());
        QCOMPARE(QToolTip::text(), QString("http://example.com"));

        m_view->ctrlHover({40, 0});
        QTRY_VERIFY(!QToolTip::isVisible());
    }

    void aSniffedLinkHasNoToolTip()
    {
        // The text is the target, so there is nothing to tell.
        m_view->writeToTerminal("sniffed", true);

        m_view->ctrlHover({0, 0});

        QVERIFY(!QToolTip::isVisible());
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
